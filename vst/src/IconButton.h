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

	void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
	{
		auto fullBounds = getLocalBounds().toFloat();
		const float cornerSize = 4.0f;
		float iconRatio = isCompact ? 0.90f : 0.65f;

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

		if (hasAccentBar && getToggleState() && isEnabled())
		{
			const float barHeight = 3.0f;
			const float barInset = 4.0f;

			juce::Rectangle<float> barRect(
				fullBounds.getX() + barInset,
				fullBounds.getBottom() - barHeight - 2.0f,
				fullBounds.getWidth() - 2.0f * barInset,
				barHeight);

			g.setColour(ColourPalette::buttonPrimary);
			g.fillRoundedRectangle(barRect, barHeight * 0.5f);
		}

		auto iconArea = fullBounds.removeFromTop(fullBounds.getHeight() * iconRatio);
		auto labelArea = fullBounds;

		const juce::Path& pathToDraw = (getToggleState() && hasToggledIcon)
			? iconPathToggled : iconPath;

		if (!pathToDraw.isEmpty())
		{
			auto iconBounds = iconArea.reduced(iconArea.getWidth() * 0.22f);
			juce::Path scaled = pathToDraw;
			scaled.scaleToFit(iconBounds.getX(), iconBounds.getY(),
				iconBounds.getWidth(), iconBounds.getHeight(), true);

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
};