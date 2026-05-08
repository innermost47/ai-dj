#pragma once
#include "ColourPalette.h"
#include "LedRadioButton.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class SendsPanel : public juce::Component, private juce::AudioProcessorValueTreeState::Listener
{
  public:
	SendsPanel(DjIaVstProcessor &processor);
	~SendsPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void parameterChanged(const juce::String &parameterID, float newValue) override;

  private:
	DjIaVstProcessor &audioProcessor;

	juce::Label delayTitleLbl;
	juce::Label delayTimeLbl, delayFeedbackLbl, delayModeLbl;
	std::vector<std::unique_ptr<LedRadioButton>> delayDivisionButtons;
	std::vector<std::unique_ptr<LedRadioButton>> delayModeButtons;
	juce::Slider delayFeedbackKnob;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;

	juce::Label reverbTitleLbl;
	juce::Label reverbSizeLbl, reverbDampingLbl, reverbWidthLbl, reverbMixLbl;
	juce::Slider reverbSizeKnob, reverbDampingKnob, reverbWidthKnob, reverbMixKnob;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSizeAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;

	void setupDelayDivisionButtons();
	void setupDelayModeButtons();
	void styleKnob(juce::Slider &s, juce::Colour col);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendsPanel)
};