#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class GateComponent : public ObsidianBaseMidiComponent
{
  public:
	GateComponent(DjIaVstProcessor &processor, TrackData *trackData);
	~GateComponent() override;
	void paint(juce::Graphics &g) override;
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
	static juce::String getRateName(int value);
	void onRateChanged();

	MidiLearnableSlider rateKnob;
	MidiLearnableSlider durationKnob;
	MidiLearnableSlider depthKnob;
	juce::Label rateLabel;
	juce::Label durationLabel;
	juce::Label depthLabel;
	juce::Label componentLabel;
	IconButton bypassGateButton{"BypassGate", ""};

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;
	void handleVBlank();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateComponent)
};