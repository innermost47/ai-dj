#include "ModulatorRow.h"
#include "ModulationEngine.h"
#include "PluginProcessor.h"
#include "TrackData.h"

ModulatorRow::ModulatorRow(DjIaVstProcessor &processor, int modIndex)
    : ObsidianBaseMidiComponent(processor), index(modIndex)
{
	setupUI();
}

ModulatorRow::~ModulatorRow()
{
	markForDestruction();
	clearAllBindings();
}

juce::String ModulatorRow::suffixFor(const juce::String &name) const
{
	return "Mod" + juce::String(index + 1) + name;
}

void ModulatorRow::setupUI()
{
	addAndMakeVisible(activeButton);
	activeButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	activeButton.setClickingTogglesState(true);
	activeButton.setShowBackground(false);
	activeButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	activeButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	activeButton.setTooltip("Enable this modulator");

	addAndMakeVisible(bipolarButton);
	bipolarButton.loadIcon(BinaryData::arrowsvertical_svg, BinaryData::arrowsvertical_svgSize);
	bipolarButton.setClickingTogglesState(true);
	bipolarButton.setShowBackground(false);
	bipolarButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bipolarButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bipolarButton.setTooltip("Bipolar: modulate above and below the knob value");

	addAndMakeVisible(componentLabel);
	componentLabel.setText("M" + juce::String(index + 1), juce::dontSendNotification);
	componentLabel.setJustificationType(juce::Justification::topLeft);
	componentLabel.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_REGULAR));
	componentLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	addAndMakeVisible(targetSelector);
	targetSelector.addItemList(getModTargetNames(), 1);
	targetSelector.setSelectedId(1, juce::dontSendNotification);
	targetSelector.setTooltip("Parameter to modulate");

	auto setupKnob = [this](MidiLearnableSlider &knob)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
		knob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
		knob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	};

	setupKnob(depthKnob);
	setupKnob(phaseKnob);

	auto setupLabel = [this](juce::Label &label, juce::String labelValue)
	{
		addAndMakeVisible(label);
		label.setText(labelValue, juce::dontSendNotification);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		label.setJustificationType(juce::Justification::centred);
		Obsidian::applyFontSize(label, Obsidian::TEXT_XXS);
	};

	setupLabel(depthLabel, "AMT");
	setupLabel(phaseLabel, "PH");

	setupShapeButtons();
	setupRateButtons();
}

void ModulatorRow::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);

	depthKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	phaseKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void ModulatorRow::setupShapeButtons()
{
	auto labels = getModShapeNames();

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::violet);
		btn->setRadioGroupId(Obsidian::RadioGroupIDs::ModShapeGroup + index);
		btn->setToggleState(i == 0, juce::dontSendNotification);

		addAndMakeVisible(*btn);
		shapeButtons.push_back(std::move(btn));
	}
}

void ModulatorRow::setupRateButtons()
{
	auto labels = getModRateNames();

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::teal);
		btn->setRadioGroupId(Obsidian::RadioGroupIDs::ModRateGroup + index);
		btn->setToggleState(i == 2, juce::dontSendNotification);

		addAndMakeVisible(*btn);
		rateButtons.push_back(std::move(btn));
	}
}

void ModulatorRow::setTrackData(TrackData *trackData)
{
	clearAllBindings();
	track = trackData;

	if (track)
		wireParameters();

	refreshFromTrack();
}

void ModulatorRow::wireParameters()
{
	registerSliderParam(suffixFor("Depth"), depthKnob);
	registerSliderParam(suffixFor("Phase"), phaseKnob);
	registerMidiLearn(suffixFor("Depth"), &depthKnob);
	registerMidiLearn(suffixFor("Phase"), &phaseKnob);
	syncSliderRange(depthKnob, fullParamId(suffixFor("Depth")));
	syncSliderRange(phaseKnob, fullParamId(suffixFor("Phase")));

	registerButtonParam(suffixFor("Active"), activeButton);
	registerMidiLearn(suffixFor("Active"), &activeButton);

	registerButtonParam(suffixFor("Bipolar"), bipolarButton);
	registerMidiLearn(suffixFor("Bipolar"), &bipolarButton);

	subscribeToParam(suffixFor("Target"));
	subscribeToParam(suffixFor("Shape"));
	subscribeToParam(suffixFor("Rate"));

	targetSelector.onChange = [this]()
	{
		auto *p = getProcessor().getParameterTreeState().getParameter(fullParamId(suffixFor("Target")));
		const int total = getNumModTargets();
		const int idx = targetSelector.getSelectedItemIndex();
		if (!p || idx < 0 || total < 2)
			return;

		p->beginChangeGesture();
		p->setValueNotifyingHost((float)idx / (float)(total - 1));
		p->endChangeGesture();

		if (onContentChanged)
			onContentChanged();
	};

	auto wireRadioGroup =
	    [this](std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> &buttons, const juce::String &name)
	{
		const juce::String paramId = fullParamId(suffixFor(name));
		const int total = (int)buttons.size();

		for (int i = 0; i < total; ++i)
		{
			buttons[i]->onClick = [this, paramId, i, total]()
			{
				auto *p = getProcessor().getParameterTreeState().getParameter(paramId);
				if (!p || total < 2)
					return;

				p->beginChangeGesture();
				p->setValueNotifyingHost((float)i / (float)(total - 1));
				p->endChangeGesture();
			};

			registerMidiLearn(suffixFor(name), buttons[i].get());
		}
	};

	wireRadioGroup(shapeButtons, "Shape");
	wireRadioGroup(rateButtons, "Rate");
}

void ModulatorRow::refreshRadioButtonsForParam(const juce::String &paramSuffix,
                                               std::vector<std::unique_ptr<MidiLearnableLedRadioButton>> &buttons)
{
	auto *p = getProcessor().getParameterTreeState().getParameter(fullParamId(paramSuffix));
	if (!p)
		return;

	const int max = (int)buttons.size() - 1;
	if (max < 0)
		return;

	const int idx = juce::jlimit(0, max, (int)(p->getValue() * max + 0.5f));
	for (int i = 0; i <= max; ++i)
		buttons[i]->setToggleState(i == idx, juce::dontSendNotification);
}

void ModulatorRow::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	if (paramSuffix == suffixFor("Shape"))
		refreshRadioButtonsForParam(paramSuffix, shapeButtons);
	else if (paramSuffix == suffixFor("Rate"))
		refreshRadioButtonsForParam(paramSuffix, rateButtons);
	else if (paramSuffix == suffixFor("Target"))
	{
		const int total = getNumModTargets();
		if (total < 2)
			return;
		const int idx = juce::jlimit(0, total - 1, (int)(normalizedValue * (total - 1) + 0.5f));
		targetSelector.setSelectedItemIndex(idx, juce::dontSendNotification);
		if (onContentChanged)
			onContentChanged();
	}
	else if (paramSuffix == suffixFor("Active"))
		activeButton.setToggleState(normalizedValue > 0.5f, juce::dontSendNotification);
	else if (paramSuffix == suffixFor("Bipolar"))
		bipolarButton.setToggleState(normalizedValue > 0.5f, juce::dontSendNotification);
}

void ModulatorRow::refreshFromTrack()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &mod = t->modulators[index];

	activeButton.setToggleState(mod.isActive(), juce::dontSendNotification);
	bipolarButton.setToggleState(mod.isBipolar(), juce::dontSendNotification);
	targetSelector.setSelectedItemIndex(mod.getTarget(), juce::dontSendNotification);
	depthKnob.setValue(mod.getDepth(), juce::dontSendNotification);
	phaseKnob.setValue(mod.getPhase(), juce::dontSendNotification);

	const int shapeIdx = juce::jlimit(0, (int)shapeButtons.size() - 1, (int)mod.getShape());
	for (int i = 0; i < (int)shapeButtons.size(); ++i)
		shapeButtons[i]->setToggleState(i == shapeIdx, juce::dontSendNotification);

	const int rateIdx = juce::jlimit(0, (int)rateButtons.size() - 1, (int)mod.getRate());
	for (int i = 0; i < (int)rateButtons.size(); ++i)
		rateButtons[i]->setToggleState(i == rateIdx, juce::dontSendNotification);

	refreshTargetAvailability();
	updateModelUI();
}

void ModulatorRow::updateModulationDisplay()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &mod = t->modulators[index];
	const float value = mod.isActive() ? mod.getCurrentValue() : 0.0f;

	if (std::abs(value - lastDisplayedValue) < 0.005f)
		return;

	lastDisplayedValue = value;
	repaint();
}

void ModulatorRow::paint(juce::Graphics &g)
{
	paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

	auto *t = getTrack();
	if (!t)
		return;

	auto &mod = t->modulators[index];
	if (!mod.isActive() || mod.getTarget() <= 0)
		return;

	auto meter = getLocalBounds().reduced(8, 4);
	meter.removeFromTop(18 + Obsidian::GAP_4);
	meter = meter.removeFromRight(6);

	g.setColour(ColourPalette::backgroundDark.withAlpha(0.6f));
	g.fillRoundedRectangle(meter.toFloat(), 2.0f);

	const auto modelColour = AiModelDefinitions::getColourForModel(t->getCurrentPage().selectedModel);

	const float centreY = (float)meter.getCentreY();
	const float halfH = (float)meter.getHeight() * 0.5f;
	const float amount = juce::jlimit(-1.0f, 1.0f, lastDisplayedValue);

	juce::Rectangle<float> fill;
	if (amount >= 0.0f)
		fill = juce::Rectangle<float>((float)meter.getX(), centreY - halfH * amount, (float)meter.getWidth(),
		                              halfH * amount);
	else
		fill = juce::Rectangle<float>((float)meter.getX(), centreY, (float)meter.getWidth(), -halfH * amount);

	if (fill.getHeight() > 0.5f)
	{
		g.setColour(modelColour.withAlpha(0.85f));
		g.fillRoundedRectangle(fill, 2.0f);
	}

	g.setColour(ColourPalette::textSecondary.withAlpha(0.4f));
	g.drawLine((float)meter.getX(), centreY, (float)meter.getRight(), centreY, 1.0f);
}

void ModulatorRow::resized()
{
	auto area = getLocalBounds().reduced(8, 4);

	auto headerRow = area.removeFromTop(18);
	activeButton.setBounds(headerRow.removeFromLeft(16));
	headerRow.removeFromLeft(Obsidian::GAP_2);
	componentLabel.setBounds(headerRow.removeFromLeft(40));
	headerRow.removeFromLeft(Obsidian::GAP_4);
	bipolarButton.setBounds(headerRow.removeFromRight(16));
	headerRow.removeFromRight(Obsidian::GAP_2);
	targetSelector.setBounds(headerRow.reduced(0, 1));

	area.removeFromTop(Obsidian::GAP_4);

	area.removeFromRight(6);
	area.removeFromRight(Obsidian::GAP_4);

	auto knobsArea = area.removeFromRight(76);
	area.removeFromRight(Obsidian::GAP_8);

	auto depthArea = knobsArea.removeFromLeft(38);
	depthLabel.setBounds(depthArea.removeFromBottom(10));
	depthKnob.setBounds(depthArea.reduced(2));

	phaseLabel.setBounds(knobsArea.removeFromBottom(10));
	phaseKnob.setBounds(knobsArea.reduced(2));

	auto shapeArea = area.removeFromTop(area.getHeight() / 2);
	area.removeFromTop(Obsidian::GAP_2);

	juce::Grid shapeGrid;
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	shapeGrid.templateRows = {Track(Fr(1)), Track(Fr(1))};
	shapeGrid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};
	shapeGrid.columnGap = juce::Grid::Px(Obsidian::GAP_2);
	shapeGrid.rowGap = juce::Grid::Px(Obsidian::GAP_2);

	for (auto &btn : shapeButtons)
		shapeGrid.items.add(juce::GridItem(*btn));

	shapeGrid.performLayout(shapeArea);

	juce::Grid rateGrid;
	rateGrid.templateRows = {Track(Fr(1)), Track(Fr(1))};
	rateGrid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};
	rateGrid.columnGap = juce::Grid::Px(Obsidian::GAP_2);
	rateGrid.rowGap = juce::Grid::Px(Obsidian::GAP_2);

	for (auto &btn : rateButtons)
		rateGrid.items.add(juce::GridItem(*btn));

	rateGrid.performLayout(area);
}

void ModulatorRow::refreshTargetAvailability()
{
	auto *t = getTrack();
	if (!t)
		return;

	const int myTarget = t->modulators[index].getTarget();

	for (int i = 1; i < getNumModTargets(); ++i)
	{
		bool takenByOther = false;

		for (int m = 0; m < kNumModSlots; ++m)
		{
			if (m == index)
				continue;
			if (t->modulators[m].getTarget() == i)
			{
				takenByOther = true;
				break;
			}
		}

		targetSelector.setItemEnabled(i + 1, !takenByOther || i == myTarget);
	}
}