#pragma once
#include <JuceHeader.h>

class ColourPicker : public juce::Component
{
public:
	std::function<void(juce::Colour)> onColourChanged;

	ColourPicker()
	{
		setPalette(getDefaultPalette());
	}

	void setPalette(const juce::Array<juce::Colour>& colours)
	{
		swatches.clear();
		for (auto c : colours)
		{
			auto* s = swatches.add(new Swatch(c));
			s->onPicked = [this, s](juce::Colour picked)
				{
					setSelectedColour(picked);
					if (onColourChanged) onColourChanged(picked);
				};
			addAndMakeVisible(s);
		}
		updateSelectionMarks();
		resized();
	}

	void setRows(int newRowCount)
	{
		rows = juce::jmax(1, newRowCount);
		resized();
	}

	void setSelectedColour(juce::Colour c)
	{
		selectedColour = c;
		updateSelectionMarks();
	}

	juce::Colour getSelectedColour() const noexcept { return selectedColour; }

	int getPreferredHeight() const noexcept
	{
		return rows * minSwatchSize + (rows - 1) * gap;
	}

	void resized() override
	{
		const int count = swatches.size();
		if (count == 0) return;

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

			swatches[i]->setBounds(
				area.getX() + xOffset,
				area.getY() + yOffset,
				sw,
				sh);
		}
	}

	static juce::Array<juce::Colour> getDefaultPalette()
	{
		return {
			juce::Colour(0xffe63946),
			juce::Colour(0xffff6b6b),
			juce::Colour(0xfff4845f),
			juce::Colour(0xffffb627),
			juce::Colour(0xfff7d060),
			juce::Colour(0xffa8e6cf),
			juce::Colour(0xff4ecdc4),
			juce::Colour(0xff06b6d4),
			juce::Colour(0xff3a86ff),
			juce::Colour(0xff6c63ff),
			juce::Colour(0xff8338ec),
			juce::Colour(0xffc04fda),
			juce::Colour(0xffff70a6),
			juce::Colour(0xff70c1b3),
			juce::Colour(0xff8d99ae),
			juce::Colour(0xffb08968),
			juce::Colour(0xff84a98c),
			juce::Colour(0xff2a9d8f)
		};
	}

private:
	struct Swatch : public juce::Component
	{
		juce::Colour colour;
		bool selected = false;
		std::function<void(juce::Colour)> onPicked;

		Swatch(juce::Colour c) : colour(c)
		{
			setMouseCursor(juce::MouseCursor::PointingHandCursor);
		}

		void paint(juce::Graphics& g) override
		{
			auto bounds = getLocalBounds().toFloat().reduced(2.0f);
			g.setColour(colour);
			g.fillRoundedRectangle(bounds, 4.0f);

			if (selected)
			{
				g.setColour(juce::Colours::white);
				g.drawRoundedRectangle(bounds.expanded(1.5f), 5.0f, 2.0f);
			}
		}

		void mouseDown(const juce::MouseEvent&) override
		{
			if (onPicked) onPicked(colour);
		}
	};

	void updateSelectionMarks()
	{
		for (auto* sw : swatches)
		{
			const bool wasSelected = sw->selected;
			sw->selected = (sw->colour == selectedColour);
			if (wasSelected != sw->selected) sw->repaint();
		}
	}

	juce::OwnedArray<Swatch> swatches;
	juce::Colour selectedColour{ 0xff6c63ff };
	int rows = 2;

	static constexpr int minSwatchSize = 32;
	static constexpr int gap = 4;
};