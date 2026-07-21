#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Flanger
{
  public:
	Flanger();

	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setRate(float r);
	void setDepth(float d);
	void setCentre(float c);
	void setFeedback(float f);
	void setMix(float m);
	void setBypassed(bool b);

	float getRate() const
	{
		return rate;
	}

	float getDepth() const
	{
		return depth;
	}

	float getCentre() const
	{
		return centre;
	}

	float getFeedback() const
	{
		return feedback;
	}

	float getMix() const
	{
		return mix;
	}

	bool isBypassed() const
	{
		return bypassed;
	}

  private:
	float rate = Obsidian::FLANGER_RATE;
	float depth = Obsidian::FLANGER_DEPTH;
	float centre = Obsidian::FLANGER_CENTRE;
	float feedback = Obsidian::FLANGER_FEEDBACK;
	float mix = Obsidian::FLANGER_MIX;

	bool bypassed = Obsidian::FLANGER_BYPASSED;

	juce::dsp::ProcessorChain<juce::dsp::Chorus<float>> processorChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Flanger)
};