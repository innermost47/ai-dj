#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Distorsion
{
  public:
	Distorsion();

	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setPre(float p);
	void setPost(float p);
	void setCut(float c);
	void setType(Obsidian::distorsionType type);
	void setBypassed(bool b);

	float getMultiplicator(Obsidian::distorsionType type);

	float getPre() const
	{
		return pre;
	}
	float getPost() const
	{
		return post;
	}
	float getCut() const
	{
		return cut;
	}

	bool isBypassed() const
	{
		return bypassed;
	}

	Obsidian::distorsionType getType() const
	{
		return distorsionType;
	}

  private:
	double sampleRate = Obsidian::SAMPLERATE;

	float pre = 1.f;
	float post = 1.f;
	float cut = 1000.f;

	bool bypassed = false;

	Obsidian::distorsionType distorsionType = Obsidian::distorsionType::soft;

	juce::dsp::ProcessorChain<juce::dsp::StateVariableTPTFilter<float>, juce::dsp::Gain<float>,
	                          juce::dsp::WaveShaper<float>, juce::dsp::Gain<float>>
	    processorChain;

	void updatePre();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Distorsion)
};