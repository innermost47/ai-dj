#pragma once
#include "AiModelDefinitions.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "DataConst.h"
#include "EscapableTextEditor.h"
#include "Fonts.h"
#include "IconButton.h"
#include "Shades.h"
#include "Sizes.h"
#include <JuceHeader.h>

class ObsidianComponent : public juce::Component
{
  public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

	void paintBaseRoundedBackground(juce::Graphics &g, juce::Colour colour);

	void paintBaseRoundedBackgroundMidWithAlpha06(juce::Graphics &g);

	void paintBaseLocalBackground(juce::Graphics &g, juce::Rectangle<int> bounds);

	void paintBaseBackgroundWithLeftBorder(juce::Graphics &g);

	void paintBaseBackgroundWithRightBorder(juce::Graphics &g);

	void drawCircleWithEllipse(juce::Graphics &g, juce::Rectangle<int> area, juce::Colour colour);
};