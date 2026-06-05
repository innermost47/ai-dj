#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Filter
{
  public:
	Filter();

	void setCutoffFrequency(float frequency);
	void setResonance(float q);
	void setSamplingRate(double sr);
	void setDrive(float newDrive);
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset();
	void setMode(juce::dsp::LadderFilterMode newMode);
	void setBypassed(bool b);

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

	bool isBypassed() const
	{
		return bypassed;
	}

	juce::dsp::LadderFilterMode getMode() const
	{
		return mode;
	}

  private:
	float cutoffFrequency = Obsidian::FILTER_CUT;
	float resonance = Obsidian::FILTER_RES;
	float drive = Obsidian::FILTER_DRIVE;

	bool bypassed = Obsidian::FILTER_BYPASSED;

	double samplingRate = Obsidian::SAMPLERATE;

	juce::dsp::LadderFilterMode mode = juce::dsp::LadderFilterMode::HPF12;

	juce::dsp::LadderFilter<float> filter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
};