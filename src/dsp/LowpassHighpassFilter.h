#pragma once
#include "JuceHeader.h"
#include <vector>

class LowpassHighpassFilter
{
  public:
	void setHighpass(bool value);
	void setCutoffFrequency(float frequency);
	void setResonance(float q);
	void setSamplingRate(double sr);
	void processBlock(juce::AudioBuffer<float> &);
	void prepare(int numChannels, juce::uint32 maxBlockSize);
	void reset();
	float softClip(float x) noexcept;

	float getCutoff() const
	{
		return cutoffFrequency;
	}
	float getResonance() const
	{
		return resonance;
	}
	bool isHighpass() const
	{
		return highpass;
	}

  private:
	bool highpass;
	float cutoffFrequency;
	float resonance;
	double samplingRate;

	juce::dsp::LadderFilter<float> filter;
};