#include "GateComponent.h"
#include "PluginProcessor.h"

GateComponent::GateComponent(DjIaVstProcessor &processor, TrackData *trackData) : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();

	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

GateComponent::~GateComponent()
{
	markForDestruction();
}

void GateComponent::handleVBlank()
{
	syncModulationRing(durationKnob, "GateDuration");
	syncModulationRing(depthKnob, "GateDepth");

	syncGlitchLock(GlitchEffectType::Gate, nullptr, &bypassGateButton);

	if (syncGlitchLedState(GlitchEffectType::Gate))
		repaint();
}

void GateComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);
	paintGlitchLed(g, GlitchEffectType::Gate);
	auto bounds = getLocalBounds().reduced(4);
	auto bypassArea = bounds.removeFromLeft(16).removeFromTop(16);
	bypassGateButton.setBounds(bypassArea);
}

juce::String GateComponent::getRateName(int value)
{
	switch (value)
	{
	case 0:
		return "1/16";
	case 1:
		return "1/8";
	case 2:
		return "1/8.";
	case 3:
		return "1/4";
	case 4:
		return "1/4.";
	case 5:
		return "1/2";
	case 6:
		return "1 bar";
	case 7:
		return "2 bars";
	default:
		return "1/16";
	}
}

void GateComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);

	if (paramSuffix == "GateRate")
	{
		rateKnob.setValue(value, juce::dontSendNotification);
		rateLabel.setText(getRateName((int)juce::roundToInt(value)), juce::dontSendNotification);
	}
	else if (paramSuffix == "GateDuration")
	{
		durationKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "GateDepth")
	{
		depthKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "GateBypassed")
	{
		bypassGateButton.setToggleState(value > .5f, juce::dontSendNotification);
	}
}

void GateComponent::setupUI()
{
	addAndMakeVisible(bypassGateButton);
	bypassGateButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassGateButton.setClickingTogglesState(true);
	bypassGateButton.setShowBackground(false);
	bypassGateButton.setToggleState(!track->gate.isBypassed(), juce::dontSendNotification);
	bypassGateButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassGateButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassGateButton.setTooltip("Enable/disable gate");

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
	setupKnob(durationKnob);
	setupKnob(depthKnob);

	rateKnob.setRange(0, 7, 1);
	rateKnob.setDoubleClickReturnValue(true, 0);
	rateKnob.setTooltip("Gate rate (tempo-synced): 1/16 to 2 bars");
	rateKnob.onValueChange = [this]() { onRateChanged(); };

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(rateLabel, getRateName(0));
	setupLabel(durationLabel, "DUR");
	setupLabel(depthLabel, "DEPTH");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("Gate", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	updateModelUI();
}

void GateComponent::onRateChanged()
{
	int value = (int)juce::roundToInt(rateKnob.getValue());
	rateLabel.setText(getRateName(value), juce::dontSendNotification);
}

void GateComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void GateComponent::resized()
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

	placeKnob(depthKnob, depthLabel);
	placeKnob(durationKnob, durationLabel);
	placeKnob(rateKnob, rateLabel);
}

void GateComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);

	auto updateColor = [this, modelColour](MidiLearnableSlider &knob)
	{ knob.setColour(juce::Slider::rotarySliderFillColourId, modelColour); };

	updateColor(rateKnob);
	updateColor(durationKnob);
	updateColor(depthKnob);

	repaint();
}

void GateComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("GateRate", rateKnob);
	setupSlider("GateDuration", durationKnob);
	setupSlider("GateDepth", depthKnob);

	registerButtonParam("GateBypassed", bypassGateButton);
	registerMidiLearn("GateBypassed", &bypassGateButton);
}