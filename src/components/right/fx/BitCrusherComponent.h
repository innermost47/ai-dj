#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>
class DjIaVstProcessor;
class BitCrusherComponent : public ObsidianBaseMidiComponent
{
  public:
	BitCrusherComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~BitCrusherComponent() override;
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
	MidiLearnableSlider bitDepthKnob;
	MidiLearnableSlider sampleRateReductionKnob;
	MidiLearnableSlider mixKnob;

	juce::Label bitDepthLabel;
	juce::Label sampleRateReductionLabel;
	juce::Label mixLabel;
	juce::Label componentLabel;

	IconButton bypassBitCrusherButton{"BypassBitCrusher", ""};
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BitCrusherComponent)
};