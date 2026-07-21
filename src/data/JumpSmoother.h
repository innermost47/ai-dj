#pragma once
#include <JuceHeader.h>

struct JumpSmoother
{
	static constexpr int kMaxLength = 96;

	double position = 0.0;
	double step = 0.0;
	int samplesRemaining = 0;
	int length = kMaxLength;

	void trigger(double oldAbsolutePosition, double signedStep, int fadeLength = kMaxLength)
	{
		position = oldAbsolutePosition;
		step = signedStep;
		length = juce::jmax(2, fadeLength);
		samplesRemaining = length;
	}

	bool isActive() const noexcept
	{
		return samplesRemaining > 0;
	}
	void reset() noexcept
	{
		samplesRemaining = 0;
	}
};