#pragma once
#include "DataConst.h"
#include <JuceHeader.h>

class SimpleEQ
{
  public:
	SimpleEQ();

	void prepare(double newSampleRate, int /*samplesPerBlock*/);
	void processBlock(juce::AudioBuffer<float> &buffer);
	void setHighGain(float gainDb);
	void setMidGain(float gainDb);
	void setLowGain(float gainDb);
	void reset();

  private:
	double sampleRate = Obsidian::SAMPLERATE;

	float highGain = 0.0f;
	float midGain = 0.0f;
	float lowGain = 0.0f;

	bool bypass = false;

	juce::IIRFilter highFilters[2];
	juce::IIRFilter midFilters[2];
	juce::IIRFilter lowFilters[2];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQ)
};