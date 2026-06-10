#include "DistortionComponent.h"
#include "PluginProcessor.h"

DistortionComponent::DistortionComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

DistortionComponent::~DistortionComponent()
{
	markForDestruction();
}

void DistortionComponent::paint(juce::Graphics &g)
{
	auto *t = getTrack();
	if (!t)
		return;
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto bounds = getLocalBounds().reduced(4);
	auto bypassArea = bounds.removeFromLeft(16).removeFromTop(16);
	bypassDistortionButton.setBounds(bypassArea);
}

void DistortionComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "DistortionType")
		refreshRadioButtonsForParam(paramSuffix);
	else if (paramSuffix == "DistortionPreGain")
	{
		preGainKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "DistortionPostGain")
	{
		postGainKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "DistortionCut")
	{
		cutKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "DistortionBypassed")
	{
		if (value > .5f)
			bypassDistortionButton.setToggleState(true, juce::dontSendNotification);
		else
			bypassDistortionButton.setToggleState(false, juce::dontSendNotification);
	}
}

void DistortionComponent::refreshRadioButtonsForParam(const juce::String &paramSuffix)
{
	juce::String paramID = getParameterPrefix() + paramSuffix;
	auto &apvts = getProcessor().getParameterTreeState();
	auto *p = apvts.getParameter(paramID);
	if (!p)
		return;

	int max = (int)distortionTypeButtons.size() - 1;
	if (max < 0)
		return;

	int idx = juce::jlimit(0, max, (int)(p->getValue() * max + 0.5f));
	for (int i = 0; i < (int)distortionTypeButtons.size(); ++i)
		distortionTypeButtons[i]->setToggleState(i == idx, juce::dontSendNotification);
}

void DistortionComponent::setupUI()
{
	addAndMakeVisible(bypassDistortionButton);
	bypassDistortionButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassDistortionButton.setClickingTogglesState(true);
	bypassDistortionButton.setShowBackground(false);
	bypassDistortionButton.setToggleState(!track->distortion.isBypassed(), juce::dontSendNotification);
	bypassDistortionButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassDistortionButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassDistortionButton.setTooltip("Enable/disable distortion");

	auto setupKnob = [this](MidiLearnableSlider &knob)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
		knob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		knob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	};

	setupKnob(postGainKnob);
	setupKnob(preGainKnob);
	setupKnob(cutKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::MIXER_KNOB_LABEL);
	};

	setupLabel(preGainLabel, "PRE");
	setupLabel(postGainLabel, "POST");
	setupLabel(cutLabel, "CUT");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("Distortion", juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	setupDistortionTypeButtons();
	updateModelUI();
}

void DistortionComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void DistortionComponent::setupDistortionTypeButtons()
{
	auto *t = getTrack();
	if (!t)
		return;
	auto &apvts = getProcessor().getParameterTreeState();
	juce::String s = "slot" + juce::String(t->slotIndex + 1);
	auto *param = apvts.getParameter(s + "DistortionType");

	juce::StringArray labels{"SOFT", "HARD", "TUBE", "FOLD", "DIODE", "CUBIC"};

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::violet);
		btn->setRadioGroupId(Obsidian::RadioGroupIDs::DistortionType);

		int currentValue = (int)(param->getValue() * (labels.size() - 1) + 0.5f);
		btn->setToggleState(i == currentValue, juce::dontSendNotification);

		int idx = i;
		int total = labels.size();
		btn->onClick = [param, idx, total]()
		{
			param->beginChangeGesture();
			param->setValueNotifyingHost((float)idx / (float)(total - 1));
			param->endChangeGesture();
		};

		addAndMakeVisible(*btn);
		distortionTypeButtons.push_back(std::move(btn));
	}
}

void DistortionComponent::resized()
{
	auto area = getLocalBounds().reduced(8, 4);

	auto labelArea = area.removeFromTop(18);
	labelArea.removeFromLeft(14);

	componentLabel.setBounds(labelArea);

	auto selector = area.removeFromLeft(area.getWidth() / 2);
	area.removeFromLeft(Obsidian::GAP_2);
	area.removeFromTop(Obsidian::GAP_8);
	auto pre = area.removeFromLeft(area.getWidth() / 3);
	auto cut = area.removeFromLeft(area.getWidth() / 2);

	juce::Grid grid;
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = {Track(Fr(1)), Track(Fr(1))};
	grid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};

	for (int i = 0; i < (int)distortionTypeButtons.size(); ++i)
	{
		juce::GridItem item(*distortionTypeButtons[i]);
		grid.items.add(item);
	}

	grid.performLayout(selector);

	juce::FlexBox preArea;
	preArea.flexDirection = juce::FlexBox::Direction::column;
	preArea.justifyContent = juce::FlexBox::JustifyContent::center;
	preArea.alignContent = juce::FlexBox::AlignContent::center;

	preArea.items.add(juce::FlexItem(preGainLabel).withFlex(0.2f));
	preArea.items.add(juce::FlexItem(preGainKnob).withFlex(0.8f));

	preArea.performLayout(pre);

	juce::FlexBox cutArea;
	cutArea.flexDirection = juce::FlexBox::Direction::column;
	cutArea.justifyContent = juce::FlexBox::JustifyContent::center;
	cutArea.alignContent = juce::FlexBox::AlignContent::center;

	cutArea.items.add(juce::FlexItem(cutLabel).withFlex(0.2f));
	cutArea.items.add(juce::FlexItem(cutKnob).withFlex(0.8f));

	cutArea.performLayout(cut);

	juce::FlexBox postArea;
	postArea.flexDirection = juce::FlexBox::Direction::column;
	postArea.justifyContent = juce::FlexBox::JustifyContent::center;
	postArea.alignContent = juce::FlexBox::AlignContent::center;

	postArea.items.add(juce::FlexItem(postGainLabel).withFlex(0.2f));
	postArea.items.add(juce::FlexItem(postGainKnob).withFlex(0.8f));

	postArea.performLayout(area);
}

void DistortionComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	preGainKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	postGainKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	cutKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void DistortionComponent::wireParameters()
{
	auto setupSlider = [this](juce::String paramSuffix, MidiLearnableSlider &knob)
	{
		registerSliderParam(paramSuffix, knob);
		registerMidiLearn(paramSuffix, &knob);
		syncSliderRange(knob, fullParamId(paramSuffix));
	};

	setupSlider("DistortionPreGain", preGainKnob);
	setupSlider("DistortionPostGain", postGainKnob);
	setupSlider("DistortionCut", cutKnob);

	registerButtonParam("DistortionBypassed", bypassDistortionButton);
	registerMidiLearn("DistortionBypassed", &bypassDistortionButton);

	subscribeToParam("DistortionType");

	auto highpassCallback = [this](const juce::String &paramID, int targetIndex, int totalCount)
	{
		return [this, paramID, targetIndex, totalCount](float value)
		{
			if (value > 0.5f)
			{
				juce::Component::SafePointer<DistortionComponent> safeThis(this);
				juce::MessageManager::callAsync(
				    [safeThis, paramID, targetIndex, totalCount]()
				    {
					    if (safeThis)
						    if (auto *p = safeThis->getProcessor().getParameterTreeState().getParameter(paramID))
						    {
							    p->beginChangeGesture();
							    p->setValueNotifyingHost((float)targetIndex / (float)(totalCount - 1));
							    p->endChangeGesture();
						    }
				    });
			}
		};
	};

	for (int i = 0; i < (int)distortionTypeButtons.size(); ++i)
	{
		registerMidiLearn("DistortionType", distortionTypeButtons[i].get(),
		                  highpassCallback("DistortionType", i, (int)distortionTypeButtons.size()));
	}
}