#pragma once
#include <JuceHeader.h>
#include "style/ColourPalette.h"

class LCDScreen : public juce::Component
{
public:
	LCDScreen()
	{
		setInterceptsMouseClicks(false, false);
	}

	void setLines(const juce::String& line1, const juce::String& line2, const juce::String& line3)
	{
		lineTop = line1;
		lineMid = line2;
		lineBot = line3;
		repaint();
	}

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();

		g.setColour(juce::Colours::black.withAlpha(0.35f));
		g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);

		g.setColour(ColourPalette::backgroundLight);
		g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);

		g.setColour(juce::Colour::fromRGBA(0, 0, 0, 20));
		for (int y = (int)bounds.getY(); y < (int)bounds.getBottom(); y += 2)
			g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());

		auto textArea = getLocalBounds().reduced(6, 4);
		const int lineH = textArea.getHeight() / 3;

		g.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
		g.setColour(ColourPalette::textSecondary);
		g.drawText(lineTop, textArea.removeFromTop(lineH),
			juce::Justification::centredLeft, true);

		g.setFont(juce::FontOptions("Courier New", 12.0f, juce::Font::bold));
		g.setColour(ColourPalette::textPrimary);
		g.drawText(lineMid, textArea.removeFromTop(lineH),
			juce::Justification::centredLeft, true);

		g.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
		g.setColour(ColourPalette::textAccent);
		g.drawText(lineBot, textArea,
			juce::Justification::centredLeft, true);
	}

private:
	juce::String lineTop;
	juce::String lineMid;
	juce::String lineBot;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDScreen)
};