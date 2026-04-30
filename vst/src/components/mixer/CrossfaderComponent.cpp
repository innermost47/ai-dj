#include "CrossfaderComponent.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

CrossfaderComponent::CrossfaderComponent(DjIaVstProcessor& processor)
	: audioProcessor(processor)
{
	setupUI();
	setupCurveButtons();
	setupMidiLearn();
	setupCurveButtonsMidiLearn();
	refreshFromProcessor();
}

CrossfaderComponent::~CrossfaderComponent()
{
}

void CrossfaderComponent::setupUI()
{
	for (int i = 0; i < 4; ++i)
	{
		addAndMakeVisible(pairSliders[i]);
		setupSlider(pairSliders[i],
			"Crossfader " + juce::String(i + 1) + " <-> " + juce::String(i + 5)
			+ " (Right-click for MIDI learn)");

		const int pairIdx = i;
		pairSliders[i].onValueChange = [this, pairIdx]()
			{
				audioProcessor.setPairCrossfaderValue(pairIdx,
					static_cast<float>(pairSliders[pairIdx].getValue()));
				updateSliderColour(pairSliders[pairIdx], pairIdx);
			};
	}

	addAndMakeVisible(globalSlider);
	setupSlider(globalSlider, "Global Crossfader DECK A <-> DECK B (Right-click for MIDI learn)");
	globalSlider.setColour(juce::Slider::thumbColourId, ColourPalette::textPrimary);
	globalSlider.onValueChange = [this]()
		{
			audioProcessor.setGlobalCrossfaderValue(static_cast<float>(globalSlider.getValue()));
		};
}

void CrossfaderComponent::setupCurveButtons()
{
	auto setupCurveBtn = [](IconButton& btn)
		{
			btn.setClickingTogglesState(false);
			btn.setHasAccentBar(true);
			btn.setCompactMode(true);
			btn.setShowBackground(false);
			btn.setShowBorder(false);
			btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
			btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
			btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
			btn.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
		};

	addAndMakeVisible(curveLinearButton);
	curveLinearButton.loadIcon(BinaryData::lin_svg, BinaryData::lin_svgSize);
	setupCurveBtn(curveLinearButton);
	curveLinearButton.setTooltip("Linear crossfade curve");
	curveLinearButton.onClick = [this]() { selectCurveMode(0); };

	addAndMakeVisible(curveEqualPowerButton);
	curveEqualPowerButton.loadIcon(BinaryData::eq_svg, BinaryData::eq_svgSize);
	setupCurveBtn(curveEqualPowerButton);
	curveEqualPowerButton.setTooltip("Equal Power crossfade curve (constant perceived volume)");
	curveEqualPowerButton.onClick = [this]() { selectCurveMode(1); };

	addAndMakeVisible(curveDjButton);
	curveDjButton.loadIcon(BinaryData::dj_svg, BinaryData::dj_svgSize);
	setupCurveBtn(curveDjButton);
	curveDjButton.setTooltip("DJ scratch curve (sharp transition)");
	curveDjButton.onClick = [this]() { selectCurveMode(2); };
}

void CrossfaderComponent::selectCurveMode(int mode)
{
	mode = juce::jlimit(0, 2, mode);
	audioProcessor.setCrossfaderCurveMode(mode);
	refreshCurveButtons();
}

void CrossfaderComponent::refreshCurveButtons()
{
	int mode = audioProcessor.getCrossfaderCurveMode();
	curveLinearButton.setToggleState(mode == 0, juce::dontSendNotification);
	curveEqualPowerButton.setToggleState(mode == 1, juce::dontSendNotification);
	curveDjButton.setToggleState(mode == 2, juce::dontSendNotification);

	curveLinearButton.repaint();
	curveEqualPowerButton.repaint();
	curveDjButton.repaint();
}

void CrossfaderComponent::setupSlider(MidiLearnableSlider& slider, const juce::String& tooltip)
{
	slider.setRange(0.0, 1.0, 0.001);
	slider.setValue(0.5, juce::dontSendNotification);
	slider.setSliderStyle(juce::Slider::LinearHorizontal);
	slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	slider.setDoubleClickReturnValue(true, 0.5);
	slider.setTooltip(tooltip);
	slider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
}

static TrackData* getTrackBySlot(DjIaVstProcessor& processor, int slotIndex)
{
	for (const auto& id : processor.getAllTrackIds())
	{
		auto* track = processor.getTrack(id);
		if (track && track->slotIndex == slotIndex)
			return track;
	}
	return nullptr;
}

void CrossfaderComponent::updateSliderColour(MidiLearnableSlider& slider, int pairIdx)
{
	auto* trackLeft = getTrackBySlot(audioProcessor, pairIdx);
	auto* trackRight = getTrackBySlot(audioProcessor, pairIdx + 4);

	juce::Colour leftColour = ColourPalette::sliderThumb;
	juce::Colour rightColour = ColourPalette::sliderThumb;

	if (trackLeft)
		leftColour = AiModelDefinitions::getColourForModel(trackLeft->selectedModel);
	if (trackRight)
		rightColour = AiModelDefinitions::getColourForModel(trackRight->selectedModel);

	float pos = static_cast<float>(slider.getValue());
	juce::Colour morphed = leftColour.interpolatedWith(rightColour, pos);

	slider.setColour(juce::Slider::thumbColourId, morphed);
	slider.repaint();
}

void CrossfaderComponent::updatePairColours()
{
	for (int i = 0; i < 4; ++i)
		updateSliderColour(pairSliders[i], i);
	repaint();
}

void CrossfaderComponent::setupMidiLearn()
{
	for (int i = 0; i < 4; ++i)
	{
		const int pairIdx = i;
		const juce::String paramId = "pairCrossfader" + juce::String(pairIdx + 1);
		const juce::String displayName = "Crossfader " + juce::String(pairIdx + 1)
			+ " <-> " + juce::String(pairIdx + 5);

		pairSliders[pairIdx].onMidiLearn = [this, pairIdx, paramId, displayName]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
				{
					editor->statusLabel.setText("Learning MIDI for " + displayName + "...",
						juce::dontSendNotification);
					editor->updateLCD();
				}
				audioProcessor.getMidiLearnManager().startLearning(
					paramId,
					&audioProcessor,
					[this, pairIdx](float value)
					{
						juce::MessageManager::callAsync([this, pairIdx, value]()
							{
								pairSliders[pairIdx].setValue(value, juce::sendNotification);
							});
					},
					displayName,
					&pairSliders[pairIdx]);
			};

		pairSliders[pairIdx].onMidiRemove = [this, paramId]()
			{
				audioProcessor.getMidiLearnManager().removeMappingForParameter(paramId);
			};
	}

	const juce::String globalParamId = "globalCrossfader";
	globalSlider.onMidiLearn = [this, globalParamId]()
		{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
			{
				editor->statusLabel.setText("Learning MIDI for Global Crossfader...",
					juce::dontSendNotification);
				editor->updateLCD();
			}
			audioProcessor.getMidiLearnManager().startLearning(
				globalParamId,
				&audioProcessor,
				[this](float value)
				{
					juce::MessageManager::callAsync([this, value]()
						{
							globalSlider.setValue(value, juce::sendNotification);
						});
				},
				"Global Crossfader",
				&globalSlider);
		};

	globalSlider.onMidiRemove = [this, globalParamId]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter(globalParamId);
		};
}

void CrossfaderComponent::setupCurveButtonsMidiLearn()
{
	auto setupCurveBtn = [this](IconButton& btn, int mode, const juce::String& displayName)
		{
			const juce::String paramId = "crossfaderCurveMode";

			btn.onMidiLearn = [this, &btn, mode, paramId, displayName]()
				{
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
					{
						editor->statusLabel.setText("Learning MIDI for " + displayName + "...",
							juce::dontSendNotification);
						editor->updateLCD();
					}
					audioProcessor.getMidiLearnManager().startLearning(
						paramId,
						&audioProcessor,
						[this, mode](float value)
						{
							if (value > 0.5f)
							{
								juce::MessageManager::callAsync([this, mode]()
									{
										selectCurveMode(mode);
									});
							}
						},
						displayName,
						&btn);
				};

			btn.onMidiRemove = [this, paramId]()
				{
					audioProcessor.getMidiLearnManager().removeMappingForParameter(paramId);
				};
		};

	setupCurveBtn(curveLinearButton, 0, "Curve Linear");
	setupCurveBtn(curveEqualPowerButton, 1, "Curve Equal Power");
	setupCurveBtn(curveDjButton, 2, "Curve DJ");
}

void CrossfaderComponent::refreshFromProcessor()
{
	for (int i = 0; i < 4; ++i)
		pairSliders[i].setValue(audioProcessor.getPairCrossfaderValue(i),
			juce::dontSendNotification);

	globalSlider.setValue(audioProcessor.getGlobalCrossfaderValue(),
		juce::dontSendNotification);

	refreshCurveButtons();
	updatePairColours();
	repaint();
}

void CrossfaderComponent::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds, 8.0f);

	g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
	g.setColour(ColourPalette::textPrimary);
	g.drawText("CROSSFADERS", bounds.toNearestInt().withHeight(12).translated(0, 6),
		juce::Justification::centred);
}

void CrossfaderComponent::paintOverChildren(juce::Graphics& g)
{
	const float ledR = 4.0f;
	g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::bold));

	for (int i = 0; i < 4; ++i)
	{
		auto rowBounds = pairRowBounds[i];
		if (rowBounds.isEmpty()) continue;

		auto* trackLeft = getTrackBySlot(audioProcessor, i);
		auto* trackRight = getTrackBySlot(audioProcessor, i + 4);

		juce::Colour leftColour = ColourPalette::sliderThumb;
		juce::Colour rightColour = ColourPalette::sliderThumb;
		if (trackLeft)  leftColour = AiModelDefinitions::getColourForModel(trackLeft->selectedModel);
		if (trackRight) rightColour = AiModelDefinitions::getColourForModel(trackRight->selectedModel);

		float labelY = (float)rowBounds.getY() - 4.0f;
		float rx = (float)rowBounds.getX();
		float rr = (float)rowBounds.getRight();

		g.setColour(leftColour.withAlpha(0.9f));
		g.fillEllipse(rx + 2.0f, labelY + 2.0f, ledR * 2.0f, ledR * 2.0f);
		g.setColour(ColourPalette::textSecondary);
		g.drawText("T" + juce::String(i + 1),
			juce::Rectangle<float>(rx + ledR * 2.0f + 5.0f, labelY, 20.0f, 12.0f).toNearestInt(),
			juce::Justification::centredLeft);

		g.setColour(ColourPalette::textSecondary);
		g.drawText("T" + juce::String(i + 5),
			juce::Rectangle<float>(rr - ledR * 2.0f - 24.0f, labelY, 20.0f, 12.0f).toNearestInt(),
			juce::Justification::centredRight);
		g.setColour(rightColour.withAlpha(0.9f));
		g.fillEllipse(rr - ledR * 2.0f - 2.0f, labelY + 2.0f, ledR * 2.0f, ledR * 2.0f);
	}
}

void CrossfaderComponent::resized()
{
	auto area = getLocalBounds().reduced(6, 4);
	area.removeFromTop(28);

	const int curveButtonsHeight = 32;
	const int curveButtonsBottomMargin = 2;

	auto curveButtonsArea = area.removeFromBottom(curveButtonsHeight);
	area.removeFromBottom(curveButtonsBottomMargin);
	curveButtonsRowBounds = curveButtonsArea;

	const int btnSpacing = 4;
	const int totalBtnWidth = curveButtonsArea.getWidth();
	const int btnW = (totalBtnWidth - btnSpacing * 2) / 3;

	curveLinearButton.setBounds(curveButtonsArea.removeFromLeft(btnW));
	curveButtonsArea.removeFromLeft(btnSpacing);
	curveEqualPowerButton.setBounds(curveButtonsArea.removeFromLeft(btnW));
	curveButtonsArea.removeFromLeft(btnSpacing);
	curveDjButton.setBounds(curveButtonsArea);

	const int sideW = 2;
	const int rowSpacing = 2;
	const int rowHeight = (area.getHeight() - rowSpacing * 3) / 4;

	for (int i = 0; i < 4; ++i)
	{
		auto rowArea = (i == 3) ? area : area.removeFromTop(rowHeight);
		pairRowBounds[i] = rowArea;

		auto sliderArea = rowArea;
		sliderArea.removeFromLeft(sideW);
		sliderArea.removeFromRight(sideW);
		pairSliders[i].setBounds(sliderArea);

		if (i < 3)
			area.removeFromTop(rowSpacing);
	}

	globalSlider.setVisible(false);
}