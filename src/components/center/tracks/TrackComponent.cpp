#include "TrackComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "WaveformDisplay.h"

TrackComponent::BorderOverlay::BorderOverlay()
{
	setInterceptsMouseClicks(false, false);
	setOpaque(false);
	blockedIcon = juce::Drawable::createFromImageData(BinaryData::prohibit_svg, BinaryData::prohibit_svgSize);
	blockedIcon->replaceColour(juce::Colours::black, ColourPalette::buttonDangerLight);
}

void TrackComponent::BorderOverlay::setVisualState(bool generating, bool samplePending, bool selected, bool dragOver,
                                                   bool blink, juce::Colour modelColour)
{
	if (generating == isGenerating && samplePending == hasSamplePending && selected == isSelected &&
	    dragOver == isDragOver && blink == blinkState && modelColour == accentColour)
		return;

	isGenerating = generating;
	hasSamplePending = samplePending;
	isSelected = selected;
	isDragOver = dragOver;
	blinkState = blink;
	accentColour = modelColour;
	repaint();
}

void TrackComponent::BorderOverlay::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	if (isDragOver && isGenerating)
	{
		auto blockedColour = ColourPalette::buttonDangerDark;

		g.setColour(blockedColour.withAlpha(Obsidian::ALPHA_01));
		g.fillRoundedRectangle(bounds, Obsidian::CORNER);

		g.setColour(blockedColour);
		g.drawRoundedRectangle(bounds.reduced(1.0f), Obsidian::CORNER, 2.5f);

		const float iconSize = 24.0f;
		const float gap = 6.0f;
		const float textHeight = 16.0f;
		const float blockHeight = iconSize + gap + textHeight;

		auto centreY = bounds.getCentreY() - blockHeight * 0.5f;

		if (blockedIcon != nullptr)
		{
			juce::Rectangle<float> iconArea(bounds.getCentreX() - iconSize * 0.5f, centreY, iconSize, iconSize);
			blockedIcon->drawWithin(g, iconArea, juce::RectanglePlacement::centred, 1.0f);
		}

		g.setColour(blockedColour);
		g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
		juce::Rectangle<float> textArea(bounds.getX(), centreY + iconSize + gap, bounds.getWidth(), textHeight);
		g.drawText("Drop disabled while generating.", textArea, juce::Justification::centred, false);

		return;
	}

	juce::Colour bgColour;
	bool fillBg = true;

	if (isDragOver && !isGenerating)
		bgColour = ColourPalette::buttonSuccess.withAlpha(0.4f);
	else if (hasSamplePending && !isGenerating)
		bgColour = ColourPalette::samplePending.withAlpha(0.15f);
	else
		fillBg = false;

	if (fillBg)
	{
		g.setColour(bgColour);
		g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	}

	juce::Colour borderColour;
	float borderWidth;

	if (isGenerating)
	{
		borderColour = blinkState ? accentColour.brighter(0.4f) : accentColour.darker(0.4f);
		borderWidth = 3.0f;
	}
	else if (hasSamplePending)
	{
		borderColour = ColourPalette::samplePending;
		borderWidth = 2.0f;
	}
	else if (isSelected)
	{
		borderColour = ColourPalette::lightGrey;
		borderWidth = 2.0f;
	}
	else
	{
		borderColour = ColourPalette::backgroundLight;
		borderWidth = 1.0f;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.reduced(1.0f), Obsidian::CORNER, borderWidth);

	if (flashAmount > 0.01f)
	{
		g.setColour(accentColour.withAlpha(flashAmount * 0.6f));
		g.drawRoundedRectangle(bounds.reduced(1.0f), Obsidian::CORNER, 2.5f + flashAmount * 2.5f);
	}
}

TrackComponent::TrackComponent(const juce::String &trackId, DjIaVstProcessor &processor)
    : ObsidianBaseMidiComponent(processor), trackId(trackId)
{
	setupUI();
	setupAdsrKnobs();
	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

TrackComponent::~TrackComponent()
{
	markForDestruction();
	vBlankAttachment.reset();
	clearAllBindings();
	setVisible(false);

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
	{
		pageButtons[i].onClick = nullptr;
	}

	sequencer.reset();
	waveformDisplay.reset();
	drawingCanvas.reset();

	if (auto *t = getTrack())
		t->onPlayStateChanged = nullptr;
	track = nullptr;
}

void TrackComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
	auto *t = getTrack();
	if (!t)
		return;
	for (auto &state : lastPageStates)
		state = PageButtonState{};
	setupPagesUI();
	if (t && t->slotIndex != -1)
		wireParameters();
	refreshWaveformDisplay();
	updateFromTrackData();
	setSelected(t->isSelected.load());
}

bool TrackComponent::isWaveformVisible() const
{
	return waveformDisplay && waveformDisplay->isVisible();
}

void TrackComponent::updateWaveformWithTimeStretch()
{
	calculateHostBasedDisplay();
}

void TrackComponent::onParameterChangedUI(const juce::String &paramSuffix, float newValue)
{
	if (paramSuffix == "Generate")
	{
		if (newValue < 0.5f)
			return;
		if (audioProcessor.getIsGenerating())
			return;
		if (onGenerateForTrack)
		{
			if (track)
			{
				auto &currentPage = track->getCurrentPage();
				currentPage.setSelectedPrompt(getSelectedPromptValue());
				currentPage.generationBpm = audioProcessor.getGlobalBpm();
				currentPage.generationKey = audioProcessor.getGlobalKey();
				currentPage.generationDuration = audioProcessor.getGlobalDuration();
			}
			onGenerateForTrack(trackId);
			return;
		}
	}
	else if (paramSuffix == "BeatRepeatActive")
	{
		auto *t = getTrack();
		if (t)
		{
			updateBeatRepeatButtonState();
			statusCallback("Beat Repeat " + juce::String(track->randomRetriggerEnabled.load() ? "ON" : "OFF"));
		}
	}
	else if (paramSuffix == "ReverseActive")
	{
		auto *t = getTrack();
		if (t)
		{
			updateReverseButtonState();
			statusCallback("Reverse " + juce::String(track->reverseActive.load() ? "OFF" : "ON"));
		}
	}
	else if (paramSuffix == "TransientScatterActive")
	{
		auto *t = getTrack();
		if (t)
		{
			updateTransientScatterButtonState();
			statusCallback("Transient Scatter " + juce::String(track->transientScatterActive.load() ? "OFF" : "ON"));
		}
	}
	else if (paramSuffix == "BeatRepeatInterval")
	{
		if (auto *t = getTrack())
			onIntervalChanged();
	}
	else if (paramSuffix == "Gain")
	{
		if (auto *t = getTrack())
			if (waveformDisplay)
				refreshWaveformDisplay();
	}
	else if (paramSuffix == "AdsrAttack" || paramSuffix == "AdsrDecay" || paramSuffix == "AdsrSustain" ||
	         paramSuffix == "AdsrRelease")
		syncAdsrToWaveform();
}

void TrackComponent::calculateHostBasedDisplay()
{
	auto *t = getTrack();
	if (!t)
		return;
	auto &currentPage = t->getCurrentPage();
	if (currentPage.numSamples == 0)
		return;
	float effectiveBpm = calculateEffectiveBpm();
	if (waveformDisplay)
	{
		waveformDisplay->setOriginalBpm(currentPage.originalBpm);
		waveformDisplay->setSampleBpm(effectiveBpm);
		if (!currentPage.audioFilePath.isEmpty())
		{
			juce::File audioFile(currentPage.audioFilePath);
			waveformDisplay->setAudioFile(audioFile);
		}
	}
}

void TrackComponent::updatePlaybackPosition(double timeInSeconds)
{
	auto *t = getTrack();
	if (!t)
		return;

	if (waveformDisplay)
	{
		bool isPlaying = t->isPlaying.load();
		waveformDisplay->setPlaybackPosition(timeInSeconds, isPlaying);
	}
}

juce::Colour TrackComponent::getCurrentModelColour() const
{
	auto *t = getTrack();
	juce::String currentModel = modelSelector.getText();
	auto &currentPage = t->getCurrentPage();
	if (currentModel.isEmpty() && t)
		currentModel = currentPage.selectedModel;
	if (currentModel.isEmpty())
	{
		const bool isLocalMode = audioProcessor.getUseLocalModel();
		auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);
		if (!modelsForMode.isEmpty())
			currentModel = modelsForMode[0];
	}
	return AiModelDefinitions::getColourForModel(currentModel);
}

void TrackComponent::syncBorderOverlay()
{
	juce::Colour overlayColour = cachedModelColour;
	if (isDragOver && isDraggingPrompt)
		overlayColour = ColourPalette::violet;

	borderOverlay.setVisualState(isGenerating, track->hasSamplePending.load(), isSelected, isDragOver, blinkState,
	                             overlayColour);
}

void TrackComponent::updateFromTrackData()
{
	auto *t = getTrack();
	if (!t)
		return;

	juce::String modelToSet = t->getCurrentPage().selectedModel;
	const bool isLocalMode = audioProcessor.getUseLocalModel();
	auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);

	if (modelToSet.isEmpty() || !modelsForMode.contains(modelToSet))
	{
		if (!isLocalMode && !t->getCurrentPage().savedModelBeforeLocal.isEmpty() &&
		    modelsForMode.contains(t->getCurrentPage().savedModelBeforeLocal))
			modelToSet = t->getCurrentPage().savedModelBeforeLocal;
		else
			modelToSet = modelsForMode[0];
		t->getCurrentPage().selectedModel = modelToSet;
	}

	modelSelector.setText(modelToSet, juce::dontSendNotification);
	if (modelSet != modelToSet)
	{
		modelSet = modelToSet;
		updateModelUI();
	}

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pageButtons[i].setVisible(true);

	pageButtons[t->currentPageIndex.load()].setToggleState(true, juce::dontSendNotification);
	updatePagesDisplay();

	randomDurationToggle.setToggleState(t->randomRetriggerDurationEnabled.load(), juce::dontSendNotification);

	bool hasOriginal = false;
	bool useOriginal = false;

	const auto &currentPage = t->getCurrentPage();
	hasOriginal = currentPage.hasOriginalVersion.load();
	useOriginal = hasOriginal && currentPage.useOriginalFile.load();

	originalSyncButton.setToggleState(useOriginal, juce::dontSendNotification);

	if (waveformDisplay)
	{
		bool isCurrentlyPlaying = t->isPlaying.load();
		if (currentPage.numSamples > 0 && currentPage.sampleRate > 0)
		{
			double startSample = currentPage.loopStart * currentPage.sampleRate;
			double currentTimeInSection = (startSample + t->readPosition.load()) / currentPage.sampleRate;
			calculateHostBasedDisplay();
			waveformDisplay->setPlaybackPosition(currentTimeInSection, isCurrentlyPlaying);
		}
	}

	if (!intervalKnob.isMouseButtonDown())
	{
		int interval = t->randomBeatRepeatInterval.load();
		intervalKnob.setValue(interval, juce::dontSendNotification);
		intervalLabel.setText(getIntervalName(interval), juce::dontSendNotification);
	}

	updateBeatRepeatButtonState();
	updateReverseButtonState();
	updateTransientScatterButtonState();
	updateRandomDurationButtonColor();

	updateButtonsEnabledState();

	if (t->isCurrentlyPlaying.load() && !isPreviewPlaying)
		previewButton.setEnabled(false);

	updateTrackInfo();
	updateAdsrKnobsFromPage();
}

float TrackComponent::calculateEffectiveBpm()
{
	auto *t = getTrack();
	if (!t)
		return 126.0f;

	auto &currentPage = t->getCurrentPage();
	float effectiveBpm = currentPage.originalBpm;

	double hostBpm = audioProcessor.getHostBpm();
	if (hostBpm > 0.0 && currentPage.originalBpm > 0.0)
	{
		effectiveBpm = (float)hostBpm;
		float pitchSemis = currentPage.pitchSemitones.load();
		float fineCents = currentPage.fineOffset.load();
		float totalSemis = pitchSemis + (fineCents / 100.0f);

		if (std::abs(totalSemis) > 0.001f)
			effectiveBpm *= std::pow(2.0f, totalSemis / 12.0f);
	}

	return juce::jlimit(40.0f, 250.0f, effectiveBpm);
}

void TrackComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDark.withAlpha(Obsidian::ALPHA_08));
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
}

void TrackComponent::setupAdsrKnobs()
{
	auto setupKnob = [&](MidiLearnableSlider &knob, juce::Label &label, const char *name, float rMin, float rMax,
	                     float def, const char *tooltip)
	{
		addAndMakeVisible(knob);
		knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		knob.setRange(rMin, rMax, 0.0);
		knob.setSkewFactorFromMidPoint(rMin + (rMax - rMin) * 0.3f);
		knob.setValue(def, juce::dontSendNotification);
		knob.setDoubleClickReturnValue(true, def);
		knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		knob.setTooltip(tooltip);
		addAndMakeVisible(label);
		label.setText(name, juce::dontSendNotification);
		label.setJustificationType(juce::Justification::centred);
		label.setFont(juce::FontOptions(9.0f));
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	};

	setupKnob(adsrAttackKnob, adsrAttackLabel, "A", Obsidian::ADSRDefaultValues::ATTACK_MIN,
	          Obsidian::ADSRDefaultValues::ATTACK_MAX, Obsidian::ADSRDefaultValues::ATTACK_DEFAULT,
	          "ADSR Attack time (seconds)");

	setupKnob(adsrDecayKnob, adsrDecayLabel, "D", Obsidian::ADSRDefaultValues::DECAY_MIN,
	          Obsidian::ADSRDefaultValues::DECAY_MAX, Obsidian::ADSRDefaultValues::DECAY_DEFAULT,
	          "ADSR Decay time (seconds)");

	setupKnob(adsrSustainKnob, adsrSustainLabel, "S", Obsidian::ADSRDefaultValues::SUSTAIN_MIN,
	          Obsidian::ADSRDefaultValues::SUSTAIN_MAX, Obsidian::ADSRDefaultValues::SUSTAIN_DEFAULT,
	          "ADSR Sustain level (0-1)");

	setupKnob(adsrReleaseKnob, adsrReleaseLabel, "R", Obsidian::ADSRDefaultValues::RELEASE_MIN,
	          Obsidian::ADSRDefaultValues::RELEASE_MAX, Obsidian::ADSRDefaultValues::RELEASE_DEFAULT,
	          "ADSR Release time (seconds)");

	adsrSustainKnob.setSkewFactor(1.0);
}

void TrackComponent::updateAdsrKnobsFromPage()
{
	auto *t = getTrack();
	if (!t)
		return;

	const auto &page = t->getCurrentPage();
	adsrAttackKnob.setValue(page.adsrAttack, juce::dontSendNotification);
	adsrDecayKnob.setValue(page.adsrDecay, juce::dontSendNotification);
	adsrSustainKnob.setValue(page.adsrSustain, juce::dontSendNotification);
	adsrReleaseKnob.setValue(page.adsrRelease, juce::dontSendNotification);

	syncAdsrToWaveform();
}

void TrackComponent::syncAdsrToWaveform()
{
	auto *t = getTrack();
	if (!waveformDisplay || !t)
		return;
	const auto &page = t->getCurrentPage();
	waveformDisplay->setAdsrParams(page.adsrAttack, page.adsrDecay, page.adsrSustain, page.adsrRelease);
}

void TrackComponent::resized()
{
	auto *t = getTrack();
	if (!t)
		return;
	auto fullBounds = getLocalBounds();

	auto area = fullBounds.reduced(6);
	auto headerArea = area.removeFromTop(32);
	auto &currentPage = t->getCurrentPage();
	auto pagesArea = headerArea.removeFromLeft(38);
	int pagesGridHeight = Obsidian::PAGE_BUTTON_SIZE * 2 + 2;
	int yOffset = (pagesArea.getHeight() - pagesGridHeight) / 2;
	pagesArea.removeFromTop(yOffset);
	pagesArea.setHeight(pagesGridHeight);
	layoutPagesButtons(pagesArea);

	{
		const int rightElementsWidth = 36 + Obsidian::SPACER_XS + 36 + Obsidian::SPACER_XS + Obsidian::SPACER_XS + 34 +
		                               Obsidian::SPACER_XS + 34 + Obsidian::SPACER_XS + 38 + Obsidian::SPACER_XS +
		                               (32 * 4);

		const int pagesWidth = 38;
		const int availableForSelectors =
		    headerArea.getWidth() - pagesWidth - rightElementsWidth - Obsidian::SPACER_SM * 2;
		const int selectorsWidth = std::max(availableForSelectors, 120);
		auto selectorsArea = headerArea.removeFromLeft(selectorsWidth);
		selectorsArea.removeFromTop(2);
		const int selectorHeight = 16;
		const int gap = 3;
		const int totalStackHeight = selectorHeight * 2 + gap;
		int selectorsYOffset = (selectorsArea.getHeight() - totalStackHeight) / 2;

		promptPresetSelector.setBounds(selectorsArea.getX(), selectorsArea.getY() + selectorsYOffset,
		                               selectorsArea.getWidth(), selectorHeight);

		modelSelector.setBounds(selectorsArea.getX(), selectorsArea.getY() + selectorsYOffset + selectorHeight + gap,
		                        selectorsArea.getWidth(), selectorHeight);
	}

	headerArea.removeFromLeft(Obsidian::SPACER_XS);

	{
		const int createButtonWidth = 28;
		generateButton.setBounds(headerArea.removeFromRight(createButtonWidth));
	}
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	const int iconBtnWidth = 26;

	originalSyncButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	previewButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	transientScatterButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	reverseButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	beatRepeatButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	randomDurationToggle.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(Obsidian::SPACER_XS);

	{
		auto knobArea = headerArea.removeFromRight(34);
		const int knobDiameter = 30;
		const int labelHeight = 8;
		const int stackHeight = knobDiameter + labelHeight;
		int knobsYOffset = (knobArea.getHeight() - stackHeight) / 2;
		intervalKnob.setBounds(knobArea.getX() + (knobArea.getWidth() - knobDiameter) / 2,
		                       knobArea.getY() + knobsYOffset, knobDiameter, knobDiameter);
		intervalLabel.setBounds(knobArea.getX(), knobArea.getY() + knobsYOffset + knobDiameter - 2, knobArea.getWidth(),
		                        labelHeight);
	}

	headerArea.removeFromLeft(Obsidian::SPACER_XS);
	{
		const int adsrKnobDiam = 30;
		const int adsrLabelH = 8;
		const int adsrStack = adsrKnobDiam + adsrLabelH;
		const int adsrSpacing = 0;
		const int adsrTotalW = (adsrKnobDiam + adsrSpacing) * 4;

		auto adsrArea = headerArea.removeFromRight(adsrTotalW);
		int adsrYOffset = (adsrArea.getHeight() - adsrStack) / 2;

		auto placeKnob = [&](MidiLearnableSlider &knob, juce::Label &label)
		{
			auto cell = adsrArea.removeFromLeft(adsrKnobDiam);
			adsrArea.removeFromLeft(adsrSpacing);
			knob.setBounds(cell.getX(), cell.getY() + adsrYOffset, adsrKnobDiam, adsrKnobDiam);
			label.setBounds(cell.getX(), cell.getY() + adsrYOffset + adsrKnobDiam - 2, adsrKnobDiam, adsrLabelH);
		};

		placeKnob(adsrAttackKnob, adsrAttackLabel);
		placeKnob(adsrDecayKnob, adsrDecayLabel);
		placeKnob(adsrSustainKnob, adsrSustainLabel);
		placeKnob(adsrReleaseKnob, adsrReleaseLabel);
	}

	if (!waveformDisplay)
	{

		waveformDisplay = std::make_unique<WaveformDisplay>(audioProcessor, t);
		waveformDisplay->addMouseListener(this, false);
		waveformDisplay->onLoopPointsChanged = [this, t](double start, double end)
		{
			auto &currentPage = t->getCurrentPage();
			const double oldLoopStart = currentPage.loopStart;
			const double sr = currentPage.sampleRate;

			currentPage.loopStart = start;
			currentPage.loopEnd = end;

			waveformDisplay->setLoopPoints(start, end);

			if (t->isPlaying.load())
			{
				const double newStartSample = start * sr;
				const double newEndSample = end * sr;
				const double oldStartSample = oldLoopStart * sr;

				double currentAbs = oldStartSample + t->readPosition.load();
				double newRelative = currentAbs - newStartSample;

				if (currentAbs < newStartSample || currentAbs >= newEndSample)
				{
					t->readPosition.store(0.0);
					t->numSamplesAccPerSequence.store(0.0);
				}
				else
					t->readPosition.store(newRelative);
			}
		};

		waveformDisplay->onAdsrAttackChanged = [this](float v)
		{
			auto *t = getTrack();
			if (!t)
				return;
			t->getCurrentPage().adsrAttack.store(v);
			if (waveformDisplay)
				waveformDisplay->repaint();
		};

		waveformDisplay->onAdsrDecayChanged = [this](float v)
		{
			auto *t = getTrack();
			if (!t)
				return;
			t->getCurrentPage().adsrDecay.store(v);
			if (waveformDisplay)
				waveformDisplay->repaint();
		};

		waveformDisplay->onAdsrSustainChanged = [this](float v)
		{
			auto *t = getTrack();
			if (!t)
				return;
			t->getCurrentPage().adsrSustain.store(v);
			if (waveformDisplay)
				waveformDisplay->repaint();
		};

		waveformDisplay->onAdsrReleaseChanged = [this](float v)
		{
			auto *t = getTrack();
			if (!t)
				return;
			t->getCurrentPage().adsrRelease.store(v);
			if (waveformDisplay)
				waveformDisplay->repaint();
		};

		addAndMakeVisible(*waveformDisplay);

		if (currentPage.numSamples > 0)
		{
			waveformDisplay->setAudioData(currentPage.audioBuffer, currentPage.sampleRate);
			waveformDisplay->setLoopPoints(currentPage.loopStart, currentPage.loopEnd);
			calculateHostBasedDisplay();
		}
	}

	if (waveformDisplay)
	{
		area.removeFromTop(8);
		waveformDisplay->setBounds(area.removeFromTop(Obsidian::WAVEFORM_HEIGHT));
		waveformDisplay->setVisible(true);
	}

	if (!sequencer)
	{
		sequencer = std::make_unique<SequencerComponent>(trackId, audioProcessor);
		addAndMakeVisible(*sequencer);
		sequencer->addMouseListener(this, false);
		sequencerVisible = true;

		juce::String currentModel = modelSelector.getText();
		if (currentModel.isEmpty() && t)
			currentModel = currentPage.selectedModel;

		if (currentModel.isEmpty())
		{
			const bool isLocalMode = audioProcessor.getUseLocalModel();
			auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);
			if (!modelsForMode.isEmpty())
				currentModel = modelsForMode[0];
		}

		sequencer->setAccentColour(AiModelDefinitions::getColourForModel(currentModel));
	}

	if (sequencer)
	{
		area.removeFromTop(5);
		sequencer->setBounds(area.removeFromTop(Obsidian::SEQUENCER_HEIGHT));
		sequencer->setVisible(true);
	}

	borderOverlay.setBounds(fullBounds);
	borderOverlay.toFront(false);
}

void TrackComponent::openDrawingCanvas()
{
	auto *t = getTrack();
	if (!t)
		return;
	if (canvasModalOpen)
		return;
	canvasModalOpen = true;

	auto *canvas = ObsidianAlertManager::showDrawingCanvas(
	    this, audioProcessor, [this](const juce::String &) {},
	    [this, t](DrawingCanvas *canvas)
	    {
		    if (canvas)
		    {
			    auto canvasState = canvas->getState();
			    juce::String stateXml = canvasState.toXml();
			    auto &currentPage = t->getCurrentPage();
			    currentPage.canvasState = stateXml;
			    currentPage.canvasData = canvasState.imageBase64;
			    currentPage.selectedKeywords = canvasState.selectedKeywords;
		    }
		    canvasModalOpen = false;
	    });

	if (canvas == nullptr)
		return;
	drawingCanvasPtr = canvas;

	const auto &currentPage = t->getCurrentPage();
	if (!currentPage.canvasState.isEmpty())
	{
		auto state = DrawingCanvas::CanvasState::fromXml(currentPage.canvasState);
		canvas->setState(state);
	}
	else if (!currentPage.canvasData.isEmpty())
		canvas->loadFromBase64(currentPage.canvasData);

	canvas->setGenerating(canvasIsGenerating);

	canvas->onGenerate = [this, canvas](const juce::String &base64Image)
	{
		auto *t = getTrack();
		if (!t)
			return;

		auto canvasState = canvas->getState();
		juce::String stateXml = canvasState.toXml();
		auto &currentPage = t->getCurrentPage();
		currentPage.canvasState = stateXml;
		currentPage.canvasData = base64Image;
		currentPage.selectedKeywords = canvasState.selectedKeywords;

		if (onGenerateWithImage)
		{
			auto keywords = canvas->getState().selectedKeywords;
			onGenerateWithImage(trackId, base64Image, keywords);
		}
	};
}

void TrackComponent::layoutPagesButtons(juce::Rectangle<int> area)
{
	int buttonSize = Obsidian::PAGE_BUTTON_SIZE;
	int spacing = 2;

	area.removeFromTop(spacing);
	auto topRow = area.removeFromTop(buttonSize);
	pageButtons[0].setBounds(topRow.removeFromLeft(buttonSize));
	topRow.removeFromLeft(spacing);
	pageButtons[1].setBounds(topRow.removeFromLeft(buttonSize));

	area.removeFromTop(spacing);

	auto bottomRow = area.removeFromTop(buttonSize);
	pageButtons[2].setBounds(bottomRow.removeFromLeft(buttonSize));
	bottomRow.removeFromLeft(spacing);
	pageButtons[3].setBounds(bottomRow.removeFromLeft(buttonSize));
}

void TrackComponent::setupPagesUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	const char *pageLabels[4] = {"A", "B", "C", "D"};
	const char *pageNames[4] = {"PageA", "PageB", "PageC", "PageD"};

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
	{
		addChildComponent(pageButtons[i]);
		pageButtons[i].setButtonText(pageLabels[i]);
		pageButtons[i].setClickingTogglesState(true);

		int groupId = 1000;

		groupId += t->slotIndex;

		pageButtons[i].setRadioGroupId(groupId);

		pageButtons[i].onClick = [this, i]() { onPageSelected(i); };

		pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
		pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight);
		pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);

		juce::String tooltip =
		    "Page " + juce::String(pageLabels[i]) + " - holds its own sample, prompt, sequence and settings.\n" +
		    "In performance mode, page switch is quantized to the next bar for sync.\n" +
		    "Switching to an empty page stops the track.\n" + "(Inspired by ReBirth RB-338's pattern banking)";
		pageButtons[i].setTooltip(tooltip);

		registerMidiLearn(pageNames[i], &pageButtons[i]);
	}
}

void TrackComponent::onPageSelected(int pageIndex)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= Obsidian::MAX_PAGES)
		return;

	if (t->currentPageIndex.load() == pageIndex && !t->pageChangePending.load())
	{
		pageButtons[pageIndex].setToggleState(true, juce::dontSendNotification);
		return;
	}

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pageButtons[i].setToggleState(i == t->currentPageIndex.load(), juce::dontSendNotification);

	if (t->pageChangePending.load() && t->pendingPageIndex.load() == pageIndex)
	{
		t->pageChangePending.store(false);
		t->pendingPageIndex.store(-1);
		lastPageStates[pageIndex] = PageButtonState{};
		updatePagesDisplay();
		statusCallback("Page change cancelled");
		return;
	}

	if (t->slotIndex != -1)
	{
		const char *pageNames[4] = {"PageA", "PageB", "PageC", "PageD"};
		juce::String paramName = "slot" + juce::String(t->slotIndex + 1) + pageNames[pageIndex];

		auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
		if (param)
			param->setValueNotifyingHost(1.0f);
	}
}

void TrackComponent::performPageChange(int pageIndex)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= Obsidian::MAX_PAGES)
		return;

	if (isPreviewPlaying)
	{
		if (onStopPreview)
			onStopPreview(trackId);
		setPreviewPlaying(false);
	}

	bool wasPlaying = t->isPlaying.load();
	bool wasArmed = t->isArmed.load();
	bool wasArmedToStop = t->isArmedToStop.load();
	bool wasCurrentlyPlaying = t->isCurrentlyPlaying.load();

	t->setCurrentPage(pageIndex);

	t->isPlaying.store(wasPlaying);
	t->isArmed.store(wasArmed);
	t->isArmedToStop.store(wasArmedToStop);
	t->isCurrentlyPlaying.store(wasCurrentlyPlaying);
	t->readPosition.store(0.0);
	t->numSamplesAccPerSequence.store(0.0);

	const auto &newPage = t->getCurrentPage();

	if (newPage.numSamples == 0)
		if (t->onPlayStateChanged)
			t->onPlayStateChanged(false);

	t->pageChangePending.store(false);
	t->pendingPageIndex.store(-1);

	updateFromTrackData();

	populatePromptPresets(newPage.selectedModel, newPage.selectedPrompt);

	if (sequencer)
	{
		sequencer->updateSequenceButtonsDisplay();
		sequencer->updateFromTrackData();
	}

	if (waveformDisplay)
	{
		if (newPage.numSamples > 0 && newPage.isLoaded.load())
		{
			waveformDisplay->setAudioData(newPage.audioBuffer, newPage.sampleRate);
			waveformDisplay->setLoopPoints(newPage.loopStart, newPage.loopEnd);
			if (!newPage.audioFilePath.isEmpty())
				waveformDisplay->setAudioFile(juce::File(newPage.audioFilePath));
			calculateHostBasedDisplay();
		}
		else
		{
			juce::AudioBuffer<float> emptyBuffer;
			emptyBuffer.setSize(2, 0);
			waveformDisplay->setAudioData(emptyBuffer, Obsidian::SAMPLERATE);
			waveformDisplay->setLoopPoints(0.0, 0.0);
		}
	}

	if (!newPage.isLoaded.load() && !newPage.audioFilePath.isEmpty())
		loadPageIfNeeded(pageIndex);

	char pageName = 'A' + static_cast<char>(pageIndex);
	statusCallback("Switched to page " + juce::String(pageName));
}

void TrackComponent::mouseDown(const juce::MouseEvent &)
{
	if (onSelectTrack)
		onSelectTrack(trackId);
}

void TrackComponent::updatePagesDisplay()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto modelColour = cachedModelColour;
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	int pendingPage = t->pageChangePending.load() ? t->pendingPageIndex.load() : -1;

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
	{
		PageButtonState newState;
		newState.isActive = (i == t->currentPageIndex.load());
		newState.isPending = (i == pendingPage);
		newState.hasAudio = t->pages[i].numSamples > 0;
		newState.blinkState = newState.isPending ? pageBlinkState : false;
		newState.modelColour = modelColour;

		if (newState == lastPageStates[i])
			continue;

		lastPageStates[i] = newState;

		if (newState.isPending)
		{
			auto blinkOn = modelColour.withAlpha(0.95f);
			auto blinkOff = modelColour.darker(0.5f).withAlpha(0.35f);

			pageButtons[i].setColour(juce::TextButton::buttonColourId, newState.blinkState ? blinkOn : blinkOff);
			pageButtons[i].setColour(juce::TextButton::textColourOffId,
			                         newState.blinkState ? textColour : modelColour.brighter(0.5f));
		}
		else if (newState.isActive)
		{
			pageButtons[i].setColour(juce::TextButton::buttonOnColourId, modelColour);
			pageButtons[i].setColour(juce::TextButton::textColourOnId, textColour);
		}
		else if (newState.hasAudio)
		{
			pageButtons[i].setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(0.3f));
			pageButtons[i].setColour(juce::TextButton::textColourOffId, modelColour.brighter(0.5f));
		}
		else
		{
			pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
			pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		}

		pageButtons[i].setToggleState(newState.isActive, juce::dontSendNotification);
		pageButtons[i].repaint();
	}
}

void TrackComponent::loadPageIfNeeded(int pageIndex)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= Obsidian::MAX_PAGES)
		return;

	auto &page = t->pages[pageIndex];
	if (page.isLoaded.load() || page.isLoading.load())
		return;

	page.isLoading = true;
	updatePagesDisplay();

	if (!page.audioFilePath.isEmpty())
	{
		juce::File audioFile(page.audioFilePath);
		if (audioFile.existsAsFile())
		{
			juce::Thread::launch([this, pageIndex, audioFile]() { loadPageAudioFile(pageIndex, audioFile); });
			return;
		}
	}

	page.isLoading = false;
	updatePagesDisplay();
}

void TrackComponent::loadPageAudioFile(int pageIndex, const juce::File &audioFile)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= Obsidian::MAX_PAGES)
		return;

	auto &page = t->pages[pageIndex];

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
		if (!reader)
		{
			page.isLoading = false;
			return;
		}

		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		page.audioBuffer.setSize(2, numSamples);
		reader->read(&page.audioBuffer, 0, numSamples, 0, true, true);

		if (numChannels == 1)
			page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);

		page.numSamples = numSamples;
		page.sampleRate = reader->sampleRate;
		page.isLoaded = true;
		page.isLoading = false;
		juce::Component::SafePointer<TrackComponent> safeThis(this);
		juce::MessageManager::callAsync(
		    [safeThis, pageIndex, t]()
		    {
			    if (safeThis)
			    {
				    if (t->currentPageIndex.load() == pageIndex)
				    {
					    safeThis->updateFromTrackData();
					    if (safeThis->waveformDisplay)
						    safeThis->refreshWaveformDisplay();
				    }
				    safeThis->updatePagesDisplay();
			    }
		    });
	}
	catch (const std::exception &)
	{
		page.isLoading = false;
		juce::Component::SafePointer<TrackComponent> safeThis(this);
		juce::MessageManager::callAsync(
		    [safeThis]()
		    {
			    if (safeThis)
				    safeThis->updatePagesDisplay();
		    });
	}
}

void TrackComponent::startGeneratingAnimation()
{
	isGenerating = true;

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pageButtons[i].setEnabled(false);

	syncBorderOverlay();
	blinkTicking = true;
}

void TrackComponent::stopGeneratingAnimation()
{
	auto *t = getTrack();
	if (!t)
		return;
	isGenerating = false;
	isDragOver = false;
	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pageButtons[i].setEnabled(true);

	if (waveformDisplay)
	{
		const auto &currentPage = t->getCurrentPage();
		if (currentPage.numSamples > 0)
		{
			waveformDisplay->setAudioData(currentPage.audioBuffer, currentPage.sampleRate);
			waveformDisplay->setLoopPoints(currentPage.loopStart, currentPage.loopEnd);
		}
	}
	syncBorderOverlay();
}

void TrackComponent::setSelected(bool s)
{
	isSelected = s;
	syncBorderOverlay();
}

void TrackComponent::handleVBlank()
{
	if (!blinkTicking)
		return;
	auto *t = getTrack();
	if (!t)
	{
		blinkTicking = false;
		return;
	}
	bool currentGlobalBlink = (juce::Time::getMillisecondCounter() / Obsidian::BLINKING_DURATION_TIME) % 2 == 0;
	bool stateChanged = false;
	if (isGenerating)
	{
		if (blinkState != currentGlobalBlink)
		{
			blinkState = currentGlobalBlink;
			syncBorderOverlay();
			stateChanged = true;
		}
	}
	if (t->pageChangePending.load())
	{
		if (pageBlinkState != currentGlobalBlink)
		{
			pageBlinkState = currentGlobalBlink;
			updatePagesDisplay();
			stateChanged = true;
		}
	}

	bool flashActive = false;
	if (borderOverlay.flashAmount > 0.01f)
	{
		borderOverlay.flashAmount *= 0.82f;
		if (borderOverlay.flashAmount <= 0.01f)
			borderOverlay.flashAmount = 0.0f;
		borderOverlay.repaint();
		flashActive = borderOverlay.flashAmount > 0.0f;
	}

	blinkTicking = isGenerating || t->pageChangePending.load() || flashActive;
}

void TrackComponent::refreshWaveformDisplay()
{
	auto *t = getTrack();
	if (!t)
		return;

	if (!waveformDisplay)
		return;

	const auto &currentPage = t->getCurrentPage();

	if (currentPage.numSamples > 0 && currentPage.isLoaded.load())
	{
		waveformDisplay->setAudioData(currentPage.audioBuffer, currentPage.sampleRate);
		waveformDisplay->setLoopPoints(currentPage.loopStart, currentPage.loopEnd);

		if (!currentPage.audioFilePath.isEmpty())
		{
			juce::File audioFile(currentPage.audioFilePath);
			waveformDisplay->setAudioFile(audioFile);
		}
		calculateHostBasedDisplay();
	}
	else
	{
		juce::AudioBuffer<float> emptyBuffer;
		emptyBuffer.setSize(2, 0);
		waveformDisplay->setAudioData(emptyBuffer, Obsidian::SAMPLERATE);
		waveformDisplay->setLoopPoints(0.0, 0.0);
	}
}

void TrackComponent::setGenerateButtonEnabled(bool enabled)
{
	generateButton.setEnabled(enabled);
}

void TrackComponent::setupUI()
{
	addAndMakeVisible(infoLabel);
	infoLabel.setText("Empty track - Generate your sample!", juce::dontSendNotification);
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR));

	promptPresetSelector.setTooltip("Select prompt for this page");
	promptPresetSelector.onChange = [this]() { onTrackPresetSelected(); };

	modelSelector.clear();

	addAndMakeVisible(promptPresetSelector);
	addAndMakeVisible(modelSelector);
	modelSelector.setTooltip("Select model for this page");

	const bool isLocalMode = audioProcessor.getUseLocalModel();
	auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);
	for (int i = 0; i < modelsForMode.size(); ++i)
		modelSelector.addItem(modelsForMode[i], i + 1);

	int trackNum = trackId.retainCharacters("0123456789").getIntValue();
	if (trackNum >= 1 && trackNum <= modelsForMode.size())
		modelSelector.setSelectedId(trackNum, juce::dontSendNotification);
	else
		modelSelector.setSelectedId(1, juce::dontSendNotification);

	updateModelUI();

	modelSelector.onChange = [this]
	{
		auto *t = getTrack();
		if (!t)
			return;
		auto selectedModel = modelSelector.getText();
		t->getCurrentPage().selectedModel = selectedModel;
		populatePromptPresets(selectedModel);
		updateModelUI();
		if (onModelChanged)
			onModelChanged(trackId);
	};

	setupIconButtons();

	addAndMakeVisible(intervalKnob);
	intervalKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	intervalKnob.setRange(1, 10, 1);
	intervalKnob.setSize(40, 40);
	intervalKnob.setDoubleClickReturnValue(true, Obsidian::RNDM_RTRGR_INTRVL);
	intervalKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	intervalKnob.setTooltip("Beat repeat duration: 4 Beats, 2 Beats, 1 Beat, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128");
	intervalKnob.onValueChange = [this]() { onIntervalChanged(); };

	addAndMakeVisible(intervalLabel);
	intervalLabel.setJustificationType(juce::Justification::centred);
	intervalLabel.setFont(juce::FontOptions(9.0f));
	intervalLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pageButtons[i].setVisible(true);

	addAndMakeVisible(borderOverlay);
}

void TrackComponent::syncTrackName(const juce::String &name)
{
	auto *t = getTrack();
	if (!t)
		return;
	t->trackName = name;
}

void TrackComponent::setupIconButtons()
{
	auto setupToggleButton = [](auto &btn)
	{
		btn.setClickingTogglesState(true);
		btn.setHasAccentBar(true);
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::buttonPrimary);
	};
	auto setupActionButton = [](auto &btn)
	{
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
	};

	addAndMakeVisible(generateButton);
	generateButton.loadIcon(BinaryData::zap_svg, BinaryData::zap_svgSize);
	generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
	generateButton.setColour(juce::TextButton::textColourOffId, ColourPalette::backgroundDeep);
	generateButton.setTooltip("Generate AI audio with current prompt for this track");

	addAndMakeVisible(previewButton);
	previewButton.loadIcon(BinaryData::headphones_svg, BinaryData::headphones_svgSize);
	previewButton.loadIconToggled(BinaryData::square_svg, BinaryData::square_svgSize);
	previewButton.setHasToggledIcon(true);
	previewButton.setLabelText("PREV");
	previewButton.setShowBackground(false);
	setupToggleButton(previewButton);
	previewButton.setTooltip("Preview sample (independent of ARM/STOP state)");
	previewButton.onClick = [this]()
	{
		auto *t = getTrack();
		if (!t)
			return;
		if (onPreviewTrack)
		{
			if (isPreviewPlaying)
			{
				if (onStopPreview)
					onStopPreview(trackId);
			}
			else
				onPreviewTrack(trackId);
		}
	};

	addAndMakeVisible(originalSyncButton);
	originalSyncButton.loadIcon(BinaryData::anchor_svg, BinaryData::anchor_svgSize);
	originalSyncButton.setLabelText("ORIG");
	originalSyncButton.setShowBackground(false);
	setupToggleButton(originalSyncButton);
	originalSyncButton.setTooltip(
	    "Play original file (bypass time-stretching). Disabled when no original version exists.");
	originalSyncButton.onClick = [this]() { toggleOriginalSync(); };

	addAndMakeVisible(beatRepeatButton);
	beatRepeatButton.loadIcon(BinaryData::repeat_svg, BinaryData::repeat_svgSize);
	beatRepeatButton.setShowBackground(false);
	setupToggleButton(beatRepeatButton);
	beatRepeatButton.setTooltip("Beat repeat - re-trigger current section at interval while ON");

	addAndMakeVisible(reverseButton);
	reverseButton.loadIcon(BinaryData::rewind_svg, BinaryData::rewind_svgSize);
	reverseButton.setShowBackground(false);
	setupToggleButton(reverseButton);
	reverseButton.setTooltip("Apply reverse effect to track");

	addAndMakeVisible(transientScatterButton);
	transientScatterButton.loadIcon(BinaryData::radioactive_svg, BinaryData::radioactive_svgSize);
	transientScatterButton.setShowBackground(false);
	setupToggleButton(transientScatterButton);
	transientScatterButton.setTooltip(
	    "Transient scatter - jumps to a new random pre-transient position in the loop window every 2 steps");

	addAndMakeVisible(randomDurationToggle);
	randomDurationToggle.loadIcon(BinaryData::shuffle_svg, BinaryData::shuffle_svgSize);
	randomDurationToggle.setShowBackground(false);
	setupToggleButton(randomDurationToggle);
	randomDurationToggle.setTooltip("Auto-randomize repeat interval on each trigger");
	randomDurationToggle.onClick = [this]()
	{
		auto *t = getTrack();
		if (!t)
			return;
		t->randomRetriggerDurationEnabled = randomDurationToggle.getToggleState();
		updateRandomDurationButtonColor();
		statusCallback("Auto-random duration: " +
		               juce::String(t->randomRetriggerDurationEnabled.load() ? "ON" : "OFF"));
	};
}

void TrackComponent::updateButtonsEnabledState()
{
	auto *t = getTrack();
	if (!t)
		return;
	bool hasAudio = t->getCurrentPage().numSamples > 0;

	bool hasOriginal = false;

	hasOriginal = t->getCurrentPage().hasOriginalVersion.load();

	previewButton.setEnabled(hasAudio);
	beatRepeatButton.setEnabled(hasAudio);
	originalSyncButton.setEnabled(hasAudio && hasOriginal);
	randomDurationToggle.setEnabled(hasAudio);
	reverseButton.setEnabled(hasAudio);
	transientScatterButton.setEnabled(hasAudio);
	intervalKnob.setEnabled(hasAudio);
	intervalLabel.setEnabled(hasAudio);
}

void TrackComponent::updateBeatRepeatButtonState()
{
	auto *t = getTrack();
	if (!t)
		return;
	beatRepeatButton.setToggleState(t->randomRetriggerEnabled.load(), juce::dontSendNotification);
}

void TrackComponent::updateReverseButtonState()
{
	auto *t = getTrack();
	if (!t)
		return;
	reverseButton.setToggleState(t->reverseActive.load(), juce::dontSendNotification);
}

void TrackComponent::updateTransientScatterButtonState()
{
	auto *t = getTrack();
	if (!t)
		return;
	transientScatterButton.setToggleState(t->transientScatterActive.load(), juce::dontSendNotification);
}

void TrackComponent::updateRandomDurationButtonColor()
{
	auto *t = getTrack();
	if (!t)
		return;
	randomDurationToggle.setToggleState(t->randomRetriggerDurationEnabled.load(), juce::dontSendNotification);
}

void TrackComponent::onIntervalChanged()
{
	auto *t = getTrack();
	if (!t)
		return;

	int value = (int)juce::roundToInt(intervalKnob.getValue());

	juce::String intervalName = getIntervalName(value);
	intervalLabel.setText(intervalName, juce::dontSendNotification);
	statusCallback("Interval: " + intervalName);
}

juce::String TrackComponent::getIntervalName(int value)
{
	switch (value)
	{
	case 1:
		return "4 Beats";
	case 2:
		return "2 Beats";
	case 3:
		return "1 Beat";
	case 4:
		return "1/2 Beat";
	case 5:
		return "1/4 Beat";
	case 6:
		return "1/8 Beat";
	case 7:
		return "1/16 Beat";
	case 8:
		return "1/32 Beat";
	case 9:
		return "1/64 Beat";
	case 10:
		return "1/128 Beat";
	default:
		return "1 Beat";
	}
}

void TrackComponent::populatePromptPresets(const juce::String &modelName, const juce::String &forceSelectedPrompt)
{
	auto *t = getTrack();
	if (!t)
		return;

	juce::String currentSelection = promptPresetSelector.getText();

	auto promptInfos = audioProcessor.getAvailablePromptsWithCategoryForModel(modelName);

	std::map<juce::String, std::vector<PromptInfo>> byCategory;
	for (const auto &info : promptInfos)
	{
		juce::String cat = info.category.isEmpty() ? "Uncategorized" : info.category;
		byCategory[cat].push_back(info);
	}

	promptPresetSelector.clear();
	promptPresets.clear();
	int itemId = 1;

	for (auto &pair : byCategory)
	{
		promptPresetSelector.addSectionHeading(pair.first);
		for (const auto &info : pair.second)
		{
			promptPresetSelector.addItem(makePromptDisplayLabel(info.text), itemId++);
			promptPresets.add(info.text);
		}
	}

	juce::String targetPrompt =
	    forceSelectedPrompt.isNotEmpty() ? forceSelectedPrompt : t->getCurrentPage().selectedPrompt;
	if (targetPrompt.isEmpty())
		targetPrompt = currentSelection;

	bool found = false;
	for (int i = 0; i < promptPresets.size(); ++i)
	{
		if (promptPresets[i] == targetPrompt)
		{
			promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
			found = true;
			break;
		}
	}

	if (!found && targetPrompt.isNotEmpty())
	{
		promptPresetSelector.addItem(makePromptDisplayLabel(targetPrompt), itemId++);
		promptPresets.add(targetPrompt);
		promptPresetSelector.setSelectedItemIndex(promptPresets.size() - 1, juce::dontSendNotification);
		found = true;
	}

	if (!found && promptPresets.size() > 0)
		promptPresetSelector.setSelectedItemIndex(0, juce::dontSendNotification);

	t->getCurrentPage().setSelectedPrompt(getSelectedPromptValue());

	if (forceSelectedPrompt.isEmpty())
		onTrackPresetSelected();
}

juce::String TrackComponent::getSelectedPromptValue() const
{
	int sel = promptPresetSelector.getSelectedItemIndex();
	if (sel >= 0 && sel < promptPresets.size())
		return promptPresets[sel];
	return promptPresetSelector.getText();
}

void TrackComponent::statusCallback(const juce::String &message)
{
	if (onStatusMessage)
	{
		onStatusMessage(message);
	}
}

void TrackComponent::toggleOriginalSync()
{
	auto *t = getTrack();
	if (!t)
		return;

	bool useOriginal = originalSyncButton.getToggleState();

	auto &currentPage = t->getCurrentPage();
	if (!currentPage.hasOriginalVersion.load())
	{
		originalSyncButton.setToggleState(!useOriginal, juce::dontSendNotification);
		originalSyncButton.setEnabled(false);
		return;
	}
	currentPage.useOriginalFile = useOriginal;

	originalSyncButton.setButtonText(useOriginal ? juce::String::fromUTF8("\xE2\x97\x8F")
	                                             : juce::String::fromUTF8("\xE2\x97\x8B"));
	originalSyncButton.setEnabled(false);
	audioProcessor.reloadTrackWithVersion(trackId, useOriginal);
	juce::Timer::callAfterDelay(500,
	                            [this]()
	                            {
		                            auto *t = getTrack();
		                            if (!t)
			                            return;

		                            const auto &currentPage = t->getCurrentPage();
		                            if (currentPage.hasOriginalVersion.load())
		                            {
			                            originalSyncButton.setEnabled(true);
		                            }
	                            });
}

void TrackComponent::onTrackPresetSelected()
{
	auto *t = getTrack();
	if (!t)
		return;
	juce::String newPrompt = getSelectedPromptValue();

	auto &currentPage = t->getCurrentPage();
	currentPage.setSelectedPrompt(newPrompt);

	promptPresetSelector.setTooltip(newPrompt);

	if (onTrackPromptChanged)
		onTrackPromptChanged(trackId, newPrompt);
}

void TrackComponent::updateTrackInfo()
{
	auto *t = getTrack();
	if (!t)
		return;
	if (!t->getCurrentPage().prompt.isEmpty())
	{
		float effectiveBpm = calculateEffectiveBpm();
		float pitchSemis = t->getCurrentPage().pitchSemitones.load();

		juce::String stretchIndicator = (pitchSemis > 0) ? " +" : (pitchSemis < 0) ? " -" : "";
		juce::String bpmInfo =
		    " | Sync: " + juce::String(effectiveBpm, 1) + " | Pitch: " + juce::String(pitchSemis, 1) + stretchIndicator;

		infoLabel.setText(t->getCurrentPage().prompt.substring(0, 30) + "..." + bpmInfo, juce::dontSendNotification);
	}
}

void TrackComponent::refreshWaveformIfNeeded()
{
	auto *t = getTrack();
	if (!t)
		return;
	if (waveformDisplay && t->getCurrentPage().numSamples > 0)
	{
		if (t->getCurrentPage().numSamples != lastWaveformNumSamples)
		{
			refreshWaveformDisplay();
			lastWaveformNumSamples = t->getCurrentPage().numSamples;
		}
	}
}

void TrackComponent::updatePromptSelection(const juce::String &promptText)
{
	auto *t = getTrack();
	if (!t)
		return;

	t->getCurrentPage().setSelectedPrompt(promptText);

	for (int i = 0; i < promptPresets.size(); ++i)
	{
		if (promptPresets[i] == promptText)
		{
			promptPresetSelector.setSelectedItemIndex(i, juce::sendNotification);
			break;
		}
	}
}

bool TrackComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
	return dragSourceDetails.description.isString() && dragSourceDetails.description.toString().isNotEmpty();
}

void TrackComponent::itemDragEnter(const SourceDetails &dragSourceDetails)
{
	auto *t = getTrack();
	if (!t)
		return;

	isDragOver = true;
	if (t->isInGeneratingProcess)
	{
		syncBorderOverlay();
		return;
	}
	isDraggingPrompt = dragSourceDetails.description.toString().startsWith("prompt:");
	syncBorderOverlay();
}

void TrackComponent::itemDragMove(const SourceDetails &)
{
}

void TrackComponent::itemDragExit(const SourceDetails &)
{
	isDragOver = false;
	isDraggingPrompt = false;
	syncBorderOverlay();
}

void TrackComponent::itemDropped(const SourceDetails &dragSourceDetails)
{
	auto *t = getTrack();
	if (!t)
		return;

	isDragOver = false;
	isDraggingPrompt = false;
	syncBorderOverlay();

	if (t->isInGeneratingProcess)
		return;

	juce::String description = dragSourceDetails.description.toString();
	if (description.isEmpty())
		return;

	if (description.startsWith("prompt:"))
	{
		juce::String promptId = description.fromFirstOccurrenceOf("prompt:", false, false);
		applyPromptFromBank(promptId);
		return;
	}

	juce::String sampleId = description;
	audioProcessor.getAudioManager().loadSampleFromBank(sampleId, trackId);

	if (auto *sampleBank = audioProcessor.getSampleBank())
	{
		auto *sampleEntry = sampleBank->getSample(sampleId);
		if (sampleEntry)
		{
			const bool isLocalMode = audioProcessor.getUseLocalModel();

			if (!sampleEntry->originalPrompt.isEmpty())
			{
				if (!isLocalMode && !sampleEntry->modelName.isEmpty())
					populatePromptPresets(sampleEntry->modelName, sampleEntry->originalPrompt);
				else
				{
					bool found = false;
					for (int i = 0; i < promptPresets.size(); ++i)
					{
						if (promptPresets[i] == sampleEntry->originalPrompt)
						{
							promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
							found = true;
							break;
						}
					}
					if (!found)
					{
						promptPresetSelector.addItem(makePromptDisplayLabel(sampleEntry->originalPrompt),
						                             promptPresets.size() + 1);
						promptPresets.add(sampleEntry->originalPrompt);
						promptPresetSelector.setSelectedItemIndex(promptPresets.size() - 1, juce::dontSendNotification);
					}
					t->getCurrentPage().setSelectedPrompt(sampleEntry->originalPrompt);
					if (onTrackPromptChanged)
						onTrackPromptChanged(trackId, sampleEntry->originalPrompt);
				}
			}

			if (!isLocalMode && !sampleEntry->modelName.isEmpty())
			{
				for (int i = 0; i < modelSelector.getNumItems(); ++i)
				{
					if (modelSelector.getItemText(i) == sampleEntry->modelName)
					{
						modelSelector.setSelectedItemIndex(i, juce::dontSendNotification);
						t->getCurrentPage().selectedModel = sampleEntry->modelName;
						break;
					}
				}
			}
		}
	}

	if (t->slotIndex >= 0 && t->slotIndex < Obsidian::MAX_TRACKS)
	{
		auto &apvts = audioProcessor.getParameterManager().getAPVTS();
		juce::String s = "slot" + juce::String(t->slotIndex + 1);

		auto resetParam = [&](const juce::String &id, float defaultValue)
		{
			if (auto *p = apvts.getParameter(id))
			{
				auto range = apvts.getParameterRange(id);
				p->setValueNotifyingHost(range.convertTo0to1(defaultValue));
			}
		};

		resetParam(s + "Pitch", 0.0f);
		resetParam(s + "Fine", 0.0f);
	}

	if (onSampleDropped)
		onSampleDropped(trackId);
	if (onStatusMessage)
		onStatusMessage("Sample loaded from bank!");

	borderOverlay.triggerFlash();
	blinkTicking = true;
}

void TrackComponent::setPreviewPlaying(bool playing)
{
	if (isPreviewPlaying == playing)
		return;
	isPreviewPlaying = playing;
	updatePreviewButton();
}

void TrackComponent::updatePreviewButton()
{
	previewButton.setToggleState(isPreviewPlaying, juce::dontSendNotification);
	previewButton.setTooltip(isPreviewPlaying ? "Stop preview" : "Preview sample (independent of ARM/STOP state)");
}

void TrackComponent::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto modelColour = getCurrentModelColour();

	if (modelColour == cachedModelColour)
	{
		if (onModelChanged)
			onModelChanged(trackId);
		return;
	}

	cachedModelColour = modelColour;

	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	intervalKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	intervalKnob.setColour(juce::Slider::thumbColourId, modelColour);

	generateButton.setColour(juce::TextButton::buttonColourId, modelColour);
	generateButton.setColour(juce::TextButton::textColourOffId, textColour);

	adsrAttackKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	adsrAttackKnob.setColour(juce::Slider::thumbColourId, modelColour);
	adsrDecayKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	adsrDecayKnob.setColour(juce::Slider::thumbColourId, modelColour);
	adsrSustainKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	adsrSustainKnob.setColour(juce::Slider::thumbColourId, modelColour);
	adsrReleaseKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	adsrReleaseKnob.setColour(juce::Slider::thumbColourId, modelColour);

	auto setupToggleColours = [&](auto &btn)
	{
		btn.setColour(juce::TextButton::textColourOffId, modelColour);
		btn.setColour(juce::TextButton::textColourOnId, modelColour);
	};

	setupToggleColours(previewButton);
	setupToggleColours(originalSyncButton);
	setupToggleColours(beatRepeatButton);
	setupToggleColours(randomDurationToggle);
	setupToggleColours(reverseButton);
	setupToggleColours(transientScatterButton);

	if (sequencer)
		sequencer->setAccentColour(modelColour);

	syncBorderOverlay();
}

void TrackComponent::detachWaveformTrack()
{
	if (waveformDisplay)
		waveformDisplay->setTrack(nullptr);
}

void TrackComponent::applyPromptFromBank(const juce::String &promptId)
{
	auto *t = getTrack();
	if (!t)
		return;
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	auto *entry = bank->getPrompt(promptId);
	if (!entry)
		return;

	const bool isLocalMode = audioProcessor.getUseLocalModel();
	if (!isLocalMode && entry->modelName.isNotEmpty())
	{
		for (int i = 0; i < modelSelector.getNumItems(); ++i)
		{
			if (modelSelector.getItemText(i) == entry->modelName)
			{
				modelSelector.setSelectedItemIndex(i, juce::sendNotification);
				t->getCurrentPage().selectedModel = entry->modelName;
				break;
			}
		}
	}

	if (entry->text.isNotEmpty())
	{
		bool found = false;
		for (int i = 0; i < promptPresets.size(); ++i)
		{
			if (promptPresets[i] == entry->text)
			{
				promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
				found = true;
				break;
			}
		}

		if (!found)
		{
			promptPresetSelector.addItem(makePromptDisplayLabel(entry->text), promptPresets.size() + 1);
			promptPresets.add(entry->text);
			promptPresetSelector.setSelectedItemIndex(promptPresets.size() - 1, juce::dontSendNotification);
		}

		t->getCurrentPage().setSelectedPrompt(entry->text);
		if (onTrackPromptChanged)
			onTrackPromptChanged(trackId, entry->text);
	}

	bank->incrementUsage(promptId);

	if (onStatusMessage)
		onStatusMessage("Prompt loaded from bank!");

	borderOverlay.triggerFlash();
	blinkTicking = true;
}

void TrackComponent::wireParameters()
{
	registerSliderParam("AdsrAttack", adsrAttackKnob);
	registerSliderParam("AdsrDecay", adsrDecayKnob);
	registerSliderParam("AdsrSustain", adsrSustainKnob);
	registerSliderParam("AdsrRelease", adsrReleaseKnob);

	registerSliderParam("BeatRepeatInterval", intervalKnob);
	registerButtonParam("BeatRepeatActive", beatRepeatButton);
	registerButtonParam("Generate", generateButton, true);
	registerButtonParam("ReverseActive", reverseButton);
	registerButtonParam("TransientScatterActive", transientScatterButton);

	registerMidiLearn("AdsrAttack", &adsrAttackKnob);
	registerMidiLearn("AdsrDecay", &adsrDecayKnob);
	registerMidiLearn("AdsrSustain", &adsrSustainKnob);
	registerMidiLearn("AdsrRelease", &adsrReleaseKnob);
	registerMidiLearn("BeatRepeatInterval", &intervalKnob);
	registerMidiLearn("BeatRepeatActive", &beatRepeatButton);
	registerMidiLearn("Generate", &generateButton);
	registerMidiLearn("ReverseActive", &reverseButton);
	registerMidiLearn("TransientScatterActive", &transientScatterButton);

	subscribeToParam("Gain");
}