#include "GlitchLegend.h"
#include "GlitchStepButton.h"

GlitchLegend::GlitchLegend()
{
	for (int i = 1; i < static_cast<int>(GlitchEffectType::Count); ++i)
	{
		auto type = static_cast<GlitchEffectType>(i);
		items.push_back({type, {}, {}});
	}
}

void GlitchLegend::resized()
{
	auto area = getLocalBounds().reduced(4, 2);
	int itemCount = static_cast<int>(items.size());
	if (itemCount == 0)
		return;

	int perRow = (itemCount + 1) / 2;
	int rowHeight = area.getHeight() / 2;
	int itemWidth = area.getWidth() / juce::jmax(1, perRow);

	for (int i = 0; i < itemCount; ++i)
	{
		int row = i / perRow;
		int col = i % perRow;
		juce::Rectangle<int> cell(area.getX() + col * itemWidth, area.getY() + row * rowHeight, itemWidth, rowHeight);
		items[i].swatchBounds = cell.removeFromLeft(9).reduced(0, 3);
		cell.removeFromLeft(3);
		items[i].textBounds = cell;
	}
}

void GlitchLegend::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_02));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);

	g.setFont(juce::FontOptions(10.0f));
	for (auto &item : items)
	{
		g.setColour(GlitchStepButton::getColourForType(item.type));
		g.fillRoundedRectangle(item.swatchBounds.toFloat(), 2.0f);
		g.setColour(ColourPalette::textPrimary);
		g.drawFittedText(getGlitchEffectShortName(item.type), item.textBounds, juce::Justification::centredLeft, 1,
		                 0.9f);
	}
}