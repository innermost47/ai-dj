#pragma once
#include "JuceHeader.h"
#include "data/TrackData.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "StateManager.h"

StateManager::StateManager(DjIaVstProcessor& processor)
	: audioProcessor(processor)
{
}

juce::ValueTree StateManager::saveState() const
{
	juce::ValueTree state("TrackManager");

	audioProcessor.trackManager.forEachTrack([&](const TrackData* track)
		{
			juce::ValueTree trackState("Track");

			trackState.setProperty("id", track->trackId, nullptr);
			trackState.setProperty("name", track->trackName, nullptr);

			trackState.setProperty("slotIndex", track->slotIndex, nullptr);
			trackState.setProperty("style", track->style, nullptr);
			trackState.setProperty("timeStretchMode", track->timeStretchMode, nullptr);
			trackState.setProperty("midiNote", track->midiNote, nullptr);
			trackState.setProperty("volume", track->volume.load(), nullptr);
			trackState.setProperty("pan", track->pan.load(), nullptr);
			trackState.setProperty("muted", track->isMuted.load(), nullptr);
			trackState.setProperty("solo", track->isSolo.load(), nullptr);
			trackState.setProperty("enabled", track->isEnabled.load(), nullptr);
			trackState.setProperty("timeStretchRatio", track->timeStretchRatio, nullptr);
			trackState.setProperty("showWaveform", track->showWaveform.load(), nullptr);
			trackState.setProperty("showSequencer", track->showSequencer.load(), nullptr);
			trackState.setProperty("isPlaying", track->isPlaying.load(), nullptr);
			trackState.setProperty("isArmed", track->isArmed.load(), nullptr);
			trackState.setProperty("isArmedToStop", track->isArmedToStop.load(), nullptr);
			trackState.setProperty("isCurrentlyPlaying", track->isCurrentlyPlaying.load(), nullptr);
			trackState.setProperty("nextHasOriginalVersion", track->nextHasOriginalVersion.load(), nullptr);
			trackState.setProperty("randomRetriggerEnabled", track->randomRetriggerEnabled.load(), nullptr);
			trackState.setProperty("randomRetriggerInterval", track->randomRetriggerInterval.load(), nullptr);
			trackState.setProperty("beatRepeatPending", track->beatRepeatPending.load(), nullptr);
			trackState.setProperty("beatRepeatStopPending", track->beatRepeatStopPending.load(), nullptr);
			trackState.setProperty("originalReadPosition", track->originalReadPosition.load(), nullptr);
			trackState.setProperty("beatRepeatStartPosition", track->beatRepeatStartPosition.load(), nullptr);
			trackState.setProperty("beatRepeatEndPosition", track->beatRepeatEndPosition.load(), nullptr);
			trackState.setProperty("beatRepeatActive", track->beatRepeatActive.load(), nullptr);
			trackState.setProperty("randomRetriggerDurationEnabled", track->randomRetriggerDurationEnabled.load(), nullptr);
			trackState.setProperty("currentPageIndex", track->currentPageIndex.load(), nullptr);

			for (int pageIndex = 0; pageIndex < 4; ++pageIndex)
			{
				auto pageState = juce::ValueTree("Page");
				const auto& page = track->pages[pageIndex];

				pageState.setProperty("index", pageIndex, nullptr);
				pageState.setProperty("audioFilePath", page.audioFilePath, nullptr);
				pageState.setProperty("numSamples", page.numSamples, nullptr);
				pageState.setProperty("sampleRate", page.sampleRate, nullptr);
				pageState.setProperty("originalBpm", page.originalBpm, nullptr);
				pageState.setProperty("prompt", page.prompt, nullptr);
				pageState.setProperty("selectedPrompt", page.selectedPrompt, nullptr);
				pageState.setProperty("selectedModel", page.selectedModel, nullptr);
				pageState.setProperty("generationPrompt", page.generationPrompt, nullptr);
				pageState.setProperty("generationBpm", page.generationBpm, nullptr);
				pageState.setProperty("generationKey", page.generationKey, nullptr);
				pageState.setProperty("generationDuration", page.generationDuration, nullptr);
				pageState.setProperty("loopStart", page.loopStart, nullptr);
				pageState.setProperty("loopEnd", page.loopEnd, nullptr);
				pageState.setProperty("useOriginalFile", page.useOriginalFile.load(), nullptr);
				pageState.setProperty("hasOriginalVersion", page.hasOriginalVersion.load(), nullptr);
				pageState.setProperty("isLoaded", page.isLoaded.load(), nullptr);
				pageState.setProperty("canvasData", page.canvasData, nullptr);
				pageState.setProperty("canvasState", page.canvasState, nullptr);
				pageState.setProperty("selectedKeywords", page.selectedKeywords.joinIntoString("|"), nullptr);
				pageState.setProperty("currentSequenceIndex", page.currentSequenceIndex, nullptr);
				pageState.setProperty("adsrAttack", page.adsrAttack.load(), nullptr);
				pageState.setProperty("adsrDecay", page.adsrDecay.load(), nullptr);
				pageState.setProperty("adsrSustain", page.adsrSustain.load(), nullptr);
				pageState.setProperty("adsrRelease", page.adsrRelease.load(), nullptr);
				pageState.setProperty("bpmOffset", page.bpmOffset.load(), nullptr);
				pageState.setProperty("loopPointsLocked", page.loopPointsLocked.load(), nullptr);

				for (int seqIdx = 0; seqIdx < 8; ++seqIdx)
				{
					juce::ValueTree sequencerState("Sequence");
					const auto& seq = page.sequences[seqIdx];

					sequencerState.setProperty("index", seqIdx, nullptr);
					sequencerState.setProperty("isPlaying", seq.isPlaying, nullptr);
					sequencerState.setProperty("currentStep", seq.currentStep, nullptr);
					sequencerState.setProperty("currentMeasure", seq.currentMeasure, nullptr);
					sequencerState.setProperty("numMeasures", seq.numMeasures, nullptr);
					sequencerState.setProperty("beatsPerMeasure", seq.beatsPerMeasure, nullptr);

					for (int m = 0; m < 4; ++m)
					{
						for (int s = 0; s < 16; ++s)
						{
							juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
							sequencerState.setProperty(stepKey, seq.steps[m][s], nullptr);

							juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
							sequencerState.setProperty(velocityKey, seq.velocities[m][s], nullptr);
						}
					}

					pageState.appendChild(sequencerState, nullptr);
				}

				trackState.appendChild(pageState, nullptr);
			}

			state.appendChild(trackState, nullptr);
		}
	);

	return state;
}

void StateManager::loadState(const juce::ValueTree& state)
{
	audioProcessor.trackManager.clearAllTracks();
	audioProcessor.trackManager.resetAllSlots();

	for (auto trackState : state)
	{
		if (!trackState.hasType("Track"))
			continue;

		auto track = std::make_unique<TrackData>();

		track->trackId = trackState.getProperty("id", juce::Uuid().toString());
		track->trackName = trackState.getProperty("name", "Track");
		track->slotIndex = trackState.getProperty("slotIndex", -1);
		track->style = trackState.getProperty("style", "");
		track->timeStretchMode = 4;
		double legacyBpmOffset = trackState.getProperty("bpmOffset", 0.0);
		track->midiNote = trackState.getProperty("midiNote", 60);
		track->volume = trackState.getProperty("volume", 0.8f);
		track->pan = trackState.getProperty("pan", 0.0f);
		track->isEnabled = trackState.getProperty("enabled", true);
		track->timeStretchRatio = trackState.getProperty("timeStretchRatio", 1.0);
		track->showWaveform = trackState.getProperty("showWaveform", false);
		track->showSequencer = trackState.getProperty("showSequencer", false);
		track->isMuted = trackState.getProperty("muted", false);
		track->isSolo = trackState.getProperty("solo", false);
		track->isPlaying = trackState.getProperty("isPlaying", false);
		track->isArmed = trackState.getProperty("isArmed", false);
		track->isArmedToStop = trackState.getProperty("isArmedToStop", false);
		track->isCurrentlyPlaying = trackState.getProperty("isCurrentlyPlaying", false);
		track->nextHasOriginalVersion = trackState.getProperty("nextHasOriginalVersion", false);
		track->randomRetriggerEnabled = trackState.getProperty("randomRetriggerEnabled", false);
		track->randomRetriggerInterval = trackState.getProperty("randomRetriggerInterval", 3);
		track->beatRepeatPending = trackState.getProperty("beatRepeatPending", false);
		track->beatRepeatStopPending = trackState.getProperty("beatRepeatStopPending", false);
		track->originalReadPosition = trackState.getProperty("originalReadPosition", 0.0);
		track->beatRepeatStartPosition = trackState.getProperty("beatRepeatStartPosition", 0.0);
		track->beatRepeatEndPosition = trackState.getProperty("beatRepeatEndPosition", 0.0);
		track->beatRepeatActive = trackState.getProperty("beatRepeatActive", false);
		track->randomRetriggerDurationEnabled = trackState.getProperty("randomRetriggerDurationEnabled", false);
		track->currentPageIndex.store(trackState.getProperty("currentPageIndex", 0));

		for (int pageIndex = 0; pageIndex < 4; ++pageIndex)
		{
			juce::ValueTree pageState;

			for (int childIndex = 0; childIndex < trackState.getNumChildren(); ++childIndex)
			{
				auto child = trackState.getChild(childIndex);
				if (child.hasType("Page"))
				{
					int storedPageIndex = child.getProperty("index", -1);
					if (storedPageIndex == pageIndex)
					{
						pageState = child;
						break;
					}
				}
			}

			if (pageState.isValid())
			{
				auto& page = track->pages[pageIndex];
				page.audioFilePath = pageState.getProperty("audioFilePath", "").toString();
				page.numSamples = pageState.getProperty("numSamples", 0);
				page.sampleRate = pageState.getProperty("sampleRate", 48000.0);
				page.originalBpm = pageState.getProperty("originalBpm", 126.0f);
				page.prompt = pageState.getProperty("prompt", "").toString();
				page.selectedPrompt = pageState.getProperty("selectedPrompt", "").toString();
				page.selectedModel = pageState.getProperty("selectedModel", "stable-audio-open-1.0").toString();
				if (page.selectedModel.isEmpty())
					page.selectedModel = "stable-audio-open-1.0";
				page.generationPrompt = pageState.getProperty("generationPrompt", "").toString();
				page.generationBpm = pageState.getProperty("generationBpm", 126.0f);
				page.generationKey = pageState.getProperty("generationKey", "").toString();
				page.generationDuration = pageState.getProperty("generationDuration", 6);
				page.loopStart = pageState.getProperty("loopStart", 0.0);
				page.loopEnd = pageState.getProperty("loopEnd", 4.0);
				page.useOriginalFile = pageState.getProperty("useOriginalFile", false);
				page.hasOriginalVersion = pageState.getProperty("hasOriginalVersion", false);
				page.canvasData = pageState.getProperty("canvasData", "").toString();
				page.canvasState = pageState.getProperty("canvasState", "").toString();
				page.bpmOffset.store(pageState.getProperty("bpmOffset", legacyBpmOffset));
				page.loopPointsLocked = pageState.getProperty("loopPointsLocked", false);

				juce::String pageKeywordsStr = pageState.getProperty("selectedKeywords", "");
				if (pageKeywordsStr.isNotEmpty())
				{
					page.selectedKeywords.addTokens(pageKeywordsStr, "|", "");
				}

				page.isLoaded = false;
				page.currentSequenceIndex = pageState.getProperty("currentSequenceIndex", 0);
				page.adsrAttack.store(pageState.getProperty("adsrAttack", 0.01f));
				page.adsrDecay.store(pageState.getProperty("adsrDecay", 4.0f));
				page.adsrSustain.store(pageState.getProperty("adsrSustain", 1.0f));
				page.adsrRelease.store(pageState.getProperty("adsrRelease", 0.0f));

				for (int seqIdx = 0; seqIdx < 8; ++seqIdx)
				{
					juce::ValueTree sequencerState;

					for (int childIndex = 0; childIndex < pageState.getNumChildren(); ++childIndex)
					{
						auto child = pageState.getChild(childIndex);
						if (child.hasType("Sequence"))
						{
							int storedSeqIndex = child.getProperty("index", -1);
							if (storedSeqIndex == seqIdx)
							{
								sequencerState = child;
								break;
							}
						}
					}

					if (sequencerState.isValid())
					{
						auto& seq = page.sequences[seqIdx];
						seq.isPlaying = sequencerState.getProperty("isPlaying", false);
						seq.currentStep = 0;
						seq.currentMeasure = 0;
						seq.numMeasures = sequencerState.getProperty("numMeasures", 1);
						seq.beatsPerMeasure = sequencerState.getProperty("beatsPerMeasure", 4);

						for (int m = 0; m < 4; ++m)
						{
							for (int s = 0; s < 16; ++s)
							{
								juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
								seq.steps[m][s] = sequencerState.getProperty(stepKey, false);

								juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
								seq.velocities[m][s] = sequencerState.getProperty(velocityKey, 0.8f);
							}
						}
					}
					else
					{
						auto& seq = page.sequences[seqIdx];
						if (seqIdx == 0)
						{
							seq.steps[0][0] = true;
							seq.velocities[0][0] = 0.8f;
						}
					}
				}

				if (!page.audioFilePath.isEmpty())
				{
					juce::File audioFile(page.audioFilePath);
					if (audioFile.existsAsFile())
					{
						juce::File fileToLoad = audioFile;

						if (page.useOriginalFile.load() && page.hasOriginalVersion.load())
						{
							char pageName = static_cast<char>('A' + pageIndex);
							juce::String fileName = audioFile.getFileNameWithoutExtension();
							juce::String legacySuffix = "_" + juce::String(65 + pageIndex);
							juce::String newSuffix = "_" + juce::String::charToString(pageName);

							juce::String baseTrackId;
							juce::String actualSuffix;

							if (fileName.endsWith(newSuffix))
							{
								baseTrackId = fileName.dropLastCharacters(newSuffix.length());
								actualSuffix = newSuffix;
							}
							else if (fileName.endsWith(legacySuffix))
							{
								baseTrackId = fileName.dropLastCharacters(legacySuffix.length());
								actualSuffix = legacySuffix;
							}
							if (baseTrackId.isNotEmpty())
							{
								juce::File originalFile = audioFile.getParentDirectory()
									.getChildFile(baseTrackId + "_original" + actualSuffix + ".wav");

								if (originalFile.existsAsFile())
								{
									fileToLoad = originalFile;
								}
							}
						}
						audioProcessor.trackManager.loadAudioFileForPage(track.get(), pageIndex, fileToLoad);
					}
				}
			}
		}

		if (track->slotIndex < 0 || track->slotIndex >= 8 || audioProcessor.trackManager.isSlotUsed(track->slotIndex))
		{
			track->slotIndex = audioProcessor.trackManager.findFreeSlot();
		}
		if (track->slotIndex >= 0 && track->slotIndex < 8)
		{
			audioProcessor.trackManager.setSlotUsed(track->slotIndex, true);
		}

		std::string stdId = track->trackId.toStdString();
		audioProcessor.trackManager.addTrack(stdId, std::move(track));
	}
	int trackCount = static_cast<int>(audioProcessor.trackManager.getNumTracks());
	for (int i = trackCount; i < 8; ++i)
	{
		auto track = std::make_unique<TrackData>();
		track->trackName = "Track " + juce::String(i + 1);
		track->midiNote = 60 + i;
		track->slotIndex = audioProcessor.trackManager.findFreeSlot();
		for (int p = 0; p < 4; ++p)
			track->pages[p].selectedModel = "stable-audio-open-1.0";
		if (track->slotIndex >= 0 && track->slotIndex < 8)
			audioProcessor.trackManager.setSlotUsed(track->slotIndex, true);
		std::string stdId = track->trackId.toStdString();
		audioProcessor.trackManager.addTrack(stdId, std::move(track));
	}
}

void StateManager::getStateInformation(juce::MemoryBlock& destData)
{
	juce::ValueTree state("DjIaVstState");

	state.setProperty("projectId", audioProcessor.getProjectId(), nullptr);
	state.setProperty("lastPrompt", audioProcessor.getLastPrompt(), nullptr);
	state.setProperty("lastKey", audioProcessor.getLastKey(), nullptr);
	state.setProperty("lastBpm", audioProcessor.getLastBpm(), nullptr);
	state.setProperty("lastPresetIndex", audioProcessor.getLastPresetIndex(), nullptr);
	state.setProperty("hostBpmEnabled", audioProcessor.isHostBpmEnabled(), nullptr);
	state.setProperty("lastDuration", audioProcessor.getLastDuration(), nullptr);
	state.setProperty("selectedTrackId", audioProcessor.getSelectedTrackId(), nullptr);
	state.setProperty("lastKeyIndex", audioProcessor.getLastKeyIndex(), nullptr);
	state.setProperty("isGenerating", audioProcessor.getIsGenerating(), nullptr);
	state.setProperty("autoLoadEnabled", audioProcessor.getAutoLoadEnabled(), nullptr);
	state.setProperty("bypassLLM", audioProcessor.getBypassLLM(), nullptr);
	state.setProperty("generatingTrackId", audioProcessor.getGeneratingTrackId(), nullptr);
	state.setProperty("bypassSequencer", audioProcessor.getBypassSequencer(), nullptr);
	state.setProperty("crossfaderValue", audioProcessor.getCrossfaderValue(), nullptr);
	state.setProperty("crossfadeMode", audioProcessor.getCrossfadeMode(), nullptr);
	state.setProperty("windowWidth", audioProcessor.getSavedWindowWidth(), nullptr);
	state.setProperty("windowHeight", audioProcessor.getSavedWindowHeight(), nullptr);
	state.setProperty("bankVisible", audioProcessor.getSavedBankVisible(), nullptr);

	juce::ValueTree midiMappingsState("MidiMappings");
	auto mappings = audioProcessor.getMidiLearnManager().getAllMappings();
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

	auto tracksState = saveState();
	state.appendChild(tracksState, nullptr);

	juce::ValueTree parametersState("Parameters");

	auto& params = audioProcessor.getParameterTreeState();
	for (const auto& paramId : audioProcessor.getBooleanParamIds())
	{
		auto* param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	for (const auto& paramId : audioProcessor.getFloatParamIds())
	{
		auto* param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	state.appendChild(parametersState, nullptr);

	auto globalGenState = juce::ValueTree("GlobalGeneration");
	globalGenState.setProperty("prompt", audioProcessor.getGlobalPrompt(), nullptr);
	globalGenState.setProperty("bpm", audioProcessor.getGlobalBpm(), nullptr);
	globalGenState.setProperty("key", audioProcessor.getGlobalKey(), nullptr);
	globalGenState.setProperty("duration", audioProcessor.getGlobalDuration(), nullptr);

	state.appendChild(globalGenState, nullptr);

	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	audioProcessor.copyXmlToBinary(*xml, destData);
}

void StateManager::setStateInformation(const void* data, int sizeInBytes)
{
	audioProcessor.setIsLoadingState(true);
	std::unique_ptr<juce::XmlElement> xml(audioProcessor.getXmlFromBinary(data, sizeInBytes));
	if (!xml || !xml->hasTagName("DjIaVstState"))
	{
		audioProcessor.setIsLoadingState(false);
		return;
	}
	juce::ValueTree state = juce::ValueTree::fromXml(*xml);

	audioProcessor.setProjectId(state.getProperty("projectId", "legacy").toString());
	audioProcessor.setLastPrompt(state.getProperty("lastPrompt", "").toString());
	audioProcessor.setLastKey(state.getProperty("lastKey", "C minor").toString());
	audioProcessor.setLastBpm(state.getProperty("lastBpm", 126.0));
	audioProcessor.setLastPresetIndex(state.getProperty("lastPresetIndex", -1));
	audioProcessor.setHostBpmEnabled(state.getProperty("hostBpmEnabled", false));
	audioProcessor.setLastDuration(state.getProperty("lastDuration", 6.0));
	audioProcessor.setLastKeyIndex(state.getProperty("lastKeyIndex", 1));
	audioProcessor.setIsGenerating(state.getProperty("isGenerating", false));
	audioProcessor.setGeneratingTrackId(state.getProperty("generatingTrackId", "").toString());
	audioProcessor.setAutoLoadEnabled(state.getProperty("autoLoadEnabled", true));
	audioProcessor.setBypassLLM(state.getProperty("bypassLLM", false));
	audioProcessor.setWindowSize(state.getProperty("windowWidth", 1620), state.getProperty("windowHeight", 840));
	audioProcessor.setBankVisible(state.getProperty("bankVisible", true));

	bool bypassValue = state.getProperty("bypassSequencer", false);
	audioProcessor.setBypassSequencer(bypassValue);
	auto tracksState = state.getChildWithName("TrackManager");
	if (tracksState.isValid())
	{
		loadState(tracksState);
	}

	audioProcessor.setSelectedTrackId(state.getProperty("selectedTrackId", "").toString());
	auto loadedTrackIds = audioProcessor.getAllTrackIds();

	if (audioProcessor.getSelectedTrackId().isEmpty() || !audioProcessor.getTrack(audioProcessor.getSelectedTrackId()))
	{
		if (!loadedTrackIds.empty())
		{
			audioProcessor.setSelectedTrackId(loadedTrackIds[0]);
		}
		else
		{
			audioProcessor.setSelectedTrackId(audioProcessor.trackManager.createTrack("Main"));
		}
	}
	audioProcessor.setCrossfaderValue((float)state.getProperty("crossfaderValue", juce::var(0.5f)));
	audioProcessor.setCrossfadeMode((int)state.getProperty("crossfadeMode", juce::var(0)));

	auto paramsCheck = state.getChildWithName("Parameters");
	const bool needsCrossfaderMigration = !paramsCheck.isValid() || !paramsCheck.hasProperty("globalCrossfader");

	if (needsCrossfaderMigration)
	{
		if (state.hasProperty("globalCrossfader"))
		{
			if (auto* p = audioProcessor.getParameters().getParameter("globalCrossfader"))
				p->setValueNotifyingHost((float)state.getProperty("globalCrossfader", 0.5f));
		}
		for (int i = 0; i < 4; ++i)
		{
			juce::String oldKey = "pairCrossfader" + juce::String(i);
			juce::String newId = "pairCrossfader" + juce::String(i + 1);
			if (state.hasProperty(oldKey))
			{
				if (auto* p = audioProcessor.getParameters().getParameter(newId))
					p->setValueNotifyingHost((float)state.getProperty(oldKey, 0.5f));
			}
		}
		if (state.hasProperty("crossfaderCurveMode"))
		{
			int mode = juce::jlimit(0, 2, (int)state.getProperty("crossfaderCurveMode", 1));
			if (auto* p = audioProcessor.getParameters().getParameter("crossfaderCurveMode"))
				p->setValueNotifyingHost(mode / 2.0f);
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		audioProcessor.setPairCrossfaderPrevious(i, audioProcessor.getPairCrossfaderParam(i));
	}
	audioProcessor.setGlobalCrossfaderPrevious(audioProcessor.getGlobalCrossfaderParam());

	juce::ValueTree midiMappingsState = state.getChildWithName("MidiMappings");
	if (midiMappingsState.isValid())
	{
		audioProcessor.getMidiLearnManager().clearAllMappings();
		for (int i = 0; i < midiMappingsState.getNumChildren(); ++i)
		{
			MidiMapping mapping;
			juce::ValueTree mappingState = midiMappingsState.getChild(i);
			mapping.midiType = mappingState.getProperty("midiType");
			mapping.midiNumber = mappingState.getProperty("midiNumber");
			mapping.midiChannel = mappingState.getProperty("midiChannel");
			mapping.parameterName = mappingState.getProperty("parameterName");
			mapping.description = mappingState.getProperty("description");
			mapping.processor = &audioProcessor;
			mapping.uiCallback = nullptr;
			audioProcessor.getMidiLearnManager().addMapping(mapping);
		}
	}
	auto globalGenState = state.getChildWithName("GlobalGeneration");
	if (globalGenState.isValid())
	{
		audioProcessor.setGlobalPrompt(globalGenState.getProperty("prompt", "Generate a techno drum loop"));
		audioProcessor.setGlobalBpm(globalGenState.getProperty("bpm", 127.0f));
		audioProcessor.setGlobalKey(globalGenState.getProperty("key", "C Minor"));
		audioProcessor.setGlobalDuration(globalGenState.getProperty("duration", 6));
	}
	auto parametersState = state.getChildWithName("Parameters");
	if (parametersState.isValid())
	{
		auto& params = audioProcessor.getParameterTreeState();
		for (int slot = 1; slot <= 8; ++slot)
		{
			for (const char* page : { "PageA", "PageB", "PageC", "PageD" })
			{
				juce::String paramId = "slot" + juce::String(slot) + page;
				if (auto* param = params.getParameter(paramId))
					param->setValueNotifyingHost(0.0f);
			}
			for (int seq = 1; seq <= 8; ++seq)
			{
				juce::String paramId = "slot" + juce::String(slot) + "Seq" + juce::String(seq);
				if (auto* param = params.getParameter(paramId))
					param->setValueNotifyingHost(0.0f);
			}
		}
		for (const auto& paramId : audioProcessor.getBooleanParamIds())
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
		for (const auto& paramId : audioProcessor.getFloatParamIds())
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
	audioProcessor.setProjectId(state.getProperty("projectId", "legacy").toString());

	if (audioProcessor.getProjectId() == "legacy" || audioProcessor.getProjectId().isEmpty())
	{
		audioProcessor.setMigrationCompleted(false);
		juce::Timer::callAfterDelay(500, [this]()
			{ audioProcessor.performMigrationIfNeeded(); });
	}
	else
	{
		audioProcessor.setMigrationCompleted(true);
	}

	juce::Timer::callAfterDelay(2000, [this]()
		{
			auto trackIds = audioProcessor.getAllTrackIds();
			for (const auto& trackId : trackIds)
			{
				TrackData* track = audioProcessor.getTrack(trackId);
				if (!track) continue;

				auto& currentPage = track->getCurrentPage();

				int slotNumber = track->slotIndex + 1;

				if (track->isCurrentlyPlaying.load())
					audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPlay(slotNumber), MidiMapping::feedbackActive);
				else if (track->isArmed.load())
					audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPlay(slotNumber), MidiMapping::feedbackPending);
				else
					audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPlay(slotNumber), MidiMapping::feedbackIdle);

				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPage(slotNumber), track->currentPageIndex.load());

				float vol = audioProcessor.getVolumeParam(track->slotIndex) ? audioProcessor.getVolumeParam(track->slotIndex) : track->volume.load();
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackVolume(slotNumber), MidiMapping::volumeToMidi(vol));

				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPan(slotNumber), MidiMapping::panToMidi(track->pan.load()));

				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackMute(slotNumber),
					track->isMuted.load() ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackSolo(slotNumber),
					track->isSolo.load() ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackBeatRepeat(slotNumber),
					track->randomRetriggerEnabled.load() ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);

				float rawPitch = audioProcessor.getPitchParam(track->slotIndex) ? audioProcessor.getPitchParam(track->slotIndex) * 8.0f : (float)currentPage.bpmOffset.load();
				float rawFine = audioProcessor.getFineParam(track->slotIndex) ? audioProcessor.getFineParam(track->slotIndex) * 2.0f : currentPage.fineOffset.load();
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPitch(slotNumber), MidiMapping::pitchToMidi(rawPitch));
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackFine(slotNumber), MidiMapping::fineToMidi(rawFine));
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackSeq(slotNumber),
					track->getCurrentPage().currentSequenceIndex);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackAdsrAttack(slotNumber), MidiMapping::adsrToMidi(audioProcessor.getAttackParam(track->slotIndex), 0.001f, 4.0f), MidiMapping::feedbackChannelShaping);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackAdsrDecay(slotNumber), MidiMapping::adsrToMidi(audioProcessor.getDecayParam(track->slotIndex), 0.001f, 4.0f), MidiMapping::feedbackChannelShaping);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackAdsrSustain(slotNumber), MidiMapping::adsrToMidi(audioProcessor.getSustainParam(track->slotIndex), 0.0f, 1.0f), MidiMapping::feedbackChannelShaping);
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackAdsrRelease(slotNumber), MidiMapping::adsrToMidi(audioProcessor.getReleaseParam(track->slotIndex), 0.001f, 4.0f), MidiMapping::feedbackChannelShaping);
			}

			for (int p = 0; p < 4; ++p)
			{
				audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackPairCrossfader(p),
					MidiMapping::volumeToMidi(audioProcessor.getPairCrossfaderValue(p)),
					MidiMapping::feedbackChannelShaping);
			}

			audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackGlobalCrossfader, MidiMapping::volumeToMidi(audioProcessor.getGlobalCrossfaderValue()), MidiMapping::feedbackChannelShaping);
			audioProcessor.sendMidiFeedback(MidiMapping::ccFeedbackCrossfaderCurve, audioProcessor.getCrossfaderCurveMode() * 63, MidiMapping::feedbackChannelShaping);
		});

	audioProcessor.getMidiLearnManager().restoreUICallbacks();
	audioProcessor.setStateReady(true);
	audioProcessor.setIsLoadingState(false);
	juce::MessageManager::callAsync([this]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor())) {
				editor->refreshTrackComponents();
				editor->updateUIFromProcessor();
			}
		});
}