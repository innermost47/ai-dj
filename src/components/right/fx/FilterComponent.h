#pragma once
#include "LedRadioButton.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

using MidiLearnableLedRadioButton = MidiLearnable<LedRadioButton>;

class FilterComponent : public ObsidianBaseMidiComponent
{
  public:
	FilterComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~FilterComponent() override;

	void paint(juce::Graphics &g) override;
	void syncParams();
	void resized() override;
	void setTrackData(TrackData *trackData);
	void updateModelUI();

  protected:
	juce::String getParameterPrefix() const override
	{
		auto *t = track.get();
		if (!t || t->slotIndex == -1)
			return {};
		return "slot" + juce::String(t->slotIndex + 1);
	}

	juce::String getMidiLearnDescriptionPrefix() const override
	{
		auto *t = track.get();
		if (!t || t->slotIndex == -1)
			return {};
		return "Slot " + juce::String(t->slotIndex + 1) + " ";
	}
	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

  private:
	void wireParameters();
	void setupCutoffModeButtons();
	void setupUI();
	void refreshRadioButtonsForParam(const juce::String &paramID);

	MidiLearnableSlider cutoffKnob;
	juce::Label cutoffLabel;
	MidiLearnableSlider resonanceKnob;
	juce::Label resonanceLabel;
	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> cutoffModeButtons;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterComponent)
};