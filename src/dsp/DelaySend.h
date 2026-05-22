#pragma once
#include "DataConst.h"
#include <JuceHeader.h>

class DelaySend
{
  public:
	enum class Mode
	{
		Stereo = 0,
		PingPong,
		Mono
	};

	enum class TimeDivision
	{
		Sixteenth = 0,
		EighthDotted,
		Eighth,
		QuarterDotted,
		Quarter,
		Half,
		Bar,
		TwoBars
	};

	DelaySend();

	void prepare(double sampleRate, int maxBlockSize);
	void reset();

	void setBpm(double bpm)
	{
		currentBpm = bpm;
		updateDelayTime();
	}
	void setTimeDivision(TimeDivision div)
	{
		timeDivision = div;
		updateDelayTime();
	}
	void setFeedback(float fb01)
	{
		feedback = juce::jlimit(0.0f, 0.95f, fb01);
	}
	void setMode(Mode m)
	{
		mode = m;
	}

	void process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples);

	void process(juce::AudioBuffer<float> &buffer)
	{
		process(buffer, 0, buffer.getNumSamples());
	}

	static juce::String getDivisionName(TimeDivision div);

  private:
	void updateDelayTime();
	void updateFilterCoefficients();

	double currentSampleRate = Obsidian::SAMPLERATE;
	double currentBpm = 120.0;
	TimeDivision timeDivision = TimeDivision::Quarter;
	float feedback = 0.4f;
	Mode mode = Mode::Stereo;

	juce::dsp::IIR::Filter<float> highPassL, highPassR;
	juce::dsp::IIR::Filter<float> lowPassL, lowPassR;

	float highPassFreq = 150.0f;
	float lowPassFreq = 6000.0f;

	juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL;
	juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineR;

	float currentDelaySamples = 22050.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelaySend)
};