#include "FlangerComponent.h"
#include "PluginProcessor.h"

FlangerComponent::FlangerComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

FlangerComponent::~FlangerComponent()
{
	markForDestruction();
}

void FlangerComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto bounds = getLocalBounds().reduced(4);
	auto bypassArea = bounds.removeFromLeft(16).removeFromTop(16);
	bypassFlangerButton.setBounds(bypassArea);
}

void FlangerComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "FlangerRate")
	{
		rateKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FlangerDepth")
	{
		depthKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FlangerCentre")
	{
		centreKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FlangerFeedback")
	{
		feedbackKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FlangerMix")
	{
		mixKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FlangerBypassed")
	{
		if (value > .5f)
			bypassFlangerButton.setToggleState(true, juce::dontSendNotification);
		else
			bypassFlangerButton.setToggleState(false, juce::dontSendNotification);
	}
}

void FlangerComponent::setupUI()
{
	addAndMakeVisible(bypassFlangerButton);
	bypassFlangerButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassFlangerButton.setClickingTogglesState(true);
	bypassFlangerButton.setShowBackground(false);
	bypassFlangerButton.setToggleState(!track->chorus.isBypassed(), juce::dontSendNotification);
	bypassFlangerButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassFlangerButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassFlangerButton.setTooltip("Enable/disable flanger");

	auto setupKnob = [this](MidiLearnableSlider &knob)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
		knob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		knob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	};

	setupKnob(rateKnob);
	setupKnob(depthKnob);
	setupKnob(feedbackKnob);
	setupKnob(centreKnob);
	setupKnob(mixKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(rateLabel, "RATE");
	setupLabel(depthLabel, "DEPTH");
	setupLabel(centreLabel, "CENTER");
	setupLabel(feedbackLabel, "FBCK");
	setupLabel(mixLabel, "MIX");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("Flanger", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	updateModelUI();
}

void FlangerComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void FlangerComponent::resized()
{
	auto area = getLocalBounds().reduced(8, 4);

	auto labelArea = area.removeFromTop(18);
	labelArea.removeFromLeft(14);

	componentLabel.setBounds(labelArea);

	auto knobAreaWidth = area.getWidth() / 5;

	auto placeKnob = [this, &area, knobAreaWidth](MidiLearnableSlider &slider, juce::Label &label)
	{
		auto column = area.removeFromLeft(knobAreaWidth);
		label.setBounds(column.removeFromBottom(8));
		slider.setBounds(column);
	};

	placeKnob(rateKnob, rateLabel);
	placeKnob(depthKnob, depthLabel);
	placeKnob(centreKnob, centreLabel);
	placeKnob(feedbackKnob, feedbackLabel);
	placeKnob(mixKnob, mixLabel);
}

void FlangerComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	auto updateColor = [this, modelColour](MidiLearnableSlider &knob)
	{ knob.setColour(juce::Slider::rotarySliderFillColourId, modelColour); };

	updateColor(rateKnob);
	updateColor(depthKnob);
	updateColor(centreKnob);
	updateColor(feedbackKnob);
	updateColor(mixKnob);

	repaint();
}

void FlangerComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("FlangerRate", rateKnob);
	setupSlider("FlangerDepth", depthKnob);
	setupSlider("FlangerCentre", centreKnob);
	setupSlider("FlangerFeedback", feedbackKnob);
	setupSlider("FlangerMix", mixKnob);

	registerButtonParam("FlangerBypassed", bypassFlangerButton);
	registerMidiLearn("FlangerBypassed", &bypassFlangerButton);
}