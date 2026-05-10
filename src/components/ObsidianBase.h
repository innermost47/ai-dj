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
		g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);
		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawRect(getLocalBounds(), 1);
	}

	void paintBaseLocalBackground(juce::Graphics &g, juce::Rectangle<int> bounds)
	{
		g.setColour(ColourPalette::backgroundDark);
		g.fillRoundedRectangle(bounds.toFloat(), ObsidianSizes::CORNER);
		g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
		g.drawRect(bounds.toFloat(), 1);
	}
};