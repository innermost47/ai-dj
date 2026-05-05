#pragma once
#include <JuceHeader.h>
#include "midi/MidiLearnableComponents.h"
#include "style/ColourPalette.h"

struct IconButtonBase
{
	void loadIcon(const char* svgData, size_t svgSize)
	{
		iconDrawable = loadSVG(svgData, svgSize);
	}

	void loadIconToggled(const char* svgData, size_t svgSize)
	{
		iconDrawableToggled = loadSVG(svgData, svgSize);
	}

	void setLabelText(const juce::String& text) { labelText = text; }
	void setCompactMode(bool compact) { isCompact = compact; }
	void setHasToggledIcon(bool has) { hasToggledIcon = has; }
	void setHasAccentBar(bool has) { hasAccentBar = has; }
	void setShowBackground(bool show) { showBackground = show; }
	void setIconSize(float size) { customIconSize = size; }
	void setShowBorder(bool show) { showBorder = show; }

protected:
	std::unique_ptr<juce::Drawable> iconDrawable;
	std::unique_ptr<juce::Drawable> iconDrawableToggled;
	juce::String labelText;
	bool isCompact = false;
	bool hasToggledIcon = false;
	bool hasAccentBar = false;
	bool showBackground = true;
	float customIconSize = -1.0f;
	bool showBorder = false;
	juce::Colour borderColour = ColourPalette::trackSelected.withAlpha(0.4f);

	static std::unique_ptr<juce::Drawable> loadSVG(const char* data, size_t size);

	void paintIconButton(juce::Graphics& g,
		juce::Button& btn,
		bool isMouseOver,
		bool isButtonDown);
};

class IconButton : public MidiLearnableButton, public IconButtonBase
{
public:
	IconButton(const juce::String& name, const juce::String& label = {});

	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
};

class IconButtonSimple : public juce::TextButton, public IconButtonBase
{
public:
	IconButtonSimple(const juce::String& name, const juce::String& label = {});

	void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
};