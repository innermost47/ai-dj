#pragma once
#include "DataConst.h"
#include "JuceHeader.h"

enum class GateRateDivision
{
	Sixteenth,
	Eighth,
	EighthDotted,
	Quarter,
	QuarterDotted,
	Half,
	OneBar,
	TwoBars
};

class Gate
{
  public:
	void prepare(double sampleRate, int samplesPerBlock);
	void reset();

	void setRate(GateRateDivision division);
	void setDuration(float pct01);
	void setDepth(float depth01);
	void setSmoothingMs(float attackMs, float releaseMs);

	void processBlock(juce::AudioBuffer<float> &buffer, double bpm, double ppqPosition, int numerator, int denominator);

	void setBypassed(bool b);

	bool isBypassed() const
	{
		return bypassed;
	}

	GateRateDivision getRate() const
	{
		return rateDivision;
	}
	float getDuration() const
	{
		return duration;
	}
	float getDepth() const
	{
		return depth;
	}

  private:
	double cycleLengthInBeats(int numerator, int denominator) const;

	double sampleRate = Obsidian::SAMPLERATE;
	GateRateDivision rateDivision = GateRateDivision::Sixteenth;
	float duration = Obsidian::GATE_DURATION;
	float depth = Obsidian::GATE_DEPTH;
	bool bypassed = Obsidian::GATE_BYPASSED;

	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother;
	float attackMs = Obsidian::GATE_ATTACK;
	float releaseMs = Obsidian::GATE_RELEASE;
	bool wasOpen = true;
};