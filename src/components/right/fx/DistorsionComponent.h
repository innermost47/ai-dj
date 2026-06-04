#pragma once
#include "LedRadioButton.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

using MidiLearnableLedRadioButton = MidiLearnable<LedRadioButton>;

class DistorsionComponent : public ObsidianBaseMidiComponent
{
  public:
	DistorsionComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~DistorsionComponent() override;

	void paint(juce::Graphics &g) override;
	void syncParams();
	void resized() override;
	void setTrackData(TrackData *trackData);
	void updateModelUI();
	void wireParameters();
	void setupUI();

	juce::String getTrackId() const
	{
		auto *t = track.get();
		if (t)
			return t->trackId;
		return "None";
	}

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
	void setupDistorsionTypeButtons();
	void refreshRadioButtonsForParam(const juce::String &paramSuffix);

	MidiLearnableSlider preGainKnob;
	MidiLearnableSlider postGainKnob;
	MidiLearnableSlider cutKnob;

	juce::Label preGainLabel;
	juce::Label postGainLabel;
	juce::Label cutLabel;

	juce::Label componentLabel;

	IconButton bypassDistorsionButton{"BypassDistorsion", ""};

	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> distorsionTypeButtons;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistorsionComponent)
};