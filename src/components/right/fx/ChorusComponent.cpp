#include "ChorusComponent.h"
#include "PluginProcessor.h"

ChorusComponent::ChorusComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

ChorusComponent::~ChorusComponent()
{
	markForDestruction();
}

void ChorusComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto bounds = getLocalBounds().reduced(4);
	auto bypassArea = bounds.removeFromLeft(16).removeFromTop(16);
	bypassChorusButton.setBounds(bypassArea);
}

void ChorusComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "ChorusRate")
	{
		rateKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "ChorusDepth")
	{
		depthKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "ChorusCentre")
	{
		centreKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "ChorusFeedback")
	{
		feedbackKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "ChorusMix")
	{
		mixKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "ChorusBypassed")
	{
		if (value > .5f)
			bypassChorusButton.setToggleState(true, juce::dontSendNotification);
		else
			bypassChorusButton.setToggleState(false, juce::dontSendNotification);
	}
}

void ChorusComponent::setupUI()
{
	addAndMakeVisible(bypassChorusButton);
	bypassChorusButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassChorusButton.setClickingTogglesState(true);
	bypassChorusButton.setShowBackground(false);
	bypassChorusButton.setToggleState(!track->chorus.isBypassed(), juce::dontSendNotification);
	bypassChorusButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassChorusButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassChorusButton.setTooltip("Enable/disable chorus");

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
	setupLabel(centreLabel, "DELAY");
	setupLabel(feedbackLabel, "FBCK");
	setupLabel(mixLabel, "MIX");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("Chorus", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	updateModelUI();
}

void ChorusComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void ChorusComponent::resized()
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

void ChorusComponent::updateModelUI()
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

void ChorusComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("ChorusRate", rateKnob);
	setupSlider("ChorusDepth", depthKnob);
	setupSlider("ChorusCentre", centreKnob);
	setupSlider("ChorusFeedback", feedbackKnob);
	setupSlider("ChorusMix", mixKnob);

	registerButtonParam("ChorusBypassed", bypassChorusButton);
	registerMidiLearn("ChorusBypassed", &bypassChorusButton);
}