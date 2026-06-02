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