#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class PhaserComponent : public ObsidianBaseMidiComponent
{
  public:
	PhaserComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~PhaserComponent() override;

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
	MidiLearnableSlider rateKnob;
	MidiLearnableSlider depthKnob;
	MidiLearnableSlider centreKnob;
	MidiLearnableSlider feedbackKnob;
	MidiLearnableSlider mixKnob;

	juce::Label rateLabel;
	juce::Label depthLabel;
	juce::Label centreLabel;
	juce::Label feedbackLabel;
	juce::Label mixLabel;

	juce::Label componentLabel;

	IconButton bypassPhaserButton{"BypassPhaser", ""};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserComponent)
};