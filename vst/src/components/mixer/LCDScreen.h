#pragma once
#include <JuceHeader.h>
#include "style/ColourPalette.h"

class LCDScreen : public juce::Component
{
public:
	LCDScreen();

	void setLines(const juce::String& line1, const juce::String& line2, const juce::String& line3);
	void paint(juce::Graphics& g) override;

private:
	juce::String lineTop;
	juce::String lineMid;
	juce::String lineBot;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDScreen)
};