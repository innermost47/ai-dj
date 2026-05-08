#include "DelaySend.h"

DelaySend::DelaySend()
{
}

void DelaySend::prepare(double sampleRate, int maxBlockSize)
{
	currentSampleRate = sampleRate;

	const int maxDelaySamples = (int)(sampleRate * 10.0);

	juce::dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = (juce::uint32)maxBlockSize;
	spec.numChannels = 1;

	delayLineL.prepare(spec);
	delayLineR.prepare(spec);
	delayLineL.setMaximumDelayInSamples(maxDelaySamples);
	delayLineR.setMaximumDelayInSamples(maxDelaySamples);

	reset();
	updateDelayTime();
}

void DelaySend::reset()
{
	delayLineL.reset();
	delayLineR.reset();
}

void DelaySend::updateDelayTime()
{
	if (currentBpm <= 0.0)
		return;

	double quarterSec = 60.0 / currentBpm;

	double divisionMultiplier = 1.0;
	switch (timeDivision)
	{
	case TimeDivision::Sixteenth:
		divisionMultiplier = 0.25;
		break;
	case TimeDivision::EighthDotted:
		divisionMultiplier = 0.75;
		break;
	case TimeDivision::Eighth:
		divisionMultiplier = 0.5;
		break;
	case TimeDivision::QuarterDotted:
		divisionMultiplier = 1.5;
		break;
	case TimeDivision::Quarter:
		divisionMultiplier = 1.0;
		break;
	case TimeDivision::Half:
		divisionMultiplier = 2.0;
		break;
	case TimeDivision::Bar:
		divisionMultiplier = 4.0;
		break;
	case TimeDivision::TwoBars:
		divisionMultiplier = 8.0;
		break;
	}

	double delaySec = quarterSec * divisionMultiplier;
	currentDelaySamples = (float)(delaySec * currentSampleRate);

	currentDelaySamples = juce::jlimit(1.0f, (float)delayLineL.getMaximumDelayInSamples() - 1, currentDelaySamples);
}

void DelaySend::process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
	if (buffer.getNumChannels() < 2 || numSamples <= 0)
		return;

	float *chL = buffer.getWritePointer(0, startSample);
	float *chR = buffer.getWritePointer(1, startSample);

	delayLineL.setDelay(currentDelaySamples);
	delayLineR.setDelay(currentDelaySamples);

	for (int i = 0; i < numSamples; ++i)
	{
		float inputL = chL[i];
		float inputR = chR[i];

		float delayedL = delayLineL.popSample(0);
		float delayedR = delayLineR.popSample(0);

		switch (mode)
		{
		case Mode::Stereo:
			delayLineL.pushSample(0, inputL + delayedL * feedback);
			delayLineR.pushSample(0, inputR + delayedR * feedback);
			break;

		case Mode::PingPong:
			delayLineL.pushSample(0, inputL + delayedR * feedback);
			delayLineR.pushSample(0, inputR + delayedL * feedback);
			break;

		case Mode::Mono:
		{
			float mono = (inputL + inputR) * 0.5f;
			float delayedMono = (delayedL + delayedR) * 0.5f;
			delayLineL.pushSample(0, mono + delayedMono * feedback);
			delayLineR.pushSample(0, mono + delayedMono * feedback);

			delayedL = delayedMono;
			delayedR = delayedMono;
			break;
		}
		}

		chL[i] = delayedL;
		chR[i] = delayedR;
	}
}

juce::String DelaySend::getDivisionName(TimeDivision div)
{
	switch (div)
	{
	case TimeDivision::Sixteenth:
		return "1/16";
	case TimeDivision::EighthDotted:
		return "1/8.";
	case TimeDivision::Eighth:
		return "1/8";
	case TimeDivision::QuarterDotted:
		return "1/4.";
	case TimeDivision::Quarter:
		return "1/4";
	case TimeDivision::Half:
		return "1/2";
	case TimeDivision::Bar:
		return "1 bar";
	case TimeDivision::TwoBars:
		return "2 bars";
	}
	return "?";
}