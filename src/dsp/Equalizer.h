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
	void updateFrequency(Obsidian::eqBands band, float value);
	void updateQ(Obsidian::eqBands band, float value);
	void updateGain(Obsidian::eqBands band, float value);
	float getGain(Obsidian::eqBands band) const;
	float getFrequency(Obsidian::eqBands band) const;
	float getQ(Obsidian::eqBands band) const;

  private:
	float sampleRate;
	juce::dsp::ProcessorChain<MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ, MonoEQ> bandsChain;
};