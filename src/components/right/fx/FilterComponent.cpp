#include "FilterComponent.h"
#include "PluginProcessor.h"

FilterComponent::FilterComponent(DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor)
{
	setTrackData(trackData);
	setupUI();
	wireParameters();
}

FilterComponent::~FilterComponent()
{
	markForDestruction();
}

void FilterComponent::paint(juce::Graphics &g)
{
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);
}

void FilterComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(paramSuffix));
	auto value = range.convertFrom0to1(normalizedValue);
	if (paramSuffix == "FilterMode")
		refreshRadioButtonsForParam(paramSuffix);
	else if (paramSuffix == "Cutoff")
	{
		cutoffKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "Resonance")
	{
		resonanceKnob.setValue(value, juce::dontSendNotification);
	}
	else if (paramSuffix == "FilterDrive")
	{
		driveKnob.setValue(value, juce::dontSendNotification);
	}
}

void FilterComponent::refreshRadioButtonsForParam(const juce::String &paramSuffix)
{
	juce::String paramID = getParameterPrefix() + paramSuffix;
	auto &apvts = getProcessor().getParameterTreeState();
	auto *p = apvts.getParameter(paramID);
	if (!p)
		return;

	int max = (int)cutoffModeButtons.size() - 1;
	if (max < 0)
		return;

	int idx = juce::jlimit(0, max, (int)(p->getValue() * max + 0.5f));
	for (int i = 0; i < (int)cutoffModeButtons.size(); ++i)
		cutoffModeButtons[i]->setToggleState(i == idx, juce::dontSendNotification);
}

void FilterComponent::setupUI()
{
	addAndMakeVisible(driveKnob);
	driveKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	driveKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	driveKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	driveKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	driveKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(driveLabel);
	driveLabel.setText("DRIVE", juce::dontSendNotification);
	driveLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	driveLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(cutoffKnob);
	cutoffKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	cutoffKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	cutoffKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	cutoffKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	cutoffKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(cutoffLabel);
	cutoffLabel.setText("CUT", juce::dontSendNotification);
	cutoffLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	cutoffLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(resonanceKnob);
	resonanceKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	resonanceKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	resonanceKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	resonanceKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	resonanceKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(resonanceLabel);
	resonanceLabel.setText("RES", juce::dontSendNotification);
	resonanceLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	resonanceLabel.setJustificationType(juce::Justification::centred);

	Obsidian::applyFontSize(driveLabel, Obsidian::TEXT_XXS);
	Obsidian::applyFontSize(cutoffLabel, Obsidian::TEXT_XXS);
	Obsidian::applyFontSize(resonanceLabel, Obsidian::TEXT_XXS);

	setupCutoffModeButtons();
	updateModelUI();
}

void FilterComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
}

void FilterComponent::setupCutoffModeButtons()
{
	auto *t = getTrack();
	if (!t)
		return;
	auto &apvts = getProcessor().getParameterTreeState();
	juce::String s = "slot" + juce::String(t->slotIndex + 1);
	auto *param = apvts.getParameter(s + "FilterMode");

	juce::StringArray labels{"LP12", "HP12", "BP12", "LP24", "HP24", "BP24"};

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::violet);
		btn->setRadioGroupId(Obsidian::RadioGroupIDs::FilterTypeGroup);

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
		cutoffModeButtons.push_back(std::move(btn));
	}
}

void FilterComponent::resized()
{
	auto area = getLocalBounds().reduced(8);

	auto selector = area.removeFromLeft(area.getWidth() / 2);
	area.removeFromLeft(Obsidian::GAP_4);
	auto drive = area.removeFromLeft(area.getWidth() / 3);
	auto cutoff = area.removeFromLeft(area.getWidth() / 2);

	juce::Grid grid;
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = {Track(Fr(1)), Track(Fr(1))};
	grid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};
	grid.columnGap = juce::Grid::Px(Obsidian::GAP_2);
	grid.rowGap = juce::Grid::Px(Obsidian::GAP_2);

	for (int i = 0; i < (int)cutoffModeButtons.size(); ++i)
	{
		juce::GridItem item(*cutoffModeButtons[i]);
		grid.items.add(item);
	}

	grid.performLayout(selector);

	juce::FlexBox driveArea;
	driveArea.flexDirection = juce::FlexBox::Direction::column;
	driveArea.justifyContent = juce::FlexBox::JustifyContent::center;
	driveArea.alignContent = juce::FlexBox::AlignContent::center;

	driveArea.items.add(juce::FlexItem(driveLabel).withFlex(0.2f));
	driveArea.items.add(juce::FlexItem(driveKnob).withFlex(0.8f));

	driveArea.performLayout(drive);

	juce::FlexBox cutoffArea;
	cutoffArea.flexDirection = juce::FlexBox::Direction::column;
	cutoffArea.justifyContent = juce::FlexBox::JustifyContent::center;
	cutoffArea.alignContent = juce::FlexBox::AlignContent::center;

	cutoffArea.items.add(juce::FlexItem(cutoffLabel).withFlex(0.2f));
	cutoffArea.items.add(juce::FlexItem(cutoffKnob).withFlex(0.8f));

	cutoffArea.performLayout(cutoff);

	juce::FlexBox resArea;
	resArea.flexDirection = juce::FlexBox::Direction::column;
	resArea.justifyContent = juce::FlexBox::JustifyContent::center;
	resArea.alignContent = juce::FlexBox::AlignContent::center;

	resArea.items.add(juce::FlexItem(resonanceLabel).withFlex(0.2f));
	resArea.items.add(juce::FlexItem(resonanceKnob).withFlex(0.8f));

	resArea.performLayout(area);
}

void FilterComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	cutoffKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	resonanceKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	driveKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void FilterComponent::wireParameters()
{
	registerSliderParam("Cutoff", cutoffKnob);
	registerSliderParam("Resonance", resonanceKnob);
	registerSliderParam("FilterDrive", driveKnob);

	registerMidiLearn("Cutoff", &cutoffKnob);
	registerMidiLearn("Resonance", &resonanceKnob);
	registerMidiLearn("FilterDrive", &driveKnob);

	syncSliderRange(cutoffKnob, fullParamId("Cutoff"));
	syncSliderRange(resonanceKnob, fullParamId("Resonance"));
	syncSliderRange(driveKnob, fullParamId("FilterDrive"));

	subscribeToParam("FilterMode");

	auto highpassCallback = [this](const juce::String &paramID, int targetIndex, int totalCount)
	{
		return [this, paramID, targetIndex, totalCount](float value)
		{
			if (value > 0.5f)
			{
				juce::MessageManager::callAsync(
				    [this, paramID, targetIndex, totalCount]()
				    {
					    if (auto *p = getProcessor().getParameterTreeState().getParameter(paramID))
					    {
						    p->beginChangeGesture();
						    p->setValueNotifyingHost((float)targetIndex / (float)(totalCount - 1));
						    p->endChangeGesture();
					    }
				    });
			}
		};
	};

	for (int i = 0; i < (int)cutoffModeButtons.size(); ++i)
	{
		registerMidiLearn("FilterMode", cutoffModeButtons[i].get(),
		                  highpassCallback("FilterMode", i, (int)cutoffModeButtons.size()));
	}
}