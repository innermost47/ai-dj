#include "LedRadioButton.h"
#include "Sizes.h"

LedRadioButton::LedRadioButton(const juce::String &text, juce::Colour activeColour)
    : juce::Button(text), activeCol(activeColour)
{
	setClickingTogglesState(true);
}

void LedRadioButton::paintButton(juce::Graphics &g, bool isMouseOver, bool /*isButtonDown*/)
{
	auto bounds = getLocalBounds().toFloat();
	bool on = getToggleState();

	const float ledSize = 8.0f;
	auto ledRect = juce::Rectangle<float>(2.0f, bounds.getCentreY() - ledSize * 0.5f, ledSize, ledSize);

	if (on)
	{
		g.setColour(activeCol.withAlpha(0.3f));
		g.fillEllipse(ledRect.expanded(2.0f));
		g.setColour(activeCol);
		g.fillEllipse(ledRect);
		g.setColour(activeCol.brighter(0.4f));
		g.fillEllipse(ledRect.reduced(1.5f));
	}
	else
	{
		g.setColour(ColourPalette::backgroundDark);
		g.fillEllipse(ledRect);
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.4f));
		g.drawEllipse(ledRect, 0.8f);
	}

	auto textArea = bounds.withTrimmedLeft(ledSize + 6.0f);
	g.setColour(on ? ColourPalette::textPrimary
	               : (isMouseOver ? ColourPalette::textSecondary : ColourPalette::textSecondary.withAlpha(0.9f)));
	g.setFont(juce::FontOptions(Obsidian::TEXT_XXS, on ? juce::Font::bold : juce::Font::plain));
	g.drawText(getButtonText(), textArea.toNearestInt(), juce::Justification::centredLeft, false);
}

void LedRadioButton::setActiveColour(juce::Colour col)
{
	activeCol = col;
	repaint();
}
