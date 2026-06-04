#include "EqualizerComponent.h"
#include "CustomLookAndFeel.h"
#include "PluginProcessor.h"

EqualizerComponent::EqualizerComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

EqualizerComponent::~EqualizerComponent()
{
	markForDestruction();
}

void EqualizerComponent::paint(juce::Graphics &g)
{
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);
}

void EqualizerComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "EQGainSubBass")
		subBassSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainBass")
		bassSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainLowMid")
		lowMidSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainMid")
		midSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainHiMid")
		highMidSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainPresence")
		presenceSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainHigh")
		highSlider.setValue(value, juce::dontSendNotification);
	else if (paramSuffix == "EQGainAir")
		airSlider.setValue(value, juce::dontSendNotification);
}

void EqualizerComponent::setupUI()
{
	auto setupSlider = [this](MidiLearnableSlider &slider)
	{
		addAndMakeVisible(slider);
		slider.setSliderStyle(juce::Slider::LinearVertical);
		slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		slider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
		slider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
		slider.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		slider.getProperties().set(CustomLookAndFeel::getDrawTicksPropertyId(), 9);
		slider.getProperties().set(CustomLookAndFeel::getDrawTicksSmallPropertyId(), true);
	};

	setupSlider(subBassSlider);
	setupSlider(bassSlider);
	setupSlider(lowMidSlider);
	setupSlider(midSlider);
	setupSlider(highMidSlider);
	setupSlider(presenceSlider);
	setupSlider(highSlider);
	setupSlider(airSlider);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(subBassLabel, "40H");
	setupLabel(bassLabel, "120H");
	setupLabel(lowMidLabel, "350H");
	setupLabel(midLabel, "1K");
	setupLabel(highMidLabel, "3K");
	setupLabel(presenceLabel, "5K");
	setupLabel(highLabel, "8K");
	setupLabel(airLabel, "15K");

	updateModelUI();
}

void EqualizerComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void EqualizerComponent::resized()
{
	auto area = getLocalBounds().reduced(8, 4);

	auto sliderAreaWidth = area.getWidth() / 8;

	auto placeSlider = [this, &area, sliderAreaWidth](MidiLearnableSlider &slider, juce::Label &label)
	{
		auto column = area.removeFromLeft(sliderAreaWidth);
		label.setBounds(column.removeFromBottom(8));
		slider.setBounds(column);
	};

	placeSlider(subBassSlider, subBassLabel);
	placeSlider(bassSlider, bassLabel);
	placeSlider(lowMidSlider, lowMidLabel);
	placeSlider(midSlider, midLabel);
	placeSlider(highMidSlider, highMidLabel);
	placeSlider(presenceSlider, presenceLabel);
	placeSlider(highSlider, highLabel);
	placeSlider(airSlider, airLabel);
}

void EqualizerComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);

	subBassSlider.setColour(juce::Slider::thumbColourId, modelColour);
	bassSlider.setColour(juce::Slider::thumbColourId, modelColour);
	lowMidSlider.setColour(juce::Slider::thumbColourId, modelColour);
	midSlider.setColour(juce::Slider::thumbColourId, modelColour);
	highMidSlider.setColour(juce::Slider::thumbColourId, modelColour);
	presenceSlider.setColour(juce::Slider::thumbColourId, modelColour);
	highSlider.setColour(juce::Slider::thumbColourId, modelColour);
	airSlider.setColour(juce::Slider::thumbColourId, modelColour);

	repaint();
}

void EqualizerComponent::wireParameters()
{
	registerSliderParam("EQGainSubBass", subBassSlider);
	registerSliderParam("EQGainBass", bassSlider);
	registerSliderParam("EQGainLowMid", lowMidSlider);
	registerSliderParam("EQGainMid", midSlider);
	registerSliderParam("EQGainHiMid", highMidSlider);
	registerSliderParam("EQGainPresence", presenceSlider);
	registerSliderParam("EQGainHigh", highSlider);
	registerSliderParam("EQGainAir", airSlider);

	registerMidiLearn("EQGainSubBass", &subBassSlider);
	registerMidiLearn("EQGainBass", &bassSlider);
	registerMidiLearn("EQGainLowMid", &lowMidSlider);
	registerMidiLearn("EQGainMid", &midSlider);
	registerMidiLearn("EQGainHiMid", &highMidSlider);
	registerMidiLearn("EQGainPresence", &presenceSlider);
	registerMidiLearn("EQGainHigh", &highSlider);
	registerMidiLearn("EQGainAir", &airSlider);

	syncSliderRange(subBassSlider, fullParamId("EQGainSubBass"));
	syncSliderRange(bassSlider, fullParamId("EQGainBass"));
	syncSliderRange(lowMidSlider, fullParamId("EQGainLowMid"));
	syncSliderRange(midSlider, fullParamId("EQGainMid"));
	syncSliderRange(highMidSlider, fullParamId("EQGainHiMid"));
	syncSliderRange(presenceSlider, fullParamId("EQGainPresence"));
	syncSliderRange(highSlider, fullParamId("EQGainHigh"));
	syncSliderRange(airSlider, fullParamId("EQGainAir"));
}