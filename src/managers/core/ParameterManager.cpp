#include "ParameterManager.h"
#include "MidiMapping.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

ParameterManager::ParameterManager(DjIaVstProcessor &processor)
    : audioProcessor(processor), apvts(processor, nullptr, "Parameters", createParameterLayout())
{
}

void ParameterManager::resolveParameters(juce::AudioProcessorValueTreeState::Listener *listener)
{
	generateParam = apvts.getRawParameterValue("generate");
	playParam = apvts.getRawParameterValue("play");
	masterVolumeParam = apvts.getRawParameterValue("masterVolume");
	masterPanParam = apvts.getRawParameterValue("masterPan");
	masterHighParam = apvts.getRawParameterValue("masterHigh");
	masterMidParam = apvts.getRawParameterValue("masterMid");
	masterLowParam = apvts.getRawParameterValue("masterLow");

	apvts.addParameterListener("generate", listener);
	apvts.addParameterListener("play", listener);

	delayDivisionParam = apvts.getRawParameterValue("delayDivision");
	delayFeedbackParam = apvts.getRawParameterValue("delayFeedback");
	delayModeParam = apvts.getRawParameterValue("delayMode");

	apvts.addParameterListener("delayDivision", listener);
	apvts.addParameterListener("delayFeedback", listener);
	apvts.addParameterListener("delayMode", listener);

	reverbSizeParam = apvts.getRawParameterValue("reverbSize");
	reverbDampingParam = apvts.getRawParameterValue("reverbDamping");
	reverbWidthParam = apvts.getRawParameterValue("reverbWidth");
	reverbMixParam = apvts.getRawParameterValue("reverbMix");

	apvts.addParameterListener("reverbSize", listener);
	apvts.addParameterListener("reverbDamping", listener);
	apvts.addParameterListener("reverbWidth", listener);
	apvts.addParameterListener("reverbMix", listener);

	for (int i = 0; i < ObsidianDataConst::MAX_TRACKS; ++i)
	{
		juce::String s = "slot" + juce::String(i + 1);

		slotVolumeParams[i] = apvts.getRawParameterValue(s + "Volume");
		slotPanParams[i] = apvts.getRawParameterValue(s + "Pan");
		slotMuteParams[i] = apvts.getRawParameterValue(s + "Mute");
		slotSoloParams[i] = apvts.getRawParameterValue(s + "Solo");
		slotPlayParams[i] = apvts.getRawParameterValue(s + "Play");
		slotStopParams[i] = apvts.getRawParameterValue(s + "Stop");
		slotGenerateParams[i] = apvts.getRawParameterValue(s + "Generate");
		slotPitchParams[i] = apvts.getRawParameterValue(s + "Pitch");
		slotFineParams[i] = apvts.getRawParameterValue(s + "Fine");
		slotRandomRetriggerParams[i] = apvts.getRawParameterValue(s + "RandomRetrigger");
		slotRetriggerIntervalParams[i] = apvts.getRawParameterValue(s + "RetriggerInterval");
		slotAdsrAttackParams[i] = apvts.getRawParameterValue(s + "AdsrAttack");
		slotAdsrDecayParams[i] = apvts.getRawParameterValue(s + "AdsrDecay");
		slotAdsrSustainParams[i] = apvts.getRawParameterValue(s + "AdsrSustain");
		slotAdsrReleaseParams[i] = apvts.getRawParameterValue(s + "AdsrRelease");
		slotDelaySendParams[i] = apvts.getRawParameterValue(s + "DelaySend");
		slotReverbSendParams[i] = apvts.getRawParameterValue(s + "ReverbSend");

		apvts.addParameterListener(s + "Generate", listener);
		apvts.addParameterListener(s + "Pitch", listener);
		apvts.addParameterListener(s + "Fine", listener);
		apvts.addParameterListener(s + "AdsrAttack", listener);
		apvts.addParameterListener(s + "AdsrDecay", listener);
		apvts.addParameterListener(s + "AdsrSustain", listener);
		apvts.addParameterListener(s + "AdsrRelease", listener);
		apvts.addParameterListener(s + "DelaySend", listener);
		apvts.addParameterListener(s + "ReverbSend", listener);
		apvts.addParameterListener(s + "Seq", listener);
		apvts.addParameterListener(s + "Play", listener);
		apvts.addParameterListener(s + "Mute", listener);
		apvts.addParameterListener(s + "Solo", listener);
		apvts.addParameterListener(s + "Volume", listener);
		apvts.addParameterListener(s + "Pan", listener);
		apvts.addParameterListener(s + "RandomRetrigger", listener);
		apvts.addParameterListener(s + "RetriggerInterval", listener);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.addParameterListener(s + page, listener);

		for (int seq = 1; seq <= ObsidianDataConst::MAX_TRACKS; ++seq)
			apvts.addParameterListener(s + "Seq" + juce::String(seq), listener);
	}

	globalCrossfaderParam = apvts.getRawParameterValue("globalCrossfader");
	crossfaderCurveModeParam = apvts.getRawParameterValue("crossfaderCurveMode");

	apvts.addParameterListener("globalCrossfader", listener);
	apvts.addParameterListener("crossfaderCurveMode", listener);

	for (int i = 0; i < ObsidianDataConst::MAX_CROSSFADER_PAIR; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i + 1);
		pairCrossfaderParams[i] = apvts.getRawParameterValue(pairId);
		apvts.addParameterListener(pairId, listener);
	}

	nextTrackParam = apvts.getRawParameterValue("nextTrack");
	prevTrackParam = apvts.getRawParameterValue("prevTrack");
	apvts.addParameterListener("nextTrack", listener);
	apvts.addParameterListener("prevTrack", listener);
}

void ParameterManager::removeAllListeners(juce::AudioProcessorValueTreeState::Listener *listener)
{
	apvts.removeParameterListener("generate", listener);
	apvts.removeParameterListener("play", listener);
	apvts.removeParameterListener("nextTrack", listener);
	apvts.removeParameterListener("prevTrack", listener);
	apvts.removeParameterListener("delayDivision", listener);
	apvts.removeParameterListener("delayFeedback", listener);
	apvts.removeParameterListener("delayMode", listener);
	apvts.removeParameterListener("reverbSize", listener);
	apvts.removeParameterListener("reverbDamping", listener);
	apvts.removeParameterListener("reverbWidth", listener);
	apvts.removeParameterListener("reverbMix", listener);

	for (int slot = 1; slot <= ObsidianDataConst::MAX_TRACKS; ++slot)
	{
		juce::String s = "slot" + juce::String(slot);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.removeParameterListener(s + page, listener);

		apvts.removeParameterListener(s + "Generate", listener);
		apvts.removeParameterListener(s + "Pitch", listener);
		apvts.removeParameterListener(s + "Fine", listener);
		apvts.removeParameterListener(s + "AdsrAttack", listener);
		apvts.removeParameterListener(s + "AdsrDecay", listener);
		apvts.removeParameterListener(s + "AdsrSustain", listener);
		apvts.removeParameterListener(s + "AdsrRelease", listener);
		apvts.removeParameterListener(s + "DelaySend", listener);
		apvts.removeParameterListener(s + "ReverbSend", listener);
		apvts.removeParameterListener(s + "Seq", listener);
		apvts.removeParameterListener(s + "Play", listener);
		apvts.removeParameterListener(s + "Mute", listener);
		apvts.removeParameterListener(s + "Solo", listener);
		apvts.removeParameterListener(s + "Volume", listener);
		apvts.removeParameterListener(s + "Pan", listener);
		apvts.removeParameterListener(s + "RandomRetrigger", listener);
		apvts.removeParameterListener(s + "RetriggerInterval", listener);
	}

	for (int i = 1; i <= 4; ++i)
		apvts.removeParameterListener("pairCrossfader" + juce::String(i), listener);

	apvts.removeParameterListener("globalCrossfader", listener);
	apvts.removeParameterListener("crossfaderCurveMode", listener);
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	auto makeTrigg = [](const juce::String &id, const juce::String &name)
	{
		auto attributes = juce::AudioParameterFloatAttributes().withAutomatable(false).withMeta(true);
		return std::make_unique<juce::AudioParameterFloat>(id, name, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f,
		                                                   attributes);
	};

	params.push_back(makeTrigg("generate", "Generate Loop"));
	params.push_back(makeTrigg("play", "Play Loop"));
	params.push_back(makeTrigg("nextTrack", "Next Track"));
	params.push_back(makeTrigg("prevTrack", "Previous Track"));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("globalCrossfader", "Global Crossfader (Deck A/B)",
	                                                             0.0f, 1.0f, 0.5f));

	params.push_back(std::make_unique<juce::AudioParameterChoice>(
	    "delayDivision", "Delay Time Division",
	    juce::StringArray{"1/16", "1/8.", "1/8", "1/4.", "1/4", "1/2", "1 bar", "2 bars"}, 4));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.4f));
	params.push_back(std::make_unique<juce::AudioParameterChoice>("delayMode", "Delay Mode",
	                                                              juce::StringArray{"Stereo", "Ping-Pong", "Mono"}, 0));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Reverb Damping", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Reverb Width", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.3f));

	for (int i = 1; i <= 4; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i);
		juce::String pairName = "Crossfader " + juce::String(i) + " <-> " + juce::String(i + 4);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(pairId, pairName, 0.0f, 1.0f, 0.5f));
	}

	params.push_back(std::make_unique<juce::AudioParameterChoice>("crossfaderCurveMode", "Crossfader Curve",
	                                                              juce::StringArray{"Linear", "Equal Power", "DJ"}, 1));

	for (int i = 1; i <= ObsidianDataConst::MAX_TRACKS; ++i)
	{
		juce::String slotId = "slot" + juce::String(i);
		juce::String slotName = "Slot " + juce::String(i);

		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Volume", slotName + " Volume", 0.0f, 1.0f, 0.8f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pan", slotName + " Pan", -1.0f, 1.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Mute", slotName + " Mute", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Solo", slotName + " Solo", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pitch", slotName + " Pitch", -12.0f, 12.0f, 0.0f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Fine", slotName + " Fine", -50.0f, 50.0f, 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "RandomRetrigger",
		                                                            slotName + " Random Retrigger", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "RetriggerInterval", slotName + " Retrigger Interval",
		                                                juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "DelaySend", slotName + " Delay Send",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "ReverbSend", slotName + " Reverb Send",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

		params.push_back(makeTrigg(slotId + "Play", slotName + " Play"));
		params.push_back(makeTrigg(slotId + "Stop", slotName + " Stop"));
		params.push_back(makeTrigg(slotId + "Generate", slotName + " Generate"));
		params.push_back(makeTrigg(slotId + "PageA", slotName + " Page A"));
		params.push_back(makeTrigg(slotId + "PageB", slotName + " Page B"));
		params.push_back(makeTrigg(slotId + "PageC", slotName + " Page C"));
		params.push_back(makeTrigg(slotId + "PageD", slotName + " Page D"));

		params.push_back(std::make_unique<juce::AudioParameterInt>(slotId + "Seq", slotName + " Sequence", 1,
		                                                           ObsidianDataConst::MAX_TRACKS, 1));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrAttack", slotName + " ADSR Attack", juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrDecay", slotName + " ADSR Decay", juce::NormalisableRange<float>(0.001f, 4.0f), 4.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "AdsrSustain", slotName + " ADSR Sustain",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrRelease", slotName + " ADSR Release", juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));
	}

	return {params.begin(), params.end()};
}

void ParameterManager::applyPlayState(bool shouldArm, TrackData *track)
{
	if (!track)
		return;
	juce::ScopedValueSetter<bool> guard(isApplyingPlayState, true);

	auto &currentPage = track->getCurrentPage();
	if (currentPage.numSamples <= 0)
	{
		return;
	}

	const bool isPlaying = track->isCurrentlyPlaying.load();
	const bool emptySeq = track->allSequencerStepsAreFalse();

	if (shouldArm && !isPlaying)
	{
		if (emptySeq)
		{
			track->setArmedToStop(false);
			track->pendingAction = TrackData::PendingAction::None;
		}
		track->setArmed(true);
	}
	else if (!shouldArm && !isPlaying)
	{
		track->pendingAction = TrackData::PendingAction::None;
		track->setArmed(false);
	}
	else if (!shouldArm && isPlaying && !emptySeq)
	{
		if (track->isArmedToStop.load())
			return;
		track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
		track->setArmed(false);
		track->setArmedToStop(true);
	}
	else if (emptySeq)
	{
		track->pendingAction = TrackData::PendingAction::None;
		track->isArmed.store(false);
		track->isArmedToStop.store(false);
		track->isPlaying.store(false);
		track->isCurrentlyPlaying.store(false);
		return;
	}
}

void ParameterManager::parameterChanged(const juce::String &parameterID, float newValue)
{

	if (parameterID == "generate" && newValue > 0.5f)
	{
		juce::MessageManager::callAsync([this]() { getAPVTS().getParameter("generate")->setValueNotifyingHost(0.0f); });
	}
	else if (parameterID == "globalCrossfader")
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshFromProcessor();
			    }
		    });
	}
	else if (parameterID.startsWith("pairCrossfader"))
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshFromProcessor();
			    }
		    });
	}
	else if (parameterID == "crossfaderCurveMode")
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshCurveButtons();
			    }
		    });
	}
	else if (parameterID.startsWith("slot"))
	{
		TrackData *track = audioProcessor.getTrackFromParamId(parameterID);
		if (!track)
			return;

		auto range = audioProcessor.getParameterTreeState().getParameterRange(parameterID);
		int slot = track->slotIndex + 1;
		int slotIdx = track->slotIndex;

		if (parameterID.contains("Page") && newValue > 0.5f)
		{
			audioProcessor.getSequencerManager().handlePageChange(parameterID);
			juce::MessageManager::callAsync(
			    [this, parameterID]()
			    {
				    if (auto *param = getAPVTS().getParameter(parameterID))
					    param->setValueNotifyingHost(0.0f);
			    });
		}
		else if (parameterID.contains("Seq"))
		{
			if (auto *param = dynamic_cast<juce::AudioParameterInt *>(getAPVTS().getParameter(parameterID)))
			{
				int targetSequence = param->get();
				int slotNum = parameterID.substring(4, 5).getIntValue();
				audioProcessor.getSequencerManager().handleSequenceChange(slotNum, targetSequence);
			}
		}
		else if ((parameterID.endsWith("AdsrAttack") || parameterID.endsWith("AdsrDecay") ||
		          parameterID.endsWith("AdsrSustain") || parameterID.endsWith("AdsrRelease")))
		{
			auto &page = track->getCurrentPage();

			if (parameterID.endsWith("AdsrAttack"))
			{
				page.adsrAttack.store(newValue);
			}
			else if (parameterID.endsWith("AdsrDecay"))
			{
				page.adsrDecay.store(newValue);
			}
			else if (parameterID.endsWith("AdsrSustain"))
			{
				page.adsrSustain.store(newValue);
			}
			else if (parameterID.endsWith("AdsrRelease"))
			{
				page.adsrRelease.store(newValue);
			}
		}
		else if (parameterID.endsWith("Mute"))
		{
			track->isMuted.store(newValue > 0.5f);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackMute(slot),
			                                                 track->isMuted.load() ? MidiMapping::feedbackActive
			                                                                       : MidiMapping::feedbackIdle);
		}
		else if (parameterID.endsWith("Volume"))
		{
			track->volume.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackVolume(slot),
			                                                 MidiMapping::volumeToMidi(getVolume(slotIdx)));
		}
		else if (parameterID.endsWith("Pan"))
		{
			track->pan.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPan(slot),
			                                                 MidiMapping::panToMidi(getPan(slotIdx)));
		}
		else if (parameterID.endsWith("Solo"))
		{
			track->isSolo.store(newValue > 0.5f);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackSolo(slot),
			                                                 track->isSolo.load() ? MidiMapping::feedbackActive
			                                                                      : MidiMapping::feedbackIdle);
		}
		else if (parameterID.endsWith("Play"))
		{
			applyPlayState(newValue > 0.5f, track);
			if (track->isCurrentlyPlaying.load())
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
				                                                 MidiMapping::feedbackActive);
			else if (track->isArmed.load())
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
				                                                 MidiMapping::feedbackPending);
			else
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
				                                                 MidiMapping::feedbackIdle);
		}
		else if (parameterID.endsWith("DelaySend"))
			track->delaySend.store(newValue);
		else if (parameterID.endsWith("ReverbSend"))
			track->reverbSend.store(newValue);
		else if (parameterID.endsWith("Pitch"))
		{
			track->getCurrentPage().pitchSemitones.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPitch(slot),
			                                                 MidiMapping::pitchToMidi(getPitch(slotIdx)));
		}
		else if (parameterID.endsWith("Fine"))
		{
			track->getCurrentPage().fineOffset.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackFine(slot),
			                                                 MidiMapping::fineToMidi(getFine(slotIdx)));
		}
		else if (parameterID.endsWith("RandomRetrigger"))
		{
			bool isEnabled = newValue > 0.5f;
			track->randomRetriggerEnabled.store(isEnabled);
			if (isEnabled)
			{
				track->beatRepeatPending.store(true);
			}
			else
			{
				track->beatRepeatStopPending.store(true);
			}
		}
		else if (parameterID.endsWith("RetriggerInterval"))
		{
			int value = (int)juce::roundToInt(newValue);
			if (track->randomRetriggerInterval.load() != newValue)
			{
				track->randomRetriggerInterval.store(value);

				if (track->beatRepeatActive.load())
				{
					double hostBpm = audioProcessor.getHostBpm();
					if (hostBpm <= 0.0)
						hostBpm = 120.0;

					double startPosition = track->beatRepeatStartPosition.load();
					double repeatDuration =
					    audioProcessor.getSequencerManager().calculateRetriggerInterval(value, hostBpm);
					double repeatDurationSamples = repeatDuration * track->getCurrentPage().sampleRate;

					const double GATE_SAMPLES = 64.0;
					double newEnd = startPosition + repeatDurationSamples - GATE_SAMPLES;

					double maxSamples = track->getCurrentPage().numSamples;
					if (newEnd > maxSamples)
						newEnd = maxSamples;
					if (newEnd <= startPosition)
						newEnd = startPosition + 1.0;

					track->beatRepeatEndPosition.store(newEnd);

					double currentPos = track->readPosition.load();
					if (currentPos >= newEnd)
					{
						track->readPosition.store(startPosition);
						track->brFadeInPending.store(64);
					}
				}
			}
		}
	}
}

void ParameterManager::handleSampleParams(int slot, TrackData *track)
{
	float paramRandomRetrigger = getRandomRetrigger(slot);
	int slotNumber = slot + 1;
	bool isRetriggerEnabled = paramRandomRetrigger > 0.5f;

	if (track->lastFeedbackBeatRepeat.load() != isRetriggerEnabled)
	{
		track->lastFeedbackBeatRepeat = isRetriggerEnabled;
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackBeatRepeat(slotNumber),
		                                                 isRetriggerEnabled ? MidiMapping::feedbackActive
		                                                                    : MidiMapping::feedbackIdle);
	}
}

void ParameterManager::handleSendsParams()
{
	const int ch = MidiMapping::feedbackChannelSends;

	auto pushFloatIfChanged = [&](std::atomic<float> &last, float cur, int cc)
	{
		if (std::abs(last.load() - cur) > 0.001f)
		{
			last.store(cur);
			audioProcessor.getMidiManager().sendMidiFeedback(cc, MidiMapping::normalizedToMidi(cur), ch);
		}
	};

	auto pushIntIfChanged = [&](std::atomic<int> &last, int cur, int cc, int total)
	{
		if (last.load() != cur)
		{
			last.store(cur);
			audioProcessor.getMidiManager().sendMidiFeedback(cc, MidiMapping::indexToMidi(cur, total), ch);
		}
	};

	pushFloatIfChanged(lastFeedbackDelayFeedback, getFeedback(), MidiMapping::ccFeedbackDelayFeedback);
	pushFloatIfChanged(lastFeedbackReverbSize, getReverbSize(), MidiMapping::ccFeedbackReverbSize);
	pushFloatIfChanged(lastFeedbackReverbDamping, getReverbDamping(), MidiMapping::ccFeedbackReverbDamping);
	pushFloatIfChanged(lastFeedbackReverbWidth, getReverbWidth(), MidiMapping::ccFeedbackReverbWidth);
	pushFloatIfChanged(lastFeedbackReverbMix, getReverbMix(), MidiMapping::ccFeedbackReverbMix);

	pushIntIfChanged(lastFeedbackDelayDivision, getDelayDivisionIndex(), MidiMapping::ccFeedbackDelayDivision, 8);
	pushIntIfChanged(lastFeedbackDelayMode, getDelayModeIndex(), MidiMapping::ccFeedbackDelayMode, 3);
}
