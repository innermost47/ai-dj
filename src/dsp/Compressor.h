#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Compressor
{
  public:
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setThreshold(float t);
	void setRatio(float r);
	void setAttack(float a);
	void setRelease(float r);
	void setMakeUpGain(float mk);

	float getThreshold() const
	{
		return threshold;
	}

	float getRatio() const
	{
		return ratio;
	}

	float getAttack() const
	{
		return attack;
	}

	float getRelease() const
	{
		return release;
	}

	float getMakeUpGain() const
	{
		return makeUpGain;
	}

  private:
	double sampleRate;
	float threshold;
	float attack;
	float release;
	float ratio;
	float makeUpGain;

	juce::dsp::Compressor<float> comp;
};