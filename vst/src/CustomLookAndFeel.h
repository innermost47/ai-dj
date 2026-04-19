#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
	static CustomLookAndFeel& getInstance()
	{
		static CustomLookAndFeel instance;
		return instance;
	}
	CustomLookAndFeel()
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
		setColour(juce::ScrollBar::thumbColourId, ColourPalette::sliderThumb);
		setColour(juce::ScrollBar::backgroundColourId, ColourPalette::backgroundDeep);
		setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
		setColour(juce::PopupMenu::backgroundColourId, ColourPalette::backgroundDark);
		setColour(juce::PopupMenu::textColourId, ColourPalette::textPrimary);
		setColour(juce::PopupMenu::highlightedBackgroundColourId, ColourPalette::buttonPrimary.withAlpha(0.4f));
		setColour(juce::PopupMenu::highlightedTextColourId, ColourPalette::textPrimary);
	}

private:
	static juce::Colour soften(const juce::Colour& colour)
	{
		return colour.withSaturation(colour.getSaturation() * 0.85f)
			.brighter(0.05f);
	}

	static juce::TextLayout layoutTooltipText(const juce::String& text, juce::Colour colour)
	{
		const float tooltipFontSize = 12.0f;
		const int maxToolTipWidth = 400;

		juce::AttributedString s;
		s.setJustification(juce::Justification::centredLeft);
		s.append(text,
			juce::Font(juce::FontOptions("Courier New", tooltipFontSize, juce::Font::plain)),
			colour);

		juce::TextLayout tl;
		tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
		return tl;
	}

public:

	juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
		juce::Point<int> screenPos,
		juce::Rectangle<int> parentArea) override
	{
		const juce::TextLayout tl(layoutTooltipText(tipText, ColourPalette::textPrimary));
		auto w = (int)(tl.getWidth() + 16.0f);
		auto h = (int)(tl.getHeight() + 10.0f);
		return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
			screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
			w, h)
			.constrainedWithin(parentArea);
	}

	void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
	{
		juce::Rectangle<float> bounds(0.0f, 0.0f, (float)width, (float)height);

		g.setColour(juce::Colours::black.withAlpha(0.4f));
		g.fillRoundedRectangle(bounds.translated(0, 2.0f), 4.0f);

		g.setColour(ColourPalette::backgroundDark);
		g.fillRoundedRectangle(bounds, 4.0f);

		g.setColour(ColourPalette::textAccent.withAlpha(0.5f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

		layoutTooltipText(text, ColourPalette::textPrimary)
			.draw(g, bounds.reduced(8.0f, 4.0f));
	}

	void drawButtonBackground(juce::Graphics& g,
		juce::Button& button,
		const juce::Colour& backgroundColour,
		bool shouldDrawButtonAsHighlighted,
		bool shouldDrawButtonAsDown) override
	{
		auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
		auto baseColour = soften(backgroundColour)
			.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

		if (shouldDrawButtonAsDown)
			baseColour = baseColour.darker(0.15f);
		else if (shouldDrawButtonAsHighlighted)
			baseColour = baseColour.brighter(0.12f);

		if (!shouldDrawButtonAsDown)
		{
			g.setColour(juce::Colours::black.withAlpha(0.3f));
			g.fillRoundedRectangle(bounds.translated(0, 1.5f), 4.0f);
		}

		g.setColour(baseColour);
		g.fillRoundedRectangle(bounds, 4.0f);

		if (!shouldDrawButtonAsDown)
		{
			g.setColour(juce::Colours::white.withAlpha(0.05f));
			auto topBounds = bounds.withHeight(bounds.getHeight() * 0.4f);
			g.fillRoundedRectangle(topBounds, 4.0f);
		}

		g.setColour(baseColour.brighter(0.2f).withAlpha(0.4f));
		g.drawRoundedRectangle(bounds, 4.0f, 0.8f);
	}

	void drawButtonText(juce::Graphics& g,
		juce::TextButton& button,
		bool /*shouldDrawButtonAsHighlighted*/,
		bool /*shouldDrawButtonAsDown*/) override
	{
		auto textColour = button.findColour(button.getToggleState()
			? juce::TextButton::textColourOnId
			: juce::TextButton::textColourOffId);

		if (!button.isEnabled())
			textColour = textColour.withAlpha(0.5f);

		g.setColour(textColour);
		g.setFont(juce::FontOptions(14.0f));

		g.drawFittedText(button.getButtonText(),
			button.getLocalBounds(),
			juce::Justification::centred,
			2,
			0.8f
		);
	}

	void drawToggleButton(juce::Graphics& g,
		juce::ToggleButton& button,
		bool shouldDrawButtonAsHighlighted,
		bool shouldDrawButtonAsDown) override
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
			g.fillRoundedRectangle(bounds.translated(0, 1.5f), 4.0f);
		}

		g.setColour(bgColour);
		g.fillRoundedRectangle(bounds, 4.0f);

		if (!shouldDrawButtonAsDown)
		{
			g.setColour(juce::Colours::white.withAlpha(0.05f));
			auto topBounds = bounds.withHeight(bounds.getHeight() * 0.4f);
			g.fillRoundedRectangle(topBounds, 4.0f);
		}

		g.setColour(bgColour.brighter(0.2f).withAlpha(0.4f));
		g.drawRoundedRectangle(bounds, 4.0f, 0.8f);

		auto textColour = button.findColour(button.getToggleState()
			? juce::TextButton::textColourOnId
			: juce::TextButton::textColourOffId);

		if (!button.isEnabled())
			textColour = textColour.withAlpha(0.5f);

		g.setColour(textColour);
		g.setFont(juce::FontOptions(14.0f));
		g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
	}

	juce::AlertWindow* createAlertWindow(
		const juce::String& title,
		const juce::String& message,
		const juce::String& button1,
		const juce::String& button2,
		const juce::String& button3,
		juce::MessageBoxIconType iconType,
		int numButtons,
		juce::Component* associatedComponent) override
	{
		auto* aw = new juce::AlertWindow(title, message, iconType, associatedComponent);

		aw->setColour(juce::AlertWindow::backgroundColourId, ColourPalette::backgroundDark);
		aw->setColour(juce::AlertWindow::textColourId, ColourPalette::textPrimary);
		aw->setColour(juce::AlertWindow::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.6f));

		return aw;
	}

	void drawAlertBox(
		juce::Graphics& g,
		juce::AlertWindow& alert,
		const juce::Rectangle<int>& textArea,
		juce::TextLayout& textLayout) override
	{
		g.fillAll(ColourPalette::backgroundDark);

		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
		g.drawRoundedRectangle(alert.getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.5f);

		auto titleBar = alert.getLocalBounds().removeFromTop(42).toFloat();
		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.2f));
		g.fillRect(titleBar);

		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
		g.drawLine(titleBar.getBottomLeft().x, titleBar.getBottom(),
			titleBar.getBottomRight().x, titleBar.getBottom(), 1.0f);

		textLayout.draw(g, textArea.toFloat());
	}

	int getAlertWindowButtonHeight() override { return 32; }

	juce::Font getAlertWindowTitleFont() override
	{
		return juce::Font(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));
	}

	juce::Font getAlertWindowMessageFont() override
	{
		return juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
	}

	juce::Font getAlertWindowFont() override
	{
		return juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
	}

	void drawComboBox(juce::Graphics& g,
		int width, int height,
		bool /*isButtonDown*/,
		int buttonX, int buttonY,
		int buttonW, int buttonH,
		juce::ComboBox& /*box*/) override
	{
		auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

		g.setColour(juce::Colours::black.withAlpha(0.3f));
		g.fillRoundedRectangle(bounds.translated(0, 1.5f), 4.0f);

		g.setColour(ColourPalette::backgroundMid);
		g.fillRoundedRectangle(bounds, 4.0f);

		g.setColour(juce::Colours::white.withAlpha(0.05f));
		auto topBounds = bounds.withHeight(bounds.getHeight() * 0.4f);
		g.fillRoundedRectangle(topBounds, 4.0f);

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.6f));
		g.drawRoundedRectangle(bounds, 4.0f, 0.8f);

		auto arrowZone = juce::Rectangle<float>((float)buttonX, (float)buttonY, (float)buttonW, (float)buttonH);
		auto arrowBounds = arrowZone.reduced(4.0f);

		juce::Path arrow;
		arrow.addTriangle(
			arrowBounds.getCentreX() - 3.0f, arrowBounds.getCentreY() - 2.0f,
			arrowBounds.getCentreX() + 3.0f, arrowBounds.getCentreY() - 2.0f,
			arrowBounds.getCentreX(), arrowBounds.getCentreY() + 2.0f);

		g.setColour(ColourPalette::textSecondary);
		g.fillPath(arrow);
	}

	void drawLabel(juce::Graphics& g, juce::Label& label) override
	{
		g.fillAll(label.findColour(juce::Label::backgroundColourId));

		if (!label.isBeingEdited())
		{
			auto alpha = label.isEnabled() ? 1.0f : 0.5f;
			auto textColour = label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha);

			g.setColour(textColour);
			g.setFont(label.getFont());

			auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

			g.drawFittedText(label.getText(),
				textArea,
				label.getJustificationType(),
				juce::jmax(1, (int)((float)textArea.getHeight() / label.getFont().getHeight())),
				1.0f);
		}
		else if (label.isEnabled())
		{
			g.setColour(label.findColour(juce::Label::outlineColourId));
		}
	}

	juce::BorderSize<int> getLabelBorderSize(juce::Label& /*label*/) override
	{
		return juce::BorderSize<int>(1, 5, 1, 5);
	}

	void drawLinearSlider(juce::Graphics& g,
		int x, int y, int width, int height,
		float sliderPos,
		float minSliderPos, float maxSliderPos,
		const juce::Slider::SliderStyle style,
		juce::Slider& /* slider */) override
	{
		if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal)
		{
			auto trackWidth = juce::jmin(6.0f, (float)(style == juce::Slider::LinearVertical ? width : height) * 0.25f);

			juce::Point<float> startPoint, endPoint;

			if (style == juce::Slider::LinearVertical)
			{
				auto cx = (float)x + (float)width * 0.5f;
				startPoint = { cx, (float)y };
				endPoint = { cx, (float)(y + height) };
			}
			else
			{
				auto cy = (float)y + (float)height * 0.5f;
				startPoint = { (float)x, cy };
				endPoint = { (float)(x + width), cy };
			}

			auto trackRect = juce::Rectangle<float>(startPoint, endPoint).expanded(trackWidth * 0.5f);

			g.setColour(ColourPalette::backgroundMid);
			g.fillRoundedRectangle(trackRect, trackWidth * 0.5f);

			g.setColour(ColourPalette::backgroundLight.withAlpha(0.8f));
			g.drawRoundedRectangle(trackRect, trackWidth * 0.5f, 0.8f);

			float denominator = maxSliderPos - minSliderPos;
			if (std::abs(denominator) > 0.001f)
			{
				float fillRatio = juce::jlimit(0.0f, 1.0f, (sliderPos - minSliderPos) / denominator);
				auto filledEnd = startPoint + (endPoint - startPoint) * fillRatio;

				if (std::isfinite(filledEnd.x) && std::isfinite(filledEnd.y))
				{
					auto fillRect = juce::Rectangle<float>(startPoint, filledEnd).expanded(trackWidth * 0.5f);

					if (fillRect.isFinite() && !fillRect.isEmpty())
					{
						g.setColour(ColourPalette::buttonPrimary);
						g.fillRoundedRectangle(fillRect, trackWidth * 0.5f);
					}
				}
			}
		}

		auto thumbWidth = 16.0f;
		g.setColour(ColourPalette::buttonPrimary);

		if (style == juce::Slider::LinearVertical)
			g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre({ (float)x + (float)width * 0.5f, sliderPos }));
		else
			g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre({ sliderPos, (float)y + (float)height * 0.5f }));

		g.setColour(juce::Colours::white.withAlpha(0.15f));
		if (style == juce::Slider::LinearVertical)
			g.fillEllipse(juce::Rectangle<float>(thumbWidth * 0.5f, thumbWidth * 0.5f)
				.withCentre({ (float)x + (float)width * 0.5f, sliderPos - thumbWidth * 0.15f }));
		else
			g.fillEllipse(juce::Rectangle<float>(thumbWidth * 0.5f, thumbWidth * 0.5f)
				.withCentre({ sliderPos, (float)y + (float)height * 0.5f - thumbWidth * 0.15f }));
	}

	void drawRotarySlider(juce::Graphics& g,
		int x, int y, int width, int height,
		float sliderPosProportional,
		float rotaryStartAngle, float rotaryEndAngle,
		juce::Slider& /* slider */) override
	{
		auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(8.0f);
		auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
		auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
		auto lineW = radius * 0.2f;
		auto arcRadius = radius - lineW * 0.5f;

		juce::Path backgroundArc;
		backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
			arcRadius, arcRadius,
			0.0f, rotaryStartAngle, rotaryEndAngle, true);

		g.setColour(ColourPalette::backgroundLight);
		g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		juce::Path valueArc;
		valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
			arcRadius, arcRadius,
			0.0f, rotaryStartAngle, toAngle, true);

		g.setColour(ColourPalette::buttonPrimary);
		g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		juce::Path pointer;
		auto pointerLength = radius * 0.6f;
		auto pointerThickness = lineW * 1.5f;
		pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
		pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

		g.setColour(ColourPalette::textPrimary);
		g.fillPath(pointer);
	}

	void drawTextEditorOutline(juce::Graphics& g,
		int width, int height,
		juce::TextEditor& textEditor) override
	{
		if (textEditor.isEnabled())
		{
			if (textEditor.hasKeyboardFocus(true))
			{
				g.setColour(ColourPalette::buttonPrimary.withAlpha(0.6f));
				g.drawRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f, 2.0f);
			}
			else
			{
				g.setColour(ColourPalette::backgroundLight);
				g.drawRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 4.0f, 1.0f);
			}
		}
	}

	void drawScrollbar(juce::Graphics& g,
		juce::ScrollBar& /*scrollbar*/,
		int x, int y,
		int width, int height,
		bool isScrollbarVertical,
		int thumbStartPosition,
		int thumbSize,
		bool isMouseOver,
		bool isMouseDown) override
	{
		g.setColour(findColour(juce::ScrollBar::backgroundColourId));
		g.fillRoundedRectangle((float)x, (float)y, (float)width, (float)height, 4.0f);

		juce::Rectangle<float> thumbBounds;

		if (isScrollbarVertical)
			thumbBounds = juce::Rectangle<float>((float)x,
				(float)thumbStartPosition,
				(float)width,
				(float)thumbSize);
		else
			thumbBounds = juce::Rectangle<float>((float)thumbStartPosition,
				(float)y,
				(float)thumbSize,
				(float)height);

		auto thumbColour = findColour(juce::ScrollBar::thumbColourId);

		if (isMouseDown)
			thumbColour = thumbColour.brighter(0.2f);
		else if (isMouseOver)
			thumbColour = thumbColour.brighter(0.1f);

		g.setColour(thumbColour);
		g.fillRoundedRectangle(thumbBounds, 4.0f);
	}

	void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
	{
		auto area = juce::Rectangle<int>(width, height).toFloat();

		g.setColour(juce::Colours::black.withAlpha(0.4f));
		g.fillRoundedRectangle(area.translated(0, 2.0f), 4.0f);

		g.setColour(ColourPalette::backgroundDark);
		g.fillRoundedRectangle(area, 4.0f);

		g.setColour(ColourPalette::textAccent.withAlpha(0.4f));
		g.drawRoundedRectangle(area.reduced(0.5f), 4.0f, 1.0f);
	}

	void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
		bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
		const juce::String& text, const juce::String& shortcutKeyText,
		const juce::Drawable* icon, const juce::Colour* textColourToUse) override
	{
		if (isSeparator)
		{
			auto r = area.reduced(5, 0);
			g.setColour(ColourPalette::backgroundLight.withAlpha(0.5f));
			g.drawLine((float)r.getX(), (float)r.getCentreY(), (float)r.getRight(), (float)r.getCentreY(), 0.5f);
			return;
		}

		auto itemArea = area.toFloat().reduced(2.0f);

		if (isHighlighted && isActive)
		{
			g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
			g.fillRoundedRectangle(itemArea, 3.0f);
		}

		auto textColour = (textColourToUse != nullptr) ? *textColourToUse : ColourPalette::textPrimary;
		if (!isActive)
			textColour = textColour.withAlpha(0.4f);

		g.setColour(textColour);
		g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

		auto r = area.reduced(10, 0);

		if (isTicked)
		{
			auto tickArea = r.removeFromLeft(15).toFloat();
			g.setColour(ColourPalette::buttonPrimary);
			g.fillEllipse(tickArea.withSizeKeepingCentre(6, 6));
		}

		g.setColour(textColour);
		g.drawText(text, r, juce::Justification::centredLeft, true);
	}
};