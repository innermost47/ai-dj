#pragma once
#include "DataConst.h"
#include <JuceHeader.h>
#include <cstdint>

/**
 * Airwindows Channel6 — Channel-side console emulation
 * Original code by Chris Johnson (airwindows), MIT License
 * JUCE wrapper for OBSIDIAN Neural
 *
 * Apply this on each track BEFORE summing into the master bus.
 * For full console emulation, pair with Console6Buss on the master.
 */
class Console6Channel
{
  public:
	enum class ConsoleType
	{
		Neve = 0,
		API,
		SSL
	};

	Console6Channel();

	void prepare(double sampleRate);
	void reset();

	void process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

	void process(juce::AudioBuffer<float> &buffer)
	{
		process(buffer, 0, buffer.getNumSamples());
	}

	void setConsoleType(ConsoleType type);
	void setDrive(float drive01)
	{
		drive = juce::jlimit(0.0f, 1.0f, drive01);
	}
	void setOutput(float output01)
	{
		output = juce::jlimit(0.0f, 1.0f, output01);
	}

  private:
	double currentSampleRate = ObsidianDataConst::SAMPLERATE;

	uint32_t fpdL = 1;
	uint32_t fpdR = 1;
	double iirSampleLA = 0.0;
	double iirSampleRA = 0.0;
	double iirSampleLB = 0.0;
	double iirSampleRB = 0.0;
	double lastSampleL = 0.0;
	double lastSampleR = 0.0;
	double iirAmount = 0.005832;
	double threshold = 0.33362176;
	bool flip = false;

	float drive = 0.0f;
	float output = 1.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Console6Channel)
};