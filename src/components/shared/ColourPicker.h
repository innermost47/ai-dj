#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class ColourPicker : public ObsidianComponent
{
  public:
	std::function<void(juce::Colour)> onColourChanged;

	ColourPicker();
	void setPalette(const juce::Array<juce::Colour> &colours);
	void setSelectedColour(juce::Colour c);
	juce::Colour getSelectedColour() const noexcept
	{
		return selectedColour;
	}

	int getPreferredHeight() const noexcept
	{
		return rows * minSwatchSize + (rows - 1) * gap;
	}

	void resized() override;
	static juce::Array<juce::Colour> getDefaultPalette();

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

		void paint(juce::Graphics &g) override
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

		void mouseDown(const juce::MouseEvent &) override
		{
			if (onPicked)
				onPicked(colour);
		}
	};

	void updateSelectionMarks();

	juce::OwnedArray<Swatch> swatches;
	juce::Colour selectedColour{0xff6c63ff};
	int rows = 2;

	static constexpr int minSwatchSize = 32;
	static constexpr int gap = 4;
};