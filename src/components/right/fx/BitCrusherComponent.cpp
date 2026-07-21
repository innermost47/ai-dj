#include "BitCrusherComponent.h"
#include "PluginProcessor.h"

BitCrusherComponent::BitCrusherComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

BitCrusherComponent::~BitCrusherComponent()
{
	markForDestruction();
}

void BitCrusherComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto bounds = getLocalBounds().reduced(4);
	auto bypassArea = bounds.removeFromLeft(16).removeFromTop(16);
	bypassBitCrusherButton.setBounds(bypassArea);
}

void BitCrusherComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "BitCrusherBitDepth")
	{
		bitDepthKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "BitCrusherRate")
	{
		sampleRateReductionKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "BitCrusherMix")
	{
		mixKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "BitCrusherBypassed")
	{
		if (value > .5f)
			bypassBitCrusherButton.setToggleState(true, juce::dontSendNotification);
		else
			bypassBitCrusherButton.setToggleState(false, juce::dontSendNotification);
	}
}

void BitCrusherComponent::setupUI()
{
	addAndMakeVisible(bypassBitCrusherButton);
	bypassBitCrusherButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassBitCrusherButton.setClickingTogglesState(true);
	bypassBitCrusherButton.setShowBackground(false);
	bypassBitCrusherButton.setToggleState(!track->bitCrusher.isBypassed(), juce::dontSendNotification);
	bypassBitCrusherButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassBitCrusherButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassBitCrusherButton.setTooltip("Enable/disable bitcrusher");

	auto setupKnob = [this](MidiLearnableSlider &knob)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
		knob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		knob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	};

	setupKnob(bitDepthKnob);
	setupKnob(sampleRateReductionKnob);
	setupKnob(mixKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(bitDepthLabel, "BITS");
	setupLabel(sampleRateReductionLabel, "RATE");
	setupLabel(mixLabel, "MIX");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("BitCrusher", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	updateModelUI();
}

void BitCrusherComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void BitCrusherComponent::resized()
{
	auto area = getLocalBounds().reduced(8, 4);

	auto labelArea = area.removeFromTop(18);
	labelArea.removeFromLeft(14);

	componentLabel.setBounds(labelArea);

	auto knobAreaWidth = area.getWidth() / 5;

	auto placeKnob = [this, &area, knobAreaWidth](MidiLearnableSlider &slider, juce::Label &label)
	{
		auto column = area.removeFromRight(knobAreaWidth);
		label.setBounds(column.removeFromBottom(8));
		slider.setBounds(column);
	};

	placeKnob(mixKnob, mixLabel);
	placeKnob(sampleRateReductionKnob, sampleRateReductionLabel);
	placeKnob(bitDepthKnob, bitDepthLabel);
}

void BitCrusherComponent::updateModelUI()
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

	updateColor(bitDepthKnob);
	updateColor(sampleRateReductionKnob);
	updateColor(mixKnob);

	repaint();
}

void BitCrusherComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("BitCrusherBitDepth", bitDepthKnob);
	setupSlider("BitCrusherRate", sampleRateReductionKnob);
	setupSlider("BitCrusherMix", mixKnob);

	registerButtonParam("BitCrusherBypassed", bypassBitCrusherButton);
	registerMidiLearn("BitCrusherBypassed", &bypassBitCrusherButton);
}