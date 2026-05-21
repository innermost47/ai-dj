#include "CustomLookAndFeel.h"
#include "BinaryData.h"
#include "Fonts.h"
#include "Shades.h"
#include "Sizes.h"

CustomLookAndFeel::CustomLookAndFeel()
{
	setColour(juce::TextButton::buttonColourId, soften(ColourPalette::backgroundLight));
	setColour(juce::TextButton::buttonOnColourId, soften(ColourPalette::buttonSuccess));
	setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
	setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
	setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
	setColour(juce::ToggleButton::tickColourId, soften(ColourPalette::buttonSuccess));
	setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
	setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
	setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
	setColour(juce::TextEditor::focusedOutlineColourId, ColourPalette::buttonPrimary.withAlpha(0.6f));
	setColour(juce::TextEditor::highlightColourId, ColourPalette::buttonPrimary.withAlpha(0.3f));
	setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
	setColour(juce::ScrollBar::thumbColourId, ColourPalette::muteActive);
	setColour(juce::ScrollBar::backgroundColourId, ColourPalette::backgroundDeep);
	setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
	setColour(juce::PopupMenu::backgroundColourId, ColourPalette::backgroundDark);
	setColour(juce::PopupMenu::textColourId, ColourPalette::textPrimary);
	setColour(juce::PopupMenu::highlightedBackgroundColourId, ColourPalette::buttonPrimary.withAlpha(0.4f));
	setColour(juce::PopupMenu::highlightedTextColourId, ColourPalette::textPrimary);
	setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
	setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
	setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
	setColour(juce::TextEditor::focusedOutlineColourId, ColourPalette::lightGrey);
	setColour(juce::TextEditor::highlightColourId, ColourPalette::lightGrey.withAlpha(0.3f));
	setColour(juce::TextEditor::highlightedTextColourId, ColourPalette::textPrimary);
	setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
	setColour(juce::CaretComponent::caretColourId, ColourPalette::lightGrey);
}

juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font &f)
{
	auto requestedName = f.getTypefaceName();

	if (requestedName == ObsidianFonts::MICHROMA->getName())
	{
		return ObsidianFonts::MICHROMA;
	}

	static auto reg =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::notoregular_ttf, BinaryData::notoregular_ttfSize);
	static auto bold = juce::Typeface::createSystemTypefaceFor(BinaryData::notobold_ttf, BinaryData::notobold_ttfSize);
	static auto ital =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::notoitalic_ttf, BinaryData::notoitalic_ttfSize);

	if (f.isBold())
		return bold;
	if (f.isItalic())
		return ital;

	return reg;
}

juce::Colour CustomLookAndFeel::soften(const juce::Colour &colour)
{
	return colour.withSaturation(colour.getSaturation() * 0.85f).brighter(0.05f);
}

juce::TextLayout CustomLookAndFeel::layoutTooltipText(const juce::String &text, juce::Colour colour)
{
	const float tooltipFontSize = 12.0f;
	const int maxToolTipWidth = 400;

	juce::AttributedString s;
	s.setJustification(juce::Justification::centredLeft);

	s.append(text, juce::Font(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(tooltipFontSize)), colour);

	juce::TextLayout tl;
	tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
	return tl;
}

juce::Rectangle<int> CustomLookAndFeel::getTooltipBounds(const juce::String &tipText, juce::Point<int> screenPos,
                                                         juce::Rectangle<int> parentArea)
{
	const juce::TextLayout tl(layoutTooltipText(tipText, ColourPalette::textPrimary));
	auto w = (int)(tl.getWidth() + 16.0f);
	auto h = (int)(tl.getHeight() + 10.0f);
	return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
	                            screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
	    .constrainedWithin(parentArea);
}

void CustomLookAndFeel::drawTooltip(juce::Graphics &g, const juce::String &text, int width, int height)
{
	juce::Rectangle<float> bounds(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);
	g.setColour(ColourPalette::lightGrey.withAlpha(0.5f));
	g.drawRoundedRectangle(bounds, ObsidianSizes::CORNER, 1.0f);
	layoutTooltipText(text, ColourPalette::textPrimary).draw(g, bounds.reduced(8.0f, 4.0f));
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &button,
                                             const juce::Colour &backgroundColour, bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
	auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
	auto baseColour = soften(backgroundColour).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

	if (shouldDrawButtonAsDown)
		baseColour = baseColour.darker(0.15f);
	else if (shouldDrawButtonAsHighlighted)
		baseColour = baseColour.brighter(0.12f);

	if (!shouldDrawButtonAsDown)
	{
		g.setColour(juce::Colours::black.withAlpha(0.3f));
		g.fillRoundedRectangle(bounds.translated(0, 1.5f), ObsidianSizes::CORNER);
	}

	g.setColour(baseColour);
	g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);

	g.setColour(baseColour.brighter(0.2f).withAlpha(0.4f));
	g.drawRoundedRectangle(bounds, ObsidianSizes::CORNER, ObsidianShades::LIGHT_BORDER);
}

void CustomLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &button,
                                       bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
	auto textColour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
	                                                            : juce::TextButton::textColourOffId);

	if (!button.isEnabled())
		textColour = textColour.withAlpha(0.5f);

	g.setColour(textColour);
	g.setFont(juce::Font(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(ObsidianSizes::TEXT_REGULAR)));

	g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 2, 0.8f);
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &button,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto bounds = button.getLocalBounds().toFloat();

	auto bgColour = button.findColour(juce::TextButton::buttonColourId);

	if (!button.isEnabled())
		bgColour = bgColour.withAlpha(0.5f);
	else if (shouldDrawButtonAsDown)
		bgColour = bgColour.darker(0.15f);
	else if (shouldDrawButtonAsHighlighted)
		bgColour = bgColour.brighter(0.12f);

	if (!shouldDrawButtonAsDown)
	{
		g.setColour(juce::Colours::black.withAlpha(0.3f));
		g.fillRoundedRectangle(bounds.translated(0, 1.5f), ObsidianSizes::CORNER);
	}

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);

	if (!shouldDrawButtonAsDown)
	{
		g.setColour(juce::Colours::white.withAlpha(0.05f));
		auto topBounds = bounds.withHeight(bounds.getHeight() * 0.4f);
		g.fillRoundedRectangle(topBounds, ObsidianSizes::CORNER);
	}

	g.setColour(bgColour.brighter(0.2f).withAlpha(0.4f));
	g.drawRoundedRectangle(bounds, ObsidianSizes::CORNER, ObsidianSizes::BORDER_WIDTH);

	auto textColour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
	                                                            : juce::TextButton::textColourOffId);

	if (!button.isEnabled())
		textColour = textColour.withAlpha(0.5f);

	g.setColour(textColour);
	g.setFont(juce::Font(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR)));
	g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

juce::AlertWindow *CustomLookAndFeel::createAlertWindow(const juce::String &title, const juce::String &message,
                                                        const juce::String & /*button1*/,
                                                        const juce::String & /*button2*/,
                                                        const juce::String & /*button3*/,
                                                        juce::MessageBoxIconType iconType, int /*numButtons*/,
                                                        juce::Component *associatedComponent)
{
	auto *aw = new juce::AlertWindow(title, message, iconType, associatedComponent);

	aw->setColour(juce::AlertWindow::backgroundColourId, ColourPalette::backgroundDark);
	aw->setColour(juce::AlertWindow::textColourId, ColourPalette::textPrimary);
	aw->setColour(juce::AlertWindow::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.6f));

	return aw;
}

void CustomLookAndFeel::drawAlertBox(juce::Graphics &g, juce::AlertWindow &alert, const juce::Rectangle<int> &textArea,
                                     juce::TextLayout &textLayout)
{
	g.fillAll(ColourPalette::backgroundDark);

	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
	g.drawRoundedRectangle(alert.getLocalBounds().toFloat().reduced(1.0f), ObsidianSizes::CORNER, 1.5f);

	auto titleBar = alert.getLocalBounds().removeFromTop(42).toFloat();
	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.2f));
	g.fillRect(titleBar);

	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
	g.drawLine(titleBar.getBottomLeft().x, titleBar.getBottom(), titleBar.getBottomRight().x, titleBar.getBottom(),
	           1.0f);

	textLayout.draw(g, textArea.toFloat());
}

juce::Font CustomLookAndFeel::getAlertWindowTitleFont()
{
	return juce::Font(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_SUBTITLE));
}

juce::Font CustomLookAndFeel::getAlertWindowMessageFont()
{
	return juce::Font(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(ObsidianSizes::TEXT_REGULAR));
}

juce::Font CustomLookAndFeel::getAlertWindowFont()
{
	return juce::Font(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));
}

void CustomLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH, juce::ComboBox &box)
{
	auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
	const float corner = ObsidianSizes::CORNER;

	if (!isButtonDown)
	{
		g.setColour(juce::Colours::black.withAlpha(0.25f));
		g.fillRoundedRectangle(bounds.translated(0, 1.0f), corner);
	}

	g.setColour(ColourPalette::backgroundMid);
	g.fillRoundedRectangle(bounds, corner);

	auto borderColour = ColourPalette::lightGrey.withAlpha(ObsidianShades::ALPHA_04);
	float borderThickness = ObsidianSizes::BORDER_WIDTH_SM;

	if (box.hasKeyboardFocus(false))
	{
		borderColour = ColourPalette::lightGrey.withAlpha(ObsidianShades::ALPHA_08);
		borderThickness = ObsidianSizes::BORDER_WIDTH_XL;
	}
	else if (box.isMouseOver())
	{
		borderColour = ColourPalette::lightGrey.withAlpha(ObsidianShades::ALPHA_06);
		borderThickness = ObsidianSizes::BORDER_WIDTH;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds, corner, borderThickness);

	auto arrowZone = juce::Rectangle<float>((float)buttonX, (float)buttonY, (float)buttonW, (float)buttonH);

	auto chevronDownSvg = juce::Drawable::createFromImageData(BinaryData::caretdown_svg, BinaryData::caretdown_svgSize);
	if (chevronDownSvg != nullptr)
	{
		chevronDownSvg->replaceColour(juce::Colours::black, ColourPalette::textPrimary);
		chevronDownSvg->drawWithin(g, arrowZone.toFloat().reduced(4), juce::RectanglePlacement::centred,
		                           ObsidianShades::ALPHA_04);
	}
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox & /*box*/)
{
	auto fontSize = ObsidianSizes::TEXT_REGULAR;
	return juce::Font(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(fontSize));
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox &box, juce::Label &label)
{
	auto buttonWidth = juce::jmin(box.getHeight(), 24);

	label.setBounds(0, 0, box.getWidth() - buttonWidth, box.getHeight());

	label.setFont(getComboBoxFont(box));
	label.setJustificationType(juce::Justification::left);
	label.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	label.setBorderSize(juce::BorderSize<int>(0, 0, 0, 0));
}

juce::PopupMenu::Options CustomLookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox &box, juce::Label &label)
{
	return juce::PopupMenu::Options()
	    .withTargetComponent(&box)
	    .withItemThatMustBeVisible(box.getSelectedId())
	    .withInitiallySelectedItem(box.getSelectedId())
	    .withMinimumWidth(box.getWidth())
	    .withMaximumNumColumns(1)
	    .withStandardItemHeight(juce::jmax(24, label.getHeight()));
}

void CustomLookAndFeel::drawLabel(juce::Graphics &g, juce::Label &label)
{
	bool isSliderTextBox = (dynamic_cast<juce::Slider *>(label.getParentComponent()) != nullptr);

	float currentSize = label.getFont().getHeight();
	auto currentFont = label.getFont();
	auto fontName = currentFont.getTypefaceName();

	if (fontName == ObsidianFonts::MICHROMA->getName())
	{
		g.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(currentSize));
	}
	else
	{
		if (label.getFont().isBold())
		{
			g.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(currentSize));
		}
		else if (label.getFont().isItalic())
		{
			g.setFont(juce::FontOptions(ObsidianFonts::NOTO_ITALIC).withHeight(currentSize));
		}
		else
		{
			g.setFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(currentSize));
		}
	}

	if (isSliderTextBox)
	{
		auto bounds = label.getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundDark);
		g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);
		g.setColour(ColourPalette::lightGrey.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), ObsidianSizes::CORNER, 0.8f);

		if (!label.isBeingEdited())
		{
			auto alpha = label.isEnabled() ? 1.0f : 0.5f;
			g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
			auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
			g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
			                 juce::jmax(1, (int)((float)textArea.getHeight() / label.getFont().getHeight())), 1.0f);
		}
		return;
	}

	g.fillAll(label.findColour(juce::Label::backgroundColourId));
	if (!label.isBeingEdited())
	{
		auto alpha = label.isEnabled() ? 1.0f : 0.5f;
		auto textColour = label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha);
		g.setColour(textColour);
		auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
		g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
		                 juce::jmax(1, (int)((float)textArea.getHeight() / label.getFont().getHeight())), 1.0f);
	}
	else if (label.isEnabled())
	{
		g.setColour(label.findColour(juce::Label::outlineColourId));
	}
}

juce::BorderSize<int> CustomLookAndFeel::getLabelBorderSize(juce::Label & /*label*/)
{
	return juce::BorderSize<int>(1, 5, 1, 5);
}

const juce::Identifier &CustomLookAndFeel::getDrawTicksPropertyId()
{
	static const juce::Identifier id("drawTicks");
	return id;
}

const juce::Identifier &CustomLookAndFeel::getDrawTicksSmallPropertyId()
{
	static const juce::Identifier id("drawTicksSmall");
	return id;
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                                         float /*minSliderPos*/, float /*maxSliderPos*/,
                                         const juce::Slider::SliderStyle style, juce::Slider &slider)
{
	auto accentColour = slider.isColourSpecified(juce::Slider::thumbColourId)
	                        ? slider.findColour(juce::Slider::thumbColourId)
	                        : ColourPalette::buttonPrimary;

	if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal)
	{
		const bool isVertical = (style == juce::Slider::LinearVertical);
		auto trackWidth = juce::jmin(5.0f, (float)(isVertical ? width : height) * 0.20f);

		juce::Point<float> startPoint, endPoint;
		if (isVertical)
		{
			auto cx = (float)x + (float)width * 0.5f;
			startPoint = {cx, (float)y};
			endPoint = {cx, (float)(y + height)};
		}
		else
		{
			auto cy = (float)y + (float)height * 0.5f;
			startPoint = {(float)x, cy};
			endPoint = {(float)(x + width), cy};
		}

		auto trackRect = juce::Rectangle<float>(startPoint, endPoint).expanded(trackWidth * 0.5f);

		const auto &props = slider.getProperties();
		int numTicks = props[getDrawTicksPropertyId()];
		if (numTicks >= 2)
		{
			const bool small = (bool)props[getDrawTicksSmallPropertyId()];
			drawGraduationTicks(g, trackRect, numTicks, isVertical, small);
		}

		g.setColour(ColourPalette::backgroundMid);
		g.fillRoundedRectangle(trackRect, trackWidth * 0.5f);

		if (isVertical)
		{
			float thumbY = sliderPos;
			float bottomY = (float)(y + height);
			float cx = (float)x + (float)width * 0.5f;

			auto fillRect = juce::Rectangle<float>(cx - trackWidth * 0.5f, thumbY, trackWidth, bottomY - thumbY);

			if (fillRect.getHeight() > 0.5f)
			{
				g.setColour(accentColour.withAlpha(0.5f));
				g.fillRoundedRectangle(fillRect, trackWidth * 0.5f);
			}
		}

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.5f));
		g.drawRoundedRectangle(trackRect, trackWidth * 0.5f, 0.6f);

		float capsuleW, capsuleH;
		if (isVertical)
		{
			capsuleW = 20.0f;
			capsuleH = 12.0f;
		}
		else
		{
			capsuleW = 22.0f;
			capsuleH = juce::jmax(20.0f, juce::jmin(30.0f, (float)height * 0.75f));
		}
		const float capsuleR = ObsidianSizes::CORNER * 2;

		juce::Rectangle<float> capsule;
		if (isVertical)
			capsule =
			    juce::Rectangle<float>(capsuleW, capsuleH).withCentre({(float)x + (float)width * 0.5f, sliderPos});
		else
			capsule =
			    juce::Rectangle<float>(capsuleW, capsuleH).withCentre({sliderPos, (float)y + (float)height * 0.5f});

		g.setColour(juce::Colours::black.withAlpha(0.5f));
		g.fillRoundedRectangle(capsule.translated(0, 2.0f), capsuleR);
		g.setColour(juce::Colours::black.withAlpha(0.3f));
		g.fillRoundedRectangle(capsule.translated(0, 1.0f), capsuleR);

		juce::ColourGradient bodyGradient(ColourPalette::backgroundLight.brighter(0.15f), capsule.getX(),
		                                  capsule.getY(), ColourPalette::backgroundMid.darker(0.2f), capsule.getX(),
		                                  capsule.getBottom(), false);
		bodyGradient.addColour(0.5, ColourPalette::backgroundLight.darker(0.05f));
		g.setGradientFill(bodyGradient);
		g.fillRoundedRectangle(capsule, capsuleR);

		g.setColour(juce::Colours::white.withAlpha(0.10f));
		g.fillRoundedRectangle(capsule.withHeight(capsule.getHeight() * 0.45f), capsuleR);

		bool isActive = slider.isMouseOverOrDragging();
		g.setColour(isActive ? ColourPalette::backgroundLight.withAlpha(0.95f)
		                     : ColourPalette::backgroundLight.withAlpha(0.55f));
		g.drawRoundedRectangle(capsule.reduced(0.5f), capsuleR, 0.8f);

		if (isVertical)
		{
			const float lineThick = 2.0f;
			float lx = capsule.getCentreX() - lineThick * 0.5f;
			float ly = capsule.getY() + 2.0f;
			float lh = capsule.getHeight() - 4.0f;
			if (lh > 1.0f)
			{
				g.setColour(accentColour.withAlpha(isActive ? 1.0f : 0.85f));
				g.fillRoundedRectangle(lx, ly, lineThick, lh, lineThick * 0.5f);
			}
		}
		else
		{
			const float lineThick = 2.0f;
			float lx = capsule.getCentreX() - lineThick * 0.5f;
			float ly = capsule.getY() + 4.0f;
			float lh = capsule.getHeight() - 8.0f;
			if (lh > 1.0f)
			{
				g.setColour(accentColour.withAlpha(isActive ? 1.0f : 0.9f));
				g.fillRoundedRectangle(lx, ly, lineThick, lh, lineThick * 0.5f);

				g.setColour(accentColour.withAlpha(isActive ? 0.4f : 0.25f));
				g.fillRoundedRectangle(lx - 1.0f, ly, lineThick + 2.0f, lh, (lineThick + 2.0f) * 0.5f);
			}
		}
	}
}

void CustomLookAndFeel::drawGraduationTicks(juce::Graphics &g, juce::Rectangle<float> trackRect, int numTicks,
                                            bool isVertical, bool small)
{
	const float majorLen = small ? 5.0f : 9.0f;
	const float minorLen = small ? 3.0f : 5.0f;
	const float majorThick = small ? 1.0f : 1.5f;
	const float minorThick = small ? 0.8f : 1.0f;
	const float majorAlpha = 0.65f;
	const float centreAlpha = 0.85f;
	const float minorAlpha = 0.32f;
	const float gap = small ? 2.0f : 3.0f;

	const int lastIdx = numTicks - 1;
	const int midIdx = lastIdx / 2;

	auto colour = ColourPalette::textPrimary;

	for (int i = 0; i < numTicks; ++i)
	{
		const float t = (float)i / (float)lastIdx;

		const bool isEdge = (i == 0 || i == lastIdx);
		const bool isMid = (i == midIdx && (numTicks % 2) == 1);
		const bool isMajor = (isEdge || isMid);

		const float len = isMajor ? majorLen : minorLen;
		const float thick = isMajor ? majorThick : minorThick;
		float alpha = minorAlpha;
		if (isMid)
			alpha = centreAlpha;
		else if (isEdge)
			alpha = majorAlpha;

		g.setColour(colour.withAlpha(alpha));

		if (isVertical)
		{
			const float ty = trackRect.getY() + t * trackRect.getHeight();
			g.fillRect(juce::Rectangle<float>(trackRect.getX() - gap - len, ty - thick * 0.5f, len, thick));
			g.fillRect(juce::Rectangle<float>(trackRect.getRight() + gap, ty - thick * 0.5f, len, thick));
		}
		else
		{
			const float tx = trackRect.getX() + t * trackRect.getWidth();
			g.fillRect(juce::Rectangle<float>(tx - thick * 0.5f, trackRect.getY() - gap - len, thick, len));
			g.fillRect(juce::Rectangle<float>(tx - thick * 0.5f, trackRect.getBottom() + gap, thick, len));
		}
	}
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                                         juce::Slider &slider)
{
	auto accentColour = slider.isColourSpecified(juce::Slider::rotarySliderFillColourId)
	                        ? slider.findColour(juce::Slider::rotarySliderFillColourId)
	                        : ColourPalette::buttonPrimary;

	auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(8.0f);
	auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
	auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
	auto lineW = radius * 0.2f;
	auto arcRadius = radius - lineW * 0.5f;

	juce::Path backgroundArc;
	backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle,
	                            rotaryEndAngle, true);

	g.setColour(ColourPalette::backgroundLight);
	g.strokePath(backgroundArc,
	             juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	juce::Path valueArc;
	valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle,
	                       toAngle, true);

	g.setColour(accentColour);
	g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	juce::Path pointer;
	auto pointerLength = radius * 0.6f;
	auto pointerThickness = lineW * 1.5f;
	pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
	pointer.applyTransform(
	    juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

	g.setColour(accentColour);
	g.fillPath(pointer);
}

void CustomLookAndFeel::drawScrollbar(juce::Graphics &g, juce::ScrollBar &scrollbar, int x, int y, int width,
                                      int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown)
{
	g.setColour(scrollbar.findColour(juce::ScrollBar::backgroundColourId));
	g.fillRect((float)x, (float)y, (float)width, (float)height);

	juce::Rectangle<float> thumbBounds;

	if (isScrollbarVertical)
		thumbBounds = juce::Rectangle<float>((float)x, (float)thumbStartPosition, (float)width, (float)thumbSize);
	else
		thumbBounds = juce::Rectangle<float>((float)thumbStartPosition, (float)y, (float)thumbSize, (float)height);

	auto thumbColour = scrollbar.findColour(juce::ScrollBar::thumbColourId);

	if (isMouseDown)
		thumbColour = thumbColour.brighter(0.2f);
	else if (isMouseOver)
		thumbColour = thumbColour.brighter(0.1f);

	g.setColour(thumbColour);
	g.fillRoundedRectangle(thumbBounds, ObsidianSizes::CORNER);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics &g, int width, int height)
{
	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(0, 0, width, height);
	g.setColour(ColourPalette::lightGrey.withAlpha(0.5f));
	g.drawRect(0, 0, width, height, 1);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area, bool isSeparator,
                                          bool isActive, bool isHighlighted, bool isTicked, bool /*hasSubMenu*/,
                                          const juce::String &text, const juce::String & /*shortcutKeyText*/,
                                          const juce::Drawable * /*icon*/, const juce::Colour *textColourToUse)
{
	if (isSeparator)
	{
		auto r = area.reduced(5, 0);
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.5f));
		g.drawLine((float)r.getX(), (float)r.getCentreY(), (float)r.getRight(), (float)r.getCentreY(), 0.5f);
		return;
	}

	if (isHighlighted && isActive)
	{
		g.setColour(ColourPalette::lightGrey.withAlpha(0.15f));
		g.fillRect(area);
	}

	auto textColour = (textColourToUse != nullptr) ? *textColourToUse : ColourPalette::textPrimary;
	if (!isActive)
		textColour = textColour.withAlpha(0.4f);
	g.setFont(juce::Font(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR)));

	auto r = area.reduced(10, 0);
	if (isTicked)
	{
		auto tickArea = r.removeFromLeft(15).toFloat();
		g.setColour(ColourPalette::lightGrey);
		g.fillEllipse(tickArea.withSizeKeepingCentre(6, 6));
	}

	g.setColour(textColour);
	g.drawText(text, r, juce::Justification::centredLeft, true);
}

void CustomLookAndFeel::fillTextEditorBackground(juce::Graphics &g, int width, int height, juce::TextEditor &textEditor)
{
	auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
	const float corner = ObsidianSizes::CORNER;

	if (!textEditor.isEnabled())
	{
		g.setColour(ColourPalette::backgroundMid.withAlpha(0.5f));
		g.fillRoundedRectangle(bounds, corner);
		return;
	}

	g.setColour(juce::Colours::black.withAlpha(0.25f));
	g.fillRoundedRectangle(bounds.translated(0, 1.0f), corner);

	g.setColour(ColourPalette::backgroundMid);
	g.fillRoundedRectangle(bounds, corner);
}

void CustomLookAndFeel::drawTextEditorOutline(juce::Graphics &g, int width, int height, juce::TextEditor &textEditor)
{
	if (!textEditor.isEnabled())
		return;

	auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
	const float corner = ObsidianSizes::CORNER;

	if (textEditor.hasKeyboardFocus(true))
	{
		g.setColour(ColourPalette::lightGrey.withAlpha(0.15f));
		g.drawRoundedRectangle(bounds.expanded(1.5f), corner + 1.5f, 1.5f);

		g.setColour(ColourPalette::lightGrey);
		g.drawRoundedRectangle(bounds, corner, 1.5f);
	}
	else if (textEditor.isMouseOver(true))
	{
		g.setColour(ColourPalette::lightGrey.withAlpha(0.5f));
		g.drawRoundedRectangle(bounds, corner, 1.0f);
	}
	else
	{
		g.setColour(ColourPalette::lightGrey.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds, corner, 0.8f);
	}
}

juce::CaretComponent *CustomLookAndFeel::createCaretComponent(juce::Component *keyFocusOwner)
{
	auto *caret = new juce::CaretComponent(keyFocusOwner);
	caret->setColour(juce::CaretComponent::caretColourId, ColourPalette::lightGrey);
	return caret;
}
