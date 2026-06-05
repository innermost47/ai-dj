#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class Distortion
{
  public:
	Distortion();

	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setPre(float p);
	void setPost(float p);
	void setCut(float c);
	void setType(Obsidian::distortionType type);
	void setBypassed(bool b);

	float getMultiplicator(Obsidian::distortionType type);

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

	Obsidian::distortionType getType() const
	{
		return distortionType;
	}

  private:
	double sampleRate = Obsidian::SAMPLERATE;

	float pre = Obsidian::DISTORTION_PRE;
	float post = Obsidian::DISTORTION_POST;
	float cut = Obsidian::DISTORTION_CUT;

	bool bypassed = Obsidian::DISTORTION_BYPASSED;

	Obsidian::distortionType distortionType = Obsidian::distortionType::soft;

	juce::dsp::ProcessorChain<juce::dsp::StateVariableTPTFilter<float>, juce::dsp::Gain<float>,
	                          juce::dsp::WaveShaper<float>, juce::dsp::Gain<float>>
	    processorChain;

	void updatePre();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Distortion)
};