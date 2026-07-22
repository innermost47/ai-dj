#include "ObsidianBase.h"
#include "AiModelDefinitions.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "Fonts.h"
#include "IconButton.h"
#include "Shades.h"
#include "Sizes.h"
#include <JuceHeader.h>

std::unique_ptr<juce::AccessibilityHandler> ObsidianComponent::createAccessibilityHandler()
{
	return createIgnoredAccessibilityHandler(*this);
}

void ObsidianComponent::paintBaseRoundedBackground(juce::Graphics &g, juce::Colour colour)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(colour);
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds, Obsidian::CORNER, Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseRoundedBackgroundMidWithAlpha06(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundMid.withAlpha(Obsidian::ALPHA_06));
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds, Obsidian::CORNER, Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseLocalBackground(juce::Graphics &g, juce::Rectangle<int> bounds)
{
	g.setColour(ColourPalette::backgroundDark);
	g.fillAll();
	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRect(bounds.toFloat(), 1);
}

void ObsidianComponent::paintBaseBackgroundWithLeftBorder(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(bounds);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawLine(0.0f, 0.0f, 0.0f, bounds.getHeight(), Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::paintBaseBackgroundWithRightBorder(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(bounds);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawLine(bounds.getWidth(), 0.0f, bounds.getWidth(), bounds.getHeight(), Obsidian::BORDER_WIDTH);
}

void ObsidianComponent::drawCircleWithEllipse(juce::Graphics &g, juce::Rectangle<int> area, juce::Colour colour)
{
	auto circleArea = area.removeFromLeft(14);
	auto circleRect = circleArea.withSizeKeepingCentre(7, 7).toFloat();

	g.setColour(colour);
	g.fillEllipse(circleRect);

	g.setColour(colour.withAlpha(0.3f));
	g.drawEllipse(circleRect.expanded(1.5f), 1.0f);
}

juce::String ObsidianComponent::makePromptDisplayLabel(const juce::String &fullPrompt)
{
	juce::String s = fullPrompt.trim();

	{
		bool stripped = true;
		while (stripped)
		{
			stripped = false;
			for (auto prefix : {"TrackType:", "Format:", "VocalType:"})
			{
				if (s.startsWith(prefix))
				{
					s = s.fromFirstOccurrenceOf(",", false, false).trim();
					stripped = true;
				}
			}
		}
	}

	if (fullPrompt.contains(" | ") && fullPrompt.startsWith("Format:"))
	{
		juce::StringArray parts;
		parts.addTokens(fullPrompt, "|", "");
		juce::StringArray kept;
		for (auto p : parts)
		{
			p = p.trim();
			for (auto key : {"Instruments:", "Moods:", "Styles:"})
			{
				if (p.startsWith(key))
				{
					juce::String val = p.fromFirstOccurrenceOf(":", false, false).trim();
					if (val.isNotEmpty())
						kept.add(val);
				}
			}
		}
		if (!kept.isEmpty())
			return kept.joinIntoString(", ");
	}

	if (s.startsWith("Solo,") || s.startsWith("Full Beat,"))
		s = s.fromFirstOccurrenceOf(",", false, false).trim();
	if (s.startsWith("Instruments: drum,"))
		s = s.fromFirstOccurrenceOf(",", false, false).trim();
	else if (s.startsWith("Instruments:"))
		s = s.fromFirstOccurrenceOf(":", false, false).trim();

	return s;
}

void ObsidianComponent::setupTabButton(IconButtonSimple &btn, std::function<void()> callback)
{
	btn.setClickingTogglesState(true);
	btn.setRadioGroupId(0xFACE);
	btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
	btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::indigo);
	btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	btn.setColour(juce::TextButton::textColourOnId, ColourPalette::backgroundDeep);
	btn.onClick = [callback]() { callback(); };
	addAndMakeVisible(btn);
}
