#pragma once
#include "JuceHeader.h"

class SimpleEQ
{
public:
	SimpleEQ() = default;

	void prepare(double newSampleRate, int /*samplesPerBlock*/)
	{
		sampleRate = newSampleRate;
		for (int ch = 0; ch < 2; ++ch)
		{
			highFilters[ch].setCoefficients(
				juce::IIRCoefficients::makeHighShelf(sampleRate, 8000.0, 0.7, 1.0));
			midFilters[ch].setCoefficients(
				juce::IIRCoefficients::makePeakFilter(sampleRate, 1000.0, 1.0, 1.0));
			lowFilters[ch].setCoefficients(
				juce::IIRCoefficients::makeLowShelf(sampleRate, 200.0, 0.7, 1.0));
		}
	}

	void processBlock(juce::AudioBuffer<float> &buffer)
	{
		if (bypass)
			return;

		const int numChannels = std::min(2, buffer.getNumChannels());
		const int numSamples = buffer.getNumSamples();

		for (int ch = 0; ch < numChannels; ++ch)
		{
			auto *channelData = buffer.getWritePointer(ch);
			lowFilters[ch].processSamples(channelData, numSamples);
			midFilters[ch].processSamples(channelData, numSamples);
			highFilters[ch].processSamples(channelData, numSamples);
		}
	}

	void setHighGain(float gainDb)
	{
		if (std::abs(gainDb - highGain) < 0.1f)
			return;

		highGain = gainDb;
		float linearGain = juce::Decibels::decibelsToGain(gainDb);

		for (int ch = 0; ch < 2; ++ch)
		{
			highFilters[ch].setCoefficients(
				juce::IIRCoefficients::makeHighShelf(sampleRate, 8000.0, 0.7, linearGain));
		}
	}

	void setMidGain(float gainDb)
	{
		if (std::abs(gainDb - midGain) < 0.1f)
			return;

		midGain = gainDb;
		float linearGain = juce::Decibels::decibelsToGain(gainDb);

		for (int ch = 0; ch < 2; ++ch)
		{
			midFilters[ch].setCoefficients(
				juce::IIRCoefficients::makePeakFilter(sampleRate, 1000.0, 1.0, linearGain));
		}
	}

	void setLowGain(float gainDb)
	{
		if (std::abs(gainDb - lowGain) < 0.1f)
			return;

		lowGain = gainDb;
		float linearGain = juce::Decibels::decibelsToGain(gainDb);

		for (int ch = 0; ch < 2; ++ch)
		{
			lowFilters[ch].setCoefficients(
				juce::IIRCoefficients::makeLowShelf(sampleRate, 200.0, 0.7, linearGain));
		}
	}

	float getHighGain() const { return highGain; }
	float getMidGain() const { return midGain; }
	float getLowGain() const { return lowGain; }

	void setBypass(bool shouldBypass) { bypass = shouldBypass; }
	bool isBypassed() const { return bypass; }

	void reset()
	{
		for (int ch = 0; ch < 2; ++ch)
		{
			highFilters[ch].reset();
			midFilters[ch].reset();
			lowFilters[ch].reset();
		}
	}

private:
	double sampleRate = 48000.0;

	float highGain = 0.0f;
	float midGain = 0.0f;
	float lowGain = 0.0f;

	bool bypass = false;

	juce::IIRFilter highFilters[2];
	juce::IIRFilter midFilters[2];
	juce::IIRFilter lowFilters[2];
};