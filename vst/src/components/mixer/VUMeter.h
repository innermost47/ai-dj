#pragma once
#include <JuceHeader.h>
#include "style/ColourPalette.h"

class VuMeter : public juce::Component
{
public:
	VuMeter() = default;
	~VuMeter() override = default;

	float getLevelLeft() const { return currentAudioLevelLeft; }
	float getLevelRight() const { return currentAudioLevelRight; }

	void updateMeter(const juce::AudioBuffer<float>* buffer, double readPos, float volume, bool isPlaying);
	void paint(juce::Graphics& g) override;
	void drawClipRect(juce::Rectangle<float>& vuArea, juce::Graphics& g, float currentAudioLevel);
	void drawPeakSegments(int numSegments, juce::Rectangle<float>& vuArea, float segmentHeight, juce::Graphics& g, float peakValue);
	void updateFromRawLevels(float rawLeft, float rawRight);

private:
	float currentAudioLevelLeft = 0.0f;
	float currentAudioLevelRight = 0.0f;
	float peakHoldLeft = 0.0f;
	float peakHoldRight = 0.0f;
	int peakHoldTimerLeft = 0;
	int peakHoldTimerRight = 0;
	bool hasSource = false;

	void fillMeterSegment(juce::Graphics& g, juce::Rectangle<float>& vuArea,
		int i, float segmentHeight, int numSegments,
		float currentLevel);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeter)
};