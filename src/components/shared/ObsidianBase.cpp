#include "ObsidianBase.h"
#include "AiModelDefinitions.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "Fonts.h"
#include "IconButton.h"
#include "Shades.h"
#include "Sizes.h"
#include <JuceHeader.h>

std::unique_ptr<juce::AccessibilityHandler> ObsidianComponent::createAccessibilityHandler()
{
	return createIgnoredAccessibilityHandler(*this);
}

void ObsidianComponent::paintBaseRoundedBackground(juce::Graphics &g, juce::Colour colour)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(colour);
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds, Obsidian::CORNER, Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseRoundedBackgroundMidWithAlpha06(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundMid.withAlpha(Obsidian::ALPHA_06));
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds, Obsidian::CORNER, Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseLocalBackground(juce::Graphics &g, juce::Rectangle<int> bounds)
{
	g.setColour(ColourPalette::backgroundDark);
	g.fillAll();
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRect(bounds.toFloat(), 1);
}

void ObsidianComponent::paintBaseBackgroundWithLeftBorder(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(bounds);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawLine(0.0f, 0.0f, 0.0f, bounds.getHeight(), Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseBackgroundWithRightBorder(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(bounds);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawLine(bounds.getWidth(), 0.0f, bounds.getWidth(), bounds.getHeight(), Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::drawCircleWithEllipse(juce::Graphics &g, juce::Rectangle<int> area, juce::Colour colour)
{
	auto circleArea = area.removeFromLeft(14);
	auto circleRect = circleArea.withSizeKeepingCentre(7, 7).toFloat();

	g.setColour(colour);
	g.fillEllipse(circleRect);

	g.setColour(colour.withAlpha(0.3f));
	g.drawEllipse(circleRect.expanded(1.5f), 1.0f);
}
