#pragma once
#include "JuceHeader.h"
#include "TrackData.h"

class TrackManager
{
public:
	TrackManager() = default;

	std::function<void(int slot, TrackData* track)> parameterUpdateCallback;

	juce::String createTrack(const juce::String& name = "Track")
	{
		juce::ScopedLock lock(tracksLock);
		for (int i = 0; i < 8; ++i)
		{
			usedSlots[i] = false;
		}
		for (const auto& pair : tracks)
		{
			if (pair.second->slotIndex >= 0 && pair.second->slotIndex < 8)
			{
				usedSlots[pair.second->slotIndex] = true;
			}
		}

		auto track = std::make_unique<TrackData>();
		track->trackName = name + " " + juce::String(tracks.size() + 1);
		track->bpmOffset = 0.0;
		track->midiNote = 60 + static_cast<int>(tracks.size());
		juce::String trackId = track->trackId;
		std::string stdId = trackId.toStdString();
		track->slotIndex = findFreeSlot();

		if (track->slotIndex != -1)
		{
			usedSlots[track->slotIndex] = true;
		}
		tracks[stdId] = std::move(track);
		trackOrder.push_back(stdId);
		return trackId;
	}

	void removeTrack(const juce::String& trackId)
	{
		juce::ScopedLock lock(tracksLock);
		std::string stdId = trackId.toStdString();
		if (auto* track = getTrack(trackId))
		{
			if (track->slotIndex != -1)
			{
				usedSlots[track->slotIndex] = false;
			}
		}
		tracks.erase(stdId);
		trackOrder.erase(std::remove(trackOrder.begin(), trackOrder.end(), stdId), trackOrder.end());
	}

	void reorderTracks(const juce::String& fromTrackId, const juce::String& toTrackId)
	{
		juce::ScopedLock lock(tracksLock);

		std::string fromStdId = fromTrackId.toStdString();
		std::string toStdId = toTrackId.toStdString();

		auto fromIt = std::find(trackOrder.begin(), trackOrder.end(), fromStdId);
		auto toIt = std::find(trackOrder.begin(), trackOrder.end(), toStdId);

		if (fromIt == trackOrder.end() || toIt == trackOrder.end())
			return;

		std::string movedId = *fromIt;
		trackOrder.erase(fromIt);

		toIt = std::find(trackOrder.begin(), trackOrder.end(), toStdId);
		trackOrder.insert(toIt, movedId);
	}

	TrackData* getTrack(const juce::String& trackId)
	{
		juce::ScopedLock lock(tracksLock);
		auto it = tracks.find(trackId.toStdString());
		return (it != tracks.end()) ? it->second.get() : nullptr;
	}

	std::vector<juce::String> getAllTrackIds() const
	{
		juce::ScopedLock lock(tracksLock);
		std::vector<juce::String> ids;
		for (const auto& stdId : trackOrder)
		{
			if (tracks.count(stdId))
			{
				ids.push_back(juce::String(stdId));
			}
		}
		return ids;
	}
	void renderAllTracks(juce::AudioBuffer<float>& outputBuffer,
		std::vector<juce::AudioBuffer<float>>& individualOutputs,
		double hostBpm)
	{
		const int numSamples = outputBuffer.getNumSamples();
		bool anyTrackSolo = false;

		{
			juce::ScopedLock lock(tracksLock);
			for (const auto& pair : tracks)
			{
				if (pair.second->isSolo.load())
				{
					anyTrackSolo = true;
					break;
				}
			}
		}

		outputBuffer.clear();
		for (auto& buffer : individualOutputs)
		{
			buffer.clear();
		}

		juce::ScopedLock lock(tracksLock);

		for (const auto& pair : tracks)
		{
			auto* track = pair.second.get();

			if (track->isEnabled.load() && track->numSamples > 0 &&
				track->slotIndex >= 0 && track->slotIndex < individualOutputs.size())
			{
				int bufferIndex = track->slotIndex;

				juce::AudioBuffer<float> tempMixBuffer(outputBuffer.getNumChannels(), numSamples);
				juce::AudioBuffer<float> tempIndividualBuffer(2, numSamples);
				tempMixBuffer.clear();
				tempIndividualBuffer.clear();

				renderSingleTrack(*track, tempMixBuffer, tempIndividualBuffer,
					numSamples, bufferIndex, hostBpm);

				bool shouldHearTrack = !track->isMuted.load() &&
					(!anyTrackSolo || track->isSolo.load());

				if (shouldHearTrack)
				{
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
					{
						outputBuffer.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
					}
				}

				for (int ch = 0; ch < std::min(2, individualOutputs[bufferIndex].getNumChannels()); ++ch)
				{
					individualOutputs[bufferIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);

					if (!shouldHearTrack)
					{
						individualOutputs[bufferIndex].applyGain(ch, 0, numSamples, 0.0f);
					}
				}
			}
		}
	}

	juce::ValueTree saveState() const
	{
		juce::ValueTree state("TrackManager");

		juce::ScopedLock lock(tracksLock);
		for (const auto& pair : tracks)
		{
			auto trackState = juce::ValueTree("Track");
			auto* track = pair.second.get();

			trackState.setProperty("id", track->trackId, nullptr);
			trackState.setProperty("name", track->trackName, nullptr);
			trackState.setProperty("slotIndex", track->slotIndex, nullptr);
			trackState.setProperty("prompt", track->prompt, nullptr);
			trackState.setProperty("style", track->style, nullptr);
			trackState.setProperty("bpm", track->bpm, nullptr);
			trackState.setProperty("originalBpm", track->originalBpm, nullptr);
			trackState.setProperty("timeStretchMode", track->timeStretchMode, nullptr);
			trackState.setProperty("bpmOffset", track->bpmOffset, nullptr);
			trackState.setProperty("midiNote", track->midiNote, nullptr);
			trackState.setProperty("loopStart", track->loopStart, nullptr);
			trackState.setProperty("loopEnd", track->loopEnd, nullptr);
			trackState.setProperty("volume", track->volume.load(), nullptr);
			trackState.setProperty("pan", track->pan.load(), nullptr);
			trackState.setProperty("muted", track->isMuted.load(), nullptr);
			trackState.setProperty("solo", track->isSolo.load(), nullptr);
			trackState.setProperty("enabled", track->isEnabled.load(), nullptr);
			trackState.setProperty("fineOffset", track->fineOffset, nullptr);
			trackState.setProperty("timeStretchRatio", track->timeStretchRatio, nullptr);
			trackState.setProperty("stagingOriginalBpm", track->stagingOriginalBpm, nullptr);
			trackState.setProperty("showWaveform", track->showWaveform, nullptr);
			trackState.setProperty("showSequencer", track->showSequencer, nullptr);
			trackState.setProperty("isPlaying", track->isPlaying.load(), nullptr);
			trackState.setProperty("isArmed", track->isArmed.load(), nullptr);
			trackState.setProperty("isArmedToStop", track->isArmedToStop.load(), nullptr);
			trackState.setProperty("isCurrentlyPlaying", track->isCurrentlyPlaying.load(), nullptr);
			trackState.setProperty("generationPrompt", track->generationPrompt, nullptr);
			trackState.setProperty("generationBpm", track->generationBpm, nullptr);
			trackState.setProperty("generationKey", track->generationKey, nullptr);
			trackState.setProperty("generationDuration", track->generationDuration, nullptr);
			trackState.setProperty("loopPointsLocked", track->loopPointsLocked.load(), nullptr);
			trackState.setProperty("selectedPrompt", track->selectedPrompt, nullptr);
			trackState.setProperty("useOriginalFile", track->useOriginalFile.load(), nullptr);
			trackState.setProperty("hasOriginalVersion", track->hasOriginalVersion.load(), nullptr);
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
			trackState.setProperty("usePages", track->usePages.load(), nullptr);
			trackState.setProperty("currentPageIndex", track->currentPageIndex, nullptr);
			trackState.setProperty("canvasData", track->canvasData, nullptr);
			trackState.setProperty("canvasState", track->canvasState, nullptr);
			trackState.setProperty("selectedKeywords", track->selectedKeywords.joinIntoString("|"), nullptr);

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

				trackState.appendChild(pageState, nullptr);
			}

			if (track->numSamples > 0 && !track->audioFilePath.isEmpty())
			{
				trackState.setProperty("audioFilePath", track->audioFilePath, nullptr);
				trackState.setProperty("sampleRate", track->sampleRate, nullptr);
				trackState.setProperty("numSamples", track->numSamples, nullptr);
				trackState.setProperty("numChannels", track->audioBuffer.getNumChannels(), nullptr);
			}
			juce::ValueTree sequencerState("Sequencer");
			sequencerState.setProperty("isPlaying", track->sequencerData.isPlaying, nullptr);
			sequencerState.setProperty("currentStep", track->sequencerData.currentStep, nullptr);
			sequencerState.setProperty("currentMeasure", track->sequencerData.currentMeasure, nullptr);
			sequencerState.setProperty("numMeasures", track->sequencerData.numMeasures, nullptr);
			sequencerState.setProperty("beatsPerMeasure", track->sequencerData.beatsPerMeasure, nullptr);
			for (int m = 0; m < 4; ++m)
			{
				for (int s = 0; s < 16; ++s)
				{
					juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
					sequencerState.setProperty(stepKey, track->sequencerData.steps[m][s], nullptr);

					juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
					sequencerState.setProperty(velocityKey, track->sequencerData.velocities[m][s], nullptr);
				}
			}
			trackState.appendChild(sequencerState, nullptr);
			state.appendChild(trackState, nullptr);
		}

		return state;
	}

	void loadState(const juce::ValueTree& state)
	{
		juce::ScopedLock lock(tracksLock);
		tracks.clear();
		trackOrder.clear();
		usedSlots.fill(false);
		for (int i = 0; i < state.getNumChildren(); ++i)
		{
			auto trackState = state.getChild(i);
			if (!trackState.hasType("Track"))
			{
				continue;
			}

			auto track = std::make_unique<TrackData>();

			track->trackId = trackState.getProperty("id", juce::Uuid().toString());
			track->trackName = trackState.getProperty("name", "Track");
			track->prompt = trackState.getProperty("prompt", "");
			track->slotIndex = trackState.getProperty("slotIndex", -1);
			track->style = trackState.getProperty("style", "");
			track->bpm = trackState.getProperty("bpm", 126.0f);
			track->originalBpm = trackState.getProperty("originalBpm", 126.0f);
			track->timeStretchMode = 4;
			track->bpmOffset = trackState.getProperty("bpmOffset", 0.0);
			track->midiNote = trackState.getProperty("midiNote", 60);
			track->loopStart = trackState.getProperty("loopStart", 0.0);
			track->loopEnd = trackState.getProperty("loopEnd", 4.0);
			track->volume = trackState.getProperty("volume", 0.8f);
			track->pan = trackState.getProperty("pan", 0.0f);
			track->isEnabled = trackState.getProperty("enabled", true);
			track->fineOffset = trackState.getProperty("fineOffset", 0.0f);
			track->timeStretchRatio = trackState.getProperty("timeStretchRatio", 1.0);
			track->stagingOriginalBpm = trackState.getProperty("stagingOriginalBpm", 126.0f);
			track->showWaveform = trackState.getProperty("showWaveform", false);
			track->showSequencer = trackState.getProperty("showSequencer", false);
			track->isMuted = trackState.getProperty("muted", false);
			track->isSolo = trackState.getProperty("solo", false);
			track->isPlaying = trackState.getProperty("isPlaying", false);
			track->isArmed = trackState.getProperty("isArmed", false);
			track->isArmedToStop = trackState.getProperty("isArmedToStop", false);
			track->isCurrentlyPlaying = trackState.getProperty("isCurrentlyPlaying", false);
			track->generationPrompt = trackState.getProperty("generationPrompt", "Generate a techno drum loop");
			track->generationBpm = trackState.getProperty("generationBpm", 127.0f);
			track->generationKey = trackState.getProperty("generationKey", "C Minor");
			track->generationDuration = trackState.getProperty("generationDuration", 6);
			track->loopPointsLocked = trackState.getProperty("loopPointsLocked", false);
			track->selectedPrompt = trackState.getProperty("selectedPrompt", "");
			track->useOriginalFile = trackState.getProperty("useOriginalFile", false);
			track->hasOriginalVersion = trackState.getProperty("hasOriginalVersion", false);
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
			track->canvasData = trackState.getProperty("canvasData", "");
			track->canvasState = trackState.getProperty("canvasState", "");
			track->usePages = trackState.getProperty("usePages", false);
			track->currentPageIndex = trackState.getProperty("currentPageIndex", 0);

			juce::String keywordsStr = trackState.getProperty("selectedKeywords", "");
			if (keywordsStr.isNotEmpty())
			{
				track->selectedKeywords.addTokens(keywordsStr, "|", "");
			}

			if (track->usePages.load())
			{
				DBG("Loading track " << track->trackName << " with pages system");

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

						juce::String pageKeywordsStr = pageState.getProperty("selectedKeywords", "");
						if (pageKeywordsStr.isNotEmpty())
						{
							page.selectedKeywords.addTokens(pageKeywordsStr, "|", "");
						}

						page.isLoaded = false;

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
											DBG("Loading ORIGINAL version for page " << (char)('A' + pageIndex) << ": " << originalFile.getFullPathName());
										}
										else
										{
											DBG("Original file not found: " << originalFile.getFullPathName());
										}
									}
								}

								DBG("Loading page " << (char)('A' + pageIndex) << " from: " << fileToLoad.getFullPathName());
								loadAudioFileForPage(track.get(), pageIndex, fileToLoad);
							}
							else
							{
								DBG("Page " << (char)('A' + pageIndex) << " file not found: " << page.audioFilePath);
								juce::String fileName = audioFile.getFileName();
								if (fileName.contains("_" + juce::String('A' + pageIndex)))
								{
									char pageName = static_cast<char>('A' + pageIndex);
									juce::String newFileName = fileName.replace("_" + juce::String('A' + pageIndex), "_" + juce::String(pageName));
									juce::File newFile = audioFile.getParentDirectory().getChildFile(newFileName);

									if (newFile.existsAsFile())
									{
										DBG("Found file with new naming: " << newFile.getFullPathName());
										loadAudioFileForPage(track.get(), pageIndex, newFile);
										page.audioFilePath = newFile.getFullPathName();
									}
								}
							}
						}
					}
					else
					{
						DBG("Page " << pageIndex << " state not found - empty page");
					}
				}

				track->syncLegacyProperties();
				DBG("Track " << track->trackName << " loaded in pages mode - current page: " << (char)('A' + track->currentPageIndex) << " with " << track->numSamples << " samples");
			}
			else
			{
				DBG("Loading track " << track->trackName << " in legacy mode");
				juce::String audioFilePath = trackState.getProperty("audioFilePath", "");
				if (audioFilePath.isNotEmpty())
				{
					juce::File audioFile(audioFilePath);
					DBG("LOADING STATE - audioFilePath: " + audioFilePath.toStdString());
					if (audioFile.existsAsFile())
					{
						DBG("File exists: YES");
						track->audioFilePath = audioFilePath;
						track->sampleRate = trackState.getProperty("sampleRate", 48000.0);
						track->numSamples = trackState.getProperty("numSamples", 0);

						juce::File fileToLoad = audioFile;
						if (track->useOriginalFile.load() && track->hasOriginalVersion.load())
						{
							juce::String originalPath = audioFilePath.replace(".wav", "_original.wav");
							juce::File originalFile(originalPath);
							if (originalFile.existsAsFile())
							{
								fileToLoad = originalFile;
								DBG("Loading original version: " + originalPath.toStdString());
							}
						}

						loadAudioFileForTrack(track.get(), fileToLoad);
						DBG("Loaded track audio from: " + fileToLoad.getFullPathName().toStdString());
					}
					else
					{
						DBG("File exists: NO");
						DBG("Audio file not found: " + audioFilePath.toStdString());
					}
				}
				else
				{
					DBG("No audioFilePath in state for track with slot index: " << juce::String(track->slotIndex));
				}
			}

			if (!track->usePages.load())
			{
				track->lastPpqPosition = -1.0;
				track->customStepCounter = 0;
				track->sequencerData.stepAccumulator = 0.0;
			}

			auto sequencerState = trackState.getChildWithName("Sequencer");
			if (sequencerState.isValid())
			{
				track->sequencerData.isPlaying = sequencerState.getProperty("isPlaying", false);
				track->sequencerData.currentStep = 0;
				track->sequencerData.currentMeasure = 0;
				track->sequencerData.numMeasures = sequencerState.getProperty("numMeasures", 1);
				track->sequencerData.beatsPerMeasure = sequencerState.getProperty("beatsPerMeasure", 4);
				for (int m = 0; m < 4; ++m)
				{
					for (int s = 0; s < 16; ++s)
					{
						juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
						track->sequencerData.steps[m][s] = sequencerState.getProperty(stepKey, false);

						juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
						track->sequencerData.velocities[m][s] = sequencerState.getProperty(velocityKey, 0.8f);
					}
				}
			}

			if (track->slotIndex < 0 || track->slotIndex >= 8 || usedSlots[track->slotIndex])
			{
				track->slotIndex = findFreeSlot();
			}
			if (track->slotIndex >= 0 && track->slotIndex < 8)
			{
				usedSlots[track->slotIndex] = true;
			}

			std::string stdId = track->trackId.toStdString();
			tracks[stdId] = std::move(track);
			trackOrder.push_back(stdId);
		}
	}

	std::array<bool, 8> usedSlots{ false };

	void loadAudioFileForPage(TrackData* track, int pageIndex, const juce::File& audioFile)
	{
		if (!track || pageIndex < 0 || pageIndex >= 4)
		{
			DBG("loadAudioFileForPage: Invalid parameters - track=" << (track ? "valid" : "null") << ", pageIndex=" << pageIndex);
			return;
		}

		auto& page = track->pages[pageIndex];

		DBG("loadAudioFileForPage: Attempting to load page " << (char)('A' + pageIndex) << " from: " << audioFile.getFullPathName());

		static juce::AudioFormatManager formatManager;
		static bool initialized = false;
		if (!initialized)
		{
			formatManager.registerBasicFormats();
			initialized = true;
		}

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
		if (!reader)
		{
			DBG("loadAudioFileForPage: Failed to create reader for page " << pageIndex << ": " << audioFile.getFullPathName());
			page.numSamples = 0;
			page.isLoaded = false;
			page.audioBuffer.setSize(0, 0);
			return;
		}

		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		DBG("loadAudioFileForPage: File info - channels=" << numChannels << ", samples=" << numSamples << ", sampleRate=" << reader->sampleRate);

		if (numSamples <= 0)
		{
			DBG("loadAudioFileForPage: No samples in file for page " << pageIndex);
			page.numSamples = 0;
			page.isLoaded = false;
			page.audioBuffer.setSize(0, 0);
			return;
		}

		page.audioBuffer.setSize(2, numSamples, false, true, true);
		page.audioBuffer.clear();

		DBG("loadAudioFileForPage: Buffer allocated - channels=" << page.audioBuffer.getNumChannels() << ", samples=" << page.audioBuffer.getNumSamples());

		if (!reader->read(&page.audioBuffer, 0, numSamples, 0, true, true))
		{
			DBG("loadAudioFileForPage: Failed to read samples for page " << pageIndex);
			page.numSamples = 0;
			page.isLoaded = false;
			page.audioBuffer.setSize(0, 0);
			return;
		}

		if (numChannels == 1)
		{
			page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);
			DBG("loadAudioFileForPage: Converted mono to stereo");
		}

		page.numSamples = numSamples;
		page.sampleRate = reader->sampleRate;
		page.isLoaded = true;
		page.isLoading = false;

		DBG("loadAudioFileForPage: SUCCESS - Page " << (char)('A' + pageIndex) << " loaded with " << page.numSamples << " samples, buffer has " << page.audioBuffer.getNumSamples() << " samples");

		float maxSample = 0.0f;
		for (int ch = 0; ch < page.audioBuffer.getNumChannels(); ++ch)
		{
			auto* channelData = page.audioBuffer.getReadPointer(ch);
			for (int i = 0; i < page.audioBuffer.getNumSamples(); ++i)
			{
				maxSample = std::max(maxSample, std::abs(channelData[i]));
			}
		}
		DBG("loadAudioFileForPage: Max sample amplitude: " << maxSample);
	}

	void loadAudioFileForTrack(TrackData* track, const juce::File& audioFile)
	{
		static juce::AudioFormatManager formatManager;
		static bool initialized = false;
		if (!initialized)
		{
			formatManager.registerBasicFormats();
			initialized = true;
		}

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));

		if (reader != nullptr)
		{
			int numChannels = reader->numChannels;
			int numSamples = static_cast<int>(reader->lengthInSamples);

			track->audioBuffer.setSize(2, numSamples);
			reader->read(&track->audioBuffer, 0, numSamples, 0, true, true);

			if (numChannels == 1)
			{
				track->audioBuffer.copyFrom(1, 0, track->audioBuffer, 0, 0, numSamples);
			}

			track->numSamples = track->audioBuffer.getNumSamples();
			track->sampleRate = reader->sampleRate;

			DBG("Loaded audio file: " + audioFile.getFullPathName() +
				" (" + juce::String(numSamples) + " samples, " +
				juce::String(track->sampleRate) + " Hz)");

			reader.reset();
		}
		else
		{
			DBG("Failed to load audio file: " + audioFile.getFullPathName());
		}
	}

private:
	mutable juce::CriticalSection tracksLock;
	std::unordered_map<std::string, std::unique_ptr<TrackData>> tracks;
	std::vector<std::string> trackOrder;

	int findFreeSlot()
	{
		DBG("Finding free slot - Current usedSlots state:");
		for (int i = 0; i < 8; ++i)
		{
			DBG("  Slot " << i << ": " << (usedSlots[i] ? "USED" : "FREE"));
		}

		DBG("Actual slot usage from tracks:");
		std::vector<bool> actualUsage(8, false);
		for (const auto& pair : tracks)
		{
			const auto& track = pair.second;
			if (track->slotIndex >= 0 && track->slotIndex < 8)
			{
				actualUsage[track->slotIndex] = true;
				DBG("  Slot " << track->slotIndex << ": USED by " << track->trackName);
			}
		}

		for (int i = 0; i < 8; ++i)
		{
			if (usedSlots[i] != actualUsage[i])
			{
				DBG("INCONSISTENCY: Slot " << i << " - usedSlots[" << i << "]=" << (usedSlots[i] ? "USED" : "FREE") << " but actually " << (actualUsage[i] ? "USED" : "FREE"));
			}
		}

		for (int i = 0; i < 8; ++i)
		{
			if (!usedSlots[i])
			{
				DBG("Found free slot: " << i);
				return i;
			}
		}

		DBG("No free slots available!");
		return -1;
	}

	void renderSingleTrack(TrackData& track,
		juce::AudioBuffer<float>& mixOutput,
		juce::AudioBuffer<float>& individualOutput,
		int numSamples, int /*trackIndex*/, double hostBpm) const
	{
		if (parameterUpdateCallback)
		{
			int slot = track.slotIndex;
			if (slot != -1)
			{
				parameterUpdateCallback(slot, &track);
			}
		}

		const juce::AudioSampleBuffer* bufferToUse = nullptr;
		int numSamplesToUse = 0;
		double sampleRateToUse = 0;
		double loopStartToUse = 0;
		double loopEndToUse = 0;
		float originalBpmToUse = 126.0f;

		if (track.usePages.load())
		{
			const auto& currentPage = track.getCurrentPage();
			bufferToUse = &currentPage.audioBuffer;
			numSamplesToUse = currentPage.numSamples;
			sampleRateToUse = currentPage.sampleRate;
			loopStartToUse = currentPage.loopStart;
			loopEndToUse = currentPage.loopEnd;
			originalBpmToUse = currentPage.originalBpm;
		}
		else
		{
			bufferToUse = &track.audioBuffer;
			numSamplesToUse = track.numSamples;
			sampleRateToUse = track.sampleRate;
			loopStartToUse = track.loopStart;
			loopEndToUse = track.loopEnd;
			originalBpmToUse = track.originalBpm;
		}

		if (numSamplesToUse == 0 || !track.isPlaying.load() || !bufferToUse)
			return;


		const float volume = juce::jlimit(0.0f, 1.0f, track.volume.load());
		const float pan = juce::jlimit(-1.0f, 1.0f, track.pan.load());

		float leftGain = 1.0f;
		float rightGain = 1.0f;
		if (pan < 0.0f)
		{
			rightGain = 1.0f + pan;
		}
		else if (pan > 0.0f)
		{
			leftGain = 1.0f - pan;
		}

		double currentPosition = track.readPosition.load();
		double playbackRatio = 1.0;

		switch (track.timeStretchMode)
		{
		case 1:
			playbackRatio = 1.0;
			break;
		case 2:
			if (originalBpmToUse > 0.0f)
			{
				float totalBpmAdjust = static_cast<float>(track.bpmOffset) + track.fineOffset;
				float adjustedBpm = originalBpmToUse + totalBpmAdjust;
				adjustedBpm = juce::jlimit(1.0f, 1000.0f, adjustedBpm);
				playbackRatio = adjustedBpm / originalBpmToUse;
			}
			break;
		case 3:
			if (originalBpmToUse > 0.0f && hostBpm > 0.0)
			{
				playbackRatio = hostBpm / originalBpmToUse;
			}
			break;
		case 4:
			if (originalBpmToUse > 0.0f && hostBpm > 0.0)
			{
				float totalManualAdjust = static_cast<float>(track.bpmOffset) + track.fineOffset;
				float effectiveHostBpm = static_cast<float>(hostBpm) + totalManualAdjust;
				effectiveHostBpm = juce::jlimit(1.0f, 1000.0f, effectiveHostBpm);
				playbackRatio = effectiveHostBpm / originalBpmToUse;
			}
			break;
		}

		double startSample = loopStartToUse * sampleRateToUse;
		double endSample = loopEndToUse * sampleRateToUse;

		startSample = juce::jlimit(0.0, (double)numSamplesToUse - 1, startSample);
		endSample = juce::jlimit(startSample + 1, (double)numSamplesToUse, endSample);

		double sectionLength = endSample - startSample;

		if (sectionLength < 100)
		{
			startSample = 0.0;
			endSample = numSamplesToUse;
			sectionLength = numSamplesToUse;
		}

		const float* leftChannel = bufferToUse->getReadPointer(0);
		const float* rightChannel = bufferToUse->getNumChannels() > 1
			? bufferToUse->getReadPointer(1)
			: leftChannel;

		const int bufferSize = bufferToUse->getNumSamples();

		const double fadeStartPosition = endSample - 64.0;
		const float fadeRcpLength = 1.0f / 64.0f;

		const bool beatRepeatActive = track.beatRepeatActive.load();
		const double beatRepeatStart = beatRepeatActive ? track.beatRepeatStartPosition.load() : 0.0;
		const double beatRepeatEnd = beatRepeatActive ? track.beatRepeatEndPosition.load() : 0.0;

		for (int i = 0; i < numSamples; ++i)
		{
			if (beatRepeatActive)
			{
				double absolutePos = startSample + currentPosition;
				if (absolutePos >= beatRepeatEnd)
				{
					currentPosition = beatRepeatStart - startSample;
					track.readPosition.store(beatRepeatStart);
				}
			}

			double absolutePosition = startSample + currentPosition;

			if (absolutePosition >= endSample)
			{
				track.readPosition = 0.0;
				track.isPlaying = false;
				return;
			}

			if (absolutePosition >= numSamplesToUse)
			{
				currentPosition = 0.0;
				absolutePosition = startSample;
			}

			int sampleIndex = static_cast<int>(absolutePosition);
			if (sampleIndex >= bufferSize)
			{
				track.isPlaying = false;
				break;
			}

			float fadeGain = 1.0f;
			if (absolutePosition > fadeStartPosition)
			{
				fadeGain = static_cast<float>(endSample - absolutePosition) * fadeRcpLength;
				fadeGain = juce::jlimit(0.0f, 1.0f, fadeGain);
			}

			float leftSample = interpolateLinear(leftChannel, absolutePosition, bufferSize);
			float rightSample = interpolateLinear(rightChannel, absolutePosition, bufferSize);

			leftSample *= volume * leftGain * fadeGain;
			rightSample *= volume * rightGain * fadeGain;

			mixOutput.addSample(0, i, leftSample);
			mixOutput.addSample(1, i, rightSample);
			individualOutput.setSample(0, i, leftSample);
			individualOutput.setSample(1, i, rightSample);

			currentPosition += playbackRatio;
		}

		track.readPosition = currentPosition;
	}

	float interpolateLinear(const float* buffer, double position, int bufferSize) const
	{
		int index = static_cast<int>(position);
		if (index >= bufferSize - 1)
			return buffer[bufferSize - 1];

		float fraction = static_cast<float>(position - index);
		return buffer[index] + fraction * (buffer[index + 1] - buffer[index]);
	}
};