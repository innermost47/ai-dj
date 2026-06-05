#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class MonoEQ
{
  public:
	MonoEQ();

	void updateCoefficients(Obsidian::filterType type, float fq, float q, float g);
	void updateFrequency(Obsidian::filterType type, float fq);
	void updateQ(Obsidian::filterType type, float q);
	void updateGain(Obsidian::filterType type, float g);
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setSampleRate(double sr);
	void init(float fr);
	float getGain() const
	{
		return gain;
	}
	float getFrequency() const
	{
		return frequency;
	}
	float getQ() const
	{
		return resonance;
	}

  private:
	double sampleRate = Obsidian::SAMPLERATE;

	float frequency = Obsidian::EQ_MID_FRQ;
	float resonance = Obsidian::EQ_BASE_RESONANCE;
	float gain = Obsidian::EQ_BANDS_GAIN;

	juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> stereoFilter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonoEQ)
};