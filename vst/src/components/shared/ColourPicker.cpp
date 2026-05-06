#include "ColourPicker.h"

ColourPicker::ColourPicker()
{
	setPalette(getDefaultPalette());
}

void ColourPicker::setPalette(const juce::Array<juce::Colour> &colours)
{
	swatches.clear();
	for (auto c : colours)
	{
		auto *s = swatches.add(new Swatch(c));
		s->onPicked = [this, s](juce::Colour picked)
		{
			setSelectedColour(picked);
			if (onColourChanged)
				onColourChanged(picked);
		};
		addAndMakeVisible(s);
	}
	updateSelectionMarks();
	resized();
}

void ColourPicker::setRows(int newRowCount)
{
	rows = juce::jmax(1, newRowCount);
	resized();
}

void ColourPicker::setSelectedColour(juce::Colour c)
{
	selectedColour = c;
	updateSelectionMarks();
}

void ColourPicker::resized()
{
	const int count = swatches.size();
	if (count == 0)
		return;

	const int cols = (count + rows - 1) / rows;
	auto area = getLocalBounds();

	const int totalGapX = gap * (cols - 1);
	const int totalGapY = gap * (rows - 1);
	const int availableW = area.getWidth() - totalGapX;
	const int availableH = area.getHeight() - totalGapY;

	const int baseW = juce::jmax(8, availableW / cols);
	const int baseH = juce::jmax(8, availableH / rows);
	const int extraW = availableW - baseW * cols;
	const int extraH = availableH - baseH * rows;

	for (int i = 0; i < count; ++i)
	{
		const int row = i / cols;
		const int col = i % cols;

		const int sw = baseW + (col < extraW ? 1 : 0);
		const int sh = baseH + (row < extraH ? 1 : 0);

		const int xOffset = col * baseW + juce::jmin(col, extraW) + col * gap;
		const int yOffset = row * baseH + juce::jmin(row, extraH) + row * gap;

		swatches[i]->setBounds(area.getX() + xOffset, area.getY() + yOffset, sw, sh);
	}
}

juce::Array<juce::Colour> ColourPicker::getDefaultPalette()
{
	return {juce::Colour(0xffe63946), juce::Colour(0xffff6b6b), juce::Colour(0xfff4845f), juce::Colour(0xffffb627),
	        juce::Colour(0xfff7d060), juce::Colour(0xffa8e6cf), juce::Colour(0xff4ecdc4), juce::Colour(0xff06b6d4),
	        juce::Colour(0xff3a86ff), juce::Colour(0xff6c63ff), juce::Colour(0xff8338ec), juce::Colour(0xffc04fda),
	        juce::Colour(0xffff70a6), juce::Colour(0xff70c1b3), juce::Colour(0xff8d99ae), juce::Colour(0xffb08968),
	        juce::Colour(0xff84a98c), juce::Colour(0xff2a9d8f)};
}

void ColourPicker::updateSelectionMarks()
{
	for (auto *sw : swatches)
	{
		const bool wasSelected = sw->selected;
		sw->selected = (sw->colour == selectedColour);
		if (wasSelected != sw->selected)
			sw->repaint();
	}
}