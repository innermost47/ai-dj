#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class BitCrusher
{
  public:
	BitCrusher();
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setBitDepth(float b);
	void setSampleRateReduction(float r);
	void setMix(float m);
	void setBypassed(bool b);
	float getBitDepth() const
	{
		return bitDepth;
	}
	float getSampleRateReduction() const
	{
		return sampleRateReduction;
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
	float bitDepth = Obsidian::BITCRUSHER_BIT_DEPTH;
	float sampleRateReduction = Obsidian::BITCRUSHER_SAMPLE_RATE_REDUCTION;
	float mix = Obsidian::BITCRUSHER_MIX;
	bool bypassed = Obsidian::BITCRUSHER_BYPASSED;

	double sampleRate = 44100.0;
	float phase = 0.0f;
	std::vector<float> holdBuffer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BitCrusher)
};