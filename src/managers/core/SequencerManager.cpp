#include "SequencerManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "StateManager.h"
#include "TrackData.h"
#include "TrackManager.h"

SequencerManager::SequencerManager(DjIaVstProcessor &processor, TrackManager &trackManager)
    : audioProcessor(processor), trackManager(trackManager)
{
}

void SequencerManager::handlePageChange(const juce::String &parameterID)
{
	juce::String slotStr = parameterID.substring(4, 5);
	int slotNumber = slotStr.getIntValue();
	char pageChar = static_cast<char>(parameterID[parameterID.length() - 1]);
	int pageIndex = pageChar - 'A';
	if (slotNumber < 1 || slotNumber > 8 || pageIndex < 0 || pageIndex > 3)
		return;
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (track && track->slotIndex == (slotNumber - 1))
		{
			if (track->pages[pageIndex].numSamples == 0)
			{
				track->setCurrentPage(pageIndex);
				track->isPlaying.store(false);
				track->isCurrentlyPlaying.store(false);
				track->isArmed.store(false);
				track->isArmedToStop.store(false);
				track->readPosition.store(0.0);
				track->numSamplesAccPerSequence.store(0.0);

				juce::String playParam = "slot" + juce::String(slotNumber) + "Play";
				if (auto *p = audioProcessor.getParameterTreeState().getParameter(playParam))
					p->setValueNotifyingHost(0.0f);

				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slotNumber),
				                                                 MidiMapping::feedbackIdle);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber), pageIndex);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackSeq(slotNumber),
				                                                 track->pages[pageIndex].currentSequenceIndex);
				if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				{
					juce::Component::SafePointer<DjIaVstEditor> safeEditor(editor);
					juce::MessageManager::callAsync(
					    [safeEditor, trackId, pageIndex]()
					    {
						    if (safeEditor == nullptr)
							    return;
						    for (auto &trackComp : safeEditor->uiTrackManager->getTrackComponents())
						    {
							    if (trackComp->getTrackId() == trackId)
							    {
								    trackComp->performPageChange(pageIndex);
								    break;
							    }
						    }
					    });
				}
				return;
			}
			bool isPlaying = false;
			if (auto currentPlayHead = audioProcessor.getPlayHead())
			{
				if (auto positionInfo = currentPlayHead->getPosition())
					isPlaying = positionInfo->getIsPlaying();
			}
			if (!isPlaying || !track->isCurrentlyPlaying.load())
			{
				track->setCurrentPage(pageIndex);
				if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				{
					juce::Component::SafePointer<DjIaVstEditor> safeEditor(editor);
					juce::MessageManager::callAsync(
					    [safeEditor, trackId, pageIndex]()
					    {
						    if (safeEditor == nullptr)
							    return;
						    for (auto &trackComp : safeEditor->uiTrackManager->getTrackComponents())
						    {
							    if (trackComp->getTrackId() == trackId)
							    {
								    trackComp->performPageChange(pageIndex);
								    break;
							    }
						    }
					    });
				}
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber), pageIndex);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackSeq(slotNumber),
				                                                 track->pages[pageIndex].currentSequenceIndex);
			}
			else
			{
				track->pageChangePending.store(true);
				track->pendingPageIndex.store(pageIndex);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber),
				                                                 MidiMapping::feedbackPending);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber),
				                                                 80 + pageIndex);
				if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				{
					juce::Component::SafePointer<DjIaVstEditor> safeEditor(editor);
					juce::MessageManager::callAsync(
					    [safeEditor, trackId, pageIndex]()
					    {
						    if (safeEditor == nullptr)
							    return;
						    for (auto &trackComp : safeEditor->uiTrackManager->getTrackComponents())
						    {
							    if (trackComp->getTrackId() == trackId)
							    {
								    trackComp->startPagePendingBlink();
								    trackComp->updatePagesDisplay();
								    safeEditor->uiStatusManager->setStatusWithTimeout(
								        "Page " + juce::String((char)('A' + pageIndex)) +
								            " will switch at next measure",
								        3000);
								    break;
							    }
						    }
					    });
				}
			}
			break;
		}
	}
}

void SequencerManager::handleSequenceChange(int slotNum, int targetSequence)
{

	if (slotNum < 1 || slotNum > 8 || targetSequence < 1 || targetSequence > 8)
		return;

	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (track && track->slotIndex == (slotNum - 1))
		{
			auto &currentPage = track->getCurrentPage();
			currentPage.currentSequenceIndex = targetSequence - 1;
			juce::MessageManager::callAsync(
			    [this]()
			    {
				    if (audioProcessor.onUIUpdateNeeded)
					    audioProcessor.onUIUpdateNeeded();
			    });
			break;
		}
	}
}

void SequencerManager::handleSequencerPlayState(bool hostIsPlaying)
{
	if (isBypassed())
		return;

	if (hostIsPlaying && !wasPlaying.load())
	{
		internalSampleCounter.store(0);
		trackManager.forEachTrackAudio(
		    [&](TrackData *track)
		    {
			    auto &seqData = track->getCurrentSequencerData();
			    seqData.isPlaying = true;
			    seqData.currentStep = 0;
			    seqData.currentMeasure = 0;
			    seqData.stepAccumulator = 0.0;
			    track->customStepCounter = 0;
			    track->lastPpqPosition = -1.0;
		    });
	}
	else if (!hostIsPlaying && wasPlaying.load())
	{
		auto trackIds = trackManager.getAllTrackIds();
		trackManager.forEachTrackAudio(
		    [&](TrackData *track)
		    {
			    bool isArmed = false;
			    if (track->isCurrentlyPlaying.load())
				    isArmed = true;
			    auto &seqData = track->getCurrentSequencerData();
			    seqData.isPlaying = false;
			    track->setStop();
			    track->isArmed.store(isArmed);
			    track->isPlaying.store(false);
			    track->isCurrentlyPlaying.store(false);
			    track->readPosition.store(0.0);
			    track->numSamplesAccPerSequence.store(0.0);
			    track->limiter.resetReductionAmount();
			    seqData.currentStep = 0;
			    seqData.currentMeasure = 0;
			    seqData.stepAccumulator = 0.0;
			    track->customStepCounter = 0;
			    track->lastPpqPosition = -1.0;
			    audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
			                                                     isArmed ? MidiMapping::feedbackPending
			                                                             : MidiMapping::feedbackIdle);
		    });
		audioProcessor.needsUIUpdate.store(true);
	}
	else if (!hostIsPlaying && !wasPlaying.load())
	{
		trackManager.forEachTrackAudio(
		    [&](TrackData *track)
		    {
			    if (track->isCurrentlyPlaying.load())
			    {
				    auto &seqData = track->getCurrentSequencerData();
				    track->isCurrentlyPlaying.store(false);
				    track->limiter.resetReductionAmount();
				    track->readPosition.store(0.0);
				    track->numSamplesAccPerSequence.store(0.0);
				    seqData.currentStep = 0;
				    seqData.currentMeasure = 0;
				    seqData.stepAccumulator = 0.0;
				    track->customStepCounter = 0;
				    track->lastPpqPosition = -1.0;
				    seqData.isPlaying = false;
				    track->isArmed.store(false);
				    track->isPlaying.store(false);
				    audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
				                                                     MidiMapping::feedbackIdle);
			    }
		    });
	}
	audioProcessor.needsUIUpdate.store(true);
	wasPlaying.store(hostIsPlaying);
}

void SequencerManager::updateSequencers(bool hostIsPlaying, int numSamples)
{
	if (isBypassed())
		return;
	auto currentPlayHead = audioProcessor.getPlayHead();
	if (!currentPlayHead)
		return;
	auto positionInfo = currentPlayHead->getPosition();
	if (!positionInfo)
		return;
	auto ppqPosition = positionInfo->getPpqPosition();
	if (!ppqPosition.hasValue())
		return;

	double currentPpq = *ppqPosition;
	double stepInPpq = 0.25;

	double hostBpm = audioProcessor.getLastHostBpmForQuantization();
	if (hostBpm <= 0.0)
		hostBpm = 120.0;
	double sampleRate = audioProcessor.getSampleRate();
	double samplesPerPpq = (60.0 / hostBpm) * sampleRate;
	double bufferDurationInPpq = numSamples / samplesPerPpq;
	double ppqEndOfBuffer = currentPpq + bufferDurationInPpq;

	trackManager.forEachTrackAudio(
	    [&](TrackData *track)
	    {
		    double expectedPpqForNextStep = track->lastPpqPosition + stepInPpq;

		    bool shouldAdvanceStep = false;
		    if (track->lastPpqPosition < 0)
		    {
			    double totalStepsFromStart = currentPpq / stepInPpq;
			    track->customStepCounter = static_cast<int>(totalStepsFromStart);
			    track->lastPpqPosition = track->customStepCounter * stepInPpq;
			    shouldAdvanceStep = true;
		    }
		    else if (ppqEndOfBuffer >= expectedPpqForNextStep)
		    {
			    track->customStepCounter++;
			    track->lastPpqPosition = expectedPpqForNextStep;
			    shouldAdvanceStep = true;
		    }

		    if (shouldAdvanceStep)
		    {
			    handleAdvanceStep(track, hostIsPlaying);
			    if (onSequencerUpdateNeeded)
				    onSequencerUpdateNeeded(track->trackId);
		    }
	    });
}

void SequencerManager::addSequencerMidiMessage(const juce::MidiMessage &message)
{
	juce::ScopedLock lock(sequencerMidiLock);
	sequencerMidiBuffer.addEvent(message, 0);
}

void SequencerManager::handleAdvanceStep(TrackData *track, bool hostIsPlaying)
{
	int numerator = audioProcessor.getTimeSignatureNumerator();
	int denominator = audioProcessor.getTimeSignatureDenominator();

	int stepsPerBeat;
	if (denominator == 8)
		stepsPerBeat = 2;
	else if (denominator == 4)
		stepsPerBeat = 4;
	else if (denominator == 2)
		stepsPerBeat = 8;
	else
		stepsPerBeat = 4;

	auto &seqData = track->getCurrentSequencerData();
	int stepsPerMeasure = numerator * stepsPerBeat;
	int newStep = track->customStepCounter % stepsPerMeasure;
	int newMeasure = (track->customStepCounter / stepsPerMeasure) % seqData.numMeasures;

	if (track->transientScatterActive.load() && !track->beatRepeatActive.load() && hostIsPlaying &&
	    track->isCurrentlyPlaying.load() && track->customStepCounter % 2 == 0)
	{
		auto &page = track->getCurrentPage();
		TrackManager::PlaybackRatioInfo ratioInfo = trackManager.getPlaybackRatio(page);
		DjIaVstProcessor::DawInfo dawInfo = audioProcessor.getDawInfo(ratioInfo.playbackRatio);

		double startPos = getTransientScatterStartPosition(track);
		track->seekTo(startPos);
	}

	if (newMeasure == 0 && newStep == 0 && track->pageChangePending.load())
	{
		int targetPage = track->pendingPageIndex.load();
		int slotNumber = track->slotIndex + 1;
		if (targetPage >= 0 && targetPage < Obsidian::MAX_PAGES)
		{
			juce::MessageManager::callAsync(
			    [this, trackId = track->trackId, targetPage, slotNumber]()
			    {
				    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				    {
					    for (auto &trackComp : editor->uiTrackManager->getTrackComponents())
					    {
						    if (editor->isBeingDestroyed.load())
							    return;
						    if (trackComp->getTrackId() == trackId)
						    {
							    trackComp->performPageChange(targetPage);
							    break;
						    }
					    }
				    }
				    else
				    {
					    TrackData *t = trackManager.getTrack(trackId);
					    if (t)
					    {
						    t->setCurrentPage(targetPage);
						    t->pageChangePending.store(false);
						    t->pendingPageIndex.store(-1);
					    }
				    }
				    audioProcessor.getMidiManager().notifyPageChangedFeedback(slotNumber, targetPage);
			    });
		}
	}

	int safeMeasure = juce::jlimit(0, seqData.numMeasures - 1, newMeasure);
	int safeStep = juce::jlimit(0, stepsPerMeasure - 1, newStep);

	bool currentStepIsActive = seqData.steps[safeMeasure][safeStep];

	if (newMeasure == 0 && track->isArmed.load() && newStep == 0 && !track->isPlaying.load() && hostIsPlaying)
		track->pendingAction = TrackData::PendingAction::StartOnNextMeasure;

	if ((newMeasure == 0 && newStep == 0) && track->pendingAction != TrackData::PendingAction::None)
		executePendingAction(track);

	seqData.currentStep = newStep;
	seqData.currentMeasure = newMeasure;

	if (currentStepIsActive && track->isCurrentlyPlaying.load() && hostIsPlaying)
		triggerSequencerStep(track);
}

void SequencerManager::triggerSequencerStep(TrackData *track)
{
	if (isBypassed())
		return;

	auto &seqData = track->getCurrentSequencerData();
	int step = seqData.currentStep;
	int measure = seqData.currentMeasure;
	track->isArmed.store(false);

	if (seqData.steps[measure][step])
	{
		if (!track->beatRepeatActive.load())
		{
			if (track->isPlaying.load())
				track->seekTo(getStartReadPosition(track));
			else
				track->readPosition.store(getStartReadPosition(track));
			if (step == 0 && measure == 0)
				track->numSamplesAccPerSequence.store(0.0);
		}
		audioProcessor.addPlayingTrack(track->midiNote, track);
		juce::MidiMessage noteOn =
		    juce::MidiMessage::noteOn(1, track->midiNote, (juce::uint8)(seqData.velocities[measure][step] * 127));
		addSequencerMidiMessage(noteOn);
	}
}

void SequencerManager::setupBeatRepeatZone(TrackData *track, double hostBpm) const
{
	auto &currentPage = track->getCurrentPage();

	double repeatDuration = calculateBeatRepeatInterval(track->randomBeatRepeatInterval.load(), hostBpm);
	double repeatDurationSamples = repeatDuration * currentPage.sampleRate;

	const double startAbs =
	    juce::jlimit(0.0, (double)currentPage.numSamples - 1.0, currentPage.loopStart * currentPage.sampleRate);
	const double endAbs =
	    juce::jlimit(startAbs + 1.0, (double)currentPage.numSamples, currentPage.loopEnd * currentPage.sampleRate);

	double anchorAbs = startAbs + track->readPosition.load();
	anchorAbs = juce::jlimit(startAbs, endAbs - 1.0, anchorAbs);

	const double GATE_SAMPLES = 64.0;
	double zoneStart, zoneEnd;

	if (track->reverseActive.load())
	{
		zoneEnd = anchorAbs;
		zoneStart = std::max(anchorAbs - repeatDurationSamples, startAbs);
		if (zoneStart + GATE_SAMPLES < zoneEnd)
			zoneStart += GATE_SAMPLES;
	}
	else
	{
		zoneStart = anchorAbs;
		zoneEnd = std::min(anchorAbs + repeatDurationSamples, endAbs);
		if (zoneEnd - GATE_SAMPLES > zoneStart)
			zoneEnd -= GATE_SAMPLES;
	}

	track->originalReadPosition.store(anchorAbs);
	track->beatRepeatStartPosition.store(zoneStart);
	track->beatRepeatEndPosition.store(zoneEnd);
	track->readPosition.store(anchorAbs - startAbs);
}

void SequencerManager::checkBeatRepeatWithSampleCounter()
{
	double hostBpm = audioProcessor.getLastHostBpmForQuantization();
	int64_t currentHalfBeatNumber = getCurrentHalfBeatNumber(hostBpm);

	trackManager.forEachTrackAudio(
	    [&](TrackData *track)
	    {
		    checkQuantizedToggle(
		        track->beatRepeatPending, track->beatRepeatStopPending, track->pendingBeatNumber,
		        track->pendingStopBeatNumber, track->beatRepeatActive, currentHalfBeatNumber,
		        [this, track, hostBpm]()
		        {
			        if (track->randomRetriggerDurationEnabled.load())
			        {
				        int randomInterval = 1 + (rand() % 10);
				        track->randomBeatRepeatInterval.store(randomInterval);
				        juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + "BeatRepeatInterval";
				        auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
				        if (param)
				        {
					        float normalizedValue = (randomInterval - 1.0f) / 9.0f;
					        param->setValueNotifyingHost(normalizedValue);
				        }
			        }

			        if (track->isPlaying.load())
				        setupBeatRepeatZone(track, hostBpm);
			        else
				        track->beatRepeatAnchorPending.store(true);

			        acquireTheoreticalPosition(track);
		        },
		        [this, track]()
		        {
			        track->beatRepeatAnchorPending.store(false);
			        releaseTheoreticalPosition(track);
			        track->randomRetriggerActive.store(false);
			        track->lastRetriggerTime.store(-1.0);
		        });
	    });
}

void SequencerManager::checkReverseWithSampleCounter()
{
	double hostBpm = audioProcessor.getLastHostBpmForQuantization();
	int64_t currentHalfBeatNumber = getCurrentHalfBeatNumber(hostBpm);

	trackManager.forEachTrackAudio(
	    [&](TrackData *track)
	    {
		    checkQuantizedToggle(
		        track->reversePending, track->reverseStopPending, track->pendingReverseBeatNumber,
		        track->pendingReverseStopBeatNumber, track->reverseActive, currentHalfBeatNumber, [this, track]()
		        { acquireTheoreticalPosition(track); }, [this, track]() { releaseTheoreticalPosition(track); });
	    });
}

void SequencerManager::acquireTheoreticalPosition(TrackData *track) const
{
	if (!track->ownsTheoreticalTimeline())
		track->theoreticalPosition.store(track->readPosition.load());
}

void SequencerManager::releaseTheoreticalPosition(TrackData *track) const
{
	if (track->ownsTheoreticalTimeline())
		return;

	const double theoretical = track->theoreticalPosition.load();
	track->numSamplesAccPerSequence.store(theoretical);
	track->seekTo(theoretical);
}

int64_t SequencerManager::getCurrentHalfBeatNumber(double hostBpm) const
{
	if (hostBpm <= 0.0)
		hostBpm = 120.0;

	double halfBeatDurationSamples = (60.0 / hostBpm) * audioProcessor.getHostSampleRate() * 0.5;
	int64_t currentSample = internalSampleCounter.load();
	int64_t currentHalfBeatNumber = currentSample / (int64_t)halfBeatDurationSamples;

	return currentHalfBeatNumber;
}

double SequencerManager::calculateBeatRepeatInterval(int intervalValue, double hostBpm) const
{
	if (hostBpm <= 0.0)
		return 1.0;

	double beatDuration = 60.0 / hostBpm;

	switch (intervalValue)
	{
	case 1:
		return beatDuration * 4.0;
	case 2:
		return beatDuration * 2.0;
	case 3:
		return beatDuration * 1.0;
	case 4:
		return beatDuration * 0.5;
	case 5:
		return beatDuration * 0.25;
	case 6:
		return beatDuration * 0.125;
	case 7:
		return beatDuration * 0.0625;
	case 8:
		return beatDuration * 0.03125;
	case 9:
		return beatDuration * 0.015625;
	case 10:
		return beatDuration * 0.0078125;
	default:
		return beatDuration;
	}
}

void SequencerManager::executePendingAction(TrackData *track)
{
	switch (track->pendingAction)
	{
	case TrackData::PendingAction::StartOnNextMeasure:
		if (!track->isPlaying.load() && track->isArmed.load())
		{
			if (!track->beatRepeatActive.load())
				track->setPlaying(true);
			auto &seqData = track->getCurrentSequencerData();
			seqData.currentStep = 0;
			seqData.currentMeasure = 0;
			seqData.stepAccumulator = 0.0;
			track->isCurrentlyPlaying.store(true);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
			                                                 MidiMapping::feedbackActive);
		}
		break;

	case TrackData::PendingAction::StopOnNextMeasure:
		track->isPlaying.store(false);
		track->isArmedToStop.store(false);
		track->isCurrentlyPlaying.store(false);
		track->limiter.resetReductionAmount();
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
		                                                 MidiMapping::feedbackIdle);
		if (audioProcessor.onUIUpdateNeeded)
			audioProcessor.onUIUpdateNeeded();
		break;

	default:
		break;
	}

	track->pendingAction = TrackData::PendingAction::None;
}

void SequencerManager::flushMidiBuffer(juce::MidiBuffer &destination, int numSamples)
{
	juce::ScopedLock lock(sequencerMidiLock);
	destination.addEvents(sequencerMidiBuffer, 0, numSamples, 0);
	sequencerMidiBuffer.clear();
}

void SequencerManager::checkTransientScatterWithSampleCounter()
{
	double hostBpm = audioProcessor.getLastHostBpmForQuantization();
	int64_t currentHalfBeatNumber = getCurrentHalfBeatNumber(hostBpm);

	trackManager.forEachTrackAudio(
	    [&](TrackData *track)
	    {
		    checkQuantizedToggle(
		        track->transientScatterPending, track->transientScatterStopPending,
		        track->pendingTransientScatterBeatNumber, track->pendingTransientScatterStopBeatNumber,
		        track->transientScatterActive, currentHalfBeatNumber, [this, track]()
		        { acquireTheoreticalPosition(track); }, [this, track]() { releaseTheoreticalPosition(track); });
	    });
}

double SequencerManager::getEffectiveWindowLength(TrackData *track, const TrackPage &page,
                                                  bool applyPlaybackRatio) const
{
	TrackManager::PlaybackRatioInfo ratioInfo = audioProcessor.getTrackManager().getPlaybackRatio(page);
	DjIaVstProcessor::DawInfo dawInfo = audioProcessor.getDawInfo(ratioInfo.playbackRatio);

	const double sampleLength = (page.loopEnd - page.loopStart) * page.sampleRate;
	double seqLenSource = dawInfo.samplesPerMeasureScaled * (double)track->getCurrentSequencerData().numMeasures;
	if (applyPlaybackRatio)
		seqLenSource *= ratioInfo.playbackRatio;

	return std::min(sampleLength, seqLenSource);
}

double SequencerManager::getStartReadPosition(TrackData *track) const
{
	if (!track->reverseActive.load())
		return 0.0;

	const auto &page = track->getCurrentPage();
	return getEffectiveWindowLength(track, page) - 1.0;
}

double SequencerManager::getTransientScatterStartPosition(TrackData *track) const
{
	auto &page = track->getCurrentPage();
	trackManager.ensureTransientsAnalyzed(page);
	TrackManager::PlaybackRatioInfo ratioInfo = trackManager.getPlaybackRatio(page);
	DjIaVstProcessor::DawInfo dawInfo = audioProcessor.getDawInfo(ratioInfo.playbackRatio);
	const double twoStepsLength = dawInfo.samplesPerStep * 2.0;
	const double startAbs = page.loopStart * page.sampleRate;
	const double windowLength = getEffectiveWindowLength(track, page, false);
	const double endAbs = startAbs + windowLength;
	const double latestAllowedStart = endAbs - twoStepsLength;

	std::vector<const TrackPage::TransientInfo *> candidates;
	double totalWeight = 0.0;
	for (const auto &t : page.transients)
	{
		if (t.position >= startAbs && t.position < latestAllowedStart)
		{
			candidates.push_back(&t);
			totalWeight += (double)t.strength * t.strength;
		}
	}

	double chosenAbs;
	if (!candidates.empty() && totalWeight > 0.0)
	{
		double r = (rand() / (double)RAND_MAX) * totalWeight;
		double acc = 0.0;
		const TrackPage::TransientInfo *pick = candidates.back();
		for (auto *t : candidates)
		{
			acc += (double)t->strength * t->strength;
			if (r <= acc)
			{
				pick = t;
				break;
			}
		}
		chosenAbs = pick->position;
	}
	else
		chosenAbs = startAbs + (rand() / (double)RAND_MAX) * std::max(1.0, windowLength - twoStepsLength);

	return juce::jlimit(0.0, std::max(0.0, windowLength - twoStepsLength), chosenAbs - startAbs);
}

void SequencerManager::checkQuantizedToggle(std::atomic<bool> &pending, std::atomic<bool> &stopPending,
                                            std::atomic<int64_t> &pendingBeat, std::atomic<int64_t> &pendingStopBeat,
                                            std::atomic<bool> &active, int64_t currentHalfBeatNumber,
                                            std::function<void()> onActivate, std::function<void()> onDeactivate)
{
	if (pending.load())
	{
		if (pendingBeat.load() < 0)
			pendingBeat.store(currentHalfBeatNumber);

		if (currentHalfBeatNumber > pendingBeat.load())
		{
			if (onActivate)
				onActivate();
			active.store(true);
			pending.store(false);
			pendingBeat.store(-1);
		}
	}

	if (stopPending.load())
	{
		if (pendingStopBeat.load() < 0)
			pendingStopBeat.store(currentHalfBeatNumber);

		if (currentHalfBeatNumber > pendingStopBeat.load())
		{
			active.store(false);
			if (onDeactivate)
				onDeactivate();
			stopPending.store(false);
			pendingStopBeat.store(-1);
		}
	}
}