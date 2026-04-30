#pragma once
#include <JuceHeader.h>
#include "midi/MidiLearnableComponents.h"
#include "style/ColourPalette.h"
#include "config/AiModelDefinitions.h"

class DjIaVstProcessor;

class CrossfaderComponent : public juce::Component
{
public:
	CrossfaderComponent(DjIaVstProcessor& processor);
	~CrossfaderComponent() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

	double getPairValue(int pairIndex) const
	{
		if (pairIndex < 0 || pairIndex >= 4) return 0.5;
		return pairSliders[pairIndex].getValue();
	}
	double getGlobalValue() const { return globalSlider.getValue(); }

	void refreshFromProcessor();
	void updatePairColours();
	void onModelChanged() { updatePairColours(); }

private:
	DjIaVstProcessor& audioProcessor;

	MidiLearnableSlider pairSliders[4];
	MidiLearnableSlider globalSlider;

	juce::Rectangle<int> pairRowBounds[4];
	juce::Rectangle<int> globalRowBounds;

	void setupUI();
	void setupSlider(MidiLearnableSlider& slider, const juce::String& tooltip);
	void setupMidiLearn();
	void updateSliderColour(MidiLearnableSlider& slider, int pairIdx);
	void paintOverChildren(juce::Graphics& g) override;

	juce::String getPairMidiId(int pairIndex) const
	{
		return "crossfader_pair_" + juce::String(pairIndex);
	}
	juce::String getGlobalMidiId() const { return "crossfader_global"; }
	juce::String getPairDisplayName(int pairIndex) const
	{
		return "Crossfader " + juce::String(pairIndex + 1) + "<>" + juce::String(pairIndex + 5);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfaderComponent)
};