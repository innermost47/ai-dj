#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"

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

	void setTopLine(const juce::String& s) { lineTop = s; repaint(); }
	void setMiddleLine(const juce::String& s) { lineMid = s; repaint(); }
	void setBottomLine(const juce::String& s) { lineBot = s; repaint(); }

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();

		g.setColour(juce::Colour::fromRGB(8, 18, 12));
		g.fillRoundedRectangle(bounds, 4.0f);

		juce::ColourGradient gradient(
			juce::Colour::fromRGB(0, 8, 4), bounds.getX(), bounds.getY(),
			juce::Colour::fromRGB(15, 30, 20), bounds.getX(), bounds.getBottom(),
			false);
		g.setGradientFill(gradient);
		g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);

		g.setColour(juce::Colour::fromRGB(30, 50, 35));
		g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);

		g.setColour(juce::Colour::fromRGBA(0, 0, 0, 40));
		for (int y = (int)bounds.getY(); y < (int)bounds.getBottom(); y += 2)
		{
			g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
		}

		const auto lcdGreen = juce::Colour::fromRGB(130, 220, 140);
		const auto lcdGreenDim = juce::Colour::fromRGB(90, 170, 100);

		g.setColour(lcdGreen);
		g.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));

		auto textArea = getLocalBounds().reduced(6, 4);
		const int lineH = textArea.getHeight() / 3;

		g.setColour(lcdGreenDim);
		g.drawText(lineTop, textArea.removeFromTop(lineH),
			juce::Justification::centredLeft, true);

		g.setColour(lcdGreen);
		g.setFont(juce::FontOptions("Courier New", 12.0f, juce::Font::bold));
		g.drawText(lineMid, textArea.removeFromTop(lineH),
			juce::Justification::centredLeft, true);

		g.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
		g.setColour(lcdGreenDim);
		g.drawText(lineBot, textArea,
			juce::Justification::centredLeft, true);
	}

private:
	juce::String lineTop;
	juce::String lineMid;
	juce::String lineBot;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LCDScreen)
};