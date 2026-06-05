#pragma once
#include "DataConst.h"
#include <JuceHeader.h>

class ReverbSend
{
  public:
	ReverbSend();

	void prepare(double sampleRate, int maxBlockSize);
	void reset();

	void process(juce::AudioBuffer<float> &buffer, int startSample = 0, int numSamples = -1);

	void setSize(float newSize)
	{
		size = juce::jlimit(0.0f, 1.0f, newSize);
		updateParameters();
	}
	void setDamping(float newDamping)
	{
		damping = juce::jlimit(0.0f, 1.0f, newDamping);
		updateParameters();
	}
	void setWidth(float newWidth)
	{
		width = juce::jlimit(0.0f, 1.0f, newWidth);
		updateParameters();
	}
	void setMix(float newMix)
	{
		mix = juce::jlimit(0.0f, 1.0f, newMix);
	}

  private:
	void updateParameters();

	juce::Reverb reverbL;
	juce::Reverb reverbR;
	double currentSampleRate = Obsidian::SAMPLERATE;

	float size = 0.5f;
	float damping = 0.5f;
	float width = 1.0f;
	float mix = 0.5f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbSend)
};