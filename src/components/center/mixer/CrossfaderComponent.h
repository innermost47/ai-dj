#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class CrossfaderComponent : public ObsidianBaseMidiComponent, public juce::Timer
{
  public:
	CrossfaderComponent(DjIaVstProcessor &processor);
	~CrossfaderComponent() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void timerCallback() override;

	void refreshFromProcessor();
	void updatePairColours();
	void onModelChanged()
	{
		updatePairColours();
	}

	void refreshCurveButtons();

  private:
	MidiLearnableSlider pairSliders[4];
	MidiLearnableSlider globalSlider;

	IconButton curveLinearButton{"CurveLinear", "LIN"};
	IconButton curveEqualPowerButton{"CurveEqualPower", "EQ"};
	IconButton curveDjButton{"CurveDJ", "DJ"};

	IconButton useCrossfaderButton{"UseCrossfader", ""};

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
	void setupSlider(MidiLearnableSlider &slider, const juce::String &tooltip);
	void drawHardwareLED(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour colour, float intensity,
	                     bool playing) const;
	void drawSegmentedCurveBackground(juce::Graphics &g) const;
	void updateSliderColour(MidiLearnableSlider &slider, int pairIdx);
	void selectCurveMode(int mode);
	void paintOverChildren(juce::Graphics &g) override;
	void wireParameters();

  protected:
	juce::String getMidiLearnDescriptionPrefix() const override
	{
		return {};
	}

	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossfaderComponent)
};