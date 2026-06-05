#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Limiter
{
  public:
	Limiter();

	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setThreshold(float t);
	void setRelease(float r);
	void setMakeUpGain(float mk);
	void resetReductionAmount();
	void setBypassed(bool b);

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

	bool isBypassed() const
	{
		return bypassed;
	}

  private:
	float threshold = Obsidian::LIMITER_THRESHOLD;
	float release = Obsidian::LIMITER_RELEASE;
	float makeUpGain = Obsidian::LIMITER_MAKEUP_GAIN;
	float reductionAmount = 0.f;

	bool bypassed = Obsidian::LIMITER_BYPASSED;

	juce::dsp::ProcessorChain<juce::dsp::Limiter<float>, juce::dsp::Gain<float>> processorChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Limiter)
};