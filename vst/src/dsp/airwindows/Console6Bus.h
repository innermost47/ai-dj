#pragma once
#include <JuceHeader.h>
#include <cstdint>

/**
 * Airwindows Console6Buss — Buss-side console emulation
 * Original code by Chris Johnson (airwindows), MIT License
 * Inverse Square decode courtesy of torridgristle (MIT)
 *
 * Apply this on the master mix AFTER summing all Console6Channel-treated tracks.
 * The Channel encodes, the Buss decodes — together they create the analog console glue.
 */
class Console6Buss
{
  public:
	Console6Buss();

	void prepare(double sampleRate);
	void reset();

	void process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

	void process(juce::AudioBuffer<float> &buffer)
	{
		process(buffer, 0, buffer.getNumSamples());
	}

	void setGain(float gainValue)
	{
		gain = gainValue;
	}

  private:
	double currentSampleRate = 48000.0;

	uint32_t fpdL = 1;
	uint32_t fpdR = 1;

	float gain = 1.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Console6Buss)
};