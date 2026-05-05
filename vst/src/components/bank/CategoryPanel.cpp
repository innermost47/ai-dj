#pragma once
#include <JuceHeader.h>
#include "CategoryPanel.h"


CategoryTag::CategoryTag(const juce::String& name) : juce::Button(name)
{
	setClickingTogglesState(true);
}

void CategoryTag::paintButton(juce::Graphics& g, bool isMouseOverButton, bool /*isButtonDown*/)
{
	auto bounds = getLocalBounds().toFloat().reduced(2.0f);
	bool active = getToggleState();

	g.setColour(active ? ColourPalette::buttonPrimary : ColourPalette::backgroundLight.withAlpha(0.4f));
	g.fillRoundedRectangle(bounds, bounds.getHeight() / 2.0f);
	if (isMouseOverButton && !active)
	{
		g.setColour(ColourPalette::textPrimary.withAlpha(0.2f));
		g.drawRoundedRectangle(bounds, bounds.getHeight() / 2.0f, 1.5f);
	}
	g.setColour(active ? juce::Colours::white : ColourPalette::textSecondary);
	g.setFont(juce::FontOptions("Courier New", 14.0f, active ? juce::Font::bold : juce::Font::plain));
	g.drawText(getButtonText(), bounds, juce::Justification::centred, true);
}

CategoryPanel::CategoryPanel(const std::vector<juce::String>& currentCategories,
	const std::vector<juce::String>& availableCategories)
{
	toggleContainer = std::make_unique<juce::Component>();
	viewport.setViewedComponent(toggleContainer.get(), false);
	viewport.setScrollBarsShown(true, false);
	addAndMakeVisible(viewport);

	for (const auto& category : availableCategories)
	{
		auto* tag = tags.add(new CategoryTag(category));

		bool isAssigned = std::find(currentCategories.begin(), currentCategories.end(), category) != currentCategories.end();
		tag->setToggleState(isAssigned, juce::dontSendNotification);

		toggleContainer->addAndMakeVisible(tag);
	}
}

void CategoryPanel::clearAll()
{
	for (auto* tag : tags)
		tag->setToggleState(false, juce::dontSendNotification);
}

std::vector<juce::String> CategoryPanel::getSelectedCategories() const
{
	std::vector<juce::String> selected;
	for (auto* tag : tags)
		if (tag->getToggleState())
			selected.push_back(tag->getButtonText());
	return selected;
}

void CategoryPanel::resized()
{
	auto bounds = getLocalBounds().reduced(10);
	viewport.setBounds(bounds);

	juce::FlexBox fb;
	fb.flexWrap = juce::FlexBox::Wrap::wrap;
	fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	fb.alignContent = juce::FlexBox::AlignContent::flexStart;

	juce::FontOptions fontOptions("Courier New", 14.0f, juce::Font::plain);
	juce::Font tagFont(fontOptions);

	for (auto* tag : tags)
	{
		juce::GlyphArrangement ga;
		ga.addLineOfText(tagFont, tag->getButtonText(), 0.0f, 0.0f);

		float textWidth = ga.getBoundingBox(0, -1, true).getWidth();

		fb.items.add(juce::FlexItem(*tag)
			.withWidth(textWidth + 30.0f)
			.withHeight(32.0f)
			.withMargin(juce::FlexItem::Margin(5)));
	}

	float width = (float)viewport.getMaximumVisibleWidth() - 15.0f;
	fb.performLayout(juce::Rectangle<float>(0, 0, width, 10000.0f));

	float maxBottom = 0.0f;
	for (auto* tag : tags)
	{
		float bottomPos = (float)tag->getBoundsInParent().getBottom();
		maxBottom = juce::jmax(maxBottom, bottomPos);
	}

	toggleContainer->setSize((int)width, (int)maxBottom + 10);
}
