#pragma once
#include "LedRadioButton.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

using MidiLearnableLedRadioButton = MidiLearnable<LedRadioButton>;

class SendsPanel : public ObsidianBaseMidiComponent
{
  public:
	SendsPanel(DjIaVstProcessor &processor);
	~SendsPanel() override = default;

	void paint(juce::Graphics &g) override;
	void syncParams();
	void resized() override;

  protected:
	juce::String getMidiLearnDescriptionPrefix() const override
	{
		return {};
	}
	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

  private:
	void wireParameters();
	void setupDelayDivisionButtons();
	void setupDelayModeButtons();
	void styleKnob(juce::Slider &s, juce::Colour col);
	void setupUI();

	void refreshRadioButtonsForParam(const juce::String &paramID);

	juce::Label delayTitleLbl;
	juce::Label delayTimeLbl, delayFeedbackLbl, delayModeLbl;
	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> delayDivisionButtons;
	std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> delayModeButtons;
	MidiLearnableSlider delayFeedbackKnob;

	juce::Label reverbTitleLbl;
	juce::Label reverbSizeLbl, reverbDampingLbl, reverbWidthLbl, reverbMixLbl;
	MidiLearnableSlider reverbSizeKnob, reverbDampingKnob, reverbWidthKnob, reverbMixKnob;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendsPanel)
};