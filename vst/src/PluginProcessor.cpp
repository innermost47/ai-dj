/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

 // "Please DON'T download this if you're a real musician"
 // - Dedicated to those who downloaded it anyway

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AudioAnalyzer.h"
#include "DummySynth.h"
#include "MidiMapping.h"
#include "SequencerComponent.h"

juce::AudioProcessor::BusesProperties DjIaVstProcessor::createBusLayout()
{
	auto layout = juce::AudioProcessor::BusesProperties();
	layout = layout.withOutput("Main", juce::AudioChannelSet::stereo(), true);
	for (int i = 0; i < MAX_TRACKS; ++i)
	{
		layout = layout.withOutput("Track " + juce::String(i + 1),
			juce::AudioChannelSet::stereo(), false);
	}
	return layout;
}

DjIaVstProcessor::DjIaVstProcessor()
	: AudioProcessor(createBusLayout()), apiClient("", "http://localhost:8000"),
	parameters(*this, nullptr, "Parameters", { std::make_unique<juce::AudioParameterBool>("generate", "Generate Loop", false), std::make_unique<juce::AudioParameterBool>("play", "Play Loop", false), std::make_unique<juce::AudioParameterFloat>("bpm", "BPM", 60.0f, 200.0f, 126.0f), std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1Volume", "Slot 1 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot1Pan", "Slot 1 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot1Mute", "Slot 1 Mute", false), std::make_unique<juce::AudioParameterBool>("slot1Solo", "Slot 1 Solo", false), std::make_unique<juce::AudioParameterBool>("slot1Play", "Slot 1 Play", false), std::make_unique<juce::AudioParameterBool>("slot1Stop", "Slot 1 Stop", false), std::make_unique<juce::AudioParameterBool>("slot1Generate", "Slot 1 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot1Pitch", "Slot 1 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1Fine", "Slot 1 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1BpmOffset", "Slot 1 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2Volume", "Slot 2 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot2Pan", "Slot 2 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot2Mute", "Slot 2 Mute", false), std::make_unique<juce::AudioParameterBool>("slot2Solo", "Slot 2 Solo", false), std::make_unique<juce::AudioParameterBool>("slot2Play", "Slot 2 Play", false), std::make_unique<juce::AudioParameterBool>("slot2Stop", "Slot 2 Stop", false), std::make_unique<juce::AudioParameterBool>("slot2Generate", "Slot 2 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot2Pitch", "Slot 2 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2Fine", "Slot 2 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2BpmOffset", "Slot 2 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3Volume", "Slot 3 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot3Pan", "Slot 3 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot3Mute", "Slot 3 Mute", false), std::make_unique<juce::AudioParameterBool>("slot3Solo", "Slot 3 Solo", false), std::make_unique<juce::AudioParameterBool>("slot3Play", "Slot 3 Play", false), std::make_unique<juce::AudioParameterBool>("slot3Stop", "Slot 3 Stop", false), std::make_unique<juce::AudioParameterBool>("slot3Generate", "Slot 3 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot3Pitch", "Slot 3 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3Fine", "Slot 3 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3BpmOffset", "Slot 3 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4Volume", "Slot 4 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot4Pan", "Slot 4 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot4Mute", "Slot 4 Mute", false), std::make_unique<juce::AudioParameterBool>("slot4Solo", "Slot 4 Solo", false), std::make_unique<juce::AudioParameterBool>("slot4Play", "Slot 4 Play", false), std::make_unique<juce::AudioParameterBool>("slot4Stop", "Slot 4 Stop", false), std::make_unique<juce::AudioParameterBool>("slot4Generate", "Slot 4 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot4Pitch", "Slot 4 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4Fine", "Slot 4 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4BpmOffset", "Slot 4 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5Volume", "Slot 5 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot5Pan", "Slot 5 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot5Mute", "Slot 5 Mute", false), std::make_unique<juce::AudioParameterBool>("slot5Solo", "Slot 5 Solo", false), std::make_unique<juce::AudioParameterBool>("slot5Play", "Slot 5 Play", false), std::make_unique<juce::AudioParameterBool>("slot5Stop", "Slot 5 Stop", false), std::make_unique<juce::AudioParameterBool>("slot5Generate", "Slot 5 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot5Pitch", "Slot 5 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5Fine", "Slot 5 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5BpmOffset", "Slot 5 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6Volume", "Slot 6 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot6Pan", "Slot 6 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot6Mute", "Slot 6 Mute", false), std::make_unique<juce::AudioParameterBool>("slot6Solo", "Slot 6 Solo", false), std::make_unique<juce::AudioParameterBool>("slot6Play", "Slot 6 Play", false), std::make_unique<juce::AudioParameterBool>("slot6Stop", "Slot 6 Stop", false), std::make_unique<juce::AudioParameterBool>("slot6Generate", "Slot 6 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot6Pitch", "Slot 6 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6Fine", "Slot 6 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6BpmOffset", "Slot 6 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7Volume", "Slot 7 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot7Pan", "Slot 7 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot7Mute", "Slot 7 Mute", false), std::make_unique<juce::AudioParameterBool>("slot7Solo", "Slot 7 Solo", false), std::make_unique<juce::AudioParameterBool>("slot7Play", "Slot 7 Play", false), std::make_unique<juce::AudioParameterBool>("slot7Stop", "Slot 7 Stop", false), std::make_unique<juce::AudioParameterBool>("slot7Generate", "Slot 7 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot7Pitch", "Slot 7 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7Fine", "Slot 7 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7BpmOffset", "Slot 7 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8Volume", "Slot 8 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot8Pan", "Slot 8 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot8Mute", "Slot 8 Mute", false), std::make_unique<juce::AudioParameterBool>("slot8Solo", "Slot 8 Solo", false), std::make_unique<juce::AudioParameterBool>("slot8Play", "Slot 8 Play", false), std::make_unique<juce::AudioParameterBool>("slot8Stop", "Slot 8 Stop", false), std::make_unique<juce::AudioParameterBool>("slot8Generate", "Slot 8 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot8Pitch", "Slot 8 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8Fine", "Slot 8 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8BpmOffset", "Slot 8 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1Speed", "Slot 1 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot2Speed", "Slot 2 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot3Speed", "Slot 3 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot4Speed", "Slot 4 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot5Speed", "Slot 5 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot6Speed", "Slot 6 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot7Speed", "Slot 7 Speed", 0.5f, 2.0f, 1.0f),
		std::make_unique<juce::AudioParameterFloat>("slot8Speed", "Slot 8 Speed", 0.5f, 2.0f, 1.0f),
		})
{
	loadGlobalConfig();
	obsidianEngine = std::make_unique<ObsidianEngine>();
	if (!obsidianEngine->initialize())
	{
		DBG("Failed to initialize OBSIDIAN Engine");
	}
	else
	{
		DBG("OBSIDIAN Engine ready!");
	}
	loadParameters();
	initTracks();
	initDummySynth();
	trackManager.parameterUpdateCallback = [this](int slot, TrackData* track)
		{
			handleSampleParams(slot, track);
		};
	startTimerHz(30);
	autoLoadEnabled.store(true);
	stateLoaded = true;
}

void DjIaVstProcessor::loadGlobalConfig()
{
	auto configFile = getGlobalConfigFile();
	DBG("Config file path: " + configFile.getFullPathName());

	if (configFile.existsAsFile())
	{
		auto configJson = juce::JSON::parse(configFile);
		DBG("JSON parsed successfully: " + juce::String(configJson.isVoid() ? "false" : "true"));
		DBG("Full JSON object: " + juce::JSON::toString(configJson));
		if (auto* object = configJson.getDynamicObject())
		{
			apiKey = object->getProperty("apiKey").toString();
			serverUrl = object->getProperty("serverUrl").toString();
			requestTimeoutMS = object->getProperty("requestTimeoutMS").toString().getIntValue();

			useLocalModel = object->getProperty("useLocalModel").toString() == "true";
			localModelsPath = object->getProperty("localModelsPath").toString();

			if (!object->hasProperty("useLocalModel"))
			{
				useLocalModel = false;
			}

			auto promptsVar = object->getProperty("customPrompts");
			DBG("Prompts property exists: " + juce::String(!promptsVar.isVoid() ? "false" : "true"));
			DBG("Prompts is array: " + juce::String(promptsVar.isArray() ? "false" : "true"));

			if (promptsVar.isArray())
			{
				customPrompts.clear();
				auto* promptsArray = promptsVar.getArray();
				DBG("Prompts array size: " + juce::String(promptsArray->size()));

				DBG("Raw promptsVar: " + juce::JSON::toString(promptsVar));

				for (int i = 0; i < promptsArray->size(); ++i)
				{
					juce::String prompt = promptsArray->getUnchecked(i).toString();
					DBG("Adding prompt " + juce::String(i) + ": '" + prompt + "'");
					customPrompts.add(prompt);
				}
			}
			setApiKey(apiKey);
			setServerUrl(serverUrl);
		}
	}
	DBG("Final customPrompts size: " + juce::String(customPrompts.size()));
}

void DjIaVstProcessor::saveGlobalConfig()
{
	auto configFile = getGlobalConfigFile();
	configFile.getParentDirectory().createDirectory();

	juce::DynamicObject::Ptr config = new juce::DynamicObject();
	config->setProperty("apiKey", apiKey);
	config->setProperty("serverUrl", serverUrl);
	config->setProperty("requestTimeoutMS", requestTimeoutMS);
	config->setProperty("useLocalModel", useLocalModel ? "true" : "false");
	config->setProperty("localModelsPath", localModelsPath);

	juce::Array<juce::var> promptsArray;
	for (const auto& prompt : customPrompts)
	{
		promptsArray.add(juce::var(prompt));
	}
	config->setProperty("customPrompts", juce::var(promptsArray));

	juce::String jsonString = juce::JSON::toString(juce::var(config.get()));
	configFile.replaceWithText(jsonString);
}

void DjIaVstProcessor::initDummySynth()
{
	for (int i = 0; i < 4; ++i)
		synth.addVoice(new DummyVoice());
	synth.addSound(new DummySound());
}

void DjIaVstProcessor::initTracks()
{
	selectedTrackId = trackManager.createTrack();
	individualOutputBuffers.resize(MAX_TRACKS);
	for (auto& buffer : individualOutputBuffers)
	{
		buffer.setSize(2, 512);
	}
}

void DjIaVstProcessor::loadParameters()
{
	generateParam = parameters.getRawParameterValue("generate");
	playParam = parameters.getRawParameterValue("play");
	masterVolumeParam = parameters.getRawParameterValue("masterVolume");
	masterPanParam = parameters.getRawParameterValue("masterPan");
	masterHighParam = parameters.getRawParameterValue("masterHigh");
	masterMidParam = parameters.getRawParameterValue("masterMid");
	masterLowParam = parameters.getRawParameterValue("masterLow");
	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotVolumeParams[i] = parameters.getRawParameterValue(slotName + "Volume");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotPanParams[i] = parameters.getRawParameterValue(slotName + "Pan");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotMuteParams[i] = parameters.getRawParameterValue(slotName + "Mute");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotSoloParams[i] = parameters.getRawParameterValue(slotName + "Solo");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotPlayParams[i] = parameters.getRawParameterValue(slotName + "Play");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotStopParams[i] = parameters.getRawParameterValue(slotName + "Stop");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotGenerateParams[i] = parameters.getRawParameterValue(slotName + "Generate");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotPitchParams[i] = parameters.getRawParameterValue(slotName + "Pitch");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotFineParams[i] = parameters.getRawParameterValue(slotName + "Fine");
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotBpmOffsetParams[i] = parameters.getRawParameterValue(slotName + "BpmOffset");
	}
	for (int i = 1; i <= 8; ++i)
	{
		parameters.addParameterListener("slot" + juce::String(i) + "Generate", this);
	}
	for (int i = 0; i < 8; ++i)
	{
		juce::String slotName = "slot" + juce::String(i + 1);
		slotSpeedParams[i] = parameters.getRawParameterValue(slotName + "Speed");
	}
	parameters.addParameterListener("generate", this);
	parameters.addParameterListener("play", this);
}

DjIaVstProcessor::~DjIaVstProcessor()
{
	stopTimer();
	try
	{
		cleanProcessor();
	}
	catch (const std::exception& e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
}

void DjIaVstProcessor::cleanProcessor()
{
	parameters.removeParameterListener("generate", this);
	parameters.removeParameterListener("play", this);
	for (int i = 1; i <= 8; ++i)
	{
		parameters.removeParameterListener("slot" + juce::String(i) + "Generate", this);
	}

	isNotePlaying = false;
	hasPendingAudioData = false;
	hasUnloadedSample = false;
	midiIndicatorCallback = nullptr;
	individualOutputBuffers.clear();
	synth.clearVoices();
	synth.clearSounds();
	obsidianEngine.reset();
}

void DjIaVstProcessor::timerCallback()
{
	if (!needsUIUpdate.load())
		return;
	if (onUIUpdateNeeded)
	{
		onUIUpdateNeeded();
	}
	needsUIUpdate = false;
}

void DjIaVstProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	hostSampleRate = newSampleRate;
	currentBlockSize = samplesPerBlock;
	synth.setCurrentPlaybackSampleRate(newSampleRate);
	for (auto& buffer : individualOutputBuffers)
	{
		buffer.setSize(2, samplesPerBlock);
		buffer.clear();
	}
	masterEQ.prepare(newSampleRate, samplesPerBlock);
}

void DjIaVstProcessor::releaseResources()
{
	for (auto& buffer : individualOutputBuffers)
	{
		buffer.setSize(0, 0);
	}
}

bool DjIaVstProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
	{
		return false;
	}
	for (int i = 1; i < layouts.outputBuses.size(); ++i)
	{
		if (!layouts.outputBuses[i].isDisabled() &&
			layouts.outputBuses[i] != juce::AudioChannelSet::stereo())
		{
			return false;
		}
	}
	return true;
}

void DjIaVstProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	checkAndSwapStagingBuffers();
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	bool hostIsPlaying = false;
	auto currentPlayHead = getPlayHead();

	double hostBpm = 126.0;
	double hostPpqPosition = 0.0;

	if (currentPlayHead)
	{
		getDawInformations(currentPlayHead, hostIsPlaying, hostBpm, hostPpqPosition);
	}
	handleSequencerPlayState(hostIsPlaying);
	updateSequencers(hostIsPlaying);

	{
		juce::ScopedLock lock(sequencerMidiLock);
		midiMessages.addEvents(sequencerMidiBuffer, 0, buffer.getNumSamples(), 0);
		sequencerMidiBuffer.clear();
	}

	processMidiMessages(midiMessages, hostIsPlaying, hostBpm);

	if (hasPendingAudioData.load())
	{
		processIncomingAudio(hostIsPlaying);
	}

	resizeIndividualsBuffers(buffer);
	clearOutputBuffers(buffer);

	auto mainOutput = getBusBuffer(buffer, false, 0);
	mainOutput.clear();

	updateTimeStretchRatios(hostBpm);

	trackManager.renderAllTracks(mainOutput, individualOutputBuffers, hostBpm);

	copyTracksToIndividualOutputs(buffer);
	applyMasterEffects(mainOutput);

	checkIfUIUpdateNeeded(midiMessages);
}

void DjIaVstProcessor::addSequencerMidiMessage(const juce::MidiMessage& message)
{
	juce::ScopedLock lock(sequencerMidiLock);
	sequencerMidiBuffer.addEvent(message, 0);
}

void DjIaVstProcessor::handleSequencerPlayState(bool hostIsPlaying)
{
	static bool wasPlaying = false;

	if (hostIsPlaying && !wasPlaying)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			if (track)
			{
				track->sequencerData.isPlaying = true;
				track->sequencerData.currentStep = 0;
				track->sequencerData.currentMeasure = 0;
				track->sequencerData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
			}
		}
	}
	else if (!hostIsPlaying && wasPlaying)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			bool arm = false;
			if (track->isCurrentlyPlaying.load())
			{
				arm = true;
			}
			if (track)
			{
				track->sequencerData.isPlaying = false;
				track->setStop();
				track->isArmed = arm;
				track->isPlaying.store(false);
				track->isCurrentlyPlaying = false;
				track->readPosition = 0.0;
				track->sequencerData.currentStep = 0;
				track->sequencerData.currentMeasure = 0;
				track->sequencerData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
			}
		}
		needsUIUpdate = true;
	}
	else if (!hostIsPlaying && !wasPlaying)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			bool arm = false;
			if (track->isCurrentlyPlaying.load())
			{
				track->isArmed = true;
				track->isCurrentlyPlaying = false;
				track->readPosition = 0.0;
				track->sequencerData.currentStep = 0;
				track->sequencerData.currentMeasure = 0;
				track->sequencerData.stepAccumulator = 0.0;
				track->customStepCounter = 0;
				track->lastPpqPosition = -1.0;
				track->sequencerData.isPlaying = false;
				track->isArmed = arm;
				track->isPlaying.store(false);
			}
		}
		needsUIUpdate = true;
	}

	wasPlaying = hostIsPlaying;
}

void DjIaVstProcessor::checkIfUIUpdateNeeded(juce::MidiBuffer& midiMessages)
{
	bool anyTrackPlaying = false;
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (track && track->isPlaying.load())
		{
			anyTrackPlaying = true;
			break;
		}
	}

	if (anyTrackPlaying || midiMessages.getNumEvents() > 0)
	{
		needsUIUpdate = true;
	}
}

void DjIaVstProcessor::applyMasterEffects(juce::AudioSampleBuffer& mainOutput)
{
	updateMasterEQ();
	masterEQ.processBlock(mainOutput);

	float targetVol = masterVolumeParam->load();
	float targetPan = masterPanParam->load();

	const float smoothingCoeff = 0.95f;
	smoothedMasterVol = smoothedMasterVol * smoothingCoeff + targetVol * (1.0f - smoothingCoeff);
	smoothedMasterPan = smoothedMasterPan * smoothingCoeff + targetPan * (1.0f - smoothingCoeff);

	mainOutput.applyGain(smoothedMasterVol);

	if (mainOutput.getNumChannels() >= 2 && std::abs(smoothedMasterPan) > 0.01f)
	{
		if (smoothedMasterPan < 0.0f)
		{
			mainOutput.applyGain(1, 0, mainOutput.getNumSamples(), 1.0f + smoothedMasterPan);
		}
		else
		{
			mainOutput.applyGain(0, 0, mainOutput.getNumSamples(), 1.0f - smoothedMasterPan);
		}
	}
}

void DjIaVstProcessor::copyTracksToIndividualOutputs(juce::AudioSampleBuffer& buffer)
{
	for (int busIndex = 1; busIndex < getTotalNumOutputChannels() / 2; ++busIndex)
	{
		if (busIndex * 2 + 1 < getTotalNumOutputChannels())
		{
			auto busBuffer = getBusBuffer(buffer, false, busIndex);

			int trackIndex = busIndex - 1;
			if (trackIndex < individualOutputBuffers.size())
			{
				for (int ch = 0; ch < std::min(busBuffer.getNumChannels(), 2); ++ch)
				{
					busBuffer.copyFrom(ch, 0, individualOutputBuffers[trackIndex], ch, 0,
						buffer.getNumSamples());
				}
			}
		}
	}
}

void DjIaVstProcessor::clearOutputBuffers(juce::AudioSampleBuffer& buffer)
{
	for (int busIndex = 0; busIndex < getTotalNumOutputChannels() / 2; ++busIndex)
	{
		if (busIndex * 2 + 1 < getTotalNumOutputChannels() && busIndex <= MAX_TRACKS)
		{
			auto busBuffer = getBusBuffer(buffer, false, busIndex);
			busBuffer.clear();
		}
	}
}

void DjIaVstProcessor::resizeIndividualsBuffers(juce::AudioSampleBuffer& buffer)
{
	for (auto& indivBuffer : individualOutputBuffers)
	{
		if (indivBuffer.getNumSamples() != buffer.getNumSamples())
		{
			indivBuffer.setSize(2, buffer.getNumSamples(), false, false, true);
		}
		indivBuffer.clear();
	}
}

void DjIaVstProcessor::getDawInformations(juce::AudioPlayHead* currentPlayHead, bool& hostIsPlaying, double& hostBpm, double& hostPpqPosition)
{
	double localSampleRate = getSampleRate();
	if (localSampleRate > 0.0)
	{
		hostSampleRate = localSampleRate;
	}

	if (auto positionInfo = currentPlayHead->getPosition())
	{
		hostIsPlaying = positionInfo->getIsPlaying();

		if (auto bpm = positionInfo->getBpm())
		{
			double newBpm = *bpm;
			hostBpm = newBpm;
			if (std::abs(newBpm - cachedHostBpm.load()) > 0.1)
			{
				cachedHostBpm = newBpm;
				if (onHostBpmChanged)
				{
					onHostBpmChanged(newBpm);
				}
			}
		}

		if (auto ppq = positionInfo->getPpqPosition())
		{
			hostPpqPosition = *ppq;
		}
		if (auto timeSig = positionInfo->getTimeSignature())
		{
			timeSignatureNumerator.store(timeSig->numerator);
			timeSignatureDenominator.store(timeSig->denominator);
		}
	}
}

void DjIaVstProcessor::updateMasterEQ()
{
	masterEQ.setHighGain(masterHighParam->load());
	masterEQ.setMidGain(masterMidParam->load());
	masterEQ.setLowGain(masterLowParam->load());
}

void DjIaVstProcessor::processMidiMessages(juce::MidiBuffer& midiMessages, bool hostIsPlaying, double hostBpm)
{
	static int totalBlocks = 0;
	totalBlocks++;

	int midiEventCount = midiMessages.getNumEvents();
	if (midiEventCount > 0)
	{
		needsUIUpdate = true;
	}
	juce::Array<int> notesPlayedInThisBuffer;
	for (const auto metadata : midiMessages)
	{
		const auto message = metadata.getMessage();
		if (midiLearnManager.processMidiForLearning(message))
		{
			continue;
		}
		midiLearnManager.processMidiMappings(message);
		handlePlayAndStop(hostIsPlaying);
		handleGenerate();
		if (hostIsPlaying)
		{
			if (message.isNoteOn())
			{
				int noteNumber = message.getNoteNumber();
				notesPlayedInThisBuffer.addIfNotAlreadyThere(noteNumber);
				playTrack(message, hostBpm);
			}
			else if (message.isNoteOff())
			{
				int noteNumber = message.getNoteNumber();
				stopNotePlaybackForTrack(noteNumber);
			}
		}
	}
	if (midiIndicatorCallback && notesPlayedInThisBuffer.size() > 0)
	{
		updateMidiIndicatorWithActiveNotes(hostBpm, notesPlayedInThisBuffer);
	}
}

void DjIaVstProcessor::previewTrack(const juce::String& trackId)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (track && track->numSamples > 0)
	{
		track->readPosition = 0.0;
		track->isPlaying.store(true);
		needsUIUpdate = true;
	}
}

void DjIaVstProcessor::playTrack(const juce::MidiMessage& message, double hostBpm)
{
	int noteNumber = message.getNoteNumber();
	juce::String noteName = juce::MidiMessage::getMidiNoteName(noteNumber, true, true, 3);
	bool trackFound = false;
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (track && track->midiNote == noteNumber)
		{
			if (trackId == trackIdWaitingForLoad)
			{
				correctMidiNoteReceived = true;
			}
			if (track->numSamples > 0)
			{
				startNotePlaybackForTrack(trackId, noteNumber, hostBpm);
				trackFound = true;
			}
			break;
		}
	}
}

void DjIaVstProcessor::updateMidiIndicatorWithActiveNotes(double hostBpm, const juce::Array<int>& triggeredNotes)
{
	juce::StringArray currentPlayingTracks;
	auto trackIds = trackManager.getAllTrackIds();

	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (track && track->isPlaying.load() && triggeredNotes.contains(track->midiNote))
		{
			juce::String noteName = juce::MidiMessage::getMidiNoteName(track->midiNote, true, true, 3);
			currentPlayingTracks.add(track->trackName + " (" + noteName + ")");
		}
	}

	if (currentPlayingTracks.size() > 0)
	{
		juce::String displayText = "Last played: " + currentPlayingTracks.joinIntoString(" + ") + " - BPM:" + juce::String(hostBpm, 0);
		midiIndicatorCallback(displayText);
	}
	else
	{
		midiIndicatorCallback("MIDI: Ready - BPM:" + juce::String(hostBpm, 0));
	}
}

void DjIaVstProcessor::handleGenerate()
{
	if (isGenerating)
		return;
	int changedSlot = midiLearnManager.changedGenerateSlotIndex.load();
	if (changedSlot >= 0)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			if (track->slotIndex == changedSlot)
			{
				bool paramGenerate = slotGenerateParams[changedSlot]->load() > 0.5f;
				if (paramGenerate)
				{
					generateLoopFromMidi(trackId);
					needsUIUpdate.store(true);
				}
				break;
			}
		}
		midiLearnManager.changedGenerateSlotIndex.store(-1);
	}
}

void DjIaVstProcessor::generateLoopFromMidi(const juce::String& trackId)
{
	if (isGenerating)
		return;

	TrackData* track = trackManager.getTrack(trackId);
	if (!track)
		return;

	setIsGenerating(true);
	setGeneratingTrackId(trackId);

	juce::MessageManager::callAsync([this, trackId]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
				editor->startGenerationUI(trackId);
			} });

			juce::Thread::launch([this, trackId]()
				{
					try {
						TrackData* track = trackManager.getTrack(trackId);
						if (!track) {
							throw std::runtime_error("Track not found");
						}

						DjIaClient::LoopRequest request;

						if (!track->selectedPrompt.isEmpty()) {
							request.prompt = track->selectedPrompt;
							request.bpm = static_cast<float>(getHostBpm());
							request.key = getGlobalKey();
							request.generationDuration = static_cast<float>(getGlobalDuration());

							request.preferredStems.clear();
							if (isGlobalStemEnabled("drums")) request.preferredStems.push_back("drums");
							if (isGlobalStemEnabled("bass")) request.preferredStems.push_back("bass");
							if (isGlobalStemEnabled("other")) request.preferredStems.push_back("other");
							if (isGlobalStemEnabled("vocals")) request.preferredStems.push_back("vocals");
							if (isGlobalStemEnabled("guitar")) request.preferredStems.push_back("guitar");
							if (isGlobalStemEnabled("piano")) request.preferredStems.push_back("piano");
						}
						else {
							request = createGlobalLoopRequest();
						}
						track->updateFromRequest(request);
						juce::String promptSource = !track->selectedPrompt.isEmpty() ?
							"track prompt: " + track->selectedPrompt.substring(0, 20) + "..." :
							"global prompt";
						juce::MessageManager::callAsync([this, promptSource]() {
							if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
								editor->statusLabel.setText("Generating with " + promptSource, juce::dontSendNotification);
							}
							});
						generateLoop(request, trackId);
					}
					catch (const std::exception& e) {
						setIsGenerating(false);
						setGeneratingTrackId("");

						juce::MessageManager::callAsync([this, trackId, error = juce::String(e.what())]() {
							if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
								editor->stopGenerationUI(trackId, false, error);
							}
							});
					} });
}

void DjIaVstProcessor::handlePlayAndStop(bool /*hostIsPlaying*/)
{
	int changedSlot = midiLearnManager.changedPlaySlotIndex.load();
	if (changedSlot >= 0)
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			if (track->slotIndex == changedSlot)
			{
				bool paramPlay = slotPlayParams[changedSlot]->load() > 0.5f;
				if (paramPlay)
				{
					track->setArmed(true);
				}
				else
				{
					track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
					track->setArmedToStop(true);
					track->setArmed(false);
				}
				break;
			}
		}
		midiLearnManager.changedPlaySlotIndex.store(-1);
	}
}

void DjIaVstProcessor::handleSampleParams(int slot, TrackData* track)
{
	float paramVolume = slotVolumeParams[slot]->load();
	float paramPan = slotPanParams[slot]->load();
	float paramPitch = slotPitchParams[slot]->load() * 8;
	float paramFine = slotFineParams[slot]->load() * 2;
	float paramSolo = slotSoloParams[slot]->load();
	float paramMute = slotMuteParams[slot]->load();
	float paramSpeed = slotSpeedParams[slot]->load();

	if (std::abs(track->volume.load() - paramVolume) > 0.01f)
	{
		track->volume = paramVolume;
	}

	if (std::abs(track->pan.load() - paramPan) > 0.01f)
	{
		track->pan = paramPan;
	}

	if (std::abs(track->bpmOffset - paramPitch) > 0.01f)
	{
		track->bpmOffset = paramPitch;
		needsUIUpdate = true;
	}

	if (std::abs(track->fineOffset - paramFine) > 0.01f)
	{
		track->fineOffset = paramFine * 0.05f;
		track->bpmOffset = paramPitch + track->fineOffset;
		needsUIUpdate = true;
	}
	bool isSolo = paramSolo > 0.5f;
	bool isMuted = paramMute > 0.5f;

	if (track->isSolo.load() != isSolo)
	{
		track->isSolo = isSolo;
	}
	if (track->isMuted.load() != isMuted)
	{
		track->isMuted = isMuted;
	}
	if (std::abs(track->speed.load() - paramSpeed) > 0.01f)
	{
		track->speed = paramSpeed;
	}
}

void DjIaVstProcessor::updateTimeStretchRatios(double hostBpm)
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (!track)
			continue;

		double bpmRatioWithPitch = 1.0;
		if (track->originalBpm > 0.0f && hostBpm > 0.0)
		{
			double hostRatio = hostBpm / track->originalBpm;
			double manualAdjust = track->bpmOffset / track->originalBpm;
			bpmRatioWithPitch = hostRatio + manualAdjust;
		}
		bpmRatioWithPitch = juce::jlimit(0.25, 4.0, bpmRatioWithPitch);
		track->cachedPlaybackRatio = bpmRatioWithPitch;

		double bpmRatioWithoutPitch = 1.0;
		if (track->originalBpm > 0.0f && hostBpm > 0.0)
		{
			bpmRatioWithoutPitch = hostBpm / track->originalBpm;
		}

		float userSpeedRatio = 1.0f;
		if (track->slotIndex != -1)
		{
			juce::String speedParamName = "slot" + juce::String(track->slotIndex + 1) + "Speed";
			if (auto* speedParam = getParameterTreeState().getParameter(speedParamName))
			{
				userSpeedRatio = speedParam->getValue() * 1.5f + 0.5f;
			}
		}

		float finalSpeedRatio = static_cast<float>(bpmRatioWithoutPitch) * userSpeedRatio;
		finalSpeedRatio = juce::jlimit(0.25f, 4.0f, finalSpeedRatio);
		track->speed = finalSpeedRatio;
		track->updateSoundTouchSettings(track->sampleRate);
	}
}

void DjIaVstProcessor::startNotePlaybackForTrack(const juce::String& trackId, int noteNumber, double /*hostBpm*/)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track || track->numSamples == 0)
		return;
	if (track->isArmedToStop.load())
	{
		return;
	}
	if (!track->isArmed.load() && !track->isCurrentlyPlaying.load())
	{
		return;
	}
	if (track->isPlaying.load())
	{
		return;
	}

	track->readPosition = 0.0;
	track->setPlaying(true);
	track->isCurrentlyPlaying.store(true);
	track->isArmed = false;
	playingTracks[noteNumber] = trackId;
}

void DjIaVstProcessor::stopNotePlaybackForTrack(int noteNumber)
{
	auto it = playingTracks.find(noteNumber);
	if (it != playingTracks.end())
	{
		TrackData* track = trackManager.getTrack(it->second);
		if (track)
		{
			track->isPlaying = false;
		}
		playingTracks.erase(it);
	}
}

juce::String DjIaVstProcessor::createNewTrack(const juce::String& name)
{
	auto trackIds = trackManager.getAllTrackIds();
	if (trackIds.size() >= MAX_TRACKS)
	{
		throw std::runtime_error("Maximum number of tracks reached (" + std::to_string(MAX_TRACKS) + ")");
	}

	juce::String trackId = trackManager.createTrack(name);
	return trackId;
}

void DjIaVstProcessor::reorderTracks(const juce::String& fromTrackId, const juce::String& toTrackId)
{
	trackManager.reorderTracks(fromTrackId, toTrackId);
}

void DjIaVstProcessor::deleteTrack(const juce::String& trackId)
{
	TrackData* trackToDelete = trackManager.getTrack(trackId);
	if (!trackToDelete)
		return;

	juce::String trackName = trackToDelete->trackName;

	juce::MessageManager::callAsync([this, trackId, trackName]()
		{ juce::AlertWindow::showAsync(
			juce::MessageBoxOptions()
			.withIconType(juce::MessageBoxIconType::QuestionIcon)
			.withTitle("Delete Track")
			.withMessage("Are you sure you want to delete '" + trackName + "'?\n\n"
				"This action cannot be undone.")
			.withButton("Delete")
			.withButton("Cancel"),
			[this, trackId](int result)
			{
				if (result == 1)
				{
					performTrackDeletion(trackId);
				}
			}); });
}

void DjIaVstProcessor::performTrackDeletion(const juce::String& trackId)
{
	TrackData* trackToDelete = trackManager.getTrack(trackId);
	if (!trackToDelete)
		return;

	int slotIndex = trackToDelete->slotIndex;
	auto trackIds = trackManager.getAllTrackIds();
	int deletedTrackIndex = -1;
	for (int i = 0; i < trackIds.size(); ++i)
	{
		if (trackIds[i] == trackId)
		{
			deletedTrackIndex = i;
			break;
		}
	}

	if (trackId == selectedTrackId)
	{
		if (trackIds.size() > 1)
		{
			if (deletedTrackIndex < trackIds.size() - 1)
			{
				selectedTrackId = trackIds[deletedTrackIndex + 1];
			}
			else if (deletedTrackIndex > 0)
			{
				selectedTrackId = trackIds[deletedTrackIndex - 1];
			}
		}
		else
		{
			selectedTrackId = trackManager.createTrack("Track");
		}
	}

	if (slotIndex != -1)
	{
		getMidiLearnManager().removeMappingsForSlot(slotIndex + 1);
	}

	trackManager.removeTrack(trackId);

	reassignTrackOutputsAndMidi();

	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->refreshTrackComponents();
				editor->toggleWaveFormButtonOnTrack();
				editor->setStatusWithTimeout("Track deleted");
			} });
}

void DjIaVstProcessor::reassignTrackOutputsAndMidi()
{
	auto trackIds = trackManager.getAllTrackIds();

	for (int i = 0; i < trackIds.size(); ++i)
	{
		TrackData* track = trackManager.getTrack(trackIds[i]);
		if (track)
		{
			int oldSlotIndex = track->slotIndex;
			int newSlotIndex = i;

			if (oldSlotIndex != newSlotIndex && oldSlotIndex != -1)
			{
				getMidiLearnManager().moveMappingsFromSlotToSlot(oldSlotIndex + 1, newSlotIndex + 1);
			}

			track->slotIndex = newSlotIndex;
			track->midiNote = 60 + i;
			trackManager.usedSlots[newSlotIndex] = true;
		}
	}
}

void DjIaVstProcessor::selectTrack(const juce::String& trackId)
{
	if (trackManager.getTrack(trackId))
	{
		selectedTrackId = trackId;
	}
}

void DjIaVstProcessor::generateLoop(const DjIaClient::LoopRequest& request, const juce::String& targetTrackId)
{
	juce::String trackId = targetTrackId.isEmpty() ? selectedTrackId : targetTrackId;

	try
	{
		if (useLocalModel)
		{
			generateLoopLocal(request, trackId);
		}
		else
		{
			generateLoopAPI(request, trackId);
		}
	}
	catch (const std::exception& e)
	{
		hasPendingAudioData = false;
		waitingForMidiToLoad = false;
		trackIdWaitingForLoad.clear();
		correctMidiNoteReceived = false;
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "Error: " + juce::String(e.what()));
	}
}

void DjIaVstProcessor::generateLoopAPI(const DjIaClient::LoopRequest& request, const juce::String& trackId)
{
	auto response = apiClient.generateLoop(request, hostSampleRate, requestTimeoutMS);

	try
	{
		if (!response.errorMessage.isEmpty())
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			notifyGenerationComplete(trackId, "ERROR: " + response.errorMessage);
			return;
		}

		if (response.audioData.getFullPathName().isEmpty() ||
			!response.audioData.exists() ||
			response.audioData.getSize() == 0)
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			notifyGenerationComplete(trackId, "Invalid response from API");
			return;
		}
	}
	catch (const std::exception& /*e*/)
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "Response validation failed");
		return;
	}

	{
		const juce::ScopedLock lock(apiLock);
		pendingTrackId = trackId;
		pendingAudioFile = response.audioData;
		hasPendingAudioData = true;
		waitingForMidiToLoad = true;
		trackIdWaitingForLoad = trackId;
		correctMidiNoteReceived = false;
	}

	if (TrackData* track = trackManager.getTrack(trackId))
	{
		track->prompt = request.prompt;
		track->bpm = request.bpm;

		juce::String stems;
		for (const auto& stem : request.preferredStems)
		{
			if (!stems.isEmpty())
				stems += ", ";
			stems += stem;
		}
		track->stems = stems;
	}

	setIsGenerating(false);
	setGeneratingTrackId("");

	juce::String successMessage = "Loop generated successfully! Press Play to listen.";
	if (response.isUnlimitedKey)
	{
		successMessage += " - Unlimited API key";
	}
	else if (response.creditsRemaining >= 0)
	{
		successMessage += " - " + juce::String(response.creditsRemaining) + " credits remaining";
	}

	notifyGenerationComplete(trackId, successMessage);
}

void DjIaVstProcessor::generateLoopLocal(const DjIaClient::LoopRequest& request, const juce::String& trackId)
{
	auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural");
	auto stableAudioDir = appDataDir.getChildFile("stable-audio");

	StableAudioEngine localEngine;
	if (!localEngine.initialize(stableAudioDir.getFullPathName()))
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "ERROR: Local models not found. Please check setup instructions.");
		return;
	}

	StableAudioEngine::GenerationParams params(request.prompt, 6.0f);
	params.sampleRate = static_cast<int>(hostSampleRate);
	params.numThreads = 4;

	auto result = localEngine.generateSample(params);

	if (!result.success || result.audioData.empty())
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "ERROR: Local generation failed - " + result.errorMessage);
		return;
	}

	juce::File tempFile = createTempAudioFile(result.audioData, result.actualDuration);
	if (!tempFile.exists() || tempFile.getSize() == 0)
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "ERROR: Failed to create audio file");
		return;
	}

	{
		const juce::ScopedLock lock(apiLock);
		pendingTrackId = trackId;
		pendingAudioFile = tempFile;
		hasPendingAudioData = true;
		waitingForMidiToLoad = true;
		trackIdWaitingForLoad = trackId;
		correctMidiNoteReceived = false;
	}

	if (TrackData* track = trackManager.getTrack(trackId))
	{
		track->prompt = request.prompt;
		track->bpm = request.bpm;
		track->stems = "";
	}

	setIsGenerating(false);
	setGeneratingTrackId("");

	juce::String successMessage = juce::String::formatted(
		"Loop generated locally! (%.1fs) Press Play to listen.",
		result.actualDuration);

	notifyGenerationComplete(trackId, successMessage);
}

juce::StringArray DjIaVstProcessor::getBuiltInPrompts() const
{
	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		return editor->getBuiltInPrompts();
	}

	return juce::StringArray({ "Techno kick rhythm",
							  "Hardcore kick pattern",
							  "Drum and bass rhythm",
							  "Dub kick rhythm",
							  "Acidic 303 bassline",
							  "Deep rolling bass",
							  "Ambient flute psychedelic",
							  "Dark atmospheric pad",
							  "Industrial noise texture",
							  "Glitchy percussion loop",
							  "Vintage analog lead",
							  "Distorted noise chops" });
}

void DjIaVstProcessor::handleGenerationComplete(const juce::String& trackId,
	const DjIaClient::LoopRequest& /*originalRequest*/,
	const ObsidianEngine::LoopResponse& response)
{
	try
	{
		if (!response.success || response.audioData.empty())
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			juce::String errorMsg = response.errorMessage.isEmpty() ? "Unknown generation error" : response.errorMessage;
			notifyGenerationComplete(trackId, "ERROR: " + errorMsg);
			return;
		}

		juce::File tempFile = createTempAudioFile(response.audioData, response.actualDuration);
		if (!tempFile.exists() || tempFile.getSize() == 0)
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			notifyGenerationComplete(trackId, "ERROR: Failed to create audio file");
			return;
		}

		{
			const juce::ScopedLock lock(apiLock);
			pendingTrackId = trackId;
			pendingAudioFile = tempFile;
			hasPendingAudioData = true;
			waitingForMidiToLoad = true;
			trackIdWaitingForLoad = trackId;
			correctMidiNoteReceived = false;
		}

		if (TrackData* track = trackManager.getTrack(trackId))
		{
			track->generationDuration = static_cast<int>(response.actualDuration);
			track->generationBpm = response.bpm;

			if (!response.stemsUsed.empty())
			{
				juce::String stems;
				for (const auto& stem : response.stemsUsed)
				{
					if (!stems.isEmpty())
						stems += ", ";
					stems += stem;
				}
				track->stems = stems;
			}
		}

		setIsGenerating(false);
		setGeneratingTrackId("");

		juce::String successMessage = juce::String::formatted(
			"Loop generated successfully! (%.1fs, %.0f BPM) Press Play to listen.",
			response.duration,
			response.bpm);

		if (!response.stemsUsed.empty())
		{
			juce::StringArray stemsArray;
			for (const auto& stem : response.stemsUsed)
			{
				stemsArray.add(stem);
			}
			successMessage += "\nStems: " + stemsArray.joinIntoString(", ");
		}

		notifyGenerationComplete(trackId, successMessage);
	}
	catch (const std::exception& e)
	{
		hasPendingAudioData = false;
		waitingForMidiToLoad = false;
		trackIdWaitingForLoad.clear();
		correctMidiNoteReceived = false;
		setIsGenerating(false);
		setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "Error processing generated audio: " + juce::String(e.what()));
	}
}

juce::File DjIaVstProcessor::createTempAudioFile(const std::vector<float>& audioData, float /*duration*/)
{
	try
	{
		juce::File tempFile = juce::File::createTempFile(".wav");
		int numSamples = static_cast<int>(audioData.size());
		juce::AudioBuffer<float> buffer(1, numSamples);
		if (audioData.size() > 0)
		{
			buffer.copyFrom(0, 0, audioData.data(), numSamples);
		}
		juce::WavAudioFormat wavFormat;
		juce::FileOutputStream* outputStream = new juce::FileOutputStream(tempFile);
		if (!outputStream->openedOk())
		{
			delete outputStream;
			return juce::File{};
		}

		std::unique_ptr<juce::AudioFormatWriter> writer(
			wavFormat.createWriterFor(
				outputStream,
				hostSampleRate,
				1,
				16,
				{},
				0));

		if (!writer)
		{
			return juce::File{};
		}

		if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
		{
			return juce::File{};
		}

		writer.reset();

		return tempFile;
	}
	catch (const std::exception& /*e*/)
	{
		return juce::File{};
	}
}

void DjIaVstProcessor::notifyGenerationComplete(const juce::String& trackId, const juce::String& message)
{
	lastGeneratedTrackId = trackId;
	pendingMessage = message;
	hasPendingNotification = true;
	triggerAsyncUpdate();
}

void DjIaVstProcessor::handleAsyncUpdate()
{
	if (!hasPendingNotification)
		return;

	hasPendingNotification = false;

	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
				if (generationListener) {
					generationListener->onGenerationComplete(lastGeneratedTrackId, pendingMessage);
				}
			} });
}

void DjIaVstProcessor::processIncomingAudio(bool hostIsPlaying)
{
	if (!hasPendingAudioData.load())
	{
		return;
	}
	if (pendingTrackId.isEmpty())
	{
		return;
	}

	TrackData* track = trackManager.getTrack(pendingTrackId);
	if (!track)
	{
		return;
	}
	if (waitingForMidiToLoad.load() && !correctMidiNoteReceived.load() && hostIsPlaying && track->isPlaying.load())
	{
		return;
	}
	if (!canLoad.load() && !autoLoadEnabled.load())
	{
		hasUnloadedSample = true;
		return;
	}

	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
				editor->statusLabel.setText("Loading sample...", juce::dontSendNotification);
			} });

			juce::Thread::launch([this, trackId = pendingTrackId, audioFile = pendingAudioFile]()
				{ loadAudioFileAsync(trackId, audioFile); });

			clearPendingAudio();
			hasUnloadedSample = false;
			waitingForMidiToLoad = false;
			correctMidiNoteReceived = false;
			canLoad = false;
			trackIdWaitingForLoad.clear();
}

void DjIaVstProcessor::checkAndSwapStagingBuffers()
{
	auto trackIds = trackManager.getAllTrackIds();

	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (!track)
			continue;
		if (track->swapRequested.exchange(false))
		{
			if (track->hasStagingData.load())
			{
				performAtomicSwap(track, trackId);
			}
		}
	}
}

void DjIaVstProcessor::performAtomicSwap(TrackData* track, const juce::String& trackId)
{
	DBG("Swapping buffer for track: " << trackId << " - New samples: " << track->stagingNumSamples.load());
	std::swap(track->audioBuffer, track->stagingBuffer);
	track->numSamples = track->stagingNumSamples.load();
	track->sampleRate = track->stagingSampleRate.load();
	track->originalBpm = track->stagingOriginalBpm;

	double sampleDuration = track->numSamples / track->sampleRate;

	if (sampleDuration <= 8.0)
	{
		track->loopStart = 0.0;
		track->loopEnd = sampleDuration;
	}
	else
	{
		double beatDuration = 60.0 / track->originalBpm;
		double fourBars = beatDuration * 16.0;
		track->loopStart = 0.0;
		track->loopEnd = std::min(fourBars, sampleDuration);
	}

	track->readPosition = 0.0;
	track->hasStagingData = false;
	track->stagingBuffer.setSize(0, 0);

	juce::MessageManager::callAsync([this, trackId]()
		{ updateWaveformDisplay(trackId); });
}

void DjIaVstProcessor::updateWaveformDisplay(const juce::String& trackId)
{
	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		for (auto& trackComp : editor->getTrackComponents())
		{
			if (trackComp->getTrackId() == trackId)
			{
				if (trackComp->isWaveformVisible())
				{
					trackComp->refreshWaveformDisplay();
				}
				break;
			}
		}
	}
}

void DjIaVstProcessor::loadAudioFileAsync(const juce::String& trackId, const juce::File& audioFile)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track)
	{
		return;
	}

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile));

		if (!reader)
		{
			return;
		}

		loadAudioToStagingBuffer(reader, track);
		processAudioBPMAndSync(track);

		auto permanentFile = getTrackAudioFile(trackId);
		permanentFile.getParentDirectory().createDirectory();

		DBG("Saving buffer(s) with " << track->stagingBuffer.getNumSamples() << " samples");
		if (track->hasOriginalVersion.load())
		{
			saveOriginalAndStretchedBuffers(track->originalStagingBuffer, track->stagingBuffer, trackId, track->stagingSampleRate);
			DBG("Both files saved for track: " << trackId);
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate);
			DBG("File saved to: " << permanentFile.getFullPathName());
		}

		track->audioFilePath = permanentFile.getFullPathName();
		track->hasStagingData = true;
		track->swapRequested = true;

		juce::MessageManager::callAsync([this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
					editor->statusLabel.setText("Sample loaded! Ready to play.", juce::dontSendNotification);
					juce::Timer::callAfterDelay(2000, [this]() {
						if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
							editor->statusLabel.setText("Ready", juce::dontSendNotification);
						}
						});
				} });
	}
	catch (const std::exception& /*e*/)
	{
		track->hasStagingData = false;
		track->swapRequested = false;
	}
}

void DjIaVstProcessor::reloadTrackWithVersion(const juce::String& trackId, bool useOriginal)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track || !track->hasOriginalVersion.load()) return;
	juce::File fileToLoad;
	if (useOriginal)
	{
		fileToLoad = getTrackAudioFile(trackId + "_original");
	}
	else
	{
		fileToLoad = getTrackAudioFile(trackId);
	}

	DBG("Loading file: " << fileToLoad.getFullPathName() << " - Exists: " << (fileToLoad.existsAsFile() ? "YES" : "NO"));
	DBG("File size: " << fileToLoad.getSize() << " bytes");
	if (!fileToLoad.existsAsFile()) return;
	juce::Thread::launch([this, trackId, fileToLoad]()
		{
			loadAudioFileForSwitch(trackId, fileToLoad);
		});
}

void DjIaVstProcessor::loadAudioFileForSwitch(const juce::String& trackId, const juce::File& audioFile)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track) return;

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile));

		if (!reader) return;
		loadAudioToStagingBuffer(reader, track);
		track->hasStagingData = true;
		track->swapRequested = true;

		juce::MessageManager::callAsync([this, trackId]()
			{
				updateWaveformDisplay(trackId);
			});
	}
	catch (const std::exception&)
	{

	}
}

void DjIaVstProcessor::saveOriginalAndStretchedBuffers(const juce::AudioBuffer<float>& originalBuffer,
	const juce::AudioBuffer<float>& stretchedBuffer,
	const juce::String& trackId,
	double sampleRate)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("AudioCache");

	auto originalFile = audioDir.getChildFile(trackId + "_original.wav");
	saveBufferToFile(originalBuffer, originalFile, sampleRate);

	auto stretchedFile = audioDir.getChildFile(trackId + ".wav");
	saveBufferToFile(stretchedBuffer, stretchedFile, sampleRate);
}

void DjIaVstProcessor::saveBufferToFile(const juce::AudioBuffer<float>& buffer,
	const juce::File& outputFile,
	double sampleRate)
{
	if (buffer.getNumSamples() == 0)
	{
		return;
	}

	juce::WavAudioFormat wavFormat;

	if (outputFile.exists())
	{
		outputFile.deleteFile();
	}
	juce::FileOutputStream* fileStream = new juce::FileOutputStream(outputFile);

	if (!fileStream->openedOk())
	{
		delete fileStream;
		return;
	}

	std::unique_ptr<juce::AudioFormatWriter> writer(
		wavFormat.createWriterFor(fileStream, sampleRate, buffer.getNumChannels(), 16, {}, 0));

	if (writer == nullptr)
	{
		delete fileStream;
		return;
	}

	writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
	writer.reset();
}

juce::File DjIaVstProcessor::getTrackAudioFile(const juce::String& trackId)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("AudioCache");

	return audioDir.getChildFile(trackId + ".wav");
}

void DjIaVstProcessor::processAudioBPMAndSync(TrackData* track)
{
	float detectedBPM = AudioAnalyzer::detectBPM(track->stagingBuffer, track->stagingSampleRate);

	double hostBpm = cachedHostBpm.load();

	bool isDoubleTempo = false;
	bool isHalfTempo = false;

	if (hostBpm > 0)
	{
		double expectedDoubleTempo = hostBpm * 2.0;
		double expectedHalfTempo = hostBpm / 2.0;
		double tolerance = hostBpm * 0.2;

		if (detectedBPM >= (expectedDoubleTempo - tolerance) &&
			detectedBPM <= (expectedDoubleTempo + tolerance))
		{
			isDoubleTempo = true;
		}
		if (detectedBPM >= (expectedHalfTempo - tolerance) &&
			detectedBPM <= (expectedHalfTempo + tolerance))
		{
			isHalfTempo = true;
		}
	}

	bool isTempoBypass = isDoubleTempo || isHalfTempo;
	bool bpmValid = (detectedBPM > 60.0f && detectedBPM < 200.0f) && !isTempoBypass;

	if (isTempoBypass)
	{
		track->stagingOriginalBpm = track->bpm;
	}
	else
	{
		track->stagingOriginalBpm = bpmValid ? detectedBPM : track->bpm;
	}

	double bpmDifference = std::abs(hostBpm - track->stagingOriginalBpm);
	bool hostBpmValid = (hostBpm > 0.0);
	bool originalBpmValid = (track->stagingOriginalBpm > 0.0f);
	bool bpmDifferenceSignificant = (bpmDifference > 1.0);

	if (hostBpmValid && originalBpmValid && bpmDifferenceSignificant && !isTempoBypass)
	{
		track->originalStagingBuffer.makeCopyOf(track->stagingBuffer);

		double stretchRatio = hostBpm / static_cast<double>(track->stagingOriginalBpm);
		AudioAnalyzer::timeStretchBuffer(track->stagingBuffer, stretchRatio, track->stagingSampleRate);
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		track->stagingOriginalBpm = static_cast<float>(hostBpm);
		track->hasOriginalVersion = true;
	}
	else
	{
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
	}
}

void DjIaVstProcessor::loadAudioToStagingBuffer(std::unique_ptr<juce::AudioFormatReader>& reader, TrackData* track)
{
	int numChannels = reader->numChannels;
	int numSamples = static_cast<int>(reader->lengthInSamples);
	double sampleRate = reader->sampleRate;

	track->stagingBuffer.setSize(2, numSamples, false, false, true);
	track->stagingBuffer.clear();

	reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

	if (numChannels == 1)
	{
		track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
	}

	track->stagingNumSamples = numSamples;
	track->stagingSampleRate = sampleRate;
}

void DjIaVstProcessor::loadPendingSample()
{
	if (hasUnloadedSample.load() && !pendingTrackId.isEmpty())
	{
		waitingForMidiToLoad = true;
		canLoad = true;
		trackIdWaitingForLoad = pendingTrackId;
	}
}

void DjIaVstProcessor::clearPendingAudio()
{
	const juce::ScopedLock lock(apiLock);
	pendingAudioFile = juce::File();
	pendingTrackId.clear();
	hasPendingAudioData = false;
}

void DjIaVstProcessor::setAutoLoadEnabled(bool enabled)
{
	autoLoadEnabled = enabled;
}

void DjIaVstProcessor::setApiKey(const juce::String& key)
{
	apiKey = key;
	apiClient.setApiKey(apiKey);
}

void DjIaVstProcessor::setServerUrl(const juce::String& url)
{
	serverUrl = url;
	apiClient.setBaseUrl(serverUrl);
}

void DjIaVstProcessor::setRequestTimeout(int newRequestTimeoutMS)
{
	this->requestTimeoutMS = newRequestTimeoutMS;
}

double DjIaVstProcessor::getHostBpm() const
{
	if (auto currentPlayHead = getPlayHead())
	{
		if (auto positionInfo = currentPlayHead->getPosition())
		{
			if (positionInfo->getBpm().hasValue())
			{
				double bpm = *positionInfo->getBpm();
				if (bpm > 0.0)
				{
					return bpm;
				}
			}
		}
	}
	return 110.0;
}

juce::AudioProcessorEditor* DjIaVstProcessor::createEditor()
{
	currentEditor = new DjIaVstEditor(*this);
	midiLearnManager.setEditor(currentEditor);
	return currentEditor;
}

void DjIaVstProcessor::addCustomPrompt(const juce::String& prompt)
{
	if (!prompt.isEmpty() && !customPrompts.contains(prompt))
	{
		customPrompts.add(prompt);
		saveGlobalConfig();
	}
}

juce::StringArray DjIaVstProcessor::getCustomPrompts() const
{
	return customPrompts;
}

void DjIaVstProcessor::clearCustomPrompts()
{
	customPrompts.clear();
}

void DjIaVstProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	juce::ValueTree state("DjIaVstState");

	state.setProperty("lastPrompt", juce::var(lastPrompt), nullptr);
	state.setProperty("lastKey", juce::var(lastKey), nullptr);
	state.setProperty("lastBpm", juce::var(lastBpm), nullptr);
	state.setProperty("lastPresetIndex", juce::var(lastPresetIndex), nullptr);
	state.setProperty("hostBpmEnabled", juce::var(hostBpmEnabled), nullptr);
	state.setProperty("lastDuration", juce::var(lastDuration), nullptr);
	state.setProperty("selectedTrackId", juce::var(selectedTrackId), nullptr);
	state.setProperty("drumsEnabled", juce::var(drumsEnabled), nullptr);
	state.setProperty("bassEnabled", juce::var(bassEnabled), nullptr);
	state.setProperty("otherEnabled", juce::var(otherEnabled), nullptr);
	state.setProperty("vocalsEnabled", juce::var(vocalsEnabled), nullptr);
	state.setProperty("guitarEnabled", juce::var(guitarEnabled), nullptr);
	state.setProperty("pianoEnabled", juce::var(pianoEnabled), nullptr);
	state.setProperty("lastKeyIndex", juce::var(lastKeyIndex), nullptr);
	state.setProperty("isGenerating", juce::var(isGenerating), nullptr);
	state.setProperty("autoLoadEnabled", juce::var(autoLoadEnabled.load()), nullptr);
	state.setProperty("generatingTrackId", juce::var(generatingTrackId), nullptr);

	juce::ValueTree midiMappingsState("MidiMappings");
	auto mappings = midiLearnManager.getAllMappings();
	for (int i = 0; i < mappings.size(); ++i)
	{
		const auto& mapping = mappings[i];
		juce::ValueTree mappingState("Mapping");
		mappingState.setProperty("midiType", mapping.midiType, nullptr);
		mappingState.setProperty("midiNumber", mapping.midiNumber, nullptr);
		mappingState.setProperty("midiChannel", mapping.midiChannel, nullptr);
		mappingState.setProperty("parameterName", mapping.parameterName, nullptr);
		mappingState.setProperty("description", mapping.description, nullptr);
		midiMappingsState.appendChild(mappingState, nullptr);
	}
	state.appendChild(midiMappingsState, nullptr);

	auto tracksState = trackManager.saveState();
	state.appendChild(tracksState, nullptr);

	juce::ValueTree parametersState("Parameters");

	auto& params = getParameterTreeState();
	for (const auto& paramId : booleanParamIds)
	{
		auto* param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	for (const auto& paramId : floatParamIds)
	{
		auto* param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	state.appendChild(parametersState, nullptr);

	auto globalGenState = juce::ValueTree("GlobalGeneration");
	globalGenState.setProperty("prompt", globalPrompt, nullptr);
	globalGenState.setProperty("bpm", globalBpm, nullptr);
	globalGenState.setProperty("key", globalKey, nullptr);
	globalGenState.setProperty("duration", globalDuration, nullptr);
	juce::String stemsString;
	for (int i = 0; i < globalStems.size(); ++i)
	{
		if (i > 0)
			stemsString += ",";
		stemsString += globalStems[i];
	}
	globalGenState.setProperty("stems", stemsString, nullptr);
	state.appendChild(globalGenState, nullptr);

	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void DjIaVstProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
	if (!xml || !xml->hasTagName("DjIaVstState"))
	{
		return;
	}
	juce::ValueTree state = juce::ValueTree::fromXml(*xml);

	lastPrompt = state.getProperty("lastPrompt", "").toString();
	lastKey = state.getProperty("lastKey", "C minor").toString();
	lastBpm = state.getProperty("lastBpm", 126.0);
	lastPresetIndex = state.getProperty("lastPresetIndex", -1);
	hostBpmEnabled = state.getProperty("hostBpmEnabled", false);
	lastDuration = state.getProperty("lastDuration", 6.0);
	drumsEnabled = state.getProperty("drumsEnabled", false);
	bassEnabled = state.getProperty("bassEnabled", false);
	otherEnabled = state.getProperty("otherEnabled", false);
	vocalsEnabled = state.getProperty("vocalsEnabled", false);
	guitarEnabled = state.getProperty("guitarEnabled", false);
	pianoEnabled = state.getProperty("pianoEnabled", false);
	lastKeyIndex = state.getProperty("lastKeyIndex", 1);
	isGenerating = state.getProperty("isGenerating", false);
	generatingTrackId = state.getProperty("generatingTrackId", "").toString();
	autoLoadEnabled.store(state.getProperty("autoLoadEnabled", true));
	auto tracksState = state.getChildWithName("TrackManager");
	if (tracksState.isValid())
	{
		trackManager.loadState(tracksState, cachedHostBpm.load());
	}

	selectedTrackId = state.getProperty("selectedTrackId", "").toString();
	auto loadedTrackIds = trackManager.getAllTrackIds();

	if (selectedTrackId.isEmpty() || !trackManager.getTrack(selectedTrackId))
	{
		if (!loadedTrackIds.empty())
		{
			selectedTrackId = loadedTrackIds[0];
		}
		else
		{
			selectedTrackId = trackManager.createTrack("Main");
		}
	}
	juce::ValueTree midiMappingsState = state.getChildWithName("MidiMappings");
	if (midiMappingsState.isValid())
	{
		midiLearnManager.clearAllMappings();
		for (int i = 0; i < midiMappingsState.getNumChildren(); ++i)
		{
			MidiMapping mapping;
			juce::ValueTree mappingState = midiMappingsState.getChild(i);
			mapping.midiType = mappingState.getProperty("midiType");
			mapping.midiNumber = mappingState.getProperty("midiNumber");
			mapping.midiChannel = mappingState.getProperty("midiChannel");
			mapping.parameterName = mappingState.getProperty("parameterName");
			mapping.description = mappingState.getProperty("description");
			mapping.processor = this;
			mapping.uiCallback = nullptr;
			midiLearnManager.addMapping(mapping);
		}
	}
	auto globalGenState = state.getChildWithName("GlobalGeneration");
	if (globalGenState.isValid())
	{
		globalPrompt = globalGenState.getProperty("prompt", "Generate a techno drum loop");
		globalBpm = globalGenState.getProperty("bpm", 127.0f);
		globalKey = globalGenState.getProperty("key", "C Minor");
		globalDuration = globalGenState.getProperty("duration", 6);
		juce::String stemsString = globalGenState.getProperty("stems", "drums,bass");
		globalStems.clear();
		if (stemsString.isNotEmpty())
		{
			juce::StringArray stemsArray = juce::StringArray::fromTokens(stemsString, ",", "");
			for (const auto& stem : stemsArray)
			{
				globalStems.push_back(stem.trim());
			}
		}
	}
	auto parametersState = state.getChildWithName("Parameters");
	if (parametersState.isValid())
	{
		auto& params = getParameterTreeState();
		for (const auto& paramId : booleanParamIds)
		{
			if (parametersState.hasProperty(paramId))
			{
				auto* param = params.getParameter(paramId);
				if (param)
				{
					float value = parametersState.getProperty(paramId, 0.0f);
					param->setValueNotifyingHost(value);
				}
			}
		}
		for (const auto& paramId : floatParamIds)
		{
			if (parametersState.hasProperty(paramId))
			{
				auto* param = params.getParameter(paramId);
				if (param)
				{
					param->setValueNotifyingHost(parametersState.getProperty(paramId, 0.0f));
				}
			}
		}
	}

	midiLearnManager.restoreUICallbacks();
	stateLoaded = true;
	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
				editor->refreshTrackComponents();
				editor->updateUIFromProcessor();
			} });
}

TrackComponent* DjIaVstProcessor::findTrackComponentByName(const juce::String& trackName, DjIaVstEditor* editor)
{
	for (auto& trackComp : editor->getTrackComponents())
	{
		if (auto* track = trackComp->getTrack())
		{
			if (track->trackName == trackName)
				return trackComp.get();
		}
	}
	return nullptr;
}

juce::Button* DjIaVstProcessor::findGenerateButtonInTrack(TrackComponent* trackComponent)
{
	return trackComponent->getGenerateButton();
}

juce::Slider* DjIaVstProcessor::findBpmOffsetSliderInTrack(TrackComponent* trackComponent)
{
	return trackComponent->getBpmOffsetSlider();
}

void DjIaVstProcessor::updateUI()
{
	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		editor->updateUIFromProcessor();
		juce::Timer::callAfterDelay(50, [this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					editor->refreshTrackComponents();
					juce::Timer::callAfterDelay(100, [this]()
						{
							updateAllWaveformsAfterLoad();
						});
				} });
	}
}

void DjIaVstProcessor::addCustomPromptsToIndexedPrompts(juce::ValueTree& promptsState, juce::Array<std::pair<int, juce::String>>& indexedPrompts)
{
	for (int i = 0; i < promptsState.getNumProperties(); ++i)
	{
		auto propertyName = promptsState.getPropertyName(i);
		if (propertyName.toString().startsWith("prompt_"))
		{
			juce::String indexStr = propertyName.toString().substring(7);
			int index = indexStr.getIntValue();
			juce::String prompt = promptsState.getProperty(propertyName, "").toString();

			if (prompt.isNotEmpty())
			{
				indexedPrompts.add({ index, prompt });
			}
		}
	}
}

void DjIaVstProcessor::loadCustomPromptsByCountProperty(juce::ValueTree& promptsState)
{
	for (int i = 0; i < promptsState.getNumChildren(); ++i)
	{
		auto promptNode = promptsState.getChild(i);
		if (promptNode.hasType("Prompt"))
		{
			juce::String prompt = promptNode.getProperty("text", "").toString();
			if (prompt.isNotEmpty())
			{
				customPrompts.add(prompt);
			}
		}
	}
}

void DjIaVstProcessor::updateAllWaveformsAfterLoad()
{
	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto& trackId : trackIds)
		{
			TrackData* track = trackManager.getTrack(trackId);
			if (track && track->numSamples > 0)
			{
				updateWaveformDisplay(trackId);
			}
		}
	}
}

void DjIaVstProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
	if (parameterID == "generate" && newValue > 0.5f)
	{
		juce::MessageManager::callAsync([this]()
			{ parameters.getParameter("generate")->setValueNotifyingHost(0.0f); });
	}
}

void DjIaVstProcessor::removeCustomPrompt(const juce::String& prompt)
{
	customPrompts.removeString(prompt);
	saveGlobalConfig();
}

void DjIaVstProcessor::editCustomPrompt(const juce::String& oldPrompt, const juce::String& newPrompt)
{
	int index = customPrompts.indexOf(oldPrompt);
	if (index >= 0 && !newPrompt.isEmpty() && !customPrompts.contains(newPrompt))
	{
		customPrompts.set(index, newPrompt);
		saveGlobalConfig();
	}
}

void DjIaVstProcessor::executePendingAction(TrackData* track) const
{
	switch (track->pendingAction)
	{
	case TrackData::PendingAction::StartOnNextMeasure:
		if (!track->isPlaying.load() && track->isArmed.load())
		{
			track->readPosition = 0.0;
			track->sequencerData.currentStep = 0;
			track->sequencerData.currentMeasure = 0;
			track->sequencerData.stepAccumulator = 0.0;
			track->isCurrentlyPlaying = true;
		}
		break;

	case TrackData::PendingAction::StopOnNextMeasure:
		track->isPlaying = false;
		track->isArmedToStop = false;
		track->isCurrentlyPlaying = false;
		if (onUIUpdateNeeded)
			onUIUpdateNeeded();
		break;

	default:
		break;
	}

	track->pendingAction = TrackData::PendingAction::None;
}

void DjIaVstProcessor::updateSequencers(bool hostIsPlaying)
{
	auto currentPlayHead = getPlayHead();
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
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
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
			}

			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				juce::Component::SafePointer<DjIaVstEditor> safeEditor(editor);
				juce::MessageManager::callAsync([safeEditor, trackId]()
					{
						if (safeEditor.getComponent() != nullptr)
						{
							if (auto* sequencer = static_cast<SequencerComponent*>(safeEditor->getSequencerForTrack(trackId)))
							{
								sequencer->updateFromTrackData();
							}
						} });
			}
		}
	}
}

void DjIaVstProcessor::handleAdvanceStep(TrackData* track, bool hostIsPlaying)
{
	int numerator = getTimeSignatureNumerator();
	int denominator = getTimeSignatureDenominator();

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

	int stepsPerMeasure = numerator * stepsPerBeat;
	int newStep = track->customStepCounter % stepsPerMeasure;
	int newMeasure = (track->customStepCounter / stepsPerMeasure) % track->sequencerData.numMeasures;

	int safeMeasure = juce::jlimit(0, track->sequencerData.numMeasures - 1, newMeasure);
	int safeStep = juce::jlimit(0, stepsPerMeasure - 1, newStep);

	bool currentStepIsActive = track->sequencerData.steps[safeMeasure][safeStep];

	if (newMeasure == 0 && track->isArmed.load() && newStep == 0 && !track->isPlaying.load() && hostIsPlaying)
	{
		track->pendingAction = TrackData::PendingAction::StartOnNextMeasure;
	}

	if ((newMeasure == 0 && newStep == 0) && track->pendingAction != TrackData::PendingAction::None)
	{
		executePendingAction(track);
	}

	track->sequencerData.currentStep = newStep;
	track->sequencerData.currentMeasure = newMeasure;

	if (currentStepIsActive &&
		track->isCurrentlyPlaying.load() && hostIsPlaying)
	{

		track->readPosition = 0.0;
		track->setPlaying(true);
		triggerSequencerStep(track);
	}
}

void DjIaVstProcessor::triggerSequencerStep(TrackData* track)
{
	int step = track->sequencerData.currentStep;
	int measure = track->sequencerData.currentMeasure;
	track->isArmed = false;
	if (track->sequencerData.steps[measure][step])
	{
		track->readPosition = 0.0;
		playingTracks[track->midiNote] = track->trackId;
		juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, track->midiNote,
			(juce::uint8)(track->sequencerData.velocities[measure][step] * 127));
		addSequencerMidiMessage(noteOn);
	}
}