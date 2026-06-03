#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class EqualizerComponent : public ObsidianBaseMidiComponent
{
  public:
	EqualizerComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~EqualizerComponent() override;

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
	MidiLearnableSlider subBassSlider;
	MidiLearnableSlider bassSlider;
	MidiLearnableSlider lowMidSlider;
	MidiLearnableSlider midSlider;
	MidiLearnableSlider highMidSlider;
	MidiLearnableSlider presenceSlider;
	MidiLearnableSlider highSlider;
	MidiLearnableSlider airSlider;

	juce::Label subBassLabel;
	juce::Label bassLabel;
	juce::Label lowMidLabel;
	juce::Label midLabel;
	juce::Label highMidLabel;
	juce::Label presenceLabel;
	juce::Label highLabel;
	juce::Label airLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerComponent)
};