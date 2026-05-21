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

	highPassL.prepare(spec);
	highPassR.prepare(spec);
	lowPassL.prepare(spec);
	lowPassR.prepare(spec);

	updateFilterCoefficients();

	reset();
	updateDelayTime();
}

void DelaySend::reset()
{
	delayLineL.reset();
	delayLineR.reset();
	highPassL.reset();
	highPassR.reset();
	lowPassL.reset();
	lowPassR.reset();
}

void DelaySend::updateFilterCoefficients()
{
	auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, highPassFreq);
	*highPassL.coefficients = *hpCoeffs;
	*highPassR.coefficients = *hpCoeffs;

	auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, lowPassFreq);
	*lowPassL.coefficients = *lpCoeffs;
	*lowPassR.coefficients = *lpCoeffs;
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

		float filteredFeedbackL = lowPassL.processSample(highPassL.processSample(delayedL));
		float filteredFeedbackR = lowPassR.processSample(highPassR.processSample(delayedR));

		switch (mode)
		{
		case Mode::Stereo:
			delayLineL.pushSample(0, inputL + filteredFeedbackL * feedback);
			delayLineR.pushSample(0, inputR + filteredFeedbackR * feedback);
			break;

		case Mode::PingPong:
			delayLineL.pushSample(0, inputL + filteredFeedbackR * feedback);
			delayLineR.pushSample(0, inputR + filteredFeedbackL * feedback);
			break;

		case Mode::Mono:
		{
			float mono = (inputL + inputR) * 0.5f;
			float filteredMono = (filteredFeedbackL + filteredFeedbackR) * 0.5f;
			delayLineL.pushSample(0, mono + filteredMono * feedback);
			delayLineR.pushSample(0, mono + filteredMono * feedback);

			delayedL = filteredMono;
			delayedR = filteredMono;
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