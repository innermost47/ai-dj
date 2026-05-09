#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
  public:
	static CustomLookAndFeel &getInstance()
	{
		static CustomLookAndFeel instance;
		return instance;
	}
	CustomLookAndFeel();

	juce::Rectangle<int> getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
	                                      juce::Rectangle<int> parentArea) override;

	void drawTooltip(juce::Graphics &g, const juce::String &text, int width, int height) override;

	void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
	                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void drawButtonText(juce::Graphics &g, juce::TextButton &button, bool /*shouldDrawButtonAsHighlighted*/,
	                    bool /*shouldDrawButtonAsDown*/) override;

	void drawToggleButton(juce::Graphics &g, juce::ToggleButton &button, bool shouldDrawButtonAsHighlighted,
	                      bool shouldDrawButtonAsDown) override;

	juce::AlertWindow *createAlertWindow(const juce::String &title, const juce::String &message,
	                                     const juce::String & /*button1*/, const juce::String & /*button2*/,
	                                     const juce::String & /*button3*/, juce::MessageBoxIconType iconType,
	                                     int /*numButtons*/, juce::Component *associatedComponent) override;

	void drawAlertBox(juce::Graphics &g, juce::AlertWindow &alert, const juce::Rectangle<int> &textArea,
	                  juce::TextLayout &textLayout) override;

	int getAlertWindowButtonHeight() override
	{
		return 32;
	}

	juce::Font getAlertWindowTitleFont() override;

	juce::Font getAlertWindowMessageFont() override;

	juce::Font getAlertWindowFont() override;

	void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
	                  int buttonW, int buttonH, juce::ComboBox &box) override;

	juce::Font getComboBoxFont(juce::ComboBox &box) override;

	void positionComboBoxText(juce::ComboBox &box, juce::Label &label) override;

	juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox &box, juce::Label &label) override;

	void drawLabel(juce::Graphics &g, juce::Label &label) override;

	juce::BorderSize<int> getLabelBorderSize(juce::Label & /*label*/) override;

	static const juce::Identifier &getDrawTicksPropertyId();

	static const juce::Identifier &getDrawTicksSmallPropertyId();

	void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
	                      float /*minSliderPos*/, float /*maxSliderPos*/, const juce::Slider::SliderStyle style,
	                      juce::Slider &slider) override;

	void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPosProportional,
	                      float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) override;

	void drawScrollbar(juce::Graphics &g, juce::ScrollBar &scrollbar, int x, int y, int width, int height,
	                   bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver,
	                   bool isMouseDown) override;

	void drawPopupMenuBackground(juce::Graphics &g, int width, int height) override;

	void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area, bool isSeparator, bool isActive,
	                       bool isHighlighted, bool isTicked, bool /*hasSubMenu*/, const juce::String &text,
	                       const juce::String & /*shortcutKeyText*/, const juce::Drawable * /*icon*/,
	                       const juce::Colour *textColourToUse) override;

	void fillTextEditorBackground(juce::Graphics &g, int width, int height, juce::TextEditor &textEditor) override;

	void drawTextEditorOutline(juce::Graphics &g, int width, int height, juce::TextEditor &textEditor) override;

	juce::CaretComponent *createCaretComponent(juce::Component *keyFocusOwner) override;

  private:
	void drawGraduationTicks(juce::Graphics &g, juce::Rectangle<float> trackRect, int numTicks, bool isVertical,
	                         bool small = false);

	static juce::Colour soften(const juce::Colour &colour);

	static juce::TextLayout layoutTooltipText(const juce::String &text, juce::Colour colour);
};