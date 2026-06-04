#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Limiter
{
  public:
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setThreshold(float t);
	void setRelease(float r);
	void setMakeUpGain(float mk);
	void resetReductionAmount();

	float getThreshold() const
	{
		return threshold;
	}

	float getRelease() const
	{
		return release;
	}

	float getMakeUpGain() const
	{
		return makeUpGain;
	}

	float getReductionAmount() const
	{
		return juce::jlimit(0.f, 1.f, reductionAmount);
	}

  private:
	float threshold;
	float release;
	float makeUpGain;
	float reductionAmount = 0.f;

	juce::dsp::Limiter<float> lim;
};