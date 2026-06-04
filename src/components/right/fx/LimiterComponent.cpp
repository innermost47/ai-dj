#include "LimiterComponent.h"
#include "PluginProcessor.h"

LimiterComponent::LimiterComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
	startTimer(30);
}

LimiterComponent::~LimiterComponent()
{
	stopTimer();
	markForDestruction();
}

void LimiterComponent::timerCallback()
{
	auto *t = getTrack();
	if (!t)
	{
		stopTimer();
		return;
	}
	if (!t->isCurrentlyPlaying.load())
	{
		repaint();
		return;
	}

	repaint();
}

void LimiterComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto bounds = getLocalBounds();
	auto circleArea = bounds.removeFromLeft(18).removeFromTop(18);
	auto circleRect = circleArea.withSizeKeepingCentre(6, 6).toFloat();

	float scale = 1.f;

	g.setColour(modelColour.withAlpha(Obsidian::ALPHA_04));
	g.drawEllipse(circleRect.getX() - (scale / 2), circleRect.getY() - (scale / 2), circleRect.getWidth() + scale,
	              circleRect.getHeight() + scale, Obsidian::BORDER_WIDTH_XS);

	float gainReduction = juce::jlimit(0.f, 1.f, (t->limiter.getReductionAmount()));

	if (gainReduction > 0.01f)
		g.setColour(modelColour.withBrightness(gainReduction));
	else
		g.setColour(modelColour.withAlpha(Obsidian::ALPHA_01));

	g.fillEllipse(circleRect);
}

void LimiterComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "LimiterThreshold")
	{
		thresholdKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "LimiterRelease")
	{
		releaseKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "LimiterMakeUpGain")
	{
		makeUpGainKnob.setValue(value, juce::dontSendNotification);
	}
}

void LimiterComponent::setupUI()
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
	setupKnob(releaseKnob);
	setupKnob(makeUpGainKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(thresholdLabel, "THRESH");
	setupLabel(releaseLabel, "RELEASE");
	setupLabel(makeUpGainLabel, "GAIN");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("Limiter", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::bottomLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	updateModelUI();
}

void LimiterComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void LimiterComponent::resized()
{
	auto area = getLocalBounds().reduced(2);

	area.removeFromBottom(Obsidian::GAP_4);

	auto knobAreaWidth = area.getWidth() / 5;

	auto componentLabelArea = area.removeFromLeft(knobAreaWidth * 2);
	componentLabel.setBounds(componentLabelArea);

	auto placeKnob = [this, &area, knobAreaWidth](MidiLearnableSlider &slider, juce::Label &label)
	{
		auto column = area.removeFromRight(knobAreaWidth);
		label.setBounds(column.removeFromBottom(8));
		slider.setBounds(column);
	};

	placeKnob(makeUpGainKnob, makeUpGainLabel);
	placeKnob(releaseKnob, releaseLabel);
	placeKnob(thresholdKnob, thresholdLabel);
}

void LimiterComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	auto updateColor = [this](MidiLearnableSlider &knob)
	{ knob.setColour(juce::Slider::rotarySliderFillColourId, modelColour); };

	updateColor(thresholdKnob);
	updateColor(releaseKnob);
	updateColor(makeUpGainKnob);

	repaint();
}

void LimiterComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("LimiterThreshold", thresholdKnob);
	setupSlider("LimiterRelease", releaseKnob);
	setupSlider("LimiterMakeUpGain", makeUpGainKnob);
}