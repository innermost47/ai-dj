#include "CrossfaderComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

CrossfaderComponent::CrossfaderComponent(DjIaVstProcessor &processor) : ObsidianBaseMidiComponent(processor)
{
	setupUI();
	setupCurveButtons();
	wireParameters();
	refreshFromProcessor();
	startTimerHz(30);
}

CrossfaderComponent::~CrossfaderComponent()
{
	markForDestruction();
	setVisible(false);
	stopTimer();
}

void CrossfaderComponent::timerCallback()
{
	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		if (!pairRowBounds[i].isEmpty())
		{
			auto rb = pairRowBounds[i];
			repaint(rb.getX(), rb.getY(), ledZoneWidth, rb.getHeight());
			repaint(rb.getRight() - ledZoneWidth, rb.getY(), ledZoneWidth, rb.getHeight());
		}
	}
}

void CrossfaderComponent::wireParameters()
{
	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		const juce::String paramId = "pairCrossfader" + juce::String(i + 1);
		registerSliderParam(paramId, pairSliders[i]);
		registerMidiLearn(paramId, &pairSliders[i]);
	}

	registerSliderParam("globalCrossfader", globalSlider);
	registerMidiLearn("globalCrossfader", &globalSlider);

	registerButtonParam("useCrossfader", useCrossfaderButton);
	registerMidiLearn("useCrossfader", &useCrossfaderButton);

	auto curveCallback = [this](int targetMode)
	{
		return [this, targetMode](float value)
		{
			if (value > 0.5f)
				juce::MessageManager::callAsync([this, targetMode]() { selectCurveMode(targetMode); });
		};
	};

	registerMidiLearn("crossfaderCurveMode", &curveLinearButton, curveCallback(0));
	registerMidiLearn("crossfaderCurveMode", &curveEqualPowerButton, curveCallback(1));
	registerMidiLearn("crossfaderCurveMode", &curveDjButton, curveCallback(2));
}

void CrossfaderComponent::setupUI()
{
	addAndMakeVisible(useCrossfaderButton);
	useCrossfaderButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	useCrossfaderButton.setClickingTogglesState(true);
	useCrossfaderButton.setShowBackground(false);
	useCrossfaderButton.setCustomIconColourToggled(ColourPalette::buttonPrimary);

	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		addAndMakeVisible(pairSliders[i]);
		setupSlider(pairSliders[i], "Crossfader " + juce::String(i + 1) + " <-> " + juce::String(i + 5) +
		                                " (Right-click for MIDI learn)");

		pairSliders[i].getProperties().set(CustomLookAndFeel::getDrawTicksPropertyId(), 9);
	}

	addAndMakeVisible(globalSlider);
	setupSlider(globalSlider, "Global Crossfader DECK A <-> DECK B (Right-click for MIDI learn)");
	globalSlider.setColour(juce::Slider::thumbColourId, ColourPalette::textPrimary);
}

void CrossfaderComponent::setupCurveButtons()
{
	auto setupCurveBtn = [](IconButton &btn)
	{
		btn.setClickingTogglesState(false);
		btn.setHasAccentBar(false);
		btn.setShowBackground(false);
		btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
		btn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
	};

	addAndMakeVisible(curveLinearButton);
	setupCurveBtn(curveLinearButton);
	curveLinearButton.setTooltip("Linear crossfade curve");
	curveLinearButton.onClick = [this]() { selectCurveMode(0); };

	addAndMakeVisible(curveEqualPowerButton);
	setupCurveBtn(curveEqualPowerButton);
	curveEqualPowerButton.setTooltip("Equal Power crossfade curve (constant perceived volume)");
	curveEqualPowerButton.onClick = [this]() { selectCurveMode(1); };

	addAndMakeVisible(curveDjButton);
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

	repaint();
}

void CrossfaderComponent::setupSlider(MidiLearnableSlider &slider, const juce::String &tooltip)
{
	slider.setRange(0.0, 1.0, 0.001);
	slider.setValue(0.5, juce::dontSendNotification);
	slider.setSliderStyle(juce::Slider::LinearHorizontal);
	slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	slider.setDoubleClickReturnValue(true, 0.5);
	slider.setTooltip(tooltip);
	slider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
}

void CrossfaderComponent::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	if (paramSuffix.startsWith("pairCrossfader"))
	{
		int idx = paramSuffix.getTrailingIntValue() - 1;
		if (idx >= 0 && idx < Obsidian::MAX_CROSSFADER_PAIR)
		{
			updateSliderColour(pairSliders[idx], idx);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPairCrossfader(idx),
			                                                 MidiMapping::volumeToMidi(normalizedValue),
			                                                 MidiMapping::feedbackChannelShaping);
		}
	}
	else if (paramSuffix == "crossfaderCurveMode")
	{
		refreshCurveButtons();
	}
	else if (paramSuffix == "useCrossfader")
	{
		bool enabled = normalizedValue > 0.5f;
		for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; i++)
		{
			pairSliders[i].setEnabled(enabled);
		}
		curveLinearButton.setEnabled(enabled);
		curveEqualPowerButton.setEnabled(enabled);
		curveDjButton.setEnabled(enabled);
	}
}

static TrackData *getTrackBySlot(DjIaVstProcessor &processor, int slotIndex)
{
	for (const auto &id : processor.getAllTrackIds())
	{
		auto *trackLocal = processor.getTrack(id);
		if (trackLocal && trackLocal->slotIndex == slotIndex)
			return trackLocal;
	}
	return nullptr;
}

void CrossfaderComponent::updateSliderColour(MidiLearnableSlider &slider, int pairIdx)
{
	auto *trackLeft = getTrackBySlot(audioProcessor, pairIdx);
	auto *trackRight = getTrackBySlot(audioProcessor, pairIdx + 4);

	juce::Colour leftColour = ColourPalette::sliderThumb;
	juce::Colour rightColour = ColourPalette::sliderThumb;

	if (trackLeft)
	{
		auto &leftPage = trackLeft->getCurrentPage();
		leftColour = AiModelDefinitions::getColourForModel(leftPage.selectedModel);
	}
	if (trackRight)
	{
		auto &rightPage = trackRight->getCurrentPage();
		rightColour = AiModelDefinitions::getColourForModel(rightPage.selectedModel);
	}

	float pos = static_cast<float>(slider.getValue());
	juce::Colour morphed = leftColour.interpolatedWith(rightColour, pos);

	slider.setColour(juce::Slider::thumbColourId, morphed);
	slider.repaint();
}

void CrossfaderComponent::updatePairColours()
{
	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
		updateSliderColour(pairSliders[i], i);
	repaint();
}

void CrossfaderComponent::refreshFromProcessor()
{
	syncBindingsFromParameters();
	refreshCurveButtons();
	updatePairColours();
	repaint();
}

void CrossfaderComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds.toFloat(), Obsidian::CORNER);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), Obsidian::CORNER, Obsidian::BORDER_WIDTH);

	drawSegmentedCurveBackground(g);
}

void CrossfaderComponent::drawHardwareLED(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour colour,
                                          float intensity, bool playing) const
{
	const float cx = bounds.getCentreX();
	const float cy = bounds.getCentreY();
	const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

	const float i = juce::jlimit(0.0f, 1.0f, intensity);

	if (i > 0.05f)
	{
		const float pulse = playing ? i : juce::jmin(1.0f, i + 0.15f);

		g.setColour(colour.withAlpha(0.15f * pulse));
		g.fillEllipse(cx - radius * 2.4f, cy - radius * 2.4f, radius * 4.8f, radius * 4.8f);

		g.setColour(colour.withAlpha(0.30f * pulse));
		g.fillEllipse(cx - radius * 1.7f, cy - radius * 1.7f, radius * 3.4f, radius * 3.4f);

		g.setColour(colour.withAlpha(0.50f * pulse));
		g.fillEllipse(cx - radius * 1.25f, cy - radius * 1.25f, radius * 2.5f, radius * 2.5f);
	}

	juce::ColourGradient bezelGrad(ColourPalette::backgroundLight.brighter(0.1f), cx - radius * 0.5f,
	                               cy - radius * 0.5f, ColourPalette::backgroundDeep, cx + radius * 0.5f,
	                               cy + radius * 0.5f, false);
	g.setGradientFill(bezelGrad);
	g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

	g.setColour(juce::Colours::black.withAlpha(0.5f));
	g.drawEllipse(cx - radius + 0.5f, cy - radius + 0.5f, radius * 2.0f - 1.0f, radius * 2.0f - 1.0f, 1.0f);

	g.setColour(juce::Colours::white.withAlpha(0.12f));
	juce::Path topHL;
	topHL.addEllipse(cx - radius * 0.85f, cy - radius * 0.85f, radius * 1.7f, radius * 0.7f);
	g.fillPath(topHL);

	const float domeR = radius * 0.55f;
	juce::Rectangle<float> dome(cx - domeR, cy - domeR, domeR * 2.0f, domeR * 2.0f);

	if (i > 0.05f)
	{
		auto bright = colour.brighter(0.4f);
		auto mid = colour;
		auto dark = colour.darker(0.6f);

		juce::ColourGradient domeGrad(bright, dome.getX() + domeR * 0.7f, dome.getY() + domeR * 0.6f, dark,
		                              dome.getCentreX(), dome.getCentreY() + domeR, true);
		domeGrad.addColour(0.6, mid);
		g.setGradientFill(domeGrad);
		g.fillEllipse(dome);

		g.setColour(juce::Colours::white.withAlpha(0.6f * i));
		g.fillEllipse(dome.getX() + domeR * 0.55f, dome.getY() + domeR * 0.45f, domeR * 0.5f, domeR * 0.4f);
	}
	else
	{
		g.setColour(colour.withAlpha(0.35f).darker(0.6f));
		g.fillEllipse(dome);
	}
}

void CrossfaderComponent::drawSegmentedCurveBackground(juce::Graphics &g) const
{
	if (curveButtonsRowBounds.isEmpty())
		return;

	auto sliderTrack = curveButtonsRowBounds.toFloat();

	g.setColour(juce::Colours::black.withAlpha(0.4f));
	g.fillRoundedRectangle(sliderTrack, Obsidian::CORNER);

	g.setColour(ColourPalette::backgroundLight);
	g.drawRoundedRectangle(sliderTrack.reduced(0.5f), Obsidian::CORNER, 0.8f);

	int activeMode = audioProcessor.getCrossfaderCurveMode();
	juce::Rectangle<int> activeBounds;
	if (activeMode == 0)
		activeBounds = curveLinearButton.getBounds();
	else if (activeMode == 1)
		activeBounds = curveEqualPowerButton.getBounds();
	else
		activeBounds = curveDjButton.getBounds();

	if (!activeBounds.isEmpty())
	{
		auto active = activeBounds.toFloat().reduced(2.0f);

		g.setColour(juce::Colours::black.withAlpha(0.4f));
		g.fillRoundedRectangle(active.translated(0, 1.0f), Obsidian::CORNER);

		juce::ColourGradient bodyGrad(ColourPalette::lightGrey.withAlpha(0.45f), active.getX(), active.getY(),
		                              ColourPalette::lightGrey.withAlpha(0.20f), active.getX(), active.getBottom(),
		                              false);
		g.setGradientFill(bodyGrad);
		g.fillRoundedRectangle(active, Obsidian::CORNER);

		g.setColour(juce::Colours::white.withAlpha(0.06f));
		g.fillRoundedRectangle(active.withHeight(active.getHeight() * 0.45f), Obsidian::CORNER);

		g.setColour(ColourPalette::lightGrey.withAlpha(0.7f));
		g.drawRoundedRectangle(active.reduced(0.5f), Obsidian::CORNER, 1.0f);
	}
}

void CrossfaderComponent::paintOverChildren(juce::Graphics &g)
{
	juce::Font monoBold(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
	g.setFont(monoBold);

	bool hostIsPlaying = false;
	double currentPpq = 0.0;

	if (auto *ph = audioProcessor.getPlayHead())
	{
		if (auto pos = ph->getPosition())
		{
			hostIsPlaying = pos->getIsPlaying();
			if (auto ppq = pos->getPpqPosition())
				currentPpq = *ppq;
		}
	}

	float beatPhaseLocal = (float)(currentPpq - std::floor(currentPpq));

	float pulseIntensity;
	if (hostIsPlaying)
	{
		pulseIntensity = std::pow(1.0f - beatPhaseLocal, 2.5f);
		pulseIntensity = juce::jmax(0.25f, pulseIntensity);
	}
	else
	{
		pulseIntensity = 1.0f;
	}

	float globalX = audioProcessor.getGlobalCrossfaderValue();
	float deckAGain = 1.0f - globalX;
	float deckBGain = globalX;

	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		auto rowBounds = pairRowBounds[i];
		if (rowBounds.isEmpty())
			continue;

		auto *trackLeft = getTrackBySlot(audioProcessor, i);
		auto *trackRight = getTrackBySlot(audioProcessor, i + 4);

		juce::Colour leftColour = ColourPalette::sliderThumb;
		juce::Colour rightColour = ColourPalette::sliderThumb;
		auto &leftPage = trackLeft->getCurrentPage();
		auto &rightPage = trackRight->getCurrentPage();
		if (trackLeft)
			leftColour = AiModelDefinitions::getColourForModel(leftPage.selectedModel);
		if (trackRight)
			rightColour = AiModelDefinitions::getColourForModel(rightPage.selectedModel);

		float pairX = audioProcessor.getPairCrossfaderValue(i);
		float leftPairGain = 1.0f - pairX;
		float rightPairGain = pairX;

		float leftIntensity =
		    audioProcessor.isUsingCrossfader() ? juce::jmax(0.0f, deckAGain * leftPairGain * pulseIntensity) : 0.0f;
		float rightIntensity =
		    audioProcessor.isUsingCrossfader() ? juce::jmax(0.0f, deckBGain * rightPairGain * pulseIntensity) : 0.0f;

		const float ledD = (float)ledDiameter;
		const int rowH = rowBounds.getHeight();
		const int rowCY = rowBounds.getCentreY();

		juce::Rectangle<float> ledLeftBounds((float)(rowBounds.getX() + ledPadX), (float)(rowCY)-ledD * 0.5f, ledD,
		                                     ledD);
		drawHardwareLED(g, ledLeftBounds, leftColour, leftIntensity, hostIsPlaying);

		g.setColour(ColourPalette::textPrimary);
		juce::Rectangle<int> labelLeft((int)(ledLeftBounds.getRight() + 6.0f), rowBounds.getY(), labelWidth, rowH);
		g.drawText("T" + juce::String(i + 1), labelLeft, juce::Justification::centredLeft);

		juce::Rectangle<float> ledRightBounds((float)(rowBounds.getRight() - ledPadX) - ledD,
		                                      (float)(rowCY)-ledD * 0.5f, ledD, ledD);
		drawHardwareLED(g, ledRightBounds, rightColour, rightIntensity, hostIsPlaying);

		juce::Rectangle<int> labelRight((int)(ledRightBounds.getX() - 6.0f - labelWidth), rowBounds.getY(), labelWidth,
		                                rowH);
		g.setColour(ColourPalette::textPrimary);
		g.drawText("T" + juce::String(i + 5), labelRight, juce::Justification::centredRight);
	}
}

void CrossfaderComponent::resized()
{
	auto area = getLocalBounds().reduced(8, 6);

	const int segmentedH = 26;
	const int segmentedTopGap = 6;

	useCrossfaderButton.setBounds(area.removeFromTop(20).removeFromLeft(20));

	auto segmentedBand = area.removeFromBottom(segmentedH);

	curveButtonsRowBounds = segmentedBand;

	const int innerPad = 3;
	auto innerSeg = segmentedBand.reduced(innerPad);
	const int segW = innerSeg.getWidth() / 3;

	curveLinearButton.setBounds(innerSeg.removeFromLeft(segW));
	curveEqualPowerButton.setBounds(innerSeg.removeFromLeft(segW));
	curveDjButton.setBounds(innerSeg);

	area.removeFromBottom(segmentedTopGap);

	const int rowSpacing = 2;
	const int rowHeight = (area.getHeight() - rowSpacing * 3) / 4;

	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		auto rowArea = (i == 3) ? area : area.removeFromTop(rowHeight);
		pairRowBounds[i] = rowArea;

		const int sideW = ledZoneWidth;
		auto sliderArea = rowArea;
		sliderArea.removeFromLeft(sideW);
		sliderArea.removeFromRight(sideW);
		pairSliders[i].setBounds(sliderArea);

		if (i < 3)
			area.removeFromTop(rowSpacing);
	}

	globalSlider.setVisible(false);
}