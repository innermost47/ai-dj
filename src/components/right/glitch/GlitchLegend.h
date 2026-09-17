#pragma once
#include "GlitchTypes.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class GlitchLegend : public ObsidianComponent
{
  public:
	GlitchLegend();
	void paint(juce::Graphics &g) override;
	void resized() override;

  private:
	struct LegendItem
	{
		GlitchEffectType type;
		juce::Rectangle<int> swatchBounds;
		juce::Rectangle<int> textBounds;
	};

	std::vector<LegendItem> items;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchLegend)
};