#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class LCDScreen : public ObsidianComponent
{
  public:
	LCDScreen();

	void setLines(const juce::String &line1, const juce::String &line2, const juce::String &line3);
	void paint(juce::Graphics &g) override;
	void setTwoLineMode(bool twoLines);

  private:
	juce::String lineTop;
	juce::String lineMid;
	juce::String lineBot;
	bool twoLineMode = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDScreen)
};