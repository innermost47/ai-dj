#include "Gate.h"
#include <cmath>

void Gate::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
	sampleRate = newSampleRate;
	gainSmoother.reset(sampleRate, 0.005);
	gainSmoother.setCurrentAndTargetValue(1.0f);
}

void Gate::reset()
{
	gainSmoother.setCurrentAndTargetValue(1.0f);
	wasOpen = true;
}

void Gate::setRate(GateRateDivision division)
{
	rateDivision = division;
}

void Gate::setBypassed(bool b)
{
	bypassed = b;
}

void Gate::setDuration(float pct01)
{
	duration = juce::jlimit(0.01f, 1.0f, pct01);
}

void Gate::setDepth(float depth01)
{
	depth = juce::jlimit(0.0f, 1.0f, depth01);
}

void Gate::setSmoothingMs(float newAttackMs, float newReleaseMs)
{
	attackMs = newAttackMs;
	releaseMs = newReleaseMs;
}

double Gate::cycleLengthInBeats(int numerator, int denominator) const
{
	if (numerator <= 0)
		numerator = 4;
	if (denominator <= 0)
		denominator = 4;

	double barLengthInQuarterNotes = numerator * (4.0 / denominator);

	switch (rateDivision)
	{
	case GateRateDivision::Sixteenth:
		return 0.25;
	case GateRateDivision::Eighth:
		return 0.5;
	case GateRateDivision::EighthDotted:
		return 0.75;
	case GateRateDivision::Quarter:
		return 1.0;
	case GateRateDivision::QuarterDotted:
		return 1.5;
	case GateRateDivision::Half:
		return 2.0;
	case GateRateDivision::OneBar:
		return barLengthInQuarterNotes;
	case GateRateDivision::TwoBars:
		return barLengthInQuarterNotes * 2.0;
	default:
		return 0.25;
	}
}

void Gate::processBlock(juce::AudioBuffer<float> &buffer, double bpm, double ppqPosition, int numerator,
                        int denominator)
{
	if (bypassed || bpm <= 0.0)
		return;

	const int numSamples = buffer.getNumSamples();
	const int numChannels = buffer.getNumChannels();

	double beatsPerSecond = bpm / 60.0;
	double cycleBeats = cycleLengthInBeats(numerator, denominator);

	for (int sample = 0; sample < numSamples; ++sample)
	{
		double currentPpq = ppqPosition + (sample / sampleRate) * beatsPerSecond;
		double phase = std::fmod(currentPpq, cycleBeats) / cycleBeats;
		if (phase < 0.0)
			phase += 1.0;

		bool shouldBeOpen = phase < duration;

		if (shouldBeOpen != wasOpen)
		{
			float targetGain = shouldBeOpen ? 1.0f : depth;
			float rampTimeMs = shouldBeOpen ? attackMs : releaseMs;
			gainSmoother.reset(sampleRate, rampTimeMs / 1000.0);
			gainSmoother.setTargetValue(targetGain);
			wasOpen = shouldBeOpen;
		}

		float gain = gainSmoother.getNextValue();

		for (int ch = 0; ch < numChannels; ++ch)
			buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
	}
}