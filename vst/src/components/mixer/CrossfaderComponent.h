#pragma once
#include <JuceHeader.h>
#include "midi/MidiLearnableComponents.h"
#include "components/shared/IconButton.h"
#include "style/ColourPalette.h"
#include "config/AiModelDefinitions.h"

class DjIaVstProcessor;

class CrossfaderComponent : public juce::Component, public juce::Timer
{
public:
	CrossfaderComponent(DjIaVstProcessor& processor);
	~CrossfaderComponent() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void timerCallback() override;

	double getPairValue(int pairIndex) const
	{
		if (pairIndex < 0 || pairIndex >= 4) return 0.5;
		return pairSliders[pairIndex].getValue();
	}
	double getGlobalValue() const { return globalSlider.getValue(); }

	void refreshFromProcessor();
	void updatePairColours();
	void onModelChanged() { updatePairColours(); }

	void refreshCurveButtons();

private:
	DjIaVstProcessor& audioProcessor;

	MidiLearnableSlider pairSliders[4];
	MidiLearnableSlider globalSlider;

	IconButton curveLinearButton{ "CurveLinear", "LIN" };
	IconButton curveEqualPowerButton{ "CurveEqualPower", "EQ" };
	IconButton curveDjButton{ "CurveDJ", "DJ" };

	juce::Rectangle<int> pairRowBounds[4];
	juce::Rectangle<int> globalRowBounds;
	juce::Rectangle<int> curveButtonsRowBounds;

	static constexpr int ledDiameter = 12;
	static constexpr int ledPadX = 3;
	static constexpr int labelWidth = 14;
	static constexpr int ledZoneWidth = ledPadX + ledDiameter + 4 + labelWidth + 4;

	double lastPpqForBeat = -1.0;
	float beatPhase = 0.0f;

	void setupUI();
	void setupCurveButtons();
	void setupSlider(MidiLearnableSlider& slider, const juce::String& tooltip);
	void setupMidiLearn();
	void setupCurveButtonsMidiLearn();
	void drawHardwareLED(juce::Graphics& g,
		juce::Rectangle<float> bounds,
		juce::Colour colour,
		float intensity,
		bool playing) const;
	void drawSegmentedCurveBackground(juce::Graphics& g) const;
	void updateSliderColour(MidiLearnableSlider& slider, int pairIdx);
	void selectCurveMode(int mode);
	void paintOverChildren(juce::Graphics& g) override;

	juce::String getPairMidiId(int pairIndex) const
	{
		return "crossfader_pair_" + juce::String(pairIndex);
	}
	juce::String getGlobalMidiId() const { return "crossfader_global"; }
	juce::String getCurveMidiId(int mode) const
	{
		return "crossfader_curve_" + juce::String(mode);
	}

	juce::String getPairDisplayName(int pairIndex) const
	{
		return "Crossfader " + juce::String(pairIndex + 1) + "<>" + juce::String(pairIndex + 5);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfaderComponent)
};