#pragma once
#include "JuceHeader.h"

class SimpleEQ
{
public:
	SimpleEQ() = default;

	void prepare(double newSampleRate, int /*samplesPerBlock*/);
	void processBlock(juce::AudioBuffer<float>& buffer);
	void setHighGain(float gainDb);
	void setMidGain(float gainDb);
	void setLowGain(float gainDb);
	void reset();

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