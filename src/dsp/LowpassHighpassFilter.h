#pragma once
#include "JuceHeader.h"
#include <vector>

class LowpassHighpassFilter
{
  public:
	void setHighpass(bool value);
	void setCutoffFrequency(float frequency);
	void setResonance(float q);
	void setSamplingRate(double sr);
	void processBlock(juce::AudioBuffer<float> &);
	void prepare(int numChannels);
	void reset();
	float softClip(float x) noexcept;

	float getCutoff() const
	{
		return cutoffFrequency;
	}
	float getResonance() const
	{
		return resonance;
	}
	bool isHighpass() const
	{
		return highpass;
	}

	struct BiquadCoeffs
	{
		float b0, b1, b2, a0, a1, a2;
	};

	BiquadCoeffs computeCoeffs(float cutoff, float resonance, bool highpass, float sampleRate) const;

  private:
	bool highpass;
	float cutoffFrequency;
	float resonance;
	double samplingRate;

	struct FilterState
	{
		float x1 = 0.f;
		float x2 = 0.f;
		float y1 = 0.f;
		float y2 = 0.f;
	};
	std::vector<float> dnBuffer;
	std::vector<FilterState> channelStates;
};