#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include <vector>

class MonoEQ
{
  public:
	void updateCoefficients(Obsidian::filterType type, float fq, float q, float g);
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void setSampleRate(double sr);
	void init(float fr);

  private:
	double sampleRate;
	float frequency;
	float resonance;
	float gain;

	juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> stereoFilter;
};