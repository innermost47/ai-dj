/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

#pragma once
#include <JuceHeader.h>
#include "DjIaClient.h"
#include "SoundTouch.h"

struct TrackData
{
	juce::String trackId;
	juce::String trackName;
	int slotIndex = -1;
	std::atomic<bool> isPlaying{ false };
	std::atomic<bool> isArmed{ false };
	juce::String audioFilePath;
	std::atomic<bool> isArmedToStop{ false };
	std::atomic<bool> isCurrentlyPlaying{ false };
	float fineOffset = 0.0f;
	std::atomic<double> cachedPlaybackRatio{ 1.0 };
	juce::AudioSampleBuffer stagingBuffer;
	std::atomic<bool> hasStagingData{ false };
	std::atomic<bool> swapRequested{ false };
	std::function<void(bool)> onPlayStateChanged;
	std::function<void(bool)> onArmedStateChanged;
	std::function<void(bool)> onArmedToStopStateChanged;
	std::atomic<int> stagingNumSamples{ 0 };
	std::atomic<double> stagingSampleRate{ 48000.0 };
	float stagingOriginalBpm = 126.0f;
	double loopStart = 0.0;
	double loopEnd = 4.0;
	float originalBpm = 126.0f;
	int timeStretchMode = 4;
	double timeStretchRatio = 1.0;
	double bpmOffset = 0.0;
	int midiNote = 60;
	juce::AudioSampleBuffer audioBuffer;
	double sampleRate = 48000.0;
	int numSamples = 0;
	std::atomic<bool> isEnabled{ true };
	std::atomic<bool> isSolo{ false };
	std::atomic<bool> isMuted{ false };
	std::atomic<bool> loopPointsLocked{ false };
	std::atomic<float> volume{ 0.8f };
	std::atomic<float> pan{ 0.0f };
	juce::String prompt;
	juce::String style;
	juce::String stems;
	int customStepCounter = 0;
	double lastPpqPosition = -1.0;
	float bpm = 126.0f;
	std::atomic<double> readPosition{ 0.0 };
	bool showWaveform = false;
	bool showSequencer = false;
	juce::String generationPrompt;
	float generationBpm;
	juce::String generationKey;
	int generationDuration;
	std::vector<juce::String> preferredStems = {};
	juce::String selectedPrompt;
	std::atomic<bool> useOriginalFile{ false };
	std::atomic<bool> hasOriginalVersion{ false };
	juce::AudioBuffer<float> originalStagingBuffer;
	std::atomic<float> speed{ 1.0f };
	std::unique_ptr<soundtouch::SoundTouch> soundTouchProcessor;
	juce::AudioBuffer<float> speedProcessedBuffer;
	std::atomic<bool> needsSpeedUpdate{ false };
	float lastProcessedSpeed = 1.0f;
	int speedBufferReadPos = 0;
	std::vector<float> interleavedTemp;


	enum class PendingAction
	{
		None,
		StartOnNextMeasure,
		StopOnNextMeasure
	};

	PendingAction pendingAction = PendingAction::None;

	DjIaClient::LoopRequest createLoopRequest() const
	{
		DjIaClient::LoopRequest request;
		request.prompt = !selectedPrompt.isEmpty() ? selectedPrompt : generationPrompt;
		request.bpm = generationBpm;
		request.key = generationKey;
		request.generationDuration = static_cast<float>(generationDuration);
		request.preferredStems = preferredStems;
		return request;
	}

	void updateFromRequest(const DjIaClient::LoopRequest& request)
	{
		generationPrompt = request.prompt;
		generationBpm = request.bpm;
		generationKey = request.key;
		generationDuration = static_cast<int>(request.generationDuration);
		preferredStems = request.preferredStems;
	}

	struct SequencerData
	{
		bool steps[4][16] = {};
		float velocities[4][16] = {};
		bool isPlaying = false;
		int currentStep = 0;
		int currentMeasure = 0;
		int numMeasures = 1;
		int beatsPerMeasure = 4;
		double stepAccumulator = 0.0;
		double samplesPerStep = 0.0;
	} sequencerData{};

	TrackData() : trackId(juce::Uuid().toString())
	{
		volume = 0.8f;
		pan = 0.0f;
		readPosition = 0.0;
		bpmOffset = 0.0;
		onPlayStateChanged = nullptr;

		soundTouchProcessor = std::make_unique<soundtouch::SoundTouch>();
		soundTouchProcessor->setChannels(2);
		soundTouchProcessor->setSampleRate(44100);
		soundTouchProcessor->setPitch(1.0);

		soundTouchProcessor->setSetting(SETTING_USE_QUICKSEEK, 1);
		soundTouchProcessor->setSetting(SETTING_USE_AA_FILTER, 1);
		soundTouchProcessor->setSetting(SETTING_SEQUENCE_MS, 40);
		soundTouchProcessor->setSetting(SETTING_SEEKWINDOW_MS, 15);
		soundTouchProcessor->setSetting(SETTING_OVERLAP_MS, 8);

		soundTouchProcessor->setTempo(1.0f);
	}

	~TrackData()
	{
		onPlayStateChanged = nullptr;
		onArmedStateChanged = nullptr;
		onArmedToStopStateChanged = nullptr;
	}

	void reset()
	{
		audioBuffer.setSize(0, 0);
		numSamples = 0;
		readPosition = 0.0;
		isEnabled = true;
		isMuted = false;
		isSolo = false;
		loopPointsLocked = false;
		volume = 0.8f;
		pan = 0.0f;
		bpmOffset = 0.0;
		useOriginalFile = false;
		hasOriginalVersion = false;
		originalStagingBuffer.setSize(0, 0);
		speed = 1.0f;
	}

	void setPlaying(bool playing)
	{
		bool wasPlaying = isPlaying.load();
		isPlaying = playing;
		if (wasPlaying != playing && onPlayStateChanged && audioBuffer.getNumChannels() > 0 && isPlaying.load())
		{
			juce::MessageManager::callAsync([this, playing]()
				{
					if (onPlayStateChanged) {
						onPlayStateChanged(playing);
					} });
		}
	}

	void setArmed(bool armed)
	{
		bool wasArmed = isArmed.load();
		isArmed = armed;
		if (wasArmed != armed && onArmedStateChanged && audioBuffer.getNumChannels() > 0 && isPlaying.load())
		{
			juce::MessageManager::callAsync([this, armed]()
				{
					if (onArmedStateChanged) {
						onArmedStateChanged(armed);
					} });
		}
	}

	void setArmedToStop(bool armedToStop)
	{
		isArmedToStop = armedToStop;
		if (onArmedToStopStateChanged && audioBuffer.getNumChannels() > 0 && isCurrentlyPlaying.load())
		{
			juce::MessageManager::callAsync([this, armedToStop]()
				{
					if (onArmedToStopStateChanged) {
						onArmedToStopStateChanged(armedToStop);
					} });
		}
	}

	void setStop()
	{
		juce::MessageManager::callAsync([this]()
			{
				if (onPlayStateChanged) {
					onPlayStateChanged(false);
				} });
	}

	void processAndFillBufferWithSpeedChange(juce::AudioBuffer<float>& outputBuffer, int currentNumSamples)
	{
		float currentSpeed = speed.load();
		const int numChannels = std::min(2, audioBuffer.getNumChannels());

		if (std::abs(currentSpeed - 1.0f) < 0.001f)
		{
			if (lastProcessedSpeed != 1.0f) {
				soundTouchProcessor->clear();
				speedProcessedBuffer.setSize(numChannels, 0);
				speedBufferReadPos = 0;
			}
			lastProcessedSpeed = 1.0f;

			copyDirectAudio(outputBuffer, currentNumSamples, numChannels);
			return;
		}

		if (std::abs(currentSpeed - lastProcessedSpeed) > 0.001f || needsSpeedUpdate.exchange(false))
		{
			configureSpeedChange(currentSpeed);
		}

		fillOutputWithTimeStretch(outputBuffer, currentNumSamples, numChannels);
	}

	void copyDirectAudio(juce::AudioBuffer<float>& outputBuffer, int currentNumSamples, int numChannels)
	{
		int samplesLeft = currentNumSamples;
		int bufferPos = 0;

		while (samplesLeft > 0)
		{
			int startPos = static_cast<int>(readPosition.load());
			int samplesToEndOfLoop = this->numSamples - startPos;
			int samplesToProcess = std::min(samplesLeft, samplesToEndOfLoop);

			if (samplesToProcess <= 0) {
				readPosition = 0.0;
				continue;
			}

			for (int ch = 0; ch < numChannels; ++ch) {
				outputBuffer.copyFrom(ch, bufferPos, audioBuffer, ch, startPos, samplesToProcess);
			}

			readPosition.store(readPosition.load() + samplesToProcess);
			if (readPosition >= this->numSamples)
				readPosition = 0.0;

			samplesLeft -= samplesToProcess;
			bufferPos += samplesToProcess;
		}
	}

	void fillOutputWithTimeStretch(juce::AudioBuffer<float>& outputBuffer, int currentNumSamples, int numChannels)
	{
		int outputPos = 0;

		while (outputPos < currentNumSamples)
		{
			int samplesAvailable = speedProcessedBuffer.getNumSamples() - speedBufferReadPos;

			if (samplesAvailable == 0)
			{
				produceMoreSamples(numChannels);
				samplesAvailable = speedProcessedBuffer.getNumSamples() - speedBufferReadPos;
			}

			int samplesToCopy = std::min(currentNumSamples - outputPos, samplesAvailable);
			if (samplesToCopy > 0)
			{
				for (int ch = 0; ch < numChannels; ++ch) {
					outputBuffer.copyFrom(ch, outputPos, speedProcessedBuffer, ch, speedBufferReadPos, samplesToCopy);
				}
				outputPos += samplesToCopy;
				speedBufferReadPos += samplesToCopy;
			}

			if (speedBufferReadPos >= speedProcessedBuffer.getNumSamples()) {
				speedProcessedBuffer.setSize(numChannels, 0);
				speedBufferReadPos = 0;
			}

			if (samplesToCopy == 0 && samplesAvailable == 0) {
				break;
			}
		}

		if (outputPos < currentNumSamples) {
			outputBuffer.clear(outputPos, currentNumSamples - outputPos);
		}
	}


	void configureSpeedChange(float currentSpeed)
	{
		if (currentSpeed <= 0.0f || !std::isfinite(currentSpeed)) {
			DBG("ERROR: Invalid speed detected: " << currentSpeed << ", resetting to 1.0");
			currentSpeed = 1.0f;
			speed = 1.0f;
		}

		currentSpeed = juce::jlimit(0.5f, 2.0f, currentSpeed);

		if (std::abs(currentSpeed - lastProcessedSpeed) > 0.1f) {
			soundTouchProcessor->clear();
			speedProcessedBuffer.setSize(speedProcessedBuffer.getNumChannels(), 0);
			speedBufferReadPos = 0;
		}

		int sequenceMs = std::max(20, std::min(80, (int)(40 / std::sqrt(currentSpeed))));
		soundTouchProcessor->setSetting(SETTING_SEQUENCE_MS, sequenceMs);

		if (currentSpeed < 1.0f) {
			soundTouchProcessor->setSetting(SETTING_OVERLAP_MS, 12);
		}
		else {
			soundTouchProcessor->setSetting(SETTING_OVERLAP_MS, 8);
		}

		soundTouchProcessor->setTempo(currentSpeed);

		lastProcessedSpeed = currentSpeed;

	}


	void updateSoundTouchSettings(double newSampleRate)
	{
		if (!soundTouchProcessor) return;

		if (newSampleRate <= 0 || !std::isfinite(newSampleRate)) {
			DBG("ERROR: Invalid sample rate: " << newSampleRate);
			return;
		}

		if (std::abs(sampleRate - newSampleRate) > 0.1)
		{
			soundTouchProcessor->clear();
			soundTouchProcessor->setSampleRate(static_cast<uint>(newSampleRate));
			sampleRate = newSampleRate;

			float currentSpeed = speed.load();
			soundTouchProcessor->setTempo(currentSpeed);
		}
	}

	void collectAllProcessedSamples(int numChannels)
	{
		const int maxReceive = 4096;
		std::vector<float> receivedBuffer(maxReceive * 2);

		int totalReceived = 0;
		int numReceived = 0;
		int maxIterations = 10;
		int iterations = 0;

		float currentSpeed = speed.load();

		do {
			numReceived = soundTouchProcessor->receiveSamples(receivedBuffer.data(), maxReceive);
			if (numReceived > 0)
			{
				int oldSize = speedProcessedBuffer.getNumSamples();

				if (currentSpeed < 1.0f && oldSize > 8192) {
					speedProcessedBuffer.setSize(numChannels, 0);
					oldSize = 0;
					speedBufferReadPos = 0;
				}

				speedProcessedBuffer.setSize(numChannels, oldSize + numReceived, true);

				for (int i = 0; i < numReceived; ++i)
				{
					speedProcessedBuffer.setSample(0, oldSize + i, receivedBuffer[i * 2]);
					if (numChannels > 1) {
						speedProcessedBuffer.setSample(1, oldSize + i, receivedBuffer[i * 2 + 1]);
					}
				}
				totalReceived += numReceived;
			}

			++iterations;
			if (iterations >= maxIterations) {
				break;
			}

		} while (numReceived > 0);
	}

	void debugSoundTouchState() const
	{
		if (!soundTouchProcessor) return;

		auto available = soundTouchProcessor->numSamples();
		auto unprocessed = soundTouchProcessor->numUnprocessedSamples();

		DBG("SoundTouch State - Available: " << available
			<< ", Unprocessed: " << unprocessed
			<< ", Speed: " << speed.load());
	}

	void produceMoreSamples(int numChannels)
	{
		float currentSpeed = speed.load();
		speedBufferReadPos = 0;

		int sourceChunkSize;
		if (currentSpeed > 1.0f)
		{
			sourceChunkSize = std::max(1024, std::min(4096, (int)(1024 * currentSpeed)));
		}
		else
		{
			sourceChunkSize = 2048;
		}
		int numPasses = 1;
		if (currentSpeed < 1.0f)
		{
			numPasses = std::max(1, std::min(4, (int)(1.0f / currentSpeed)));
		}

		for (int pass = 0; pass < numPasses; ++pass)
		{
			int startPos = static_cast<int>(readPosition.load());
			int samplesToEndOfLoop = this->numSamples - startPos;
			int samplesToRead = std::min(sourceChunkSize, samplesToEndOfLoop);

			if (samplesToRead > 0)
			{
				processChunkWithSoundTouch(startPos, samplesToRead, numChannels);
				readPosition.store(readPosition.load() + samplesToRead);
				if (readPosition >= this->numSamples)
					readPosition = 0.0;
			}
			else
			{
				break;
			}
		}

		if (numPasses == 1 && readPosition >= this->numSamples)
		{
			soundTouchProcessor->flush();
			readPosition = 0.0;
		}

		collectAllProcessedSamples(numChannels);
	}

	void processChunkWithSoundTouch(int startPos, int samplesToRead, int numChannels)
	{
		interleavedTemp.resize(samplesToRead * 2);

		for (int i = 0; i < samplesToRead; ++i)
		{
			interleavedTemp[i * 2] = audioBuffer.getSample(0, startPos + i);
			interleavedTemp[i * 2 + 1] = numChannels > 1 ?
				audioBuffer.getSample(1, startPos + i) :
				audioBuffer.getSample(0, startPos + i);
		}

		soundTouchProcessor->putSamples(interleavedTemp.data(), samplesToRead);
	}
};