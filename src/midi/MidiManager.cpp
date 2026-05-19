#include "MidiManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

MidiManager::MidiManager(DjIaVstProcessor &processor, MidiLearnManager &midiLearnManager)
    : audioProcessor(processor), midiLearnManager(midiLearnManager)
{
}

void MidiManager::sendMidiFeedback(int cc, int value, int channel)
{
	juce::ScopedLock lock(feedbackMidiLock);
	feedbackMidiBuffer.addEvent(juce::MidiMessage::controllerEvent(channel, cc, value), 0);
}
void MidiManager::sendMidiFeedback(int cc, int value)
{
	sendMidiFeedback(cc, value, MidiMapping::feedbackChannelMixer);
}

void MidiManager::sendFullStateFeedback()
{
	auto &pm = audioProcessor.getParameterManager();
	auto trackIds = audioProcessor.getTrackManager().getAllTrackIds();

	for (const auto &trackId : trackIds)
	{
		TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
		if (!track)
			continue;

		int slot = track->slotIndex + 1;
		int slotIdx = track->slotIndex;

		if (track->isCurrentlyPlaying.load())
			sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot), MidiMapping::feedbackActive);
		else if (track->isArmed.load())
			sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot), MidiMapping::feedbackPending);
		else
			sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot), MidiMapping::feedbackIdle);

		sendMidiFeedback(MidiMapping::ccFeedbackPage(slot), track->currentPageIndex.load());

		sendMidiFeedback(MidiMapping::ccFeedbackVolume(slot), MidiMapping::volumeToMidi(pm.getVolume(slotIdx)));
		sendMidiFeedback(MidiMapping::ccFeedbackPan(slot), MidiMapping::panToMidi(pm.getPan(slotIdx)));
		sendMidiFeedback(MidiMapping::ccFeedbackPitch(slot), MidiMapping::pitchToMidi(pm.getPitch(slotIdx)));
		sendMidiFeedback(MidiMapping::ccFeedbackFine(slot), MidiMapping::fineToMidi(pm.getFine(slotIdx)));
		sendMidiFeedback(MidiMapping::ccFeedbackMute(slot),
		                 track->isMuted.load() ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
		sendMidiFeedback(MidiMapping::ccFeedbackSolo(slot),
		                 track->isSolo.load() ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
		sendMidiFeedback(MidiMapping::ccFeedbackBeatRepeat(slot), track->randomRetriggerEnabled.load()
		                                                              ? MidiMapping::feedbackActive
		                                                              : MidiMapping::feedbackIdle);
		sendMidiFeedback(MidiMapping::ccFeedbackSeq(slot), track->getCurrentPage().currentSequenceIndex);

		sendMidiFeedback(MidiMapping::ccFeedbackAdsrAttack(slot),
		                 MidiMapping::adsrToMidi(pm.getAttack(slotIdx), 0.001f, 4.0f),
		                 MidiMapping::feedbackChannelShaping);
		sendMidiFeedback(MidiMapping::ccFeedbackAdsrDecay(slot),
		                 MidiMapping::adsrToMidi(pm.getDecay(slotIdx), 0.001f, 4.0f),
		                 MidiMapping::feedbackChannelShaping);
		sendMidiFeedback(MidiMapping::ccFeedbackAdsrSustain(slot),
		                 MidiMapping::adsrToMidi(pm.getSustain(slotIdx), 0.0f, 1.0f),
		                 MidiMapping::feedbackChannelShaping);
		sendMidiFeedback(MidiMapping::ccFeedbackAdsrRelease(slot),
		                 MidiMapping::adsrToMidi(pm.getRelease(slotIdx), 0.001f, 4.0f),
		                 MidiMapping::feedbackChannelShaping);
		sendMidiFeedback(MidiMapping::ccFeedbackDelaySend(slot), MidiMapping::volumeToMidi(pm.getDelaySend(slotIdx)),
		                 MidiMapping::feedbackChannelSends);
		sendMidiFeedback(MidiMapping::ccFeedbackReverbSend(slot), MidiMapping::volumeToMidi(pm.getReverbSend(slotIdx)),
		                 MidiMapping::feedbackChannelSends);
	}

	for (int p = 0; p < ObsidianDataConst::MAX_CROSSFADER_PAIR; ++p)
	{
		sendMidiFeedback(MidiMapping::ccFeedbackPairCrossfader(p),
		                 MidiMapping::volumeToMidi(audioProcessor.getPairCrossfaderValue(p)),
		                 MidiMapping::feedbackChannelShaping);
	}
	sendMidiFeedback(MidiMapping::ccFeedbackGlobalCrossfader,
	                 MidiMapping::volumeToMidi(audioProcessor.getGlobalCrossfaderValue()),
	                 MidiMapping::feedbackChannelShaping);
	sendMidiFeedback(MidiMapping::ccFeedbackCrossfaderCurve, audioProcessor.getCrossfaderCurveMode() * 63,
	                 MidiMapping::feedbackChannelShaping);

	const int chFx = MidiMapping::feedbackChannelSends;

	sendMidiFeedback(MidiMapping::ccFeedbackDelayFeedback, MidiMapping::normalizedToMidi(pm.getFeedback()), chFx);
	sendMidiFeedback(MidiMapping::ccFeedbackReverbSize, MidiMapping::normalizedToMidi(pm.getReverbSize()), chFx);
	sendMidiFeedback(MidiMapping::ccFeedbackReverbDamping, MidiMapping::normalizedToMidi(pm.getReverbDamping()), chFx);
	sendMidiFeedback(MidiMapping::ccFeedbackReverbWidth, MidiMapping::normalizedToMidi(pm.getReverbWidth()), chFx);
	sendMidiFeedback(MidiMapping::ccFeedbackReverbMix, MidiMapping::normalizedToMidi(pm.getReverbMix()), chFx);

	sendMidiFeedback(MidiMapping::ccFeedbackDelayDivision, MidiMapping::indexToMidi(pm.getDelayDivisionIndex(), 8),
	                 chFx);
	sendMidiFeedback(MidiMapping::ccFeedbackDelayMode, MidiMapping::indexToMidi(pm.getDelayModeIndex(), 3), chFx);
}

void MidiManager::notifyPageChangedFeedback(int slotNumber, int pageIndex)
{
	sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber), pageIndex);
	auto trackIds = audioProcessor.getTrackManager().getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
		if (track && track->slotIndex == (slotNumber - 1))
		{
			sendMidiFeedback(MidiMapping::ccFeedbackSeq(slotNumber), track->pages[pageIndex].currentSequenceIndex);
			break;
		}
	}
}

void MidiManager::flushFeedbackBuffer(juce::MidiBuffer &destination, int numSamples)
{
	juce::ScopedLock lock(feedbackMidiLock);
	destination.addEvents(feedbackMidiBuffer, 0, numSamples, 0);
	feedbackMidiBuffer.clear();
}

void MidiManager::processMidiMessages(juce::MidiBuffer &midiMessages, bool hostIsPlaying, double hostBpm)
{
	static int totalBlocks = 0;
	totalBlocks++;

	int midiEventCount = midiMessages.getNumEvents();
	if (midiEventCount > 0)
	{
		audioProcessor.needsUIUpdate.store(true);
	}
	juce::Array<int> notesPlayedInThisBuffer;
	for (const auto metadata : midiMessages)
	{
		const auto message = metadata.getMessage();
		if (midiLearnManager.processMidiForLearning(message))
		{
			continue;
		}
		if (message.isController() && message.getChannel() == 1 &&
		    message.getControllerNumber() == MidiMapping::ccRequestState && message.getControllerValue() == 127)
		{
			sendFullStateFeedback();
			continue;
		}
		midiLearnManager.processMidiMappings(message);
		handlePlayAndStop(hostIsPlaying);
		audioProcessor.getGenerationManager().handleGenerate();
		if (hostIsPlaying)
		{
			if (message.isNoteOn())
			{
				int noteNumber = message.getNoteNumber();
				notesPlayedInThisBuffer.addIfNotAlreadyThere(noteNumber);
				audioProcessor.playTrack(message, hostBpm);
			}
			else if (message.isNoteOff())
			{
				int noteNumber = message.getNoteNumber();
				audioProcessor.stopNotePlaybackForTrack(noteNumber);
			}
		}
	}
	if (midiIndicatorCallback && notesPlayedInThisBuffer.size() > 0)
	{
		updateMidiIndicatorWithActiveNotes(hostBpm, notesPlayedInThisBuffer);
	}
}

void MidiManager::handlePlayAndStop(bool /*hostIsPlaying*/)
{
	int changedSlot = midiLearnManager.changedPlaySlotIndex.load();
	if (changedSlot >= 0)
	{
		auto trackIds = audioProcessor.getTrackManager().getAllTrackIds();
		for (const auto &trackId : trackIds)
		{
			TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
			if (track->slotIndex == changedSlot)
			{
				bool paramPlay = audioProcessor.getParameterManager().getPlay(changedSlot);
				if (paramPlay)
				{
					track->setArmed(true);
					sendMidiFeedback(MidiMapping::ccFeedbackPlay(changedSlot + 1), MidiMapping::feedbackPending);
				}
				else
				{
					track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
					track->setArmedToStop(true);
					track->setArmed(false);
					sendMidiFeedback(MidiMapping::ccFeedbackPlay(changedSlot + 1), MidiMapping::feedbackPending);
				}
				break;
			}
		}
		midiLearnManager.changedPlaySlotIndex.store(-1);
	}
}

void MidiManager::updateMidiIndicatorWithActiveNotes(double hostBpm, const juce::Array<int> &triggeredNotes)
{
	juce::StringArray currentPlayingTracks;
	auto trackIds = audioProcessor.getTrackManager().getAllTrackIds();

	for (const auto &trackId : trackIds)
	{
		TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
		if (track && track->isPlaying.load() && triggeredNotes.contains(track->midiNote))
		{
			juce::String noteName = juce::MidiMessage::getMidiNoteName(track->midiNote, true, true, 3);
			currentPlayingTracks.add(track->trackName + " (" + noteName + ")");
		}
	}

	if (currentPlayingTracks.size() > 0)
	{
		juce::String displayText = currentPlayingTracks.size() > 1
		                               ? currentPlayingTracks[0] + "+" + juce::String(currentPlayingTracks.size() - 1) +
		                                     " " + juce::String(hostBpm, 0)
		                               : currentPlayingTracks[0] + " " + juce::String(hostBpm, 0);
		midiIndicatorCallback(displayText);
	}
	else
	{
		midiIndicatorCallback("BPM:" + juce::String(hostBpm, 0));
	}
}