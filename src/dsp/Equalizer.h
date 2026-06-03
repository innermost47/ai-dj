#pragma once
#include "DataConst.h"
#include "JuceHeader.h"
#include "MonoEQ.h"
#include <vector>

class Equalizer
{
  public:
	void prepare(const juce::dsp::ProcessSpec &spec);
	void reset() noexcept;
	void process(juce::dsp::ProcessContextReplacing<float> &context);
	void update(Obsidian::eqBands band, float frequency, float q, float gain);

  private:
	float sampleRate;
	juce::dsp::ProcessorChain<MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ> bandsChain;
};