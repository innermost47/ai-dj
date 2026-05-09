#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

class LedRadioButton : public juce::Button
{
  public:
	LedRadioButton(const juce::String &text, juce::Colour activeColour = ColourPalette::violet);

	void paintButton(juce::Graphics &g, bool isMouseOver, bool /*isButtonDown*/) override;

	void setActiveColour(juce::Colour col);

  private:
	juce::Colour activeCol;
};