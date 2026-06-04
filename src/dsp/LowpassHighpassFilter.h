#pragma once
#include "JuceHeader.h"
#include <vector>

class LowpassHighpassFilter
{
  public:
	void setCutoffFrequency(float frequency);
	void setResonance(float q);
	void setSamplingRate(double sr);
	void setDrive(float newDrive);
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset();
	void setMode(juce::dsp::LadderFilterMode newMode);

	float getCutoff() const
	{
		return cutoffFrequency;
	}
	float getResonance() const
	{
		return resonance;
	}
	float getDrive() const
	{
		return drive;
	}
	juce::dsp::LadderFilterMode getMode() const
	{
		return mode;
	}

  private:
	float cutoffFrequency;
	float resonance;
	float drive;

	double samplingRate;

	juce::dsp::LadderFilterMode mode;

	juce::dsp::LadderFilter<float> filter;
};