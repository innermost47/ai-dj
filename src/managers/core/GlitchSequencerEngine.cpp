#include "GlitchSequencerEngine.h"
#include "SequencerManager.h"
#include "TrackData.h"
#include <cmath>

void GlitchSequencerEngine::startBeatRepeat(TrackData &track, double hostBpm)
{
	if (track.beatRepeatActive.load())
		return;

	track.beatRepeatPending.store(false);
	track.beatRepeatStopPending.store(false);
	track.pendingBeatNumber.store(-1);
	track.pendingStopBeatNumber.store(-1);
	track.randomRetriggerEnabled.store(true);

	if (sequencerManager)
	{
		if (track.isPlaying.load())
			sequencerManager->setupBeatRepeatZone(&track, hostBpm);
		else
			track.beatRepeatAnchorPending.store(true);

		sequencerManager->acquireTheoreticalPosition(&track);
	}

	track.beatRepeatActive.store(true);
}

void GlitchSequencerEngine::refreshMasksAfterEdit(TrackData &track)
{
	if (!track.glitchSequencerActive.load())
	{
		track.glitchOwnedMask.store(0);
		track.glitchReservedMask.store(0);
		track.glitchFxEnabledMask.store(0);
		applyEffectiveBypasses(track);
		return;
	}

	const int seqIdx = juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load());
	const int oldOwned = track.glitchOwnedMask.load();
	const int newOwned = rebuildOwnedEffectsMask(track.glitchSequences[seqIdx]);

	track.glitchOwnedMask.store(newOwned);
	track.glitchReservedMask.store(computeReservedMask(track));

	const int leaving = oldOwned & ~newOwned;
	if (leaving != 0)
	{
		clearTransportEffects(track, leaving);

		if (leaving & effectBit(GlitchEffectType::Chorus))
			restoreMix(track, GlitchEffectType::Chorus);
		if (leaving & effectBit(GlitchEffectType::Phaser))
			restoreMix(track, GlitchEffectType::Phaser);
		if (leaving & effectBit(GlitchEffectType::Flanger))
			restoreMix(track, GlitchEffectType::Flanger);
		if (leaving & effectBit(GlitchEffectType::BitCrusher))
			restoreMix(track, GlitchEffectType::BitCrusher);
	}

	track.glitchFxEnabledMask.store(0);
	track.lastTriggeredGlitchStep.store(-1);
	applyEffectiveBypasses(track);
}

void GlitchSequencerEngine::stopBeatRepeat(TrackData &track)
{
	if (!track.beatRepeatActive.load() && !track.beatRepeatPending.load())
		return;

	track.beatRepeatPending.store(false);
	track.beatRepeatStopPending.store(false);
	track.pendingBeatNumber.store(-1);
	track.pendingStopBeatNumber.store(-1);
	track.beatRepeatActive.store(false);
	track.randomRetriggerEnabled.store(false);
	track.beatRepeatAnchorPending.store(false);
	track.randomRetriggerActive.store(false);
	track.lastRetriggerTime.store(-1.0);

	if (sequencerManager)
		sequencerManager->releaseTheoreticalPosition(&track);
}

void GlitchSequencerEngine::applyEffectiveBypasses(TrackData &track)
{
	const int owned = track.glitchOwnedMask.load();
	const int userOn = track.userFxEnabledMask.load();
	const int glitchOn = track.glitchFxEnabledMask.load() & owned;

	const int on = (userOn & ~owned) | glitchOn;

	track.gate.setBypassed((on & effectBit(GlitchEffectType::Gate)) == 0);
	track.filter.setBypassed((on & effectBit(GlitchEffectType::Filter)) == 0);
	track.chorus.setBypassed((on & effectBit(GlitchEffectType::Chorus)) == 0);
	track.phaser.setBypassed((on & effectBit(GlitchEffectType::Phaser)) == 0);
	track.flanger.setBypassed((on & effectBit(GlitchEffectType::Flanger)) == 0);
	track.bitCrusher.setBypassed((on & effectBit(GlitchEffectType::BitCrusher)) == 0);
	track.distortion.setBypassed((on & effectBit(GlitchEffectType::Distortion)) == 0);
}

void GlitchSequencerEngine::setUserFxEnabled(TrackData &track, GlitchEffectType type, bool enabled)
{
	const int bit = effectBit(type);
	if ((bit & kBypassableMask) == 0)
		return;

	int cur = track.userFxEnabledMask.load();
	int next;
	do
	{
		next = enabled ? (cur | bit) : (cur & ~bit);
	} while (!track.userFxEnabledMask.compare_exchange_weak(cur, next));

	applyEffectiveBypasses(track);
}

void GlitchSequencerEngine::syncUserMaskFromDsp(TrackData &track)
{
	int mask = 0;
	if (!track.gate.isBypassed())
		mask |= effectBit(GlitchEffectType::Gate);
	if (!track.filter.isBypassed())
		mask |= effectBit(GlitchEffectType::Filter);
	if (!track.chorus.isBypassed())
		mask |= effectBit(GlitchEffectType::Chorus);
	if (!track.phaser.isBypassed())
		mask |= effectBit(GlitchEffectType::Phaser);
	if (!track.flanger.isBypassed())
		mask |= effectBit(GlitchEffectType::Flanger);
	if (!track.bitCrusher.isBypassed())
		mask |= effectBit(GlitchEffectType::BitCrusher);
	if (!track.distortion.isBypassed())
		mask |= effectBit(GlitchEffectType::Distortion);

	track.userFxEnabledMask.store(mask);
}
void GlitchSequencerEngine::setSequencerActive(TrackData &track, bool shouldBeActive)
{
	if (shouldBeActive == track.glitchSequencerActive.load())
		return;

	if (shouldBeActive)
	{
		const int seqIdx = juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load());
		track.glitchOwnedMask.store(rebuildOwnedEffectsMask(track.glitchSequences[seqIdx]));
		track.glitchReservedMask.store(computeReservedMask(track));
		track.glitchFxEnabledMask.store(0);
		track.lastTriggeredGlitchStep.store(-1);
		track.metaStepSilent.store(false);
		track.glitchSequencerActive.store(true);
	}
	else
	{
		const int owned = track.glitchOwnedMask.load();

		track.glitchFxEnabledMask.store(0);
		track.glitchOwnedMask.store(0);
		track.glitchReservedMask.store(0);
		track.metaStepSilent.store(false);
		track.glitchSequencerActive.store(false);

		restoreMix(track, GlitchEffectType::Chorus);
		restoreMix(track, GlitchEffectType::Phaser);
		restoreMix(track, GlitchEffectType::Flanger);
		restoreMix(track, GlitchEffectType::BitCrusher);

		clearTransportEffects(track, owned);
	}

	applyEffectiveBypasses(track);
}

GlitchEffectType GlitchSequencerEngine::pickRandomEffect(TrackData &track)
{
	const int count = static_cast<int>(GlitchEffectType::Random) - static_cast<int>(GlitchEffectType::Reverse);
	const int idx = track.glitchRandom.nextInt(count);
	return static_cast<GlitchEffectType>(static_cast<int>(GlitchEffectType::Reverse) + idx);
}

void GlitchSequencerEngine::triggerStep(TrackData &track, GlitchEffectType type, double hostBpm)
{
	const int owned = track.glitchOwnedMask.load();

	if (type == GlitchEffectType::Random)
		type = pickRandomEffect(track);

	const int bit = (type == GlitchEffectType::None) ? 0 : effectBit(type);

	track.glitchFxEnabledMask.store(bit & kBypassableMask);

	const int mixTargets[] = {(int)GlitchEffectType::Chorus, (int)GlitchEffectType::Phaser,
	                          (int)GlitchEffectType::Flanger, (int)GlitchEffectType::BitCrusher};

	for (int t : mixTargets)
	{
		const auto fx = static_cast<GlitchEffectType>(t);
		if ((owned & effectBit(fx)) == 0)
			continue;

		if (type == fx)
			forceMix(track, fx);
		else
			restoreMix(track, fx);
	}

	applyEffectiveBypasses(track);

	if (owned & effectBit(GlitchEffectType::Reverse))
		track.reverseActive.store(type == GlitchEffectType::Reverse);
	if (owned & effectBit(GlitchEffectType::TransientScatter))
		track.transientScatterActive.store(type == GlitchEffectType::TransientScatter);

	if (type == GlitchEffectType::BeatRepeat)
		startBeatRepeat(track, hostBpm);
	else if (owned & effectBit(GlitchEffectType::BeatRepeat))
		stopBeatRepeat(track);
}

int GlitchSequencerEngine::rebuildOwnedEffectsMask(const GlitchSequence &seq)
{
	int mask = 0;
	const int n = seq.getNumSteps();

	for (int s = 0; s < n; ++s)
	{
		const auto t = seq.getStep(s);
		if (t == GlitchEffectType::None)
			continue;

		if (t == GlitchEffectType::Random)
		{
			for (int i = static_cast<int>(GlitchEffectType::Reverse); i < static_cast<int>(GlitchEffectType::Random);
			     ++i)
				mask |= effectBit(static_cast<GlitchEffectType>(i));
		}
		else
			mask |= effectBit(t);
	}

	return mask;
}

void GlitchSequencerEngine::processBlock(TrackData &track, double ppqPosition, double hostBpm, double /*sampleRate*/,
                                         int /*numSamples*/)
{
	if (!track.glitchSequencerActive.load() || hostBpm <= 0.0)
		return;

	auto &meta = track.glitchMetaSequence;
	const bool metaOn = !meta.isBypassed() && meta.getNumSteps() > 0;

	int seqIdx = juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load());
	auto *seq = &track.glitchSequences[seqIdx];

	int numSteps = seq->getNumSteps();
	if (numSteps <= 0)
		return;

	double stepLen = seq->getStepLength() * 0.25;
	if (stepLen <= 0.0)
		return;

	if (!metaOn)
	{
		track.metaCurrentStep.store(-1);
		track.glitchCycleStartPpq.store(-1.0);
		track.metaStepSilent.store(false);

		int currentStep = static_cast<int>(std::floor(ppqPosition / stepLen)) % numSteps;
		if (currentStep < 0)
			currentStep += numSteps;

		advanceStepIfNeeded(track, *seq, currentStep, hostBpm);
		return;
	}

	double cycleStart = track.glitchCycleStartPpq.load();
	const double cycleLen = numSteps * stepLen;

	if (cycleStart < 0.0 || ppqPosition < cycleStart)
	{
		cycleStart = std::floor(ppqPosition / cycleLen) * cycleLen;
		track.glitchCycleStartPpq.store(cycleStart);
		track.metaCurrentStep.store(0);
		applyMetaStep(track, 0);

		seqIdx = juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load());
		seq = &track.glitchSequences[seqIdx];
		numSteps = seq->getNumSteps();
		stepLen = seq->getStepLength() * 0.25;
		if (numSteps <= 0 || stepLen <= 0.0)
			return;
	}

	if (ppqPosition >= cycleStart + cycleLen)
	{
		const int nextMeta = (track.metaCurrentStep.load() + 1) % meta.getNumSteps();
		track.metaCurrentStep.store(nextMeta);
		track.glitchCycleStartPpq.store(cycleStart + cycleLen);
		track.lastTriggeredGlitchStep.store(-1);
		applyMetaStep(track, nextMeta);

		seqIdx = juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load());
		seq = &track.glitchSequences[seqIdx];
		numSteps = seq->getNumSteps();
		stepLen = seq->getStepLength() * 0.25;
		if (numSteps <= 0 || stepLen <= 0.0)
			return;

		cycleStart = track.glitchCycleStartPpq.load();
	}

	if (track.metaStepSilent.load())
	{
		track.lastTriggeredGlitchStep.store(-1);
		return;
	}

	int currentStep = static_cast<int>(std::floor((ppqPosition - cycleStart) / stepLen));
	currentStep = juce::jlimit(0, numSteps - 1, currentStep);

	advanceStepIfNeeded(track, *seq, currentStep, hostBpm);
}

void GlitchSequencerEngine::applyMetaStep(TrackData &track, int metaStep)
{
	const int seqNumber = track.glitchMetaSequence.getStep(metaStep);
	const int oldOwned = track.glitchOwnedMask.load();

	if (seqNumber <= 0)
	{
		track.metaStepSilent.store(true);
		track.glitchFxEnabledMask.store(0);
		applyEffectiveBypasses(track);
		clearTransportEffects(track, oldOwned);
		return;
	}

	track.metaStepSilent.store(false);
	track.glitchReservedMask.store(computeReservedMask(track));

	const int target = juce::jlimit(0, 7, seqNumber - 1);
	if (target == track.currentGlitchSequenceIndex.load())
		return;

	clearTransportEffects(track, oldOwned);

	track.currentGlitchSequenceIndex.store(target);
	track.glitchOwnedMask.store(rebuildOwnedEffectsMask(track.glitchSequences[target]));
	track.glitchFxEnabledMask.store(0);
	track.lastTriggeredGlitchStep.store(-1);
	applyEffectiveBypasses(track);
}

void GlitchSequencerEngine::advanceStepIfNeeded(TrackData &track, GlitchSequence &seq, int currentStep, double hostBpm)
{
	if (currentStep == track.lastTriggeredGlitchStep.load())
		return;

	const int newOwned = rebuildOwnedEffectsMask(seq);
	const int oldOwned = track.glitchOwnedMask.load();

	if (newOwned != oldOwned)
	{
		track.glitchOwnedMask.store(newOwned);

		const int leaving = oldOwned & ~newOwned;
		if (leaving & effectBit(GlitchEffectType::Reverse))
			track.reverseActive.store(false);
		if (leaving & effectBit(GlitchEffectType::TransientScatter))
			track.transientScatterActive.store(false);
		if (leaving & effectBit(GlitchEffectType::BeatRepeat))
			stopBeatRepeat(track);
	}

	track.glitchReservedMask.store(computeReservedMask(track));

	track.lastTriggeredGlitchStep.store(currentStep);
	triggerStep(track, seq.getStep(currentStep), hostBpm);
}

void GlitchSequencerEngine::clearTransportEffects(TrackData &track, int mask)
{
	if (mask & effectBit(GlitchEffectType::Reverse))
		track.reverseActive.store(false);
	if (mask & effectBit(GlitchEffectType::TransientScatter))
		track.transientScatterActive.store(false);
	if (mask & effectBit(GlitchEffectType::BeatRepeat))
		stopBeatRepeat(track);
}

void GlitchSequencerEngine::forceMix(TrackData &track, GlitchEffectType type)
{
	const int bit = effectBit(type);
	if (track.glitchForcedMixMask.load() & bit)
		return;

	switch (type)
	{
	case GlitchEffectType::Chorus:
		track.mixSnapshotBeforeGlitch[0] = track.chorus.getMix();
		track.chorus.setMix(1.0f);
		break;
	case GlitchEffectType::Phaser:
		track.mixSnapshotBeforeGlitch[1] = track.phaser.getMix();
		track.phaser.setMix(1.0f);
		break;
	case GlitchEffectType::Flanger:
		track.mixSnapshotBeforeGlitch[2] = track.flanger.getMix();
		track.flanger.setMix(1.0f);
		break;
	case GlitchEffectType::BitCrusher:
		track.mixSnapshotBeforeGlitch[3] = track.bitCrusher.getMix();
		track.bitCrusher.setMix(1.0f);
		break;
	default:
		return;
	}

	track.glitchForcedMixMask.store(track.glitchForcedMixMask.load() | bit);
}

void GlitchSequencerEngine::restoreMix(TrackData &track, GlitchEffectType type)
{
	const int bit = effectBit(type);
	if ((track.glitchForcedMixMask.load() & bit) == 0)
		return;

	switch (type)
	{
	case GlitchEffectType::Chorus:
		track.chorus.setMix(track.mixSnapshotBeforeGlitch[0]);
		break;
	case GlitchEffectType::Phaser:
		track.phaser.setMix(track.mixSnapshotBeforeGlitch[1]);
		break;
	case GlitchEffectType::Flanger:
		track.flanger.setMix(track.mixSnapshotBeforeGlitch[2]);
		break;
	case GlitchEffectType::BitCrusher:
		track.bitCrusher.setMix(track.mixSnapshotBeforeGlitch[3]);
		break;
	default:
		return;
	}

	track.glitchForcedMixMask.store(track.glitchForcedMixMask.load() & ~bit);
}

int GlitchSequencerEngine::computeReservedMask(TrackData &track)
{
	auto &meta = track.glitchMetaSequence;

	if (meta.isBypassed())
		return rebuildOwnedEffectsMask(
		    track.glitchSequences[juce::jlimit(0, 7, track.currentGlitchSequenceIndex.load())]);

	int mask = 0;
	const int n = meta.getNumSteps();

	for (int i = 0; i < n; ++i)
	{
		const int seqNumber = meta.getStep(i);
		if (seqNumber <= 0)
			continue;

		const int idx = juce::jlimit(0, 7, seqNumber - 1);
		mask |= rebuildOwnedEffectsMask(track.glitchSequences[idx]);
	}

	return mask;
}