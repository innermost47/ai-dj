#pragma once
#include <JuceHeader.h>
#include "MidiLearnableComponents.h"
#include "ColourPalette.h"

class IconButton : public MidiLearnableButton
{
public:
	IconButton(const juce::String& name, const juce::String& label)
		: MidiLearnableButton(),
		labelText(label)
	{
		setName(name);
		setButtonText(label);
	}

	void setIconPath(const juce::Path& path) { iconPath = path; repaint(); }
	void setLabelText(const juce::String& text) { labelText = text; repaint(); }
	void setCompactMode(bool compact) { isCompact = compact; repaint(); }
	void setIconPathToggled(const juce::Path& path) { iconPathToggled = path; repaint(); }
	void setHasToggledIcon(bool has) { hasToggledIcon = has; repaint(); }
	void setHasAccentBar(bool has) { hasAccentBar = has; repaint(); }
	void setShowBackground(bool show) { showBackground = show; }

	void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
	{
		auto fullBounds = getLocalBounds().toFloat();
		const float cornerSize = 4.0f;

		if (showBackground)
		{
			juce::Colour bgColour = getToggleState()
				? findColour(juce::TextButton::buttonOnColourId)
				: findColour(juce::TextButton::buttonColourId);
			if (!isEnabled())
				bgColour = bgColour.withMultipliedAlpha(0.4f);
			else if (isButtonDown)
				bgColour = bgColour.darker(0.08f);
			else if (isMouseOver)
				bgColour = bgColour.darker(0.03f);
			g.setColour(bgColour);
			g.fillRoundedRectangle(fullBounds.reduced(1.0f), cornerSize);
			g.setColour(ColourPalette::backgroundLight.darker(0.15f).withAlpha(isEnabled() ? 0.8f : 0.3f));
			g.drawRoundedRectangle(fullBounds.reduced(1.0f), cornerSize, 1.0f);
		}

		const float topPadding = 3.0f;
		const float bottomPadding = 2.0f;

		const float accentBarSlot = hasAccentBar ? 3.0f : 0.0f;
		const float accentGap = hasAccentBar ? 2.0f : 0.0f;
		const bool  drawAccentBar = hasAccentBar && getToggleState() && isEnabled();

		const float labelHeight = labelText.isNotEmpty() ? (isCompact ? 8.0f : 10.0f) : 0.0f;
		const float labelGap = (labelHeight > 0.0f) ? 2.0f : 0.0f;

		auto contentArea = fullBounds.reduced(2.0f, 0.0f);
		contentArea.removeFromTop(topPadding);
		contentArea.removeFromBottom(bottomPadding);

		if (accentBarSlot > 0.0f)
		{
			auto barArea = contentArea.removeFromBottom(accentBarSlot);
			if (drawAccentBar)
			{
				const float barInset = 4.0f;
				juce::Rectangle<float> barRect(
					barArea.getX() + barInset,
					barArea.getY(),
					barArea.getWidth() - 2.0f * barInset,
					accentBarSlot);
				g.setColour(ColourPalette::buttonPrimary);
				g.fillRoundedRectangle(barRect, accentBarSlot * 0.5f);
			}
			contentArea.removeFromBottom(accentGap);
		}

		juce::Rectangle<float> labelArea;
		if (labelHeight > 0.0f)
		{
			labelArea = contentArea.removeFromBottom(labelHeight);
			contentArea.removeFromBottom(labelGap);
		}

		auto iconArea = contentArea;

		const juce::Path& pathToDraw = (getToggleState() && hasToggledIcon)
			? iconPathToggled : iconPath;

		if (!pathToDraw.isEmpty() && iconArea.getHeight() > 2.0f)
		{
			const float marginH = isCompact ? 0.10f : 0.18f;
			const float marginV = isCompact ? 0.05f : 0.10f;

			auto iconBounds = iconArea.reduced(iconArea.getWidth() * marginH,
				iconArea.getHeight() * marginV);

			float side = std::min(iconBounds.getWidth(), iconBounds.getHeight());
			juce::Rectangle<float> squareIconBounds(
				iconBounds.getCentreX() - side * 0.5f,
				iconBounds.getCentreY() - side * 0.5f,
				side, side);

			juce::Path scaled = pathToDraw;
			scaled.scaleToFit(squareIconBounds.getX(), squareIconBounds.getY(),
				squareIconBounds.getWidth(), squareIconBounds.getHeight(), true);

			juce::Colour iconColour = isEnabled()
				? (getToggleState() ? findColour(juce::TextButton::textColourOnId)
					: findColour(juce::TextButton::textColourOffId))
				: ColourPalette::textSecondary.withAlpha(0.3f);

			g.setColour(iconColour);
			g.strokePath(scaled, juce::PathStrokeType(1.6f,
				juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
		}

		if (labelText.isNotEmpty())
		{
			juce::Colour labelCol = getToggleState()
				? findColour(juce::TextButton::textColourOnId)
				: findColour(juce::TextButton::textColourOffId);

			g.setColour(isEnabled() ? labelCol : labelCol.withAlpha(0.3f));
			g.setFont(juce::FontOptions(isCompact ? 6.5f : 8.5f, juce::Font::bold));
			g.drawText(labelText, labelArea.toNearestInt(),
				juce::Justification::centred, false);
		}
	}

private:
	juce::Path iconPath;
	juce::Path iconPathToggled;
	juce::String labelText;
	bool hasToggledIcon = false;
	bool isCompact = false;
	bool hasAccentBar = false;
	bool showBackground = true;
};