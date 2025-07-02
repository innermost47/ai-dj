/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

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

		int trackIndex = 0;
		for (const auto& pair : tracks)
		{
			if (trackIndex >= individualOutputs.size())
				break;

			auto* track = pair.second.get();

			if (track->isEnabled.load() && track->numSamples > 0)
			{
				juce::AudioBuffer<float> tempMixBuffer(outputBuffer.getNumChannels(), numSamples);
				juce::AudioBuffer<float> tempIndividualBuffer(2, numSamples);
				tempMixBuffer.clear();
				tempIndividualBuffer.clear();

				renderSingleTrack(*track, tempMixBuffer, tempIndividualBuffer,
					numSamples, trackIndex, hostBpm);

				bool shouldHearTrack = !track->isMuted.load() &&
					(!anyTrackSolo || track->isSolo.load());

				if (shouldHearTrack)
				{
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
					{
						outputBuffer.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
					}
				}

				for (int ch = 0; ch < std::min(2, individualOutputs[trackIndex].getNumChannels()); ++ch)
				{
					individualOutputs[trackIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);

					if (!shouldHearTrack)
					{
						individualOutputs[trackIndex].applyGain(ch, 0, numSamples, 0.0f);
					}
				}
			}

			trackIndex++;
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
			trackState.setProperty("stems", track->stems, nullptr);
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
			trackState.setProperty("speed", track->speed.load(), nullptr);
			juce::String stemsString;
			for (int i = 0; i < track->preferredStems.size(); ++i)
			{
				if (i > 0)
					stemsString += ",";
				stemsString += track->preferredStems[i];
			}
			trackState.setProperty("preferredStems", stemsString, nullptr);
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

	void loadState(const juce::ValueTree& state, std::atomic<bool> cachedHostBpm)
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
			track->stems = trackState.getProperty("stems", "");
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
			track->isPlaying = trackState.getProperty("isplaying", false);
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
			juce::String stemsString = trackState.getProperty("preferredStems", "drums,bass");
			track->speed = trackState.getProperty("speed", 1.0f);
			track->lastPpqPosition = -1.0;
			track->customStepCounter = 0;
			track->sequencerData.stepAccumulator = 0.0;
			track->preferredStems.clear();
			if (stemsString.isNotEmpty())
			{
				juce::StringArray stemsArray = juce::StringArray::fromTokens(stemsString, ",", "");
				for (const auto& stem : stemsArray)
				{
					track->preferredStems.push_back(stem.trim());
				}
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
			juce::String audioFilePath = trackState.getProperty("audioFilePath", "");
			if (audioFilePath.isNotEmpty())
			{
				juce::File audioFile(audioFilePath);
				DBG("LOADING STATE - audioFilePath: " + audioFilePath.toStdString());
				if (audioFile.existsAsFile())
				{
					DBG("File exists: YES");
				}
				else
				{
					DBG("File exists: NO");
				}

				if (audioFile.existsAsFile())
				{
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

					loadAudioFileForTrack(track.get(), fileToLoad, cachedHostBpm.load());
					DBG("Loaded track audio from: " + fileToLoad.getFullPathName().toStdString());
				}
				else
				{
					DBG("Audio file not found: " + audioFilePath.toStdString());
				}
			}
			else
			{
				DBG("No audioFilePath in state for track with slot index: " << juce::String(track->slotIndex));
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

	void loadAudioFileForTrack(TrackData* track, const juce::File& audioFile, std::atomic<bool> /*cachedHostBpm*/)
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile));

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

	void renderSingleTrack(TrackData& track,
		juce::AudioBuffer<float>& mixOutput,
		juce::AudioBuffer<float>& individualOutput,
		int numSamples, int /*trackIndex*/,
		double /*hostBpm*/) const
	{
		if (parameterUpdateCallback)
		{
			int slot = track.slotIndex;
			if (slot != -1)
			{
				parameterUpdateCallback(slot, &track);
			}
		}

		if (track.numSamples == 0 || !track.isPlaying.load())
			return;

		const float volume = juce::jlimit(0.0f, 1.0f, track.volume.load());
		const float pan = juce::jlimit(-1.0f, 1.0f, track.pan.load());

		individualOutput.clear();

		// ✅ Détecter si on utilise pitch OU fine pitch
		bool hasPitchChange = std::abs(track.fineOffset) > 0.001f || std::abs(track.bpmOffset) > 0.001f;

		if (hasPitchChange)
		{
			double currentPosition = track.readPosition.load();
			double playbackRatio = track.cachedPlaybackRatio.load();

			double startSample = track.loopStart * track.sampleRate;
			double endSample = track.loopEnd * track.sampleRate;
			startSample = juce::jlimit(0.0, (double)track.numSamples - 1, startSample);
			endSample = juce::jlimit(startSample + 1, (double)track.numSamples, endSample);
			double sectionLength = endSample - startSample;

			if (sectionLength < 100)
			{
				startSample = 0.0;
				endSample = track.numSamples;
				sectionLength = track.numSamples;
			}

			for (int i = 0; i < numSamples; ++i)
			{
				double absolutePosition = startSample + currentPosition;

				if (absolutePosition >= endSample)
				{
					currentPosition = 0.0;
					absolutePosition = startSample;
					track.isPlaying = false;
					return;
				}

				if (absolutePosition >= track.numSamples)
				{
					currentPosition = 0.0;
					absolutePosition = startSample;
				}

				for (int ch = 0; ch < std::min(2, track.audioBuffer.getNumChannels()); ++ch)
				{
					int sampleIndex = static_cast<int>(absolutePosition);
					if (sampleIndex >= track.audioBuffer.getNumSamples())
					{
						track.isPlaying = false;
						break;
					}

					float sample = track.audioBuffer.getSample(ch, sampleIndex);

					if (absolutePosition > (endSample - 64))
					{
						float fadeGain = (static_cast<float>(endSample) - static_cast<float>(absolutePosition)) / 64.0f;
						fadeGain = juce::jlimit(0.0f, 1.0f, fadeGain);
						sample *= fadeGain;
					}

					individualOutput.setSample(ch, i, sample);
				}

				currentPosition += playbackRatio;
			}

			track.readPosition = currentPosition;
		}
		else
		{
			track.processAndFillBufferWithSpeedChange(individualOutput, numSamples);
		}

		float leftGain = volume;
		float rightGain = volume;

		if (pan < 0.0f)
		{
			rightGain *= (1.0f + pan);
		}
		else if (pan > 0.0f)
		{
			leftGain *= (1.0f - pan);
		}

		for (int ch = 0; ch < std::min(2, individualOutput.getNumChannels()); ++ch)
		{
			float gain = (ch == 0) ? leftGain : rightGain;

			for (int i = 0; i < numSamples; ++i)
			{
				float sample = individualOutput.getSample(ch, i) * gain;
				mixOutput.addSample(ch, i, sample);
			}
		}
	}

	float interpolateLinear(const float* buffer, double position, int bufferSize)
	{
		int index = static_cast<int>(position);
		if (index >= bufferSize - 1)
			return buffer[bufferSize - 1];

		float fraction = static_cast<float>(position - index);
		return buffer[index] + fraction * (buffer[index + 1] - buffer[index]);
	}
};