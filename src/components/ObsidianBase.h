#pragma once
#include "AiModelDefinitions.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "IconButton.h"
#include "Shades.h"
#include "Sizes.h"
#include <JuceHeader.h>

class ObsidianComponent : public juce::Component
{
  public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	void paintBaseBackground(juce::Graphics &g)
	{
		auto bounds = getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundDark);
		g.fillAll();
		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawRect(getLocalBounds(), 1);
	}

	void paintBaseRoundedBackgroundMidWithAlpha06(juce::Graphics &g)
	{
		auto bounds = getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundMid.withAlpha(ObsidianShades::ALPHA_06));
		g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);
		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawRoundedRectangle(bounds, ObsidianSizes::CORNER, ObsidianSizes::BORDER_WIDTH);
	}

	void paintBaseLocalBackground(juce::Graphics &g, juce::Rectangle<int> bounds)
	{
		g.setColour(ColourPalette::backgroundDark);
		g.fillAll();
		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawRect(bounds.toFloat(), 1);
	}

	void paintBaseBackgroundWithLeftBorder(juce::Graphics &g)
	{
		auto bounds = getLocalBounds().toFloat();

		g.setColour(ColourPalette::backgroundDark);
		g.fillRect(bounds);

		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawLine(0.0f, 0.0f, 0.0f, bounds.getHeight(), ObsidianSizes::BORDER_WIDTH);
	}
};