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
	for (int i = 0; i < MAX_TRACKS + 1; ++i)
	{
		layout = layout.withOutput("Track " + juce::String(i + 1),
			juce::AudioChannelSet::stereo(), false);
	}
	return layout;
}

DjIaVstProcessor::DjIaVstProcessor()
	: AudioProcessor(createBusLayout()), apiClient("", "http://localhost:8000"),
	parameters(*this, nullptr, "Parameters", { std::make_unique<juce::AudioParameterBool>("generate", "Generate Loop", false), std::make_unique<juce::AudioParameterBool>("play", "Play Loop", false), std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1Volume", "Slot 1 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot1Pan", "Slot 1 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot1Mute", "Slot 1 Mute", false), std::make_unique<juce::AudioParameterBool>("slot1Solo", "Slot 1 Solo", false), std::make_unique<juce::AudioParameterBool>("slot1Play", "Slot 1 Play", false), std::make_unique<juce::AudioParameterBool>("slot1Stop", "Slot 1 Stop", false), std::make_unique<juce::AudioParameterBool>("slot1Generate", "Slot 1 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot1Pitch", "Slot 1 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1Fine", "Slot 1 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot1BpmOffset", "Slot 1 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2Volume", "Slot 2 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot2Pan", "Slot 2 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot2Mute", "Slot 2 Mute", false), std::make_unique<juce::AudioParameterBool>("slot2Solo", "Slot 2 Solo", false), std::make_unique<juce::AudioParameterBool>("slot2Play", "Slot 2 Play", false), std::make_unique<juce::AudioParameterBool>("slot2Stop", "Slot 2 Stop", false), std::make_unique<juce::AudioParameterBool>("slot2Generate", "Slot 2 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot2Pitch", "Slot 2 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2Fine", "Slot 2 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot2BpmOffset", "Slot 2 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3Volume", "Slot 3 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot3Pan", "Slot 3 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot3Mute", "Slot 3 Mute", false), std::make_unique<juce::AudioParameterBool>("slot3Solo", "Slot 3 Solo", false), std::make_unique<juce::AudioParameterBool>("slot3Play", "Slot 3 Play", false), std::make_unique<juce::AudioParameterBool>("slot3Stop", "Slot 3 Stop", false), std::make_unique<juce::AudioParameterBool>("slot3Generate", "Slot 3 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot3Pitch", "Slot 3 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3Fine", "Slot 3 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot3BpmOffset", "Slot 3 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4Volume", "Slot 4 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot4Pan", "Slot 4 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot4Mute", "Slot 4 Mute", false), std::make_unique<juce::AudioParameterBool>("slot4Solo", "Slot 4 Solo", false), std::make_unique<juce::AudioParameterBool>("slot4Play", "Slot 4 Play", false), std::make_unique<juce::AudioParameterBool>("slot4Stop", "Slot 4 Stop", false), std::make_unique<juce::AudioParameterBool>("slot4Generate", "Slot 4 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot4Pitch", "Slot 4 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4Fine", "Slot 4 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot4BpmOffset", "Slot 4 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5Volume", "Slot 5 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot5Pan", "Slot 5 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot5Mute", "Slot 5 Mute", false), std::make_unique<juce::AudioParameterBool>("slot5Solo", "Slot 5 Solo", false), std::make_unique<juce::AudioParameterBool>("slot5Play", "Slot 5 Play", false), std::make_unique<juce::AudioParameterBool>("slot5Stop", "Slot 5 Stop", false), std::make_unique<juce::AudioParameterBool>("slot5Generate", "Slot 5 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot5Pitch", "Slot 5 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5Fine", "Slot 5 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot5BpmOffset", "Slot 5 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6Volume", "Slot 6 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot6Pan", "Slot 6 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot6Mute", "Slot 6 Mute", false), std::make_unique<juce::AudioParameterBool>("slot6Solo", "Slot 6 Solo", false), std::make_unique<juce::AudioParameterBool>("slot6Play", "Slot 6 Play", false), std::make_unique<juce::AudioParameterBool>("slot6Stop", "Slot 6 Stop", false), std::make_unique<juce::AudioParameterBool>("slot6Generate", "Slot 6 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot6Pitch", "Slot 6 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6Fine", "Slot 6 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot6BpmOffset", "Slot 6 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7Volume", "Slot 7 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot7Pan", "Slot 7 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot7Mute", "Slot 7 Mute", false), std::make_unique<juce::AudioParameterBool>("slot7Solo", "Slot 7 Solo", false), std::make_unique<juce::AudioParameterBool>("slot7Play", "Slot 7 Play", false), std::make_unique<juce::AudioParameterBool>("slot7Stop", "Slot 7 Stop", false), std::make_unique<juce::AudioParameterBool>("slot7Generate", "Slot 7 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot7Pitch", "Slot 7 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7Fine", "Slot 7 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot7BpmOffset", "Slot 7 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8Volume", "Slot 8 Volume", 0.0f, 1.0f, 0.8f), std::make_unique<juce::AudioParameterFloat>("slot8Pan", "Slot 8 Pan", -1.0f, 1.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot8Mute", "Slot 8 Mute", false), std::make_unique<juce::AudioParameterBool>("slot8Solo", "Slot 8 Solo", false), std::make_unique<juce::AudioParameterBool>("slot8Play", "Slot 8 Play", false), std::make_unique<juce::AudioParameterBool>("slot8Stop", "Slot 8 Stop", false), std::make_unique<juce::AudioParameterBool>("slot8Generate", "Slot 8 Generate", false), std::make_unique<juce::AudioParameterFloat>("slot8Pitch", "Slot 8 Pitch", -12.0f, 12.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8Fine", "Slot 8 Fine", -50.0f, 50.0f, 0.0f), std::make_unique<juce::AudioParameterFloat>("slot8BpmOffset", "Slot 8 BPM Offset", -20.0f, 20.0f, 0.0f), std::make_unique<juce::AudioParameterBool>("slot1RandomRetrigger", "Slot 1 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot1RetriggerInterval", "Slot 1 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot2RandomRetrigger", "Slot 2 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot2RetriggerInterval", "Slot 2 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot3RandomRetrigger", "Slot 3 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot3RetriggerInterval", "Slot 3 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot4RandomRetrigger", "Slot 4 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot4RetriggerInterval", "Slot 4 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot5RandomRetrigger", "Slot 5 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot5RetriggerInterval", "Slot 5 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot6RandomRetrigger", "Slot 6 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot6RetriggerInterval", "Slot 6 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot7RandomRetrigger", "Slot 7 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot7RetriggerInterval", "Slot 7 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("slot8RandomRetrigger", "Slot 8 Random Retrigger", false), std::make_unique<juce::AudioParameterFloat>("slot8RetriggerInterval", "Slot 8 Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f), std::make_unique<juce::AudioParameterBool>("nextTrack", "Next Track", false), std::make_unique<juce::AudioParameterBool>("prevTrack", "Previous Track", false), std::make_unique<juce::AudioParameterBool>("slot1PageA", "Slot 1 Page A", false), std::make_unique<juce::AudioParameterBool>("slot1PageB", "Slot 1 Page B", false), std::make_unique<juce::AudioParameterBool>("slot1PageC", "Slot 1 Page C", false), std::make_unique<juce::AudioParameterBool>("slot1PageD", "Slot 1 Page D", false), std::make_unique<juce::AudioParameterBool>("slot2PageA", "Slot 2 Page A", false), std::make_unique<juce::AudioParameterBool>("slot2PageB", "Slot 2 Page B", false), std::make_unique<juce::AudioParameterBool>("slot2PageC", "Slot 2 Page C", false), std::make_unique<juce::AudioParameterBool>("slot2PageD", "Slot 2 Page D", false), std::make_unique<juce::AudioParameterBool>("slot3PageA", "Slot 3 Page A", false), std::make_unique<juce::AudioParameterBool>("slot3PageB", "Slot 3 Page B", false), std::make_unique<juce::AudioParameterBool>("slot3PageC", "Slot 3 Page C", false), std::make_unique<juce::AudioParameterBool>("slot3PageD", "Slot 3 Page D", false), std::make_unique<juce::AudioParameterBool>("slot4PageA", "Slot 4 Page A", false), std::make_unique<juce::AudioParameterBool>("slot4PageB", "Slot 4 Page B", false), std::make_unique<juce::AudioParameterBool>("slot4PageC", "Slot 4 Page C", false), std::make_unique<juce::AudioParameterBool>("slot4PageD", "Slot 4 Page D", false), std::make_unique<juce::AudioParameterBool>("slot5PageA", "Slot 5 Page A", false), std::make_unique<juce::AudioParameterBool>("slot5PageB", "Slot 5 Page B", false), std::make_unique<juce::AudioParameterBool>("slot5PageC", "Slot 5 Page C", false), std::make_unique<juce::AudioParameterBool>("slot5PageD", "Slot 5 Page D", false), std::make_unique<juce::AudioParameterBool>("slot6PageA", "Slot 6 Page A", false), std::make_unique<juce::AudioParameterBool>("slot6PageB", "Slot 6 Page B", false), std::make_unique<juce::AudioParameterBool>("slot6PageC", "Slot 6 Page C", false), std::make_unique<juce::AudioParameterBool>("slot6PageD", "Slot 6 Page D", false), std::make_unique<juce::AudioParameterBool>("slot7PageA", "Slot 7 Page A", false), std::make_unique<juce::AudioParameterBool>("slot7PageB", "Slot 7 Page B", false), std::make_unique<juce::AudioParameterBool>("slot7PageC", "Slot 7 Page C", false), std::make_unique<juce::AudioParameterBool>("slot7PageD", "Slot 7 Page D", false), std::make_unique<juce::AudioParameterBool>("slot8PageA", "Slot 8 Page A", false), std::make_unique<juce::AudioParameterBool>("slot8PageB", "Slot 8 Page B", false), std::make_unique<juce::AudioParameterBool>("slot8PageC", "Slot 8 Page C", false), std::make_unique<juce::AudioParameterBool>("slot8PageD", "Slot 8 Page D", false) })
{
	midiLearnManager.setProcessor(this);
	projectId = "legacy";
	loadGlobalConfig();
	obsidianEngine = std::make_unique<ObsidianEngine>();
	sharedFormatManager.registerBasicFormats();
	if (!obsidianEngine->initialize())
	{
		DBG("Failed to initialize OBSIDIAN Engine");
	}
	else
	{
		DBG("OBSIDIAN Engine ready!");
	}
	sampleBankInitFuture = std::async(std::launch::async, [this]()
		{
			sampleBank = std::make_unique<SampleBank>();
			sampleBankReady = true; });
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
	juce::Timer::callAfterDelay(1000, [this]()
		{ performMigrationIfNeeded(); });
}

void DjIaVstProcessor::performMigrationIfNeeded()
{
	if (migrationCompleted)
		return;

	auto legacyDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("AudioCache");

	auto trackIds = trackManager.getAllTrackIds();
	juce::Array<juce::File> filesToMigrate;

	for (const auto& trackId : trackIds)
	{
		auto mainFile = legacyDir.getChildFile(trackId + ".wav");
		if (mainFile.existsAsFile())
		{
			filesToMigrate.add(mainFile);
		}

		auto originalFile = legacyDir.getChildFile(trackId + "_original.wav");
		if (originalFile.existsAsFile())
		{
			filesToMigrate.add(originalFile);
		}
	}

	if (filesToMigrate.size() > 0 && projectId == "legacy")
	{
		projectId = juce::Uuid().toString();
		auto newProjectDir = legacyDir.getChildFile(projectId);
		newProjectDir.createDirectory();

		for (auto& file : filesToMigrate)
		{
			auto newLocation = newProjectDir.getChildFile(file.getFileName());
			file.moveFileTo(newLocation);
			DBG("Migrated: " + file.getFileName() + " to project folder");
		}

		updateTrackPathsAfterMigration();
		DBG("Migration completed for " + juce::String(filesToMigrate.size()) + " files");
	}
	else if (projectId == "legacy")
	{
		projectId = juce::Uuid().toString();
	}

	migrationCompleted = true;
}

void DjIaVstProcessor::updateTrackPathsAfterMigration()
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (track && !track->audioFilePath.isEmpty())
		{
			juce::File oldPath(track->audioFilePath);
			if (oldPath.exists())
			{
				track->audioFilePath = getTrackAudioFile(trackId).getFullPathName();
			}
		}
	}
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
			auto keywordsVar = object->getProperty("customKeywords");
			if (keywordsVar.isArray())
			{
				customKeywords.clear();
				auto* keywordsArray = keywordsVar.getArray();
				for (int i = 0; i < keywordsArray->size(); ++i)
				{
					juce::String keyword = keywordsArray->getUnchecked(i).toString();
					customKeywords.add(keyword);
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

	juce::Array<juce::var> keywordsArray;
	for (const auto& keyword : customKeywords)
	{
		keywordsArray.add(juce::var(keyword));
	}
	config->setProperty("customKeywords", juce::var(keywordsArray));

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
		slotRandomRetriggerParams[i] = parameters.getRawParameterValue(slotName + "RandomRetrigger");
		slotRetriggerIntervalParams[i] = parameters.getRawParameterValue(slotName + "RetriggerInterval");
	}
	for (int slot = 1; slot <= 8; ++slot)
	{
		for (const char* page : { "PageA", "PageB", "PageC", "PageD" })
		{
			juce::String paramName = "slot" + juce::String(slot) + page;
			parameters.addParameterListener(paramName, this);
		}
	}

	nextTrackParam = parameters.getRawParameterValue("nextTrack");
	prevTrackParam = parameters.getRawParameterValue("prevTrack");

	parameters.addParameterListener("nextTrack", this);
	parameters.addParameterListener("prevTrack", this);

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
	parameters.removeParameterListener("nextTrack", this);
	parameters.removeParameterListener("prevTrack", this);
	for (int slot = 1; slot <= 8; ++slot)
	{
		for (const char* page : { "PageA", "PageB", "PageC", "PageD" })
		{
			juce::String paramName = "slot" + juce::String(slot) + page;
			parameters.removeParameterListener(paramName, this);
		}
	}
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
	internalSampleCounter += buffer.getNumSamples();
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
		lastHostBpmForQuantization.store(hostBpm);
	}
	handleSequencerPlayState(hostIsPlaying);
	updateSequencers(hostIsPlaying);
	checkBeatRepeatWithSampleCounter();
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
	handlePreviewPlaying(buffer);
	applyMasterEffects(mainOutput);
	checkIfUIUpdateNeeded(midiMessages);
}

void DjIaVstProcessor::clearMasterChannel(juce::AudioSampleBuffer& mainOutput)
{
	bool isMultiOutputActive = getBusCount(false) > 1;
	if (isMultiOutputActive)
	{
		mainOutput.clear();
	}
}

void DjIaVstProcessor::handlePreviewPlaying(juce::AudioSampleBuffer& buffer)
{
	if (isPreviewPlaying.load())
	{
		juce::ScopedLock lock(previewLock);
		if (previewBuffer.getNumSamples() > 0)
		{
			double currentPos = previewPosition.load();
			double ratio = previewSampleRate.load() / hostSampleRate;

			const int previewBusIndex = 9;
			auto previewOutput = getBusBuffer(buffer, false, previewBusIndex);

			for (int i = 0; i < buffer.getNumSamples(); ++i)
			{
				int sampleIndex = (int)currentPos;
				if (sampleIndex >= previewBuffer.getNumSamples())
				{
					isPreviewPlaying = false;
					break;
				}
				for (int ch = 0; ch < std::min(previewOutput.getNumChannels(), 2); ++ch)
				{
					float sample = previewBuffer.getSample(ch, sampleIndex) * 0.7f;
					previewOutput.addSample(ch, i, sample);
				}
				currentPos += ratio;
			}
			previewPosition.store(currentPos);
		}
	}
}

void DjIaVstProcessor::addSequencerMidiMessage(const juce::MidiMessage& message)
{
	juce::ScopedLock lock(sequencerMidiLock);
	sequencerMidiBuffer.addEvent(message, 0);
}

void DjIaVstProcessor::handleSequencerPlayState(bool hostIsPlaying)
{
	if (getBypassSequencer())
	{
		return;
	}
	static bool wasPlaying = false;

	if (hostIsPlaying && !wasPlaying)
	{
		internalSampleCounter.store(0);
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
	if (!currentPreviewTrackId.isEmpty() && currentPreviewTrackId != trackId)
	{
		stopTrackPreview(currentPreviewTrackId);
	}

	TrackData* track = trackManager.getTrack(trackId);
	if (track && track->numSamples > 0)
	{
		track->readPosition = 0.0;
		track->isPlaying.store(true);
		needsUIUpdate = true;

		currentPreviewTrackId = trackId;

		if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
		{
			auto* trackComp = editor->getTrackComponent(trackId);
			if (trackComp)
			{
				trackComp->setPreviewPlaying(true);
			}
		}
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

void DjIaVstProcessor::generateSampleWithImage(const juce::String& trackId, const juce::String& base64Image, const juce::StringArray& keywords)
{
	if (isGenerating)
	{
		return;
	}

	TrackData* track = trackManager.getTrack(trackId);
	if (!track)
	{
		return;
	}

	setIsGenerating(true);
	setGeneratingTrackId(trackId);

	juce::MessageManager::callAsync([this, trackId]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->startGenerationUI(trackId);
				editor->statusLabel.setText("Analyzing image and generating audio...", juce::dontSendNotification);
			} });

			juce::Thread::launch([this, trackId, base64Image, keywords]()
				{
					try
					{
						TrackData* track = trackManager.getTrack(trackId);
						if (!track)
						{
							throw std::runtime_error("Track not found");
						}

						DjIaClient::LoopRequest request;
						request.generationDuration = static_cast<float>(getGlobalDuration());

						if (track->usePages.load())
						{
							auto& currentPage = track->getCurrentPage();
							request.bpm = currentPage.generationBpm > 0 ? currentPage.generationBpm : static_cast<float>(getHostBpm());
							request.key = !currentPage.generationKey.isEmpty() ? currentPage.generationKey : getGlobalKey();
							request.generationDuration = currentPage.generationDuration > 0 ? currentPage.generationDuration : static_cast<float>(getGlobalDuration());
						}
						else
						{
							request.bpm = track->generationBpm > 0 ? track->generationBpm : static_cast<float>(getHostBpm());
							request.key = !track->generationKey.isEmpty() ? track->generationKey : getGlobalKey();
							request.generationDuration = track->generationDuration > 0 ? track->generationDuration : static_cast<float>(getGlobalDuration());
						}

						if (request.bpm <= 0) request.bpm = 127.0f;
						if (request.key.isEmpty()) request.key = "C Minor";
						if (request.generationDuration <= 0) request.generationDuration = 6.0f;

						request.prompt = "";
						request.useImage = true;
						request.imageBase64 = base64Image;
						request.keywords = keywords;

						generateLoopWithImage(request, trackId, 300000);
					}
					catch (const std::exception& e)
					{
						setIsGenerating(false);
						setGeneratingTrackId("");

						juce::String errorMessage = juce::String(e.what());

						juce::MessageManager::callAsync([this, trackId, errorMessage]()
							{
								if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
								{
									editor->stopGenerationUI(trackId, false, errorMessage);
								}
							});
					} });
}

void DjIaVstProcessor::generateLoopWithImage(const DjIaClient::LoopRequest& request, const juce::String& trackId, int timeoutMS)
{
	auto response = apiClient.generateLoop(request, hostSampleRate, timeoutMS);

	try
	{
		if (!response.errorMessage.isEmpty())
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "ERROR: " + response.errorMessage);
			return;
		}

		if (response.audioData.getFullPathName().isEmpty() ||
			!response.audioData.exists() ||
			response.audioData.getSize() == 0)
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			reEnableCanvasGenerate();
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
		pendingDetectedBpm = response.detectedBpm;
		hasPendingAudioData = true;
		waitingForMidiToLoad = true;
		trackIdWaitingForLoad = trackId;
		correctMidiNoteReceived = false;
	}

	if (TrackData* track = trackManager.getTrack(trackId))
	{
		juce::String generatedPrompt = "Generated from image";

		if (track->usePages.load())
		{
			auto& currentPage = track->getCurrentPage();
			currentPage.prompt = generatedPrompt;
			currentPage.generationPrompt = generatedPrompt;
			currentPage.generationKey = response.key;
			track->syncLegacyProperties();
		}
		else
		{
			track->prompt = generatedPrompt;
			track->generationPrompt = generatedPrompt;
			track->generationKey = response.key;
		}
	}

	setIsGenerating(false);
	setGeneratingTrackId("");
	reEnableCanvasGenerate();

	juce::String successMessage = "Audio generated from image! Press Play to listen.";

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

void DjIaVstProcessor::reEnableCanvasGenerate()
{
	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->reEnableCanvasForTrack();
			} });
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
						request.generationDuration = static_cast<float>(getGlobalDuration());

						if (track->usePages.load()) {
							auto& currentPage = track->getCurrentPage();

							if (!currentPage.selectedPrompt.isEmpty()) {
								request.prompt = currentPage.selectedPrompt;
								request.bpm = currentPage.generationBpm > 0 ? currentPage.generationBpm : static_cast<float>(getHostBpm());
								request.key = !currentPage.generationKey.isEmpty() ? currentPage.generationKey : getGlobalKey();
							}
							else {
								request = createGlobalLoopRequest();
								currentPage.selectedPrompt = request.prompt;
								currentPage.generationBpm = request.bpm;
								currentPage.generationKey = request.key;
							}

							track->syncLegacyProperties();
						}
						else {
							if (!track->selectedPrompt.isEmpty()) {
								request.prompt = track->selectedPrompt;
								request.bpm = static_cast<float>(getHostBpm());
								request.key = getGlobalKey();
							}
							else {
								request = createGlobalLoopRequest();
							}
							track->updateFromRequest(request);
						}

						juce::String promptSource = !request.prompt.isEmpty() ?
							"track prompt: " + request.prompt.substring(0, 20) + "..." :
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
	float paramRandomRetrigger = slotRandomRetriggerParams[slot]->load();
	float paramRetriggerInterval = slotRetriggerIntervalParams[slot]->load();

	bool isRetriggerEnabled = paramRandomRetrigger > 0.5f;
	int retriggerInterval = juce::jlimit(1, 10, (int)juce::roundToInt(paramRetriggerInterval));

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
	if (track->randomRetriggerEnabled.load() != isRetriggerEnabled)
	{
		track->randomRetriggerEnabled = isRetriggerEnabled;
		if (!isRetriggerEnabled)
		{
			track->beatRepeatStopPending.store(true);
		}
		else
		{
			track->beatRepeatPending.store(true);
		}
	}

	if (track->randomRetriggerInterval.load() != retriggerInterval)
	{
		track->randomRetriggerInterval = retriggerInterval;

		if (track->beatRepeatActive.load())
		{
			double hostBpm = lastHostBpmForQuantization.load();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double startPosition = track->beatRepeatStartPosition.load();

			double repeatDuration = calculateRetriggerInterval(retriggerInterval, hostBpm);
			double repeatDurationSamples = repeatDuration * track->sampleRate;

			track->beatRepeatEndPosition.store(startPosition + repeatDurationSamples);

			double maxSamples = track->numSamples;
			if (track->beatRepeatEndPosition.load() > maxSamples)
			{
				track->beatRepeatEndPosition.store(maxSamples);
			}
		}
	}
}

void DjIaVstProcessor::checkBeatRepeatWithSampleCounter()
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (!track)
			continue;

		if (track->beatRepeatPending.load())
		{
			double hostBpm = lastHostBpmForQuantization.load();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double halfBeatDurationSamples = (60.0 / hostBpm) * hostSampleRate * 0.5;
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
					auto* param = getParameterTreeState().getParameter(paramName);
					if (param)
					{
						float normalizedValue = (randomInterval - 1.0f) / 9.0f;
						param->setValueNotifyingHost(normalizedValue);
					}
				}

				double currentPosition = track->readPosition.load();
				double repeatDuration = calculateRetriggerInterval(track->randomRetriggerInterval.load(), hostBpm);
				double repeatDurationSamples = repeatDuration * track->sampleRate;

				track->originalReadPosition.store(currentPosition);
				track->beatRepeatStartPosition.store(currentPosition);
				track->beatRepeatEndPosition.store(currentPosition + repeatDurationSamples);

				double maxSamples = track->numSamples;
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
			double hostBpm = lastHostBpmForQuantization.load();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double halfBeatDurationSamples = (60.0 / hostBpm) * hostSampleRate * 0.5;
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
				DBG("Beat repeat stopped at sample: " << currentSample);
			}
		}
	}
}

double DjIaVstProcessor::calculateRetriggerInterval(int intervalValue, double hostBpm) const
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

void DjIaVstProcessor::updateTimeStretchRatios(double hostBpm)
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (!track)
			continue;

		double ratio = 1.0;

		switch (track->timeStretchMode)
		{
		case 1:
		case 3:
			ratio = 1.0;
			break;

		case 2:
		case 4:
			if (track->originalBpm > 0.0f && hostBpm > 0.0)
			{
				double hostRatio = hostBpm / track->originalBpm;
				double manualAdjust = track->bpmOffset / track->originalBpm;
				ratio = hostRatio + manualAdjust;
			}
			break;
		}

		ratio = juce::jlimit(0.25, 4.0, ratio);
		track->cachedPlaybackRatio = ratio;
	}
}

void DjIaVstProcessor::startNotePlaybackForTrack(const juce::String& trackId, int noteNumber, double /*hostBpm*/)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track || track->numSamples == 0)
		return;
	if (getBypassSequencer())
	{
		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		track->setPlaying(true);
		track->isCurrentlyPlaying.store(true);
		playingTracks[noteNumber] = trackId;
		return;
	}
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

	if (!track->beatRepeatActive.load())
	{
		track->readPosition = 0.0;
	}
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
	if (slotIndex != -1)
	{
		getMidiLearnManager().removeMappingForParameter("promptSelector_slot" + juce::String(slotIndex + 1));
		getMidiLearnManager().removeMappingsForSlot(slotIndex + 1);
	}
	if (sampleBank && !trackToDelete->currentSampleId.isEmpty())
	{
		sampleBank->markSampleAsUnused(trackToDelete->currentSampleId, projectId);
	}
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

	juce::String trackToSelect = selectedTrackId;

	if (trackId == selectedTrackId)
	{
		if (trackIds.size() > 1)
		{
			if (deletedTrackIndex < trackIds.size() - 1)
			{
				trackToSelect = trackIds[deletedTrackIndex + 1];
			}
			else if (deletedTrackIndex > 0)
			{
				trackToSelect = trackIds[deletedTrackIndex - 1];
			}
		}
		else
		{
			trackToSelect = trackManager.createTrack("Track");
		}
		selectedTrackId = trackToSelect;
	}

	if (slotIndex != -1)
	{
		getMidiLearnManager().removeMappingsForSlot(slotIndex + 1);
	}
	trackManager.removeTrack(trackId);
	reassignTrackOutputsAndMidi();

	juce::MessageManager::callAsync([this, trackToSelect]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->refreshTrackComponents();
				editor->refreshWavevormsAndSequencers();

				juce::Timer::callAfterDelay(50, [editor, trackToSelect]()
					{
						if (editor->mixerPanel)
						{
							editor->mixerPanel->trackSelected(trackToSelect);
						}
					});

				editor->setStatusWithTimeout("Track deleted");
			} });
}

void DjIaVstProcessor::reassignTrackOutputsAndMidi()
{
	auto trackIds = trackManager.getAllTrackIds();

	std::map<int, std::vector<MidiMapping>> savedMappings;

	for (int i = 0; i < trackIds.size(); ++i)
	{
		TrackData* track = trackManager.getTrack(trackIds[i]);
		if (track && track->slotIndex != i)
		{
			int oldSlotNumber = track->slotIndex + 1;
			int newSlotNumber = i + 1;

			auto& manager = getMidiLearnManager();
			auto allMappings = manager.getAllMappings();

			for (const auto& mapping : allMappings)
			{
				if (mapping.parameterName.startsWith("slot" + juce::String(oldSlotNumber)))
				{
					MidiMapping newMapping = mapping;

					juce::String suffix = mapping.parameterName.substring(4);
					newMapping.parameterName = "slot" + juce::String(newSlotNumber) + suffix.substring(1);

					newMapping.description = newMapping.description.replace(
						"Slot " + juce::String(oldSlotNumber),
						"Slot " + juce::String(newSlotNumber));

					savedMappings[newSlotNumber].push_back(newMapping);
				}
			}

			DBG("Track moving from slot " << oldSlotNumber << " to slot " << newSlotNumber);
		}
	}

	for (const auto& pair : savedMappings)
	{
		(void)pair;
		int oldSlotNumber = 0;
		for (int i = 0; i < trackIds.size(); ++i)
		{
			TrackData* track = trackManager.getTrack(trackIds[i]);
			if (track && track->slotIndex + 1 != i + 1)
			{
				oldSlotNumber = track->slotIndex + 1;
				getMidiLearnManager().removeMappingsForSlot(oldSlotNumber);
				break;
			}
		}
	}

	for (int i = 0; i < trackIds.size(); ++i)
	{
		TrackData* track = trackManager.getTrack(trackIds[i]);
		if (track)
		{
			track->slotIndex = i;
			track->midiNote = 60 + i;
			trackManager.usedSlots[i] = true;
		}
	}

	for (const auto& pair : savedMappings)
	{
		for (const auto& mapping : pair.second)
		{
			getMidiLearnManager().addMapping(mapping);
			DBG("Restored mapping: " << mapping.parameterName);
		}
	}

	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->refreshMixerChannels();
			} });
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
			DjIaClient::LoopRequest apiRequest = request;
			generateLoopAPI(apiRequest, trackId);
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
		reEnableCanvasGenerate();
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
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "ERROR: " + response.errorMessage);
			return;
		}

		if (response.audioData.getFullPathName().isEmpty() ||
			!response.audioData.exists() ||
			response.audioData.getSize() == 0)
		{
			setIsGenerating(false);
			setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "Invalid response from API");
			return;
		}
	}
	catch (const std::exception& /*e*/)
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "Response validation failed");
		return;
	}

	{
		const juce::ScopedLock lock(apiLock);
		pendingTrackId = trackId;
		pendingAudioFile = response.audioData;
		pendingDetectedBpm = response.detectedBpm;
		hasPendingAudioData = true;
		waitingForMidiToLoad = true;
		trackIdWaitingForLoad = trackId;
		correctMidiNoteReceived = false;
	}

	if (TrackData* track = trackManager.getTrack(trackId))
	{
		track->prompt = request.prompt;
		track->bpm = request.bpm;
	}

	setIsGenerating(false);
	setGeneratingTrackId("");
	reEnableCanvasGenerate();

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

void DjIaVstProcessor::loadSampleFromBank(const juce::String& sampleId, const juce::String& trackId)
{
	if (!sampleBank)
		return;

	auto* sampleEntry = sampleBank->getSample(sampleId);
	if (!sampleEntry)
		return;

	juce::File sampleFile(sampleEntry->filePath);
	if (!sampleFile.exists())
		return;

	TrackData* track = trackManager.getTrack(trackId);
	if (!track)
		return;

	if (!track->currentSampleId.isEmpty() && track->currentSampleId != sampleId)
	{
		sampleBank->markSampleAsUnused(track->currentSampleId, projectId);
		DBG("Marked previous sample as unused: " + track->currentSampleId);
	}

	isLoadingFromBank = true;
	currentBankLoadTrackId = trackId;
	sampleBank->markSampleAsUsed(sampleId, projectId);

	if (track)
	{
		track->currentSampleId = sampleId;
	}

	juce::Thread::launch([this, trackId, sampleFile, sampleId]()
		{
			TrackData* track = trackManager.getTrack(trackId);
			if (!track) return;

			if (track->usePages.load()) {
				loadSampleToBankPage(trackId, track->currentPageIndex, sampleFile, sampleId);
			}
			else {
				loadAudioFileAsync(trackId, sampleFile);
			}

			juce::Timer::callAfterDelay(2000, [this]()
				{
					isLoadingFromBank = false;
					currentBankLoadTrackId.clear();
				}); });
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
		reEnableCanvasGenerate();
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
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "ERROR: Local generation failed - " + result.errorMessage);
		return;
	}

	juce::File tempFile = createTempAudioFile(result.audioData, result.actualDuration);
	if (!tempFile.exists() || tempFile.getSize() == 0)
	{
		setIsGenerating(false);
		setGeneratingTrackId("");
		reEnableCanvasGenerate();
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
	}

	setIsGenerating(false);
	setGeneratingTrackId("");
	reEnableCanvasGenerate();

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
		}

		setIsGenerating(false);
		setGeneratingTrackId("");

		juce::String successMessage = juce::String::formatted(
			"Loop generated successfully! (%.1fs, %.0f BPM) Press Play to listen.",
			response.duration,
			response.bpm);

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

	if (track->usePages.load())
	{
		auto& currentPage = track->getCurrentPage();
		bool preservedHasOriginal = currentPage.hasOriginalVersion.load();
		std::swap(currentPage.audioBuffer, track->stagingBuffer);
		currentPage.numSamples = track->stagingNumSamples.load();
		currentPage.sampleRate = track->stagingSampleRate.load();
		currentPage.originalBpm = track->stagingOriginalBpm;
		currentPage.isLoaded = true;

		if (track->isVersionSwitch)
		{
			currentPage.hasOriginalVersion.store(preservedHasOriginal);
			currentPage.loopStart = track->preservedLoopStart;
			currentPage.loopEnd = track->preservedLoopEnd;
			track->loopPointsLocked = track->preservedLoopLocked;
			double maxDuration = currentPage.numSamples / currentPage.sampleRate;
			currentPage.loopEnd = std::min(currentPage.loopEnd, maxDuration);
			currentPage.loopStart = std::min(currentPage.loopStart, currentPage.loopEnd);
			track->isVersionSwitch = false;
		}
		else
		{
			currentPage.hasOriginalVersion.store(track->nextHasOriginalVersion.load());
			currentPage.useOriginalFile = false;
			double sampleDuration = currentPage.numSamples / currentPage.sampleRate;
			if (sampleDuration <= 8.0)
			{
				currentPage.loopStart = 0.0;
				currentPage.loopEnd = sampleDuration;
			}
			else
			{
				double beatDuration = 60.0 / currentPage.originalBpm;
				double fourBars = beatDuration * 16.0;
				currentPage.loopStart = 0.0;
				currentPage.loopEnd = std::min(fourBars, sampleDuration);
			}
		}
		track->syncLegacyProperties();
	}
	else
	{
		std::swap(track->audioBuffer, track->stagingBuffer);
		track->numSamples = track->stagingNumSamples.load();
		track->sampleRate = track->stagingSampleRate.load();
		track->originalBpm = track->stagingOriginalBpm;
		track->hasOriginalVersion.store(track->nextHasOriginalVersion.load());

		if (track->isVersionSwitch)
		{
			track->loopStart = track->preservedLoopStart;
			track->loopEnd = track->preservedLoopEnd;
			track->loopPointsLocked = track->preservedLoopLocked;
			double maxDuration = track->numSamples / track->sampleRate;
			track->loopEnd = std::min(track->loopEnd, maxDuration);
			track->loopStart = std::min(track->loopStart, track->loopEnd);
			track->isVersionSwitch = false;
		}
		else
		{
			track->useOriginalFile = false;
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
		}

		track->readPosition = 0.0;
		track->hasStagingData = false;
		track->stagingBuffer.setSize(0, 0);
	}

	juce::MessageManager::callAsync([this, trackId]()
		{ updateWaveformDisplay(trackId); });

	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		juce::MessageManager::callAsync([editor, trackId]()
			{ editor->onSampleLoaded(trackId); });
	}
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
		std::unique_ptr<juce::AudioFormatReader> reader(
			sharedFormatManager.createReaderFor(audioFile));

		if (!reader)
		{
			return;
		}

		loadAudioToStagingBuffer(reader, track);
		processAudioBPMAndSync(track);
		juce::File permanentFile;
		if (track->usePages.load())
		{
			permanentFile = getTrackPageAudioFile(trackId, track->currentPageIndex);
		}
		else
		{
			permanentFile = getTrackAudioFile(trackId);
		}
		permanentFile.getParentDirectory().createDirectory();

		DBG("Saving buffer(s) with " << track->stagingBuffer.getNumSamples() << " samples");
		if (track->nextHasOriginalVersion.load())
		{
			saveOriginalAndStretchedBuffers(track->originalStagingBuffer, track->stagingBuffer, trackId, track->stagingSampleRate);
			DBG("Both files saved for track: " << trackId);
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate);
			DBG("File saved to: " << permanentFile.getFullPathName());
		}

		if (track->usePages.load())
		{
			auto& currentPage = track->getCurrentPage();
			currentPage.audioFilePath = permanentFile.getFullPathName();
		}
		else
		{
			track->audioFilePath = permanentFile.getFullPathName();
		}
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
	if (!track)
		return;

	juce::File fileToLoad;

	if (track->usePages.load())
	{
		if (!track->getCurrentPage().hasOriginalVersion.load())
			return;

		if (useOriginal)
		{
			char pageName = static_cast<char>('A' + track->currentPageIndex);
			auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
				.getChildFile("OBSIDIAN-Neural")
				.getChildFile("AudioCache");
			if (projectId != "legacy" && !projectId.isEmpty())
			{
				audioDir = audioDir.getChildFile(projectId);
			}
			fileToLoad = audioDir.getChildFile(trackId + "_original_" + juce::String(pageName) + ".wav");
			juce::MessageManager::callAsync([this]()
				{
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
					{
						editor->statusLabel.setText("Original file loaded...", juce::dontSendNotification);
					} });
		}
		else
		{
			fileToLoad = getTrackPageAudioFile(trackId, track->currentPageIndex);
			juce::MessageManager::callAsync([this]()
				{
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
					{
						editor->statusLabel.setText("Stretched file loaded...", juce::dontSendNotification);
					} });
		}
	}
	else
	{
		if (!track->hasOriginalVersion.load())
			return;

		if (useOriginal)
		{
			fileToLoad = getTrackAudioFile(trackId + "_original");
		}
		else
		{
			fileToLoad = getTrackAudioFile(trackId);
		}
	}

	DBG("reloadTrackWithVersion: Loading file: " << fileToLoad.getFullPathName() << " - Exists: " << (fileToLoad.existsAsFile() ? "YES" : "NO"));

	if (!fileToLoad.existsAsFile())
	{
		DBG("File not found, trying legacy naming...");
		if (track->usePages.load())
		{
			int asciiCode = 'A' + track->currentPageIndex;
			auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
				.getChildFile("OBSIDIAN-Neural")
				.getChildFile("AudioCache");
			if (projectId != "legacy" && !projectId.isEmpty())
			{
				audioDir = audioDir.getChildFile(projectId);
			}
			if (useOriginal)
			{
				fileToLoad = audioDir.getChildFile(trackId + "_" + juce::String(asciiCode) + "_original.wav");
			}
			else
			{
				fileToLoad = audioDir.getChildFile(trackId + "_" + juce::String(asciiCode) + ".wav");
			}
			DBG("Trying legacy naming: " << fileToLoad.getFullPathName());
		}

		if (!fileToLoad.existsAsFile())
			return;
	}

	if (track->usePages.load())
	{
		int currentPageIndex = track->currentPageIndex;
		juce::Thread::launch([this, trackId, currentPageIndex, fileToLoad]()
			{ loadAudioFileForPageSwitch(trackId, currentPageIndex, fileToLoad); });
	}
	else
	{
		juce::Thread::launch([this, trackId, fileToLoad]()
			{ loadAudioFileForSwitch(trackId, fileToLoad); });
	}
}

void DjIaVstProcessor::loadAudioFileForPageSwitch(const juce::String& trackId, int pageIndex, const juce::File& audioFile)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto& page = track->pages[pageIndex];
	double preservedLoopStart = page.loopStart;
	double preservedLoopEnd = page.loopEnd;
	bool preservedLocked = track->loopPointsLocked.load();

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
		if (!reader)
			return;
		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		track->stagingBuffer.setSize(2, numSamples);
		reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

		if (numChannels == 1)
		{
			track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
		}

		track->stagingNumSamples = numSamples;
		track->stagingSampleRate = reader->sampleRate;

		track->isVersionSwitch = true;
		track->preservedLoopStart = preservedLoopStart;
		track->preservedLoopEnd = preservedLoopEnd;
		track->preservedLoopLocked = preservedLocked;

		if (pageIndex == track->currentPageIndex)
		{
			track->hasStagingData = true;
			track->swapRequested = true;
		}
		else
		{
			page.audioBuffer.makeCopyOf(track->stagingBuffer);
			page.numSamples = numSamples;
			page.sampleRate = reader->sampleRate;
			page.isLoaded = true;
		}

		juce::MessageManager::callAsync([this, trackId, pageIndex]()
			{ updateWaveformDisplay(trackId); });
	}
	catch (const std::exception&)
	{
		page.loopStart = preservedLoopStart;
		page.loopEnd = preservedLoopEnd;
		track->loopPointsLocked = preservedLocked;
	}
}

void DjIaVstProcessor::loadAudioFileForSwitch(const juce::String& trackId, const juce::File& audioFile)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track)
		return;
	double preservedLoopStart = track->loopStart;
	double preservedLoopEnd = track->loopEnd;
	bool preservedLocked = track->loopPointsLocked.load();

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile));

		if (!reader)
			return;
		loadAudioToStagingBuffer(reader, track);
		track->isVersionSwitch = true;
		track->preservedLoopStart = preservedLoopStart;
		track->preservedLoopEnd = preservedLoopEnd;
		track->preservedLoopLocked = preservedLocked;
		track->hasStagingData = true;
		track->swapRequested = true;

		juce::MessageManager::callAsync([this, trackId]()
			{ updateWaveformDisplay(trackId); });
	}
	catch (const std::exception&)
	{
		track->loopStart = preservedLoopStart;
		track->loopEnd = preservedLoopEnd;
		track->loopPointsLocked = preservedLocked;
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

	if (projectId != "legacy" && !projectId.isEmpty())
	{
		audioDir = audioDir.getChildFile(projectId);
	}
	audioDir.createDirectory();

	TrackData* track = trackManager.getTrack(trackId);

	juce::File originalFile;
	juce::File stretchedFile;

	if (track && track->usePages.load())
	{
		char pageName = static_cast<char>('A' + track->currentPageIndex);
		originalFile = audioDir.getChildFile(trackId + "_original_" + juce::String(pageName) + ".wav");
		stretchedFile = audioDir.getChildFile(trackId + "_" + juce::String(pageName) + ".wav");
	}
	else
	{
		originalFile = audioDir.getChildFile(trackId + "_original.wav");
		stretchedFile = audioDir.getChildFile(trackId + ".wav");
	}

	saveBufferToFile(originalBuffer, originalFile, sampleRate);
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

	if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
	{
		writer.reset();
		return;
	}
	writer.reset();

	if (sampleBank && outputFile.getFileName().endsWith(".wav") && !isLoadingFromBank.load())
	{
		juce::String filename = outputFile.getFileNameWithoutExtension();

		if (filename.contains("_original"))
		{
			DBG("Skipping original file for bank: " + filename);
			return;
		}

		juce::String trackId = filename;

		for (char page = 'A'; page <= 'D'; ++page)
		{
			juce::String pageSuffix = "_" + juce::String::charToString(page);
			if (trackId.endsWith(pageSuffix))
			{
				trackId = trackId.dropLastCharacters(2);
				DBG("Removed new format page suffix: " + pageSuffix);
				break;
			}
		}

		for (int asciiCode = 65; asciiCode <= 68; ++asciiCode)
		{
			juce::String asciiSuffix = "_" + juce::String(asciiCode);
			if (trackId.endsWith(asciiSuffix))
			{
				trackId = trackId.dropLastCharacters(asciiSuffix.length());
				DBG("Removed legacy ASCII page suffix: " + asciiSuffix);
				break;
			}
		}

		DBG("Extracted trackId: " + trackId + " from filename: " + filename);

		if (trackId == currentBankLoadTrackId)
		{
			DBG("Skipping bank save - loading from bank: " + trackId);
			return;
		}

		TrackData* track = trackManager.getTrack(trackId);
		if (!track)
		{
			DBG("Track not found for ID: " + trackId);
			return;
		}

		juce::String prompt;
		float bpm = 126.0f;
		juce::String key = "Unknown";

		if (track->usePages.load())
		{
			auto& currentPage = track->getCurrentPage();
			prompt = currentPage.generationPrompt;
			if (prompt.isEmpty())
				prompt = currentPage.selectedPrompt;
			bpm = currentPage.generationBpm > 0 ? currentPage.generationBpm : currentPage.originalBpm;
			key = currentPage.generationKey.isEmpty() ? "Unknown" : currentPage.generationKey;

			DBG("Using pages - Page " + juce::String((char)('A' + track->currentPageIndex)) +
				" - Prompt: " + prompt + " - BPM: " + juce::String(bpm));
		}
		else
		{
			prompt = track->generationPrompt;
			if (prompt.isEmpty())
				prompt = track->selectedPrompt;
			bpm = track->generationBpm > 0 ? track->generationBpm : track->originalBpm;
			key = track->generationKey.isEmpty() ? "Unknown" : track->generationKey;

			DBG("Not using pages - Prompt: " + prompt + " - BPM: " + juce::String(bpm));
		}

		if (prompt.isEmpty())
		{
			DBG("No prompt found for track: " + trackId);
			return;
		}

		if (!track->currentSampleId.isEmpty())
		{
			sampleBank->markSampleAsUnused(track->currentSampleId, projectId);
			DBG("Marked previous sample as unused: " + track->currentSampleId);
		}

		juce::String sampleId = sampleBank->addSample(prompt, outputFile, bpm, key);

		if (!sampleId.isEmpty())
		{
			sampleBank->markSampleAsUsed(sampleId, projectId);
			track->currentSampleId = sampleId;
			DBG("Sample added to bank: " + sampleId + " for prompt: " + prompt);

			if (track->usePages.load())
			{
				track->getCurrentPage().generationPrompt = "";
			}
			else
			{
				track->generationPrompt = "";
			}
		}
		else
		{
			DBG("Failed to add sample to bank");
		}
	}
}

juce::File DjIaVstProcessor::getTrackAudioFile(const juce::String& trackId)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("AudioCache");
	if (projectId != "legacy" && !projectId.isEmpty())
	{
		audioDir = audioDir.getChildFile(projectId);
	}

	TrackData* track = trackManager.getTrack(trackId);
	if (track && track->usePages.load())
	{
		char pageName = static_cast<char>('A' + track->currentPageIndex);
		return audioDir.getChildFile(trackId + "_" + juce::String(pageName) + ".wav");
	}

	return audioDir.getChildFile(trackId + ".wav");
}

void DjIaVstProcessor::processAudioBPMAndSync(TrackData* track)
{
	track->nextHasOriginalVersion.store(false);

	float serverDetectedBpm = pendingDetectedBpm.load();
	float soundTouchDetectedBpm = AudioAnalyzer::detectBPM(track->stagingBuffer, track->stagingSampleRate);

	double hostBpm = cachedHostBpm.load();

	float correctedServerBpm = serverDetectedBpm;
	float correctedSoundTouchBpm = soundTouchDetectedBpm;

	if (hostBpm > 0)
	{
		double tolerance = hostBpm * 0.2;

		if (serverDetectedBpm > 0.0f)
		{
			if (serverDetectedBpm >= (hostBpm * 0.5 - tolerance) &&
				serverDetectedBpm <= (hostBpm * 0.5 + tolerance))
			{
				correctedServerBpm = serverDetectedBpm * 2.0f;
				DBG("Server BPM corrected for half tempo: " + juce::String(serverDetectedBpm, 2) +
					" -> " + juce::String(correctedServerBpm, 2));
			}
			else if (serverDetectedBpm >= (hostBpm * 2.0 - tolerance) &&
				serverDetectedBpm <= (hostBpm * 2.0 + tolerance))
			{
				correctedServerBpm = serverDetectedBpm / 2.0f;
				DBG("Server BPM corrected for double tempo: " + juce::String(serverDetectedBpm, 2) +
					" -> " + juce::String(correctedServerBpm, 2));
			}
		}

		if (soundTouchDetectedBpm > 0.0f)
		{
			if (soundTouchDetectedBpm >= (hostBpm * 0.5 - tolerance) &&
				soundTouchDetectedBpm <= (hostBpm * 0.5 + tolerance))
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm * 2.0f;
				DBG("SoundTouch BPM corrected for half tempo: " + juce::String(soundTouchDetectedBpm, 2) +
					" -> " + juce::String(correctedSoundTouchBpm, 2));
			}
			else if (soundTouchDetectedBpm >= (hostBpm * 2.0 - tolerance) &&
				soundTouchDetectedBpm <= (hostBpm * 2.0 + tolerance))
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm / 2.0f;
				DBG("SoundTouch BPM corrected for double tempo: " + juce::String(soundTouchDetectedBpm, 2) +
					" -> " + juce::String(correctedSoundTouchBpm, 2));
			}
		}
	}

	float detectedBPM;

	if (serverDetectedBpm > 0.0f && soundTouchDetectedBpm > 0.0f && hostBpm > 0.0)
	{
		float serverDiff = std::abs(correctedServerBpm - static_cast<float>(hostBpm));
		float soundTouchDiff = std::abs(correctedSoundTouchBpm - static_cast<float>(hostBpm));

		if (serverDiff < soundTouchDiff)
		{
			detectedBPM = correctedServerBpm;
			DBG("Using server-detected BPM: " + juce::String(detectedBPM, 2) +
				" (diff: " + juce::String(serverDiff, 2) + " vs SoundTouch diff: " + juce::String(soundTouchDiff, 2) + ")");
		}
		else
		{
			detectedBPM = correctedSoundTouchBpm;
			DBG("Using SoundTouch-detected BPM: " + juce::String(detectedBPM, 2) +
				" (diff: " + juce::String(soundTouchDiff, 2) + " vs server diff: " + juce::String(serverDiff, 2) + ")");
		}
	}
	else if (serverDetectedBpm > 0.0f)
	{
		detectedBPM = correctedServerBpm;
		DBG("Using server-detected BPM (SoundTouch unavailable): " + juce::String(detectedBPM, 2));
	}
	else
	{
		detectedBPM = correctedSoundTouchBpm;
		DBG("Using SoundTouch-detected BPM (server unavailable): " + juce::String(detectedBPM, 2));
	}

	pendingDetectedBpm.store(-1.0f);

	bool bpmValid = (detectedBPM > 60.0f && detectedBPM < 200.0f);
	track->stagingOriginalBpm = bpmValid ? detectedBPM : static_cast<float>(hostBpm);

	double bpmDifference = std::abs(hostBpm - track->stagingOriginalBpm);
	bool hostBpmValid = (hostBpm > 0.0);
	bool originalBpmValid = (track->stagingOriginalBpm > 0.0f);
	bool bpmDifferenceSignificant = (bpmDifference > 0.01 && bpmDifference < 5.0);

	if ((hostBpmValid && originalBpmValid && bpmDifferenceSignificant) || useLocalModel)
	{
		track->originalStagingBuffer.makeCopyOf(track->stagingBuffer);
		double stretchRatio = hostBpm / static_cast<double>(track->stagingOriginalBpm);
		AudioAnalyzer::timeStretchBufferHQ(track->stagingBuffer, stretchRatio, track->stagingSampleRate);
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		track->stagingOriginalBpm = static_cast<float>(hostBpm);
		track->nextHasOriginalVersion.store(true);

		DBG("Time-stretched from " + juce::String(detectedBPM, 2) +
			" to " + juce::String(hostBpm, 2) + " BPM (ratio: " + juce::String(stretchRatio, 3) + ")");
	}
	else
	{
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		track->stagingOriginalBpm = static_cast<float>(hostBpm);
		track->nextHasOriginalVersion.store(false);

		if (bpmDifferenceSignificant)
		{
			DBG("BPM difference (" + juce::String(bpmDifference, 2) +
				") is significant but outside stretch range (0.01-5.0), using as-is");
		}
		else
		{
			DBG("BPM is close enough (" + juce::String(bpmDifference, 2) +
				" diff), no time-stretch needed");
		}
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

	state.setProperty("projectId", projectId, nullptr);
	state.setProperty("lastPrompt", juce::var(lastPrompt), nullptr);
	state.setProperty("lastKey", juce::var(lastKey), nullptr);
	state.setProperty("lastBpm", juce::var(lastBpm), nullptr);
	state.setProperty("lastPresetIndex", juce::var(lastPresetIndex), nullptr);
	state.setProperty("hostBpmEnabled", juce::var(hostBpmEnabled), nullptr);
	state.setProperty("lastDuration", juce::var(lastDuration), nullptr);
	state.setProperty("selectedTrackId", juce::var(selectedTrackId), nullptr);
	state.setProperty("lastKeyIndex", juce::var(lastKeyIndex), nullptr);
	state.setProperty("isGenerating", juce::var(isGenerating), nullptr);
	state.setProperty("autoLoadEnabled", juce::var(autoLoadEnabled.load()), nullptr);
	state.setProperty("generatingTrackId", juce::var(generatingTrackId), nullptr);
	state.setProperty("bypassSequencer", juce::var(getBypassSequencer()), nullptr);

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

	projectId = state.getProperty("projectId", "legacy").toString();
	lastPrompt = state.getProperty("lastPrompt", "").toString();
	lastKey = state.getProperty("lastKey", "C minor").toString();
	lastBpm = state.getProperty("lastBpm", 126.0);
	lastPresetIndex = state.getProperty("lastPresetIndex", -1);
	hostBpmEnabled = state.getProperty("hostBpmEnabled", false);
	lastDuration = state.getProperty("lastDuration", 6.0);
	lastKeyIndex = state.getProperty("lastKeyIndex", 1);
	isGenerating = state.getProperty("isGenerating", false);
	generatingTrackId = state.getProperty("generatingTrackId", "").toString();
	autoLoadEnabled.store(state.getProperty("autoLoadEnabled", true));
	bool bypassValue = state.getProperty("bypassSequencer", false);
	setBypassSequencer(bypassValue);
	auto tracksState = state.getChildWithName("TrackManager");
	if (tracksState.isValid())
	{
		trackManager.loadState(tracksState);
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
	projectId = state.getProperty("projectId", "legacy").toString();

	if (projectId == "legacy" || projectId.isEmpty())
	{
		migrationCompleted = false;
		juce::Timer::callAfterDelay(500, [this]()
			{ performMigrationIfNeeded(); });
	}
	else
	{
		migrationCompleted = true;
	}
	juce::Timer::callAfterDelay(1000, [this]()
		{
			auto trackIds = trackManager.getAllTrackIds();
			for (const auto& trackId : trackIds) {
				TrackData* track = trackManager.getTrack(trackId);
				if (track) {
					if (track->usePages.load()) {
						DBG("setStateInformation: Track " << track->trackName << " uses pages - skipping legacy reload");
						continue;
					}
					if (track->numSamples == 0 && !track->audioFilePath.isEmpty()) {
						juce::File audioFile(track->audioFilePath);
						if (audioFile.existsAsFile()) {
							DBG("setStateInformation: Reloading legacy track " << track->trackName);
							trackManager.loadAudioFileForTrack(track, audioFile);
						}
					}
				}
			} });
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
	else if (parameterID == "nextTrack" && newValue > 0.5f)
	{
		selectNextTrack();
		juce::MessageManager::callAsync([this]()
			{ parameters.getParameter("nextTrack")->setValueNotifyingHost(0.0f); });
	}
	else if (parameterID == "prevTrack" && newValue > 0.5f)
	{
		selectPreviousTrack();
		juce::MessageManager::callAsync([this]()
			{ parameters.getParameter("prevTrack")->setValueNotifyingHost(0.0f); });
	}
	else if (parameterID.startsWith("slot") && parameterID.contains("Page") && newValue > 0.5f)
	{
		handlePageChange(parameterID);
		juce::MessageManager::callAsync([this, parameterID]()
			{
				auto* param = parameters.getParameter(parameterID);
				if (param)
					param->setValueNotifyingHost(0.0f); });
	}
}

void DjIaVstProcessor::handlePageChange(const juce::String& parameterID)
{
	juce::String slotStr = parameterID.substring(4, 5);
	int slotNumber = slotStr.getIntValue();

	char pageChar = static_cast<char>(parameterID[parameterID.length() - 1]);
	int pageIndex = pageChar - 'A';

	if (slotNumber < 1 || slotNumber > 8 || pageIndex < 0 || pageIndex > 3)
		return;

	auto trackIds = trackManager.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = trackManager.getTrack(trackId);
		if (track && track->slotIndex == (slotNumber - 1))
		{
			if (track->pages[pageIndex].numSamples == 0)
			{
				track->setCurrentPage(pageIndex);
				if (!getActiveEditor())
				{
					track->isPlaying = false;
					track->isCurrentlyPlaying = false;
					track->readPosition = 0.0;
				}

				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					juce::MessageManager::callAsync([editor, trackId, pageIndex]()
						{
							for (auto& trackComp : editor->getTrackComponents())
							{
								if (trackComp->getTrackId() == trackId)
								{
									trackComp->performPageChange(pageIndex);
									break;
								}
							} });
				}

				DBG("Page change immediate (empty page): slot " << slotNumber << " -> page " << (char)('A' + pageIndex));
				return;
			}

			bool isPlaying = false;
			if (auto currentPlayHead = getPlayHead())
			{
				if (auto positionInfo = currentPlayHead->getPosition())
				{
					isPlaying = positionInfo->getIsPlaying();
				}
			}

			if (!isPlaying || !track->isCurrentlyPlaying.load())
			{
				track->setCurrentPage(pageIndex);

				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					juce::MessageManager::callAsync([editor, trackId, pageIndex]()
						{
							for (auto& trackComp : editor->getTrackComponents())
							{
								if (trackComp->getTrackId() == trackId)
								{
									trackComp->performPageChange(pageIndex);
									break;
								}
							} });
				}

				DBG("Page change immediate (not playing): slot " << slotNumber << " -> page " << (char)('A' + pageIndex));
			}
			else
			{
				track->pageChangePending = true;
				track->pendingPageIndex = pageIndex;

				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					juce::MessageManager::callAsync([editor, trackId, pageIndex]()
						{
							for (auto& trackComp : editor->getTrackComponents())
							{
								if (trackComp->getTrackId() == trackId)
								{
									if (!trackComp->isTimerRunning())
									{
										trackComp->startTimer(200);
									}
									trackComp->updatePagesDisplay();
									editor->setStatusWithTimeout("Page " + juce::String((char)('A' + pageIndex)) +
										" will switch at next measure", 3000);
									break;
								}
							} });
				}

				DBG("Page change pending: slot " << slotNumber << " -> page " << (char)('A' + pageIndex) << " (will switch at next measure)");
			}

			break;
		}
	}
}

void DjIaVstProcessor::selectNextTrack()
{
	auto trackIds = trackManager.getAllTrackIds();
	if (trackIds.size() <= 1)
		return;

	int currentIndex = -1;
	for (size_t i = 0; i < trackIds.size(); ++i)
	{
		if (trackIds[i] == selectedTrackId)
		{
			currentIndex = static_cast<int>(i);
			break;
		}
	}

	if (currentIndex >= 0)
	{
		size_t nextIndex = (static_cast<size_t>(currentIndex) + 1) % trackIds.size();
		selectedTrackId = trackIds[nextIndex];

		juce::MessageManager::callAsync([this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					editor->updateSelectedTrack();
					TrackData* track = trackManager.getTrack(selectedTrackId);
					if (track)
					{
						editor->setStatusWithTimeout("Selected: " + track->trackName, 2000);
					}
				} });
	}
}

void DjIaVstProcessor::selectPreviousTrack()
{
	auto trackIds = trackManager.getAllTrackIds();
	if (trackIds.size() <= 1)
		return;

	int currentIndex = -1;
	for (size_t i = 0; i < trackIds.size(); ++i)
	{
		if (trackIds[i] == selectedTrackId)
		{
			currentIndex = static_cast<int>(i);
			break;
		}
	}

	if (currentIndex >= 0)
	{
		size_t trackCount = trackIds.size();
		size_t prevIndex = (static_cast<size_t>(currentIndex) + trackCount - 1) % trackCount;
		selectedTrackId = trackIds[prevIndex];

		juce::MessageManager::callAsync([this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					editor->updateSelectedTrack();
					TrackData* track = trackManager.getTrack(selectedTrackId);
					if (track)
					{
						editor->setStatusWithTimeout("Selected: " + track->trackName, 2000);
					}
				} });
	}
}

void DjIaVstProcessor::triggerGlobalGeneration()
{
	if (isGenerating)
	{
		juce::MessageManager::callAsync([this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					editor->setStatusWithTimeout("Generation already in progress, please wait", 3000);
				} });
				return;
	}

	if (selectedTrackId.isEmpty())
	{
		juce::MessageManager::callAsync([this]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
				{
					editor->setStatusWithTimeout("No track selected for generation", 3000);
				} });
				return;
	}

	syncSelectedTrackWithGlobalPrompt();

	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				editor->onGenerateButtonClicked();
			}
			else
			{
				generateLoopFromGlobalSettings();
			} });
}

void DjIaVstProcessor::syncSelectedTrackWithGlobalPrompt()
{
	TrackData* track = trackManager.getTrack(selectedTrackId);
	if (!track)
		return;

	juce::String currentGlobalPrompt = getGlobalPrompt();

	track->selectedPrompt = currentGlobalPrompt;

	juce::MessageManager::callAsync([this, currentGlobalPrompt]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
			{
				for (auto& trackComp : editor->getTrackComponents())
				{
					if (trackComp->getTrackId() == selectedTrackId)
					{
						trackComp->updatePromptSelection(currentGlobalPrompt);
						break;
					}
				}

				editor->setStatusWithTimeout("Track prompt synced: " + currentGlobalPrompt.substring(0, 30) + "...", 2000);
			} });
}

void DjIaVstProcessor::generateLoopFromGlobalSettings()
{
	if (isGenerating)
		return;

	TrackData* track = trackManager.getTrack(selectedTrackId);
	if (!track)
		return;

	syncSelectedTrackWithGlobalPrompt();
	setIsGenerating(true);
	setGeneratingTrackId(selectedTrackId);

	juce::Thread::launch([this]()
		{
			try
			{
				TrackData* track = trackManager.getTrack(selectedTrackId);
				if (!track) return;

				if (track->usePages.load()) {
					auto& currentPage = track->getCurrentPage();

					currentPage.selectedPrompt = getGlobalPrompt();
					currentPage.generationBpm = getGlobalBpm();
					currentPage.generationKey = getGlobalKey();
					currentPage.generationDuration = getGlobalDuration();

					track->syncLegacyProperties();
				}

				auto request = createGlobalLoopRequest();
				generateLoop(request, selectedTrackId);
			}
			catch (const std::exception& /*e*/)
			{
				setIsGenerating(false);
				setGeneratingTrackId("");
			} });
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
			if (!track->beatRepeatActive.load())
			{
				track->readPosition = 0.0;
			}
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
	if (getBypassSequencer())
	{
		return;
	}
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

	if (newMeasure == 0 && newStep == 0 && track->pageChangePending.load())
	{
		int targetPage = track->pendingPageIndex.load();
		if (targetPage >= 0 && targetPage < 4)
		{
			juce::MessageManager::callAsync([this, trackId = track->trackId, targetPage]()
				{
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
					{
						for (auto& trackComp : editor->getTrackComponents())
						{
							if (trackComp->getTrackId() == trackId)
							{
								trackComp->performPageChange(targetPage);
								break;
							}
						}
					}
					else
					{
						TrackData* t = trackManager.getTrack(trackId);
						if (t)
						{
							t->setCurrentPage(targetPage);
							t->pageChangePending = false;
							t->pendingPageIndex = -1;
							DBG("Page changed without UI at measure boundary: " << (char)('A' + targetPage));
						}
					} });
		}
	}

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

		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		track->setPlaying(true);
		triggerSequencerStep(track);
	}
}

bool DjIaVstProcessor::previewSampleFromBank(const juce::String& sampleId)
{
	if (!sampleBank)
		return false;
	auto* entry = sampleBank->getSample(sampleId);
	if (!entry)
		return false;

	juce::File sampleFile(entry->filePath);
	if (!sampleFile.exists())
		return false;

	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();
	std::unique_ptr<juce::AudioFormatReader> testReader(formatManager.createReaderFor(sampleFile));
	if (!testReader)
	{
		DBG("Cannot read audio file: " + sampleFile.getFullPathName());
		return false;
	}

	stopSamplePreview();

	juce::Thread::launch([this, sampleFile]()
		{
			juce::AudioFormatManager formatManager;
			formatManager.registerBasicFormats();
			std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
			if (!reader) {
				juce::ScopedLock lock(previewLock);
				isPreviewPlaying = false;
				return;
			}

			{
				juce::ScopedLock lock(previewLock);
				previewBuffer.setSize(2, (int)reader->lengthInSamples);
				reader->read(&previewBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
				if (reader->numChannels == 1)
				{
					previewBuffer.copyFrom(1, 0, previewBuffer, 0, 0, previewBuffer.getNumSamples());
				}
				previewSampleRate = reader->sampleRate;
				previewPosition = 0.0;
				isPreviewPlaying = true;
			}
			DBG("Preview loaded: " + sampleFile.getFileName()); });

	return true;
}

void DjIaVstProcessor::triggerSequencerStep(TrackData* track)
{
	if (getBypassSequencer())
	{
		return;
	}
	int step = track->sequencerData.currentStep;
	int measure = track->sequencerData.currentMeasure;
	track->isArmed = false;
	if (track->sequencerData.steps[measure][step])
	{
		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		playingTracks[track->midiNote] = track->trackId;
		juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, track->midiNote,
			(juce::uint8)(track->sequencerData.velocities[measure][step] * 127));
		addSequencerMidiMessage(noteOn);
	}
}

void DjIaVstProcessor::stopSamplePreview()
{
	isPreviewPlaying = false;
	previewPosition = 0.0;
	if (!currentPreviewTrackId.isEmpty())
	{
		if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
		{
			auto* trackComp = editor->getTrackComponent(currentPreviewTrackId);
			if (trackComp)
			{
				trackComp->setPreviewPlaying(false);
			}
		}
		currentPreviewTrackId = "";
	}
}

juce::File DjIaVstProcessor::getTrackPageAudioFile(const juce::String& trackId, int pageIndex)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("AudioCache");
	if (projectId != "legacy" && !projectId.isEmpty())
	{
		audioDir = audioDir.getChildFile(projectId);
	}

	char pageName = static_cast<char>('A' + pageIndex);
	juce::String filename = trackId + "_" + juce::String(pageName) + ".wav";
	return audioDir.getChildFile(filename);
}

void DjIaVstProcessor::loadSampleToBankPage(const juce::String& trackId, int pageIndex, const juce::File& sampleFile, const juce::String& sampleId)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto& page = track->pages[pageIndex];

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
		if (!reader)
			return;

		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		track->stagingBuffer.setSize(2, numSamples);
		reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

		if (numChannels == 1)
		{
			track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
		}

		track->stagingNumSamples = numSamples;
		track->stagingSampleRate = reader->sampleRate;
		track->stagingOriginalBpm = 126.0f;

		processAudioBPMAndSync(track);

		auto permanentFile = getTrackPageAudioFile(trackId, pageIndex);
		permanentFile.getParentDirectory().createDirectory();

		DBG("Saving bank sample to page " << (char)('A' + pageIndex) << ": " << permanentFile.getFullPathName());

		if (track->nextHasOriginalVersion.load())
		{
			auto originalFile = getTrackPageAudioFile(trackId + "_original", pageIndex);
			auto stretchedFile = getTrackPageAudioFile(trackId, pageIndex);
			saveBufferToFile(track->originalStagingBuffer, originalFile, track->stagingSampleRate);
			saveBufferToFile(track->stagingBuffer, stretchedFile, track->stagingSampleRate);
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate);
		}

		page.audioFilePath = permanentFile.getFullPathName();
		page.numSamples = track->stagingNumSamples.load();
		page.sampleRate = track->stagingSampleRate.load();
		page.originalBpm = track->stagingOriginalBpm;
		page.isLoaded = true;
		page.isLoading = false;

		auto* sampleEntry = sampleBank->getSample(sampleId);
		if (sampleEntry)
		{
			page.prompt = sampleEntry->originalPrompt;
			page.selectedPrompt = sampleEntry->originalPrompt;
			page.generationBpm = sampleEntry->bpm;
			page.generationKey = sampleEntry->key;
		}

		if (pageIndex == track->currentPageIndex)
		{
			track->hasStagingData = true;
			track->swapRequested = true;
		}

		juce::MessageManager::callAsync([this, trackId, pageIndex]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor())) {
					editor->setStatusWithTimeout("Sample loaded to page " + juce::String((char)('A' + pageIndex)) + "!");
					TrackData* track = trackManager.getTrack(trackId);
					if (track && pageIndex == track->currentPageIndex) {
						for (auto& trackComp : editor->getTrackComponents()) {
							if (trackComp->getTrackId() == trackId) {
								trackComp->updateFromTrackData();
								if (trackComp->isWaveformVisible()) {
									trackComp->refreshWaveformDisplay();
								}
								break;
							}
						}
					}
				} });
	}
	catch (const std::exception& /*e*/)
	{
		DBG("Failed to load sample from bank to page");
	}
}

juce::File DjIaVstProcessor::getExportDirectory()
{
	auto documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
	auto exportDir = documentsDir.getChildFile("OBSIDIAN_Exports");

	if (!exportDir.exists())
		exportDir.createDirectory();

	return exportDir;
}

juce::File DjIaVstProcessor::exportSampleForDragDrop(const juce::File& originalFile)
{
	if (!originalFile.existsAsFile())
		return juce::File();

	auto exportDir = getExportDirectory();

	auto now = juce::Time::getCurrentTime();
	juce::String timestamp = now.formatted("%Y%m%d_%H%M%S");

	juce::String baseName = originalFile.getFileNameWithoutExtension();
	juce::String extension = originalFile.getFileExtension();
	juce::String newFileName = baseName + "_" + timestamp + extension;

	auto exportFile = exportDir.getChildFile(newFileName);

	if (originalFile.copyFileTo(exportFile))
	{
		DBG("Sample exported for drag&drop: " + exportFile.getFullPathName());
		return exportFile;
	}

	DBG("Failed to export sample for drag&drop");
	return juce::File();
}

void DjIaVstProcessor::stopTrackPreview(const juce::String& trackId)
{
	TrackData* track = trackManager.getTrack(trackId);
	if (track)
	{
		track->isPlaying.store(false);
		track->readPosition = 0.0;
	}

	if (currentPreviewTrackId == trackId)
	{
		currentPreviewTrackId = "";
	}

	if (auto* editor = dynamic_cast<DjIaVstEditor*>(getActiveEditor()))
	{
		auto* trackComp = editor->getTrackComponent(trackId);
		if (trackComp)
		{
			trackComp->setPreviewPlaying(false);
		}
	}
}