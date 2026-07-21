#include "TrackManager.h"
#include "PluginProcessor.h"
#include "TrackData.h"

TrackManager::TrackManager(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

void TrackManager::prepareTrack(TrackData &track)
{
	track.playGate.reset(currentSampleRate, JumpSmoother::kMaxLength / currentSampleRate);

	if (!track.isPrepared.load())
	{
		track.delaySendProcessor.prepare(currentSampleRate, currentMaxBlockSize);
		track.reverbSendProcessor.prepare(currentSampleRate, currentMaxBlockSize);

		track.filter.setMode(juce::dsp::LadderFilterMode::HPF12);
		track.filter.setSamplingRate(currentSampleRate);
		track.filter.setCutoffFrequency(Obsidian::FILTER_CUT);
		track.filter.setResonance(Obsidian::FILTER_RES);
		track.filter.setDrive(Obsidian::FILTER_DRIVE);
		track.filter.setBypassed(Obsidian::FILTER_BYPASSED);

		track.compressor.setThreshold(Obsidian::COMPRESSOR_THRESHOLD);
		track.compressor.setRatio(Obsidian::COMPRESSOR_RATIO);
		track.compressor.setAttack(Obsidian::COMPRESSOR_ATTACK);
		track.compressor.setRelease(Obsidian::COMPRESSOR_RELEASE);
		track.compressor.setMakeUpGain(Obsidian::COMPRESSOR_MAKEUP_GAIN);
		track.compressor.setBypassed(Obsidian::COMPRESSOR_BYPASSED);

		track.limiter.setThreshold(Obsidian::LIMITER_THRESHOLD);
		track.limiter.setRelease(Obsidian::LIMITER_RELEASE);
		track.limiter.setMakeUpGain(Obsidian::LIMITER_MAKEUP_GAIN);
		track.limiter.setBypassed(Obsidian::LIMITER_BYPASSED);

		track.chorus.setRate(Obsidian::CHORUS_RATE);
		track.chorus.setDepth(Obsidian::CHORUS_DEPTH);
		track.chorus.setCentre(Obsidian::CHORUS_CENTRE);
		track.chorus.setFeedback(Obsidian::CHORUS_FEEDBACK);
		track.chorus.setMix(Obsidian::CHORUS_MIX);
		track.chorus.setBypassed(Obsidian::CHORUS_BYPASSED);

		track.phaser.setRate(Obsidian::PHASER_RATE);
		track.phaser.setDepth(Obsidian::PHASER_DEPTH);
		track.phaser.setCentre(Obsidian::PHASER_CENTRE);
		track.phaser.setFeedback(Obsidian::PHASER_FEEDBACK);
		track.phaser.setMix(Obsidian::PHASER_MIX);
		track.phaser.setBypassed(Obsidian::PHASER_BYPASSED);

		track.flanger.setRate(Obsidian::FLANGER_RATE);
		track.flanger.setDepth(Obsidian::FLANGER_DEPTH);
		track.flanger.setCentre(Obsidian::FLANGER_CENTRE);
		track.flanger.setFeedback(Obsidian::FLANGER_FEEDBACK);
		track.flanger.setMix(Obsidian::FLANGER_MIX);
		track.flanger.setBypassed(Obsidian::FLANGER_BYPASSED);

		track.bitCrusher.setBitDepth(Obsidian::BITCRUSHER_BIT_DEPTH);
		track.bitCrusher.setSampleRateReduction(Obsidian::BITCRUSHER_SAMPLE_RATE_REDUCTION);
		track.bitCrusher.setMix(Obsidian::BITCRUSHER_MIX);
		track.bitCrusher.setBypassed(Obsidian::BITCRUSHER_BYPASSED);

		track.distortion.setPre(Obsidian::DISTORTION_PRE);
		track.distortion.setPost(Obsidian::DISTORTION_POST);
		track.distortion.setCut(Obsidian::DISTORTION_CUT);
		track.distortion.setType(Obsidian::distortionType::soft);
		track.distortion.setBypassed(Obsidian::DISTORTION_BYPASSED);

		track.equalizer.setBypassed(Obsidian::EQ_BYPASSED);

		track.isPrepared.store(true);

		juce::dsp::ProcessSpec spec = juce::dsp::ProcessSpec();
		spec.maximumBlockSize = static_cast<juce::uint32>(currentMaxBlockSize);
		spec.numChannels = 2;
		spec.sampleRate = currentSampleRate;

		track.distortion.prepare(spec);
		track.equalizer.prepare(spec);
		track.filter.prepare(spec);
		track.chorus.prepare(spec);
		track.phaser.prepare(spec);
		track.flanger.prepare(spec);
		track.bitCrusher.prepare(spec);
		track.compressor.prepare(spec);
		track.limiter.prepare(spec);
	}
}

juce::String TrackManager::createTrack(const juce::String &name)
{
	juce::ScopedLock lock(tracksLock);
	std::fill(std::begin(usedSlots), std::end(usedSlots), false);
	for (const auto &pair : tracks)
	{
		if (pair.second->slotIndex >= 0 && pair.second->slotIndex < Obsidian::MAX_TRACKS)
		{
			usedSlots[pair.second->slotIndex] = true;
		}
	}

	auto track = std::make_unique<TrackData>();
	track->trackName = name + " " + juce::String(tracks.size() + 1);
	track->midiNote = 60 + static_cast<int>(tracks.size());
	juce::String trackId = track->trackId;
	juce::String stdId = trackId.toStdString();
	track->slotIndex = findFreeSlot();

	if (track->slotIndex != -1)
		usedSlots[track->slotIndex] = true;
	if (track->slotIndex == 0)
		track->isSelected.store(true);

	if (audioPrepared)
		prepareTrack(*track);

	tracks[stdId] = std::move(track);
	trackOrder.push_back(stdId);
	publishSlotRegistry();
	return trackId;
}

void TrackManager::addTrack(const juce::String &trackId, std::unique_ptr<TrackData> track)
{
	const juce::ScopedLock sl(tracksLock);

	if (audioPrepared && track)
		prepareTrack(*track);

	tracks[trackId] = std::move(track);
	trackOrder.push_back(trackId);
	publishSlotRegistry();
}

TrackData *TrackManager::getTrack(const juce::String &trackId)
{
	juce::ScopedLock lock(tracksLock);
	auto it = tracks.find(trackId.toStdString());
	return (it != tracks.end()) ? it->second.get() : nullptr;
}

std::vector<juce::String> TrackManager::getAllTrackIds() const
{
	juce::ScopedLock lock(tracksLock);
	std::vector<juce::String> ids;
	for (const auto &stdId : trackOrder)
		if (tracks.count(stdId))
			ids.push_back(juce::String(stdId));

	return ids;
}

void TrackManager::prepareSends(double sampleRate, int maxBlockSize)
{
	const juce::ScopedLock sl(tracksLock);
	currentSampleRate = sampleRate;
	currentMaxBlockSize = maxBlockSize;
	audioPrepared = true;

	perTrackFxBuffer.setSize(2, maxBlockSize, false, false, true);
	tempMixBuffer.setSize(2, maxBlockSize, false, false, true);
	tempIndividualBuffer.setSize(2, maxBlockSize, false, false, true);

	int interval = static_cast<int>(sampleRate * 0.05);
	for (const auto &pair : tracks)
	{
		if (pair.second)
		{
			pair.second->meterUpdateInterval = interval;
			prepareTrack(*pair.second);
		}
	}
}

void TrackManager::processPerTrackDelays(std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                         juce::AudioBuffer<float> &mainOutput, double hostBpm,
                                         DelaySend::TimeDivision division, float feedback, DelaySend::Mode mode,
                                         int numSamples)
{
	if (!audioPrepared)
		return;
	if (perTrackFxBuffer.getNumSamples() < numSamples)
		perTrackFxBuffer.setSize(2, numSamples, false, false, true);

	const juce::ScopedLock sl(tracksLock);

	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it == tracks.end() || !it->second)
			continue;

		TrackData *track = it->second.get();
		const int slot = track->slotIndex;
		if (slot < 0 || slot >= (int)individualOutputs.size())
			continue;

		auto &trackBuffer = individualOutputs[slot];
		if (trackBuffer.getNumChannels() < 2)
			continue;

		track->delaySendProcessor.setBpm(hostBpm);
		track->delaySendProcessor.setTimeDivision(division);
		track->delaySendProcessor.setFeedback(feedback);
		track->delaySendProcessor.setMode(mode);

		const float sendLevel = track->delaySend.load();

		perTrackFxBuffer.clear(0, numSamples);
		if (sendLevel > 0.0001f)
			for (int ch = 0; ch < 2; ++ch)
				perTrackFxBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, sendLevel);

		track->delaySendProcessor.process(perTrackFxBuffer, 0, numSamples);

		for (int ch = 0; ch < 2; ++ch)
			trackBuffer.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);

		for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
			mainOutput.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);
	}
}

void TrackManager::processPerTrackReverbs(std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                          juce::AudioBuffer<float> &mainOutput, float size, float damping, float width,
                                          float mix, int numSamples)
{
	if (!audioPrepared)
		return;

	if (perTrackFxBuffer.getNumSamples() < numSamples)
		perTrackFxBuffer.setSize(2, numSamples, false, false, true);

	const juce::ScopedLock sl(tracksLock);

	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it == tracks.end() || !it->second)
			continue;

		TrackData *track = it->second.get();
		const int slot = track->slotIndex;
		if (slot < 0 || slot >= (int)individualOutputs.size())
			continue;

		auto &trackBuffer = individualOutputs[slot];
		if (trackBuffer.getNumChannels() < 2)
			continue;

		track->reverbSendProcessor.setSize(size);
		track->reverbSendProcessor.setDamping(damping);
		track->reverbSendProcessor.setWidth(width);
		track->reverbSendProcessor.setMix(mix);

		const float sendLevel = track->reverbSend.load();

		perTrackFxBuffer.clear(0, numSamples);
		if (sendLevel > 0.0001f)
		{
			for (int ch = 0; ch < 2; ++ch)
				perTrackFxBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, sendLevel);
		}

		track->reverbSendProcessor.process(perTrackFxBuffer, 0, numSamples);

		for (int ch = 0; ch < 2; ++ch)
			trackBuffer.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);

		for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
			mainOutput.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);
	}
}

void TrackManager::publishSlotRegistry()
{
	std::array<TrackData *, Obsidian::MAX_TRACKS> next{};
	for (const auto &pair : tracks)
	{
		auto *t = pair.second.get();
		if (t && t->slotIndex >= 0 && t->slotIndex < Obsidian::MAX_TRACKS)
			next[(size_t)t->slotIndex] = t;
	}
	for (size_t i = 0; i < (size_t)Obsidian::MAX_TRACKS; ++i)
		slotRegistry[i].store(next[i], std::memory_order_release);
}

void TrackManager::renderAllTracks(juce::AudioBuffer<float> &outputBuffer,
                                   std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                   juce::AudioBuffer<float> &previewOutput, const float pairPrev[4],
                                   const float pairCurrent[4], float globalPrev, float globalCurrent, int curveMode,
                                   double sampleRate, bool useCrossfader)
{
	const int numSamples = outputBuffer.getNumSamples();
	if (tempMixBuffer.getNumSamples() < numSamples)
	{
		tempMixBuffer.setSize(2, numSamples, false, false, true);
		tempIndividualBuffer.setSize(2, numSamples, false, false, true);
	}
	bool anyTrackSolo = false;
	forEachTrackAudio(
	    [&](TrackData *t)
	    {
		    if (t->isSolo.load())
			    anyTrackSolo = true;
	    });
	outputBuffer.clear();
	for (auto &buffer : individualOutputs)
		buffer.clear();

	forEachTrackAudio(
	    [&](TrackData *track)
	    {
		    auto &currentPage = track->getCurrentPage();
		    if (track->isEnabled.load() && currentPage.numSamples > 0 && track->slotIndex >= 0 &&
		        track->slotIndex < individualOutputs.size())
		    {
			    int bufferIndex = track->slotIndex;
			    tempMixBuffer.clear(0, 0, numSamples);
			    tempMixBuffer.clear(1, 0, numSamples);
			    tempIndividualBuffer.clear(0, 0, numSamples);
			    tempIndividualBuffer.clear(1, 0, numSamples);
			    renderSingleTrack(*track, tempMixBuffer, tempIndividualBuffer, numSamples, sampleRate);

			    track->consoleChannel.process(tempIndividualBuffer, 0, numSamples);

			    for (int ch = 0; ch < std::min(tempMixBuffer.getNumChannels(), tempIndividualBuffer.getNumChannels());
			         ++ch)
				    tempMixBuffer.copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);

			    bool isA = track->isDeckA();
			    constexpr float kUnityHeadroom = 0.707f;
			    float trackVol = track->volume.load();

			    float deckGainStart = useCrossfader ? 1.0f : trackVol * kUnityHeadroom;
			    float deckGainEnd = useCrossfader ? 1.0f : trackVol * kUnityHeadroom;
			    int pairIdx = track->getPairIndex();
			    if (pairIdx >= 0 && pairIdx < Obsidian::MAX_CROSSFADER_PAIR && useCrossfader)
			    {
				    float pairXfStart = pairPrev[pairIdx];
				    float globalXfStart = globalPrev;
				    float pairGainStart = applyCrossfadeCurve(pairXfStart, isA, curveMode);
				    float globalGainStart = applyCrossfadeCurve(globalXfStart, isA, curveMode);
				    deckGainStart = pairGainStart * globalGainStart;

				    float pairXfEnd = pairCurrent[pairIdx];
				    float globalXfEnd = globalCurrent;
				    float pairGainEnd = applyCrossfadeCurve(pairXfEnd, isA, curveMode);
				    float globalGainEnd = applyCrossfadeCurve(globalXfEnd, isA, curveMode);
				    deckGainEnd = pairGainEnd * globalGainEnd;
			    }

			    bool shouldHearTrack = !track->isMuted.load() && (!anyTrackSolo || track->isSolo.load());

			    if (track->isPreviewMode.load())
			    {
				    if (previewOutput.getNumChannels() >= 2)
					    for (int ch = 0; ch < previewOutput.getNumChannels(); ++ch)
						    previewOutput.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
				    else
					    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
						    outputBuffer.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
			    }
			    else
			    {
				    if (shouldHearTrack)
					    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
						    outputBuffer.addFromWithRamp(ch, 0, tempMixBuffer.getReadPointer(ch), numSamples,
						                                 deckGainStart, deckGainEnd);
				    for (int ch = 0; ch < std::min(2, individualOutputs[bufferIndex].getNumChannels()); ++ch)
				    {
					    if (!shouldHearTrack)
					    {
						    individualOutputs[bufferIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);
						    individualOutputs[bufferIndex].applyGain(ch, 0, numSamples, 0.0f);
					    }
					    else
					    {
						    individualOutputs[bufferIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);
						    individualOutputs[bufferIndex].applyGainRamp(ch, 0, numSamples, deckGainStart, deckGainEnd);
					    }
				    }
			    }
		    }
	    });
}

void TrackManager::loadAudioFileForPage(TrackData *track, int pageIndex, const juce::File &audioFile)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto &page = track->pages[pageIndex];

	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();

	std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
	if (!reader)
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	int numChannels = reader->numChannels;
	int numSamples = static_cast<int>(reader->lengthInSamples);

	if (numSamples <= 0)
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	page.audioBuffer.setSize(2, numSamples, false, true, true);
	page.audioBuffer.clear();

	if (!reader->read(&page.audioBuffer, 0, numSamples, 0, true, true))
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	if (numChannels == 1)
		page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);

	page.numSamples = numSamples;
	page.sampleRate = reader->sampleRate;
	page.isLoaded = true;
	page.isLoading = false;

	float maxSample = 0.0f;
	for (int ch = 0; ch < page.audioBuffer.getNumChannels(); ++ch)
	{
		auto *channelData = page.audioBuffer.getReadPointer(ch);
		for (int i = 0; i < page.audioBuffer.getNumSamples(); ++i)
			maxSample = std::max(maxSample, std::abs(channelData[i]));
	}
}

size_t TrackManager::getNumTracks() const
{
	const juce::ScopedLock sl(tracksLock);
	return tracks.size();
}

void TrackManager::clearAllTracks()
{
	{
		const juce::ScopedLock sl(tracksLock);
		for (auto &slot : slotRegistry)
			slot.store(nullptr, std::memory_order_release);
	}
	juce::Thread::sleep(50);
	const juce::ScopedLock sl(tracksLock);
	tracks.clear();
	trackOrder.clear();
}

void TrackManager::forEachTrack(std::function<void(const juce::String &, const TrackData *)> callback) const
{
	const juce::ScopedLock sl(tracksLock);
	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it != tracks.end())
			callback(id, it->second.get());
	}
}

void TrackManager::forEachTrack(std::function<void(const TrackData *)> callback) const
{
	const juce::ScopedLock sl(tracksLock);
	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it != tracks.end())
			callback(it->second.get());
	}
}

bool TrackManager::isSlotUsed(int index) const
{
	const juce::ScopedLock sl(tracksLock);
	if (index >= 0 && index < usedSlots.size())
		return usedSlots[index];
	return false;
}

void TrackManager::setSlotUsed(int index, bool used)
{
	const juce::ScopedLock sl(tracksLock);
	if (index >= 0 && index < usedSlots.size())
		usedSlots[index] = used;
}

void TrackManager::resetAllSlots()
{
	const juce::ScopedLock sl(tracksLock);
	usedSlots.fill(false);
}

int TrackManager::findFreeSlot()
{
	std::vector<bool> actualUsage(8, false);
	for (const auto &pair : tracks)
	{
		const auto &track = pair.second;
		if (track->slotIndex >= 0 && track->slotIndex < Obsidian::MAX_TRACKS)
			actualUsage[track->slotIndex] = true;
	}

	for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
	{
		if (!usedSlots[i])
			return i;
	}

	return -1;
}

void TrackManager::updateBeatRepeat(TrackData *track, int value, double hostBpm, double repeatDuration)
{
	if (track->randomBeatRepeatInterval.load() != value)
	{
		track->randomBeatRepeatInterval.store(value);

		if (track->beatRepeatActive.load())
		{
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			const auto &page = track->getCurrentPage();
			double repeatDurationSamples = repeatDuration * page.sampleRate;

			const double startAbs = juce::jlimit(0.0, (double)page.numSamples - 1.0, page.loopStart * page.sampleRate);
			const double endAbs = juce::jlimit(startAbs + 1.0, (double)page.numSamples, page.loopEnd * page.sampleRate);

			const double anchorAbs = track->originalReadPosition.load();
			const double GATE_SAMPLES = 64.0;
			const bool rev = track->reverseActive.load();

			if (rev)
			{
				double newStart = std::max(anchorAbs - repeatDurationSamples, startAbs);
				if (newStart + GATE_SAMPLES < anchorAbs)
					newStart += GATE_SAMPLES;
				if (newStart >= anchorAbs)
					newStart = anchorAbs - 1.0;
				track->beatRepeatStartPosition.store(newStart);
			}
			else
			{
				double newEnd = std::min(anchorAbs + repeatDurationSamples, endAbs);
				if (newEnd - GATE_SAMPLES > anchorAbs)
					newEnd -= GATE_SAMPLES;
				if (newEnd <= anchorAbs)
					newEnd = anchorAbs + 1.0;
				track->beatRepeatEndPosition.store(newEnd);
			}
		}
	}
}

TrackManager::PageInfo TrackManager::getPageInfo(const TrackPage &page, double sampleRate) const
{
	return PageInfo{page,
	                &page.audioBuffer,
	                page.numSamples,
	                sampleRate,
	                page.loopStart,
	                page.loopEnd,
	                page.originalBpm,
	                page.adsrAttack.load(),
	                page.adsrDecay.load(),
	                page.adsrSustain.load(),
	                page.adsrRelease.load()};
}

TrackManager::PlaybackRatioInfo TrackManager::getPlaybackRatio(const TrackPage &page) const
{
	double playbackRatio = 1.0;

	float pitchSemis = juce::jlimit(-12.0f, 12.0f, page.pitchSemitones.load());
	float fineCents = juce::jlimit(-50.0f, 50.0f, page.fineOffset.load());
	float totalSemis = pitchSemis + (fineCents / 100.0f);

	if (std::abs(totalSemis) > 0.001f)
		playbackRatio *= std::pow(2.0f, totalSemis / 12.0f);

	PlaybackRatioInfo info = PlaybackRatioInfo{playbackRatio, pitchSemis, fineCents, totalSemis};

	return info;
}

TrackManager::TrackInfo TrackManager::getTrackInfo(const TrackData &track, const TrackPage &page,
                                                   const PageInfo &pageInfo) const
{
	const float volume = juce::jlimit(0.0f, 1.0f, track.volume.load());
	const float pan = juce::jlimit(-1.0f, 1.0f, track.pan.load());

	float leftGain = 1.0f;
	float rightGain = 1.0f;
	if (pan < 0.0f)
		rightGain = 1.0f + pan;
	else if (pan > 0.0f)
		leftGain = 1.0f - pan;

	double currentPosition = track.readPosition.load();
	PlaybackRatioInfo playbackRatioInfo = getPlaybackRatio(page);

	double startSample = pageInfo.loopStartToUse * pageInfo.sampleRateToUse;
	double endSample = pageInfo.loopEndToUse * pageInfo.sampleRateToUse;

	startSample = juce::jlimit(0.0, (double)pageInfo.numSamplesToUse - 1, startSample);
	endSample = juce::jlimit(startSample + 1, (double)pageInfo.numSamplesToUse, endSample);

	return TrackInfo{volume,
	                 pan,
	                 leftGain,
	                 rightGain,
	                 currentPosition,
	                 playbackRatioInfo.playbackRatio,
	                 playbackRatioInfo.pitchSemis,
	                 playbackRatioInfo.fineCents,
	                 playbackRatioInfo.totalSemis,
	                 startSample,
	                 endSample};
}

TrackManager::FadeInfo TrackManager::getFadeInfo(TrackData &track, const TrackInfo &trackInfo,
                                                 const TrackPage &page) const
{
	DjIaVstProcessor::DawInfo dawInfo = audioProcessor.getDawInfo(trackInfo.playbackRatio);

	const bool beatRepeatActive = track.beatRepeatActive.load();
	const bool rev = track.reverseActive.load();
	const double sampleLength = trackInfo.endSample - trackInfo.startSample;

	const SequencerData &seqData = page.getCurrentSequence();
	const double totalSamplesPerSequence = dawInfo.samplesPerMeasureScaled * (double)seqData.numMeasures;

	const bool reverseWillStop = rev && (sampleLength < totalSamplesPerSequence * trackInfo.playbackRatio);
	const bool fadeOutArmed = rev ? reverseWillStop : true;

	const double distSource = rev ? trackInfo.currentPosition : (sampleLength - trackInfo.currentPosition);
	const double samplesUntilEnd = distSource / trackInfo.playbackRatio;

	return FadeInfo{beatRepeatActive,
	                track.beatRepeatEndPosition.load(),
	                track.beatRepeatStartPosition.load(),
	                fadeOutArmed,
	                samplesUntilEnd,
	                dawInfo.safetyFadeLength,
	                totalSamplesPerSequence};
}

void TrackManager::renderSingleTrack(TrackData &track, juce::AudioBuffer<float> &mixOutput,
                                     juce::AudioBuffer<float> &individualOutput, int numSamples,
                                     double sampleRate) const
{
	auto *safeCallback = parameterUpdateCallback.load();
	if (safeCallback != nullptr && *safeCallback)
	{
		int slot = track.slotIndex;
		if (slot != -1)
			(*safeCallback)(slot, &track);
	}

	auto handleEndOfPreview = [this, &track]()
	{
		if (track.isPreviewMode.load() && !track.previewEndPending.exchange(true))
		{
			if (onPreviewEnded)
				onPreviewEnded(track.trackId);
		}
	};

	auto stopTrackNow = [&]()
	{
		track.playGate.setCurrentAndTargetValue(0.0f);
		track.jumpSmoother.reset();
		track.isPlaying.store(false);
		track.readPosition.store(0.0);
		track.numSamplesAccPerSequence.store(0.0);
		handleEndOfPreview();
	};

	const auto &currentPage = track.getCurrentPage();
	PageInfo pageInfo = getPageInfo(currentPage, sampleRate);

	if (pageInfo.numSamplesToUse == 0 || !track.isPlaying.load() || !pageInfo.bufferToUse)
	{
		track.audioLevelLeft.store(0.0f);
		track.audioLevelRight.store(0.0f);
		track.meterAccumPeakLeft = 0.0f;
		track.meterAccumPeakRight = 0.0f;
		track.meterSampleCounter = 0;
		track.jumpSmoother.reset();
		track.seekPending.store(false);
		track.stopRequested.store(false);
		track.playGate.setCurrentAndTargetValue(0.0f);
		return;
	}

	if (track.playGate.getCurrentValue() <= 0.0f && track.playGate.getTargetValue() <= 0.0f)
	{
		track.playGate.setTargetValue(1.0f);
		track.filter.reset();
		track.equalizer.reset();
		track.chorus.reset();
		track.phaser.reset();
		track.flanger.reset();
		track.compressor.reset();
		track.limiter.reset();
		track.distortion.reset();
		track.bitCrusher.reset();
	}

	if (track.stopRequested.exchange(false))
		track.playGate.setTargetValue(0.0f);

	TrackInfo trackInfo = getTrackInfo(track, currentPage, pageInfo);

	if (track.seekPending.exchange(false))
	{
		const double signedStep = track.reverseActive.load() ? -trackInfo.playbackRatio : trackInfo.playbackRatio;
		track.jumpSmoother.trigger(trackInfo.startSample + track.seekFromPosition.load(), signedStep);
		trackInfo.currentPosition = track.readPosition.load();
		track.playGate.setTargetValue(1.0f);
	}

	double sectionLength = trackInfo.endSample - trackInfo.startSample;

	if (sectionLength < 100)
	{
		trackInfo.startSample = 0.0;
		trackInfo.endSample = pageInfo.numSamplesToUse;
		sectionLength = pageInfo.numSamplesToUse;
	}

	const float *leftChannel = pageInfo.bufferToUse->getReadPointer(0);
	const float *rightChannel =
	    pageInfo.bufferToUse->getNumChannels() > 1 ? pageInfo.bufferToUse->getReadPointer(1) : leftChannel;

	const int bufferSize = pageInfo.bufferToUse->getNumSamples();

	FadeInfo fadeInfo = getFadeInfo(track, trackInfo, currentPage);

	auto refreshSamplesUntilEnd = [&]()
	{
		const double distSource =
		    track.reverseActive.load() ? trackInfo.currentPosition : (sectionLength - trackInfo.currentPosition);
		fadeInfo.samplesUntilEnd = distSource / trackInfo.playbackRatio;
	};

	bool stopNow = false;

	for (int i = 0; i < numSamples; ++i)
	{
		if (track.playGate.getTargetValue() <= 0.0f && !track.playGate.isSmoothing())
		{
			stopNow = true;
			break;
		}

		double absolutePosition = trackInfo.startSample + trackInfo.currentPosition;

		if (fadeInfo.fadeOutArmed && !fadeInfo.beatRepeatActive && fadeInfo.samplesUntilEnd >= 0 &&
		    fadeInfo.samplesUntilEnd <= fadeInfo.safetyFadeLength)
			track.playGate.setTargetValue(0.0f);

		if (fadeInfo.beatRepeatActive)
		{
			const bool rev = track.reverseActive.load();
			const double zoneLenOut = (fadeInfo.beatRepeatEnd - fadeInfo.beatRepeatStart) / trackInfo.playbackRatio;
			const int fadeLen = (int)std::min<double>(JumpSmoother::kMaxLength, zoneLenOut);

			if (!rev && absolutePosition >= fadeInfo.beatRepeatEnd)
			{
				track.jumpSmoother.trigger(absolutePosition, trackInfo.playbackRatio, fadeLen);
				trackInfo.currentPosition = fadeInfo.beatRepeatStart - trackInfo.startSample;
				track.readPosition.store(trackInfo.currentPosition);
				absolutePosition = trackInfo.startSample + trackInfo.currentPosition;
				refreshSamplesUntilEnd();
			}
			else if (rev && absolutePosition <= fadeInfo.beatRepeatStart)
			{
				track.jumpSmoother.trigger(absolutePosition, -trackInfo.playbackRatio, fadeLen);
				trackInfo.currentPosition = (fadeInfo.beatRepeatEnd - 1.0) - trackInfo.startSample;
				track.readPosition.store(trackInfo.currentPosition);
				absolutePosition = trackInfo.startSample + trackInfo.currentPosition;
				refreshSamplesUntilEnd();
			}
		}

		if (absolutePosition >= trackInfo.endSample && !track.reverseActive.load())
		{
			stopNow = true;
			break;
		}

		if (absolutePosition >= pageInfo.numSamplesToUse && !track.reverseActive.load())
		{
			trackInfo.currentPosition = 0.0;
			absolutePosition = trackInfo.startSample;
			refreshSamplesUntilEnd();
		}

		if (track.reverseActive.load() && !fadeInfo.beatRepeatActive && absolutePosition <= trackInfo.startSample)
		{
			const double sequenceLengthSource = fadeInfo.totalSamplesPerSequence * trackInfo.playbackRatio;
			const double startReadPosition = std::min(sectionLength, sequenceLengthSource) - 1.0;

			if (sectionLength < sequenceLengthSource && track.readPosition.load() != startReadPosition)
			{
				stopNow = true;
				break;
			}

			track.jumpSmoother.trigger(absolutePosition, -trackInfo.playbackRatio);
			trackInfo.currentPosition = sequenceLengthSource - 1.0;
			track.readPosition.store(trackInfo.currentPosition);
			absolutePosition = trackInfo.startSample + trackInfo.currentPosition;
			refreshSamplesUntilEnd();
		}

		int sampleIndex = static_cast<int>(absolutePosition);
		if (sampleIndex >= bufferSize)
		{
			stopNow = true;
			break;
		}

		float adsrGain = getADSRGain(absolutePosition, trackInfo.startSample, sectionLength, pageInfo);
		const float oldAdsrGain =
		    track.jumpSmoother.isActive()
		        ? getADSRGain(track.jumpSmoother.position, trackInfo.startSample, sectionLength, pageInfo)
		        : 0.0f;

		prepareOutput(adsrGain, oldAdsrGain, leftChannel, rightChannel, absolutePosition, bufferSize, individualOutput,
		              track, i, trackInfo, fadeInfo);

		fadeInfo.samplesUntilEnd -= 1.0;
	}
	if (!stopNow)
		track.numSamplesAccPerSequence.store(track.numSamplesAccPerSequence.load() + numSamples);

	handleOutput(individualOutput, mixOutput, track, trackInfo.currentPosition, trackInfo.volume);

	if (stopNow)
		stopTrackNow();
}

void TrackManager::prepareOutput(float adsrGain, float oldAdsrGain, const float *leftChannel, const float *rightChannel,
                                 double absolutePosition, int bufferSize, juce::AudioSampleBuffer &individualOutput,
                                 TrackData &track, int i, TrackInfo &trackInfo, FadeInfo &fadeInfo) const
{
	float leftSample = interpolateLinear(leftChannel, absolutePosition, bufferSize) * adsrGain;
	float rightSample = interpolateLinear(rightChannel, absolutePosition, bufferSize) * adsrGain;

	applyJumpSmoothing(track, leftSample, rightSample, leftChannel, rightChannel, bufferSize, oldAdsrGain);

	float gainDb = juce::jlimit(-60.0f, 12.0f, track.getCurrentPage().gain.load());
	float gainLinear = std::pow(10.0f, gainDb / 20.0f);

	float driftCoeff = track.analogDriftCoeff.load();
	float driftGain = 1.0f;
	if (driftCoeff < 0.999f)
	{
		double phase = (track.readPosition.load() + i) * 0.0000045;
		float depth = 1.0f - driftCoeff;
		float slow = std::sin((float)phase) * std::sin((float)phase * 0.37f + 1.3f);
		driftGain = 1.0f - depth * 0.5f * (0.5f + 0.5f * slow);
	}

	const float commonGain = track.playGate.getNextValue() * gainLinear * driftGain;
	leftSample *= trackInfo.leftGain * commonGain;
	rightSample *= trackInfo.rightGain * commonGain;

	individualOutput.setSample(0, i, leftSample);
	individualOutput.setSample(1, i, rightSample);

	if (track.reverseActive.load())
		trackInfo.currentPosition -= trackInfo.playbackRatio;
	else
		trackInfo.currentPosition += trackInfo.playbackRatio;

	if (track.ownsTheoreticalTimeline())
	{
		double newTheoretical = track.theoreticalPosition.load() + trackInfo.playbackRatio;

		const double seqLen = fadeInfo.totalSamplesPerSequence;
		if (seqLen > 0.0 && newTheoretical >= seqLen)
			newTheoretical = std::fmod(newTheoretical, seqLen);

		track.theoreticalPosition.store(newTheoretical);
	}
}

void TrackManager::handleOutput(juce::AudioSampleBuffer &individualOutput, juce::AudioSampleBuffer &mixOutput,
                                TrackData &track, double &currentPosition, float volume) const
{
	auto block = juce::dsp::AudioBlock<float>(individualOutput);
	auto blockToUse = block.getSubBlock(0, individualOutput.getNumSamples());
	auto contextToUse = juce::dsp::ProcessContextReplacing<float>(blockToUse);

	track.distortion.process(contextToUse);
	track.bitCrusher.process(contextToUse);
	track.equalizer.process(contextToUse);
	track.filter.process(contextToUse);
	track.chorus.process(contextToUse);
	track.flanger.process(contextToUse);
	track.phaser.process(contextToUse);
	track.compressor.process(contextToUse);
	track.limiter.process(contextToUse);

	individualOutput.applyGain(volume);

	const int numSamplesProcessed = individualOutput.getNumSamples();
	const float *postLeft = individualOutput.getReadPointer(0);
	const float *postRight = individualOutput.getReadPointer(1);

	for (int i = 0; i < numSamplesProcessed; i++)
	{
		float absLeft = std::abs(postLeft[i]);
		float absRight = std::abs(postRight[i]);

		if (absLeft > track.meterAccumPeakLeft)
			track.meterAccumPeakLeft = absLeft;
		if (absRight > track.meterAccumPeakRight)
			track.meterAccumPeakRight = absRight;

		track.meterSampleCounter++;

		if (track.meterSampleCounter >= track.meterUpdateInterval)
		{
			track.audioLevelLeft.store(juce::jlimit(0.0f, 1.0f, track.meterAccumPeakLeft));
			track.audioLevelRight.store(juce::jlimit(0.0f, 1.0f, track.meterAccumPeakRight));

			track.meterAccumPeakLeft = 0.0f;
			track.meterAccumPeakRight = 0.0f;
			track.meterSampleCounter = 0;
		}

		mixOutput.addSample(0, i, individualOutput.getSample(0, i));
		mixOutput.addSample(1, i, individualOutput.getSample(1, i));
	}
	track.readPosition.store(currentPosition);
}

float TrackManager::interpolateLinear(const float *buffer, double position, int bufferSize) const
{
	int index = static_cast<int>(position);
	if (index < 0)
		return buffer[0];
	if (index >= bufferSize - 1)
		return buffer[bufferSize - 1];

	float fraction = static_cast<float>(position - index);
	return buffer[index] + fraction * (buffer[index + 1] - buffer[index]);
}

float TrackManager::getADSRGain(double absolutePosition, double startSample, double sectionLength, PageInfo &info) const
{
	float adsrGain = 1.f;
	double posInSection = (absolutePosition - startSample) / info.sampleRateToUse;
	double sectionDuration = sectionLength / info.sampleRateToUse;

	float totalADR = info.adsrAttack + info.adsrDecay + info.adsrRelease;
	float scale = 1.0f;
	if (totalADR > (float)sectionDuration * 0.95f)
		scale = (float)sectionDuration * 0.95f / totalADR;

	float a = info.adsrAttack * scale;
	float d = info.adsrDecay * scale;
	float r = info.adsrRelease * scale;
	double releaseStart = sectionDuration - (double)r;

	if (posInSection < 0.0)
		adsrGain = 0.0f;
	else if (a > 0.0f && posInSection < (double)a)
		adsrGain = (float)(posInSection / a);
	else if (d > 0.0f && posInSection < (double)(a + d))
	{
		float t = (float)((posInSection - a) / d);
		adsrGain = 1.0f - t * (1.0f - info.adsrSustain);
	}
	else if (posInSection < releaseStart)
		adsrGain = info.adsrSustain;
	else if (r > 0.0f && posInSection < sectionDuration)
	{
		float t = (float)((posInSection - releaseStart) / r);
		adsrGain = info.adsrSustain * (1.0f - t);
	}
	else
		adsrGain = 0.0f;

	adsrGain = juce::jlimit(0.0f, 1.0f, adsrGain);
	return adsrGain;
}

float TrackManager::applyCrossfadeCurve(float xfaderValue, bool isDeckA, int curveMode)
{
	float x = isDeckA ? (1.0f - xfaderValue) : xfaderValue;
	x = juce::jlimit(0.0f, 1.0f, x);

	switch (curveMode)
	{
	case 0:
		return x;

	case 1:
	{
		const float s = std::sin(x * juce::MathConstants<float>::halfPi);
		return s * s;
	}
	case 2:
	{
		if (x < 0.1f)
			return 0.0f;
		if (x > 0.9f)
			return 1.0f;

		const float remapped = (x - 0.1f) / 0.8f;
		const float s = std::sin(remapped * juce::MathConstants<float>::halfPi);
		return s * s;
	}
	default:
		return x;
	}
}

void TrackManager::ensureTransientsAnalyzed(TrackPage &page) const
{
	const int bufferSamples = page.audioBuffer.getNumSamples();
	const int nch = page.audioBuffer.getNumChannels();
	const int safeNumSamples = juce::jmin(page.numSamples, bufferSamples);

	if (page.transientsAnalyzedForNumSamples == page.numSamples && !page.transientPositions.empty())
		return;
	page.transientPositions.clear();
	page.transients.clear();
	page.transientsAnalyzedForNumSamples = page.numSamples;

	if (safeNumSamples <= 0 || nch == 0)
		return;

	const int windowSize = 512;
	const int hopSize = 256;

	if (safeNumSamples < windowSize)
		return;

	const float *dataL = page.audioBuffer.getReadPointer(0);
	const float *dataR = nch > 1 ? page.audioBuffer.getReadPointer(1) : dataL;

	std::vector<float> energy;
	for (int pos = 0; pos + windowSize <= safeNumSamples; pos += hopSize)
	{
		float sum = 0.0f;
		for (int i = 0; i < windowSize; ++i)
		{
			float s = juce::jmax(std::abs(dataL[pos + i]), std::abs(dataR[pos + i]));
			sum += s * s;
		}
		energy.push_back(std::sqrt(sum / windowSize));
	}

	const float riseThreshold = 2.f;
	const int minGapWindows = (int)(0.05 * page.sampleRate / hopSize);
	int lastTransientWindow = -minGapWindows;

	for (size_t i = 1; i < energy.size(); ++i)
	{
		if (energy[i] > energy[i - 1] * riseThreshold && energy[i] > 0.02f &&
		    (int)i - lastTransientWindow >= minGapWindows)
		{
			const int posSamples = (int)i * hopSize;

			float rise = energy[i] / (energy[i - 1] + 1.0e-6f);
			float peakEnergy = energy[i];
			float raw = std::log10(1.0f + rise) * peakEnergy;

			page.transientPositions.push_back(posSamples);
			page.transients.push_back({posSamples, raw});

			lastTransientWindow = (int)i;
		}
	}

	float maxStrength = 0.0f;
	for (auto &t : page.transients)
		maxStrength = juce::jmax(maxStrength, t.strength);
	if (maxStrength > 0.0f)
		for (auto &t : page.transients)
			t.strength /= maxStrength;
}

void TrackManager::applyJumpSmoothing(TrackData &track, float &left, float &right, const float *leftChannel,
                                      const float *rightChannel, int bufferSize, float oldAdsrGain) const
{
	auto &js = track.jumpSmoother;
	if (!js.isActive())
		return;

	const float oldWeight = (float)js.samplesRemaining / (float)js.length;
	const float oldL = interpolateLinear(leftChannel, js.position, bufferSize) * oldAdsrGain;
	const float oldR = interpolateLinear(rightChannel, js.position, bufferSize) * oldAdsrGain;
	left += (oldL - left) * oldWeight;
	right += (oldR - right) * oldWeight;

	js.position += js.step;
	--js.samplesRemaining;
}