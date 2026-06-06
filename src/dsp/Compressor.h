#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Compressor
{
  public:
	Compressor();

	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setThreshold(float t);
	void setRatio(float r);
	void setAttack(float a);
	void setRelease(float r);
	void setMakeUpGain(float mk);
	void setBypassed(bool b);

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

	bool isBypassed() const
	{
		return bypassed;
	}

  private:
	double sampleRate = Obsidian::SAMPLERATE;

	float threshold = Obsidian::COMPRESSOR_THRESHOLD;
	float attack = Obsidian::COMPRESSOR_ATTACK;
	float release = Obsidian::COMPRESSOR_RELEASE;
	float ratio = Obsidian::COMPRESSOR_RATIO;
	float makeUpGain = Obsidian::COMPRESSOR_MAKEUP_GAIN;

	bool bypassed = Obsidian::COMPRESSOR_BYPASSED;

	juce::dsp::ProcessorChain<juce::dsp::Compressor<float>, juce::dsp::Gain<float>> processorChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compressor)
};