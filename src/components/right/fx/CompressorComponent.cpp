#include "CompressorComponent.h"
#include "PluginProcessor.h"

CompressorComponent::CompressorComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

CompressorComponent::~CompressorComponent()
{
	markForDestruction();
}

void CompressorComponent::paint(juce::Graphics &g)
{
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);
}

void CompressorComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "CompressorThreshold")
	{
		thresholdKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "CompressorRatio")
	{
		ratioKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "CompressorAttack")
	{
		attackKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "CompressorRelease")
	{
		releaseKnob.setValue(value, juce::dontSendNotification);
	}
}

void CompressorComponent::setupUI()
{
	auto setupKnob = [this](MidiLearnableSlider &knob)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
		knob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		knob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	};

	setupKnob(thresholdKnob);
	setupKnob(ratioKnob);
	setupKnob(attackKnob);
	setupKnob(releaseKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(thresholdLabel, "THRESHOLD");
	setupLabel(ratioLabel, "RATIO");
	setupLabel(attackLabel, "ATTACK");
	setupLabel(releaseLabel, "RELEASE");

	updateModelUI();
}

void CompressorComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void CompressorComponent::resized()
{
	auto area = getLocalBounds().reduced(2);

	area.removeFromBottom(Obsidian::GAP_4);

	auto knobAreaWidth = area.getWidth() / 4;

	auto placeKnob = [this, &area, knobAreaWidth](MidiLearnableSlider &slider, juce::Label &label)
	{
		auto column = area.removeFromLeft(knobAreaWidth);
		label.setBounds(column.removeFromBottom(8));
		slider.setBounds(column);
	};

	placeKnob(thresholdKnob, thresholdLabel);
	placeKnob(ratioKnob, ratioLabel);
	placeKnob(attackKnob, attackLabel);
	placeKnob(releaseKnob, releaseLabel);
}

void CompressorComponent::updateModelUI()
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

	updateColor(thresholdKnob);
	updateColor(ratioKnob);
	updateColor(attackKnob);
	updateColor(releaseKnob);

	repaint();
}

void CompressorComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("CompressorThreshold", thresholdKnob);
	setupSlider("CompressorRatio", ratioKnob);
	setupSlider("CompressorAttack", attackKnob);
	setupSlider("CompressorRelease", releaseKnob);
}