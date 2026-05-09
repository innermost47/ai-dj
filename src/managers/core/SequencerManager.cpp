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
				if (!audioProcessor.getActiveEditor())
				{
					track->isPlaying = false;
					track->isCurrentlyPlaying = false;
					track->readPosition = 0.0;
				}
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
				track->pageChangePending = true;
				track->pendingPageIndex = pageIndex;
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
								    if (!trackComp->isTimerRunning())
									    trackComp->startTimer(200);
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

void SequencerManager::handleSequenceChange(const juce::String &parameterID)
{
	juce::String slotStr = parameterID.substring(4, 5);
	juce::String seqStr = parameterID.substring(8, 9);

	int slotNumber = slotStr.getIntValue();
	int seqNumber = seqStr.getIntValue();

	if (slotNumber < 1 || slotNumber > 8 || seqNumber < 1 || seqNumber > 8)
		return;

	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (track && track->slotIndex == (slotNumber - 1))
		{
			auto &currentPage = track->getCurrentPage();
			currentPage.currentSequenceIndex = seqNumber - 1;
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackSeq(slotNumber), seqNumber - 1);
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
	{
		return;
	}
	static bool wasPlaying = false;

	if (hostIsPlaying && !wasPlaying)
	{
		internalSampleCounter.store(0);
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto &trackId : trackIds)
		{
			TrackData *track = trackManager.getTrack(trackId);
			if (track)
			{
				auto &seqData = track->getCurrentSequencerData();
				seqData.isPlaying = true;
				seqData.currentStep = 0;
				seqData.currentMeasure = 0;
				seqData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
			}
		}
	}
	else if (!hostIsPlaying && wasPlaying)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto &trackId : trackIds)
		{
			TrackData *track = trackManager.getTrack(trackId);
			bool arm = false;
			if (track->isCurrentlyPlaying.load())
				arm = true;
			if (track)
			{
				auto &seqData = track->getCurrentSequencerData();
				seqData.isPlaying = false;
				track->setStop();
				track->isArmed = arm;
				track->isPlaying.store(false);
				track->isCurrentlyPlaying = false;
				track->readPosition = 0.0;
				seqData.currentStep = 0;
				seqData.currentMeasure = 0;
				seqData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
				                                                 MidiMapping::feedbackIdle);
			}
		}
		audioProcessor.needsUIUpdate.store(true);
	}
	else if (!hostIsPlaying && !wasPlaying)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto &trackId : trackIds)
		{
			TrackData *track = trackManager.getTrack(trackId);
			if (track && track->isCurrentlyPlaying.load())
			{
				auto &seqData = track->getCurrentSequencerData();
				track->isArmed = true;
				track->isCurrentlyPlaying = false;
				track->readPosition = 0.0;
				seqData.currentStep = 0;
				seqData.currentMeasure = 0;
				seqData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
				seqData.isPlaying = false;
				track->isArmed = false;
				track->isPlaying.store(false);
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
				                                                 MidiMapping::feedbackIdle);
			}
		}
	}
	audioProcessor.needsUIUpdate.store(true);
	wasPlaying = hostIsPlaying;
}

void SequencerManager::updateSequencers(bool hostIsPlaying)
{
	if (isBypassed())
	{
		return;
	}
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

	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (track)
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
			else if (currentPpq >= expectedPpqForNextStep)
			{
				track->customStepCounter++;
				track->lastPpqPosition = expectedPpqForNextStep;
				shouldAdvanceStep = true;
			}

			if (shouldAdvanceStep)
			{
				handleAdvanceStep(track, hostIsPlaying);
				if (onSequencerUpdateNeeded)
					onSequencerUpdateNeeded(trackId);
			}
		}
	}
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
	{
		stepsPerBeat = 2;
	}
	else if (denominator == 4)
	{
		stepsPerBeat = 4;
	}
	else if (denominator == 2)
	{
		stepsPerBeat = 8;
	}
	else
	{
		stepsPerBeat = 4;
	}

	auto &seqData = track->getCurrentSequencerData();
	int stepsPerMeasure = numerator * stepsPerBeat;
	int newStep = track->customStepCounter % stepsPerMeasure;
	int newMeasure = (track->customStepCounter / stepsPerMeasure) % seqData.numMeasures;

	if (newMeasure == 0 && newStep == 0 && track->pageChangePending.load())
	{
		int targetPage = track->pendingPageIndex.load();
		int slotNumber = track->slotIndex + 1;
		if (targetPage >= 0 && targetPage < 4)
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
						    t->pageChangePending = false;
						    t->pendingPageIndex = -1;
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
	{
		track->pendingAction = TrackData::PendingAction::StartOnNextMeasure;
	}

	if ((newMeasure == 0 && newStep == 0) && track->pendingAction != TrackData::PendingAction::None)
	{
		executePendingAction(track);
	}

	seqData.currentStep = newStep;
	seqData.currentMeasure = newMeasure;

	if (currentStepIsActive && track->isCurrentlyPlaying.load() && hostIsPlaying)
	{

		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		track->setPlaying(true);
		triggerSequencerStep(track);
	}
}

void SequencerManager::triggerSequencerStep(TrackData *track)
{
	if (isBypassed())
	{
		return;
	}

	auto &seqData = track->getCurrentSequencerData();
	int step = seqData.currentStep;
	int measure = seqData.currentMeasure;
	track->isArmed = false;

	if (seqData.steps[measure][step])
	{
		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		audioProcessor.addPlayingTrack(track->midiNote, track->trackId);
		juce::MidiMessage noteOn =
		    juce::MidiMessage::noteOn(1, track->midiNote, (juce::uint8)(seqData.velocities[measure][step] * 127));
		addSequencerMidiMessage(noteOn);
	}
}

void SequencerManager::checkBeatRepeatWithSampleCounter()
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);

		if (!track)
			continue;

		auto &currentPage = track->getCurrentPage();

		if (track->beatRepeatPending.load())
		{
			double hostBpm = audioProcessor.getLastHostBpmForQuantization();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double halfBeatDurationSamples = (60.0 / hostBpm) * audioProcessor.getHostSampleRate() * 0.5;
			int64_t currentSample = internalSampleCounter.load();
			int64_t currentHalfBeatNumber = currentSample / (int64_t)halfBeatDurationSamples;

			if (track->pendingBeatNumber.load() < 0)
			{
				track->pendingBeatNumber.store(currentHalfBeatNumber);
			}

			if (currentHalfBeatNumber > track->pendingBeatNumber.load())
			{
				if (track->randomRetriggerDurationEnabled.load())
				{
					int randomInterval = 1 + (rand() % 10);
					track->randomRetriggerInterval.store(randomInterval);
					juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + "RetriggerInterval";
					auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
					if (param)
					{
						float normalizedValue = (randomInterval - 1.0f) / 9.0f;
						param->setValueNotifyingHost(normalizedValue);
					}
				}

				double currentPosition = track->readPosition.load();
				double repeatDuration = calculateRetriggerInterval(track->randomRetriggerInterval.load(), hostBpm);
				double repeatDurationSamples = repeatDuration * currentPage.sampleRate;

				track->originalReadPosition.store(currentPosition);
				track->beatRepeatStartPosition.store(currentPosition);
				track->beatRepeatEndPosition.store(currentPosition + repeatDurationSamples);

				double maxSamples = currentPage.numSamples;
				if (track->beatRepeatEndPosition.load() > maxSamples)
				{
					track->beatRepeatEndPosition.store(maxSamples);
				}

				track->beatRepeatActive.store(true);
				track->beatRepeatPending.store(false);
				track->pendingBeatNumber.store(-1);
				track->readPosition.store(track->beatRepeatStartPosition.load());
			}
		}

		if (track->beatRepeatStopPending.load())
		{
			double hostBpm = audioProcessor.getLastHostBpmForQuantization();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double halfBeatDurationSamples = (60.0 / hostBpm) * audioProcessor.getHostSampleRate() * 0.5;
			int64_t currentSample = internalSampleCounter.load();
			int64_t currentHalfBeatNumber = currentSample / (int64_t)halfBeatDurationSamples;

			if (track->pendingStopBeatNumber.load() < 0)
			{
				track->pendingStopBeatNumber.store(currentHalfBeatNumber);
			}

			if (currentHalfBeatNumber > track->pendingStopBeatNumber.load())
			{
				track->beatRepeatActive.store(false);
				track->beatRepeatStopPending.store(false);
				track->randomRetriggerActive.store(false);
				track->lastRetriggerTime.store(-1.0);
				track->readPosition.store(track->originalReadPosition.load());
				track->pendingStopBeatNumber.store(-1);
			}
		}
	}
}

double SequencerManager::calculateRetriggerInterval(int intervalValue, double hostBpm) const
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
			{
				track->readPosition = 0.0;
			}
			auto &seqData = track->getCurrentSequencerData();
			seqData.currentStep = 0;
			seqData.currentMeasure = 0;
			seqData.stepAccumulator = 0.0;
			track->isCurrentlyPlaying = true;
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
			                                                 MidiMapping::feedbackActive);
		}
		break;

	case TrackData::PendingAction::StopOnNextMeasure:
		track->isPlaying = false;
		track->isArmedToStop = false;
		track->isCurrentlyPlaying = false;
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