#pragma once
#include "DataConst.h"
#include "LedRadioButton.h"
#include "MidiLearnableComponents.h"
#include "ModulationTypes.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
using MidiLearnableLedRadioButton = MidiLearnable<LedRadioButton>;

class ModulatorRow : public ObsidianBaseMidiComponent
{
  public:
	ModulatorRow(DjIaVstProcessor &processor, int modIndex);
	~ModulatorRow() override;

	void setTrackData(TrackData *trackData);
	void refreshFromTrack();
	void updateModulationDisplay();
	void updateModelUI();

	int getRequiredHeight() const
	{
		return 108;
	}

	std::function<void()> onContentChanged;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void refreshTargetAvailability();

  protected:
	juce::String getParameterPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "slot" + juce::String(t->slotIndex + 1);
	}

	juce::String getMidiLearnDescriptionPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "Slot " + juce::String(t->slotIndex + 1) + " ";
	}

	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

  private:
	void setupUI();
	void setupShapeButtons();
	void setupRateButtons();
	void wireParameters();
	void refreshRadioButtonsForParam(const juce::String &paramSuffix,
	                                 std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> &buttons);
	juce::String suffixFor(const juce::String &name) const;

	int index;

	IconButton activeButton{"ModActive", ""};
	IconButton bipolarButton{"ModBipolar", ""};
	juce::Label componentLabel;

	juce::ComboBox targetSelector;

	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> shapeButtons;
	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> rateButtons;

	MidiLearnableSlider depthKnob;
	MidiLearnableSlider phaseKnob;
	juce::Label depthLabel;
	juce::Label phaseLabel;

	float lastDisplayedValue = 0.0f;

	inline juce::StringArray getModShapeNames()
	{
		return juce::StringArray{"Sin", "Tri", "Saw", "Sqr", "Rmp", "S&H"};
	}

	inline juce::StringArray getModRateNames()
	{
		return juce::StringArray{"4B", "2B", "1B", "1/2", "1/4", "1/8", "1/16"};
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorRow)
};