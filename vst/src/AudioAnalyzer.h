#pragma once
#include "JuceHeader.h"
#include "SoundTouch.h"
#include "BPMDetect.h"

class AudioAnalyzer
{
public:
	static float detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate)
	{
		if (buffer.getNumSamples() == 0)
			return 0.0f;

		try
		{
			soundtouch::BPMDetect bpmDetect(1, (int)sampleRate);

			int maxSamplesToAnalyze = (int)(sampleRate * 10.0);
			int samplesToAnalyze = std::min(buffer.getNumSamples(), maxSamplesToAnalyze);

			std::vector<float> monoData;
			monoData.reserve(samplesToAnalyze);

			bool retFlag;
			float retVal = normalizeAudio(buffer, monoData, retFlag, samplesToAnalyze);
			if (retFlag)
				return retVal;

			chunkAnalysis(monoData, bpmDetect);

			float detectedBPM = bpmDetect.getBpm();

			if (detectedBPM >= 30.0f && detectedBPM <= 300.0f)
			{
				DBG("BPM detected: " << detectedBPM);
				return detectedBPM;
			}

			DBG("BPM detection failed, returning 0");
			return 0.0f;
		}
		catch (const std::exception& e)
		{
			DBG("BPM detection error: " << e.what());
			return 0.0f;
		}
	}
	static float returnDetectedBPMorFallback(float detectedBPM,
		const juce::AudioSampleBuffer& /*buffer*/,
		double /*sampleRate*/)
	{
		if (detectedBPM >= 30.0f && detectedBPM <= 300.0f)
		{
			return detectedBPM;
		}
		return 0.0f;
	}

	static void chunkAnalysis(std::vector<float>& monoData, soundtouch::BPMDetect& bpmDetect)
	{
		const int chunkSize = 4096;

		for (size_t i = 0; i < monoData.size(); i += chunkSize)
		{
			size_t remaining = std::min((size_t)chunkSize, monoData.size() - i);
			bpmDetect.inputSamples(&monoData[i], (int)remaining);
		}
	}

	static float normalizeAudio(const juce::AudioSampleBuffer& buffer,
		std::vector<float>& monoData,
		bool& retFlag,
		int maxSamples = -1)
	{
		retFlag = true;

		int samplesToProcess = (maxSamples > 0) ? std::min(maxSamples, buffer.getNumSamples())
			: buffer.getNumSamples();

		float maxLevel = 0.0f;
		for (int i = 0; i < samplesToProcess; ++i)
		{
			float mono = buffer.getSample(0, i);
			if (buffer.getNumChannels() > 1)
			{
				mono = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
			}

			maxLevel = std::max(maxLevel, std::abs(mono));
			monoData.push_back(mono);
		}

		if (maxLevel < 0.001f)
		{
			return 0.0f;
		}

		float normalizeGain = 0.5f / maxLevel;
		for (auto& sample : monoData)
		{
			sample *= normalizeGain;
		}

		retFlag = false;
		return 0.0f;
	}

	static void timeStretchBufferFast(juce::AudioBuffer<float>& buffer,
		double ratio,
		double sampleRate)
	{
		timeStretchBuffer(buffer, ratio, sampleRate, false);
	}

	static void timeStretchBufferHQ(juce::AudioBuffer<float>& buffer,
		double ratio,
		double sampleRate)
	{
		timeStretchBuffer(buffer, ratio, sampleRate, true);
	}

	static void timeStretchBuffer(juce::AudioBuffer<float>& buffer,
		double ratio,
		double sampleRate,
		bool highQuality = false)
	{
		if (std::abs(ratio - 1.0) < 0.001 || buffer.getNumSamples() == 0)
			return;

		try
		{
			soundtouch::SoundTouch soundTouch;
			soundTouch.setSampleRate((int)sampleRate);
			soundTouch.setChannels(buffer.getNumChannels());

			if (highQuality)
			{
				soundTouch.setSetting(SETTING_USE_QUICKSEEK, 0);
				soundTouch.setSetting(SETTING_USE_AA_FILTER, 1);
				soundTouch.setSetting(SETTING_AA_FILTER_LENGTH, 64);
				soundTouch.setSetting(SETTING_SEQUENCE_MS, 82);
				soundTouch.setSetting(SETTING_SEEKWINDOW_MS, 28);
				soundTouch.setSetting(SETTING_OVERLAP_MS, 12);
			}
			else
			{
				soundTouch.setSetting(SETTING_USE_QUICKSEEK, 1);
				soundTouch.setSetting(SETTING_USE_AA_FILTER, 1);
				soundTouch.setSetting(SETTING_AA_FILTER_LENGTH, 32);
				soundTouch.setSetting(SETTING_SEQUENCE_MS, 40);
				soundTouch.setSetting(SETTING_SEEKWINDOW_MS, 15);
				soundTouch.setSetting(SETTING_OVERLAP_MS, 8);
			}

			double clampedRatio = juce::jlimit(0.5, 2.0, ratio);
			if (clampedRatio != ratio)
			{
				DBG("Warning: Ratio clamped from " << ratio << " to " << clampedRatio);
			}

			soundTouch.setTempoChange((clampedRatio - 1.0) * 100.0);

			if (buffer.getNumChannels() == 1)
			{
				soundTouch.putSamples(buffer.getReadPointer(0), buffer.getNumSamples());
			}
			else
			{
				std::vector<float> interleavedInput;
				interleavedInput.reserve(buffer.getNumSamples() * 2);

				for (int i = 0; i < buffer.getNumSamples(); ++i)
				{
					interleavedInput.push_back(buffer.getSample(0, i));
					interleavedInput.push_back(buffer.getSample(1, i));
				}

				soundTouch.putSamples(interleavedInput.data(), buffer.getNumSamples());
			}

			soundTouch.flush();

			int outputSamples = soundTouch.numSamples();
			if (outputSamples > 0)
			{
				buffer.setSize(buffer.getNumChannels(), outputSamples, false, false, true);

				if (buffer.getNumChannels() == 1)
				{
					soundTouch.receiveSamples(buffer.getWritePointer(0), outputSamples);
				}
				else
				{
					std::vector<float> interleavedOutput(outputSamples * 2);
					soundTouch.receiveSamples(interleavedOutput.data(), outputSamples);

					for (int i = 0; i < outputSamples; ++i)
					{
						buffer.setSample(0, i, interleavedOutput[i * 2]);
						buffer.setSample(1, i, interleavedOutput[i * 2 + 1]);
					}
				}

				DBG("Time stretch completed: " << clampedRatio << "x ("
					<< buffer.getNumSamples() << " -> " << outputSamples << " samples)");
			}
		}
		catch (const std::exception& e)
		{
			DBG("Time stretch error: " << e.what());
		}
	}
};