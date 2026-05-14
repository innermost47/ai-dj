#pragma once
#include "ColourPalette.h"
#include "MidiLearnableComponents.h"
#include <JuceHeader.h>

struct IconButtonBase
{
	void loadIcon(const char *svgData, size_t svgSize)
	{
		iconDrawable = loadSVG(svgData, svgSize);
	}

	void loadIconToggled(const char *svgData, size_t svgSize)
	{
		iconDrawableToggled = loadSVG(svgData, svgSize);
	}

	void setLabelText(const juce::String &text)
	{
		labelText = text;
	}
	void setCompactMode(bool compact)
	{
		isCompact = compact;
	}
	void setHasToggledIcon(bool has)
	{
		hasToggledIcon = has;
	}
	void setHasAccentBar(bool has)
	{
		hasAccentBar = has;
	}
	void setShowBackground(bool show)
	{
		showBackground = show;
	}
	void setIconSize(float size)
	{
		customIconSize = size;
	}
	void setShowBorder(bool show)
	{
		showBorder = show;
	}

	void setCustomIconColour(juce::Colour colour)
	{
		customIconColour = colour;
		hasCustomIconColour = true;
	}
	void setCustomIconColourToggled(juce::Colour colour)
	{
		customIconColourToggled = colour;
		hasCustomIconColourToggled = true;
	}
	void clearCustomIconColour()
	{
		hasCustomIconColour = false;
		hasCustomIconColourToggled = false;
	}

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
	juce::Colour customIconColour;
	juce::Colour customIconColourToggled;
	bool hasCustomIconColour = false;
	bool hasCustomIconColourToggled = false;
	juce::Colour borderColour = ColourPalette::lightGrey.withAlpha(0.4f);

	static std::unique_ptr<juce::Drawable> loadSVG(const char *data, size_t size);

	void paintIconButton(juce::Graphics &g, juce::Button &btn, bool isMouseOver, bool isButtonDown);
};

class IconButton : public MidiLearnableButton, public IconButtonBase
{
  public:
	IconButton(const juce::String &name, const juce::String &label = {});

	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	void paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown) override;
};

class IconButtonSimple : public juce::TextButton, public IconButtonBase
{
  public:
	IconButtonSimple(const juce::String &name, const juce::String &label = {});

	void paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown) override;
};

class IconButtonRepeat : public juce::TextButton, public IconButtonBase, private juce::Timer
{
  public:
	IconButtonRepeat(const juce::String &name, const juce::String &label = {});

	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	void paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown) override;

  private:
	void mouseDown(const juce::MouseEvent &e) override;
	void mouseUp(const juce::MouseEvent &e) override;
	void timerCallback() override;

	int repeatCount = 0;
};