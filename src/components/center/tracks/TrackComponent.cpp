#include "TrackComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "WaveformDisplay.h"

TrackComponent::TrackComponent(const juce::String &trackId, DjIaVstProcessor &processor)
    : ObsidianBaseMidiComponent(processor), trackId(trackId)
{
	setupUI();
	setupAdsrKnobs();
}

TrackComponent::~TrackComponent()
{
	setVisible(false);
	markForDestruction();
	stopTimer();

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].onClick = nullptr;
	}

	sequencer.reset();
	waveformDisplay.reset();
	drawingCanvas.reset();

	if (auto *t = getTrack())
	{
		t->onPlayStateChanged = nullptr;
	}
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
	{
		wireParameters();
	}
	refreshWaveformDisplay();
	updateFromTrackData();
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
		if (newValue > 0.5f && audioProcessor.getIsGenerating() || newValue < 0.5f)
			return;
		if (onGenerateForTrack)
		{
			if (track)
			{
				auto &currentPage = track->getCurrentPage();
				currentPage.setSelectedPrompt(promptPresetSelector.getText());
				currentPage.generationBpm = audioProcessor.getGlobalBpm();
				currentPage.generationKey = audioProcessor.getGlobalKey();
				currentPage.generationDuration = audioProcessor.getGlobalDuration();
			}
			onGenerateForTrack(trackId);
			return;
		}
	}
	else if (paramSuffix == "RandomRetrigger")
	{
		auto *t = getTrack();
		if (t)
		{
			bool isEnabled = newValue > 0.5f;
			t->randomRetriggerEnabled = isEnabled;
			if (isEnabled)
			{
				t->beatRepeatPending.store(true);
			}
			else
			{
				t->beatRepeatStopPending.store(true);
			}
			updateBeatRepeatButtonColor();
			statusCallback("Beat Repeat " + juce::String(isEnabled ? "ON" : "OFF"));
		}
	}
	else if (paramSuffix == "RetriggerInterval")
	{
		if (auto *t = getTrack())
		{
			onIntervalChanged();
		}
	}
	else if (paramSuffix == "AdsrAttack" || paramSuffix == "AdsrDecay" || paramSuffix == "AdsrSustain" ||
	         paramSuffix == "AdsrRelease")
	{
		syncAdsrToWaveform();
	}
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

	borderOverlay.setVisualState(isGenerating, hasSamplePending, isSelected, isDragOver, blinkState, overlayColour);
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
		{
			modelToSet = t->getCurrentPage().savedModelBeforeLocal;
		}
		else
		{
			modelToSet = modelsForMode[0];
		}
		t->getCurrentPage().selectedModel = modelToSet;
	}

	modelSelector.setText(modelToSet, juce::dontSendNotification);
	updateModelUI();

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].setVisible(true);
	}
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
		int interval = t->randomRetriggerInterval.load();
		intervalKnob.setValue(interval, juce::dontSendNotification);
		intervalLabel.setText(getIntervalName(interval), juce::dontSendNotification);
	}

	updateBeatRepeatButtonColor();
	updateRandomDurationButtonColor();

	updateButtonsEnabledState();

	if (t->isCurrentlyPlaying.load() && !isPreviewPlaying)
	{
		previewButton.setEnabled(false);
	}

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
	switch (t->timeStretchMode)
	{
	case 1:
		effectiveBpm = currentPage.originalBpm;
		break;
	case 2:
		effectiveBpm = currentPage.originalBpm + static_cast<float>(currentPage.bpmOffset.load());
		break;
	case 3:
	{
		double hostBpm = audioProcessor.getHostBpm();
		if (hostBpm > 0.0 && currentPage.originalBpm > 0.0)
		{
			effectiveBpm = (float)hostBpm;
		}
	}
	break;
	case 4:
	{
		double hostBpm = audioProcessor.getHostBpm();
		if (hostBpm > 0.0 && currentPage.originalBpm > 0.0)
		{
			effectiveBpm = (float)hostBpm + static_cast<float>(currentPage.bpmOffset.load());
		}
	}
	break;
	}
	return juce::jlimit(40.0f, 250.0f, effectiveBpm);
}

void TrackComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDark.withAlpha(ObsidianShades::ALPHA_08));
	g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);
}

void TrackComponent::setSamplePending(bool pending)
{
	if (hasSamplePending == pending)
		return;
	hasSamplePending = pending;
	syncBorderOverlay();
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

	setupKnob(adsrAttackKnob, adsrAttackLabel, "A", 0.001f, 4.0f, 0.0f, "ADSR Attack time (seconds)");
	setupKnob(adsrDecayKnob, adsrDecayLabel, "D", 0.001f, 4.0f, 4.0f, "ADSR Decay time (seconds)");
	setupKnob(adsrSustainKnob, adsrSustainLabel, "S", 0.0f, 1.0f, 1.0f, "ADSR Sustain level (0-1)");
	setupKnob(adsrReleaseKnob, adsrReleaseLabel, "R", 0.001f, 4.0f, 0.0f, "ADSR Release time (seconds)");

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
	int pagesGridHeight = ObsidianSizes::PAGE_BUTTON_SIZE * 2 + 2;
	int yOffset = (pagesArea.getHeight() - pagesGridHeight) / 2;
	pagesArea.removeFromTop(yOffset);
	pagesArea.setHeight(pagesGridHeight);
	layoutPagesButtons(pagesArea);

	{
		const int rightElementsWidth = 36 + ObsidianSizes::SPACER_SM + 36 + ObsidianSizes::SPACER_SM +
		                               ObsidianSizes::SPACER_SM + 34 + ObsidianSizes::SPACER_SM + 34 +
		                               ObsidianSizes::SPACER_SM + 38 + ObsidianSizes::SPACER_SM + (32 * 4);

		const int pagesWidth = 38;
		const int availableForSelectors =
		    headerArea.getWidth() - pagesWidth - rightElementsWidth - ObsidianSizes::SPACER_SM * 2;
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

	headerArea.removeFromLeft(ObsidianSizes::SPACER_SM);

	{
		const int createButtonWidth = 34;
		generateButton.setBounds(headerArea.removeFromRight(createButtonWidth));
	}
	headerArea.removeFromRight(ObsidianSizes::SPACER_MD);

	{
		const int labelledButtonWidth = 36;
		originalSyncButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
		headerArea.removeFromRight(ObsidianSizes::SPACER_SM);
		previewButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
	}
	headerArea.removeFromRight(ObsidianSizes::SPACER_SM);

	const int iconBtnWidth = 34;
	beatRepeatButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(ObsidianSizes::SPACER_SM);

	randomDurationToggle.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(ObsidianSizes::SPACER_SM);

	{
		auto knobArea = headerArea.removeFromRight(38);
		const int knobDiameter = 34;
		const int labelHeight = 8;
		const int stackHeight = knobDiameter + labelHeight;
		int knobsYOffset = (knobArea.getHeight() - stackHeight) / 2;
		intervalKnob.setBounds(knobArea.getX() + (knobArea.getWidth() - knobDiameter) / 2,
		                       knobArea.getY() + knobsYOffset, knobDiameter, knobDiameter);
		intervalLabel.setBounds(knobArea.getX(), knobArea.getY() + knobsYOffset + knobDiameter - 2, knobArea.getWidth(),
		                        labelHeight);
	}

	headerArea.removeFromLeft(ObsidianSizes::SPACER_SM);
	{
		const int adsrKnobDiam = 32;
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
				}
				else
				{
					t->readPosition.store(newRelative);
				}
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
		waveformDisplay->setBounds(area.removeFromTop(ObsidianSizes::WAVEFORM_HEIGHT));
		waveformDisplay->setVisible(true);
	}

	if (!sequencer)
	{
		sequencer = std::make_unique<SequencerComponent>(trackId, audioProcessor);
		addAndMakeVisible(*sequencer);
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
		sequencer->setBounds(area.removeFromTop(ObsidianSizes::SEQUENCER_HEIGHT));
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
	{
		canvas->loadFromBase64(currentPage.canvasData);
	}

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
	int buttonSize = ObsidianSizes::PAGE_BUTTON_SIZE;
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

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
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

		registerMidiLearn(pageNames[i], &pageButtons[i]);
	}
}

void TrackComponent::onPageSelected(int pageIndex)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
		return;

	if (t->currentPageIndex.load() == pageIndex && !t->pageChangePending.load())
	{
		pageButtons[pageIndex].setToggleState(true, juce::dontSendNotification);
		return;
	}

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].setToggleState(i == t->currentPageIndex.load(), juce::dontSendNotification);
	}

	if (t->pageChangePending.load() && t->pendingPageIndex.load() == pageIndex)
	{
		t->pageChangePending.store(false);
		t->pendingPageIndex.store(-1);
		stopTimer();
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
		{
			param->setValueNotifyingHost(1.0f);
		}
	}
}

void TrackComponent::performPageChange(int pageIndex)
{
	auto *t = getTrack();
	if (!t)
		return;
	if (pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
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

	const auto &newPage = t->getCurrentPage();

	if (newPage.numSamples == 0)
	{
		t->isPlaying.store(false);
		t->isCurrentlyPlaying.store(false);
		t->isArmed.store(false);
		t->isArmedToStop.store(false);
		t->readPosition.store(0.0);
		if (t->onPlayStateChanged)
		{
			t->onPlayStateChanged(false);
		}
	}

	t->pageChangePending.store(false);
	t->pendingPageIndex.store(-1);

	if (!isGenerating && !t->pageChangePending.load())
	{
		stopTimer();
	}

	updateFromTrackData();

	juce::StringArray prompts = audioProcessor.getAvailablePromptsForModel(newPage.selectedModel);
	updatePromptPresets(prompts, newPage.selectedPrompt);

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
			waveformDisplay->setAudioData(emptyBuffer, ObsidianDataConst::SAMPLERATE);
			waveformDisplay->setLoopPoints(0.0, 0.0);
		}
	}

	if (!newPage.isLoaded.load() && !newPage.audioFilePath.isEmpty())
	{
		loadPageIfNeeded(pageIndex);
	}

	char pageName = 'A' + static_cast<char>(pageIndex);
	statusCallback("Switched to page " + juce::String(pageName));
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

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
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
	if (pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
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
	if (pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
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
		{
			page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);
		}

		page.numSamples = numSamples;
		page.sampleRate = reader->sampleRate;
		page.isLoaded = true;
		page.isLoading = false;

		juce::MessageManager::callAsync(
		    [this, pageIndex, t]()
		    {
			    if (t->currentPageIndex.load() == pageIndex)
			    {
				    updateFromTrackData();
				    if (waveformDisplay)
				    {
					    refreshWaveformDisplay();
				    }
			    }
			    updatePagesDisplay();
		    });
	}
	catch (const std::exception &)
	{
		page.isLoading = false;

		juce::MessageManager::callAsync([this]() { updatePagesDisplay(); });
	}
}

void TrackComponent::startGeneratingAnimation()
{
	isGenerating = true;

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].setEnabled(false);
	}

	syncBorderOverlay();

	if (!isTimerRunning())
	{
		startTimer(200);
	}
}

void TrackComponent::stopGeneratingAnimation()
{
	auto *t = getTrack();
	if (!t)
		return;
	isGenerating = false;

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].setEnabled(true);
	}

	if (!t->pageChangePending.load())
	{
		stopTimer();
	}

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

void TrackComponent::timerCallback()
{
	auto *t = getTrack();
	if (!t)
	{
		stopTimer();
		return;
	}
	if (isGenerating)
	{
		blinkState = !blinkState;
		syncBorderOverlay();
	}

	if (t->pageChangePending.load())
	{
		pageBlinkState = !pageBlinkState;
		updatePagesDisplay();
	}

	if (!isGenerating && !t->pageChangePending.load())
		stopTimer();
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
		waveformDisplay->setAudioData(emptyBuffer, ObsidianDataConst::SAMPLERATE);
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
	infoLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));

	promptPresetSelector.setTooltip("Select prompt for this page");
	promptPresetSelector.onChange = [this]() { onTrackPresetSelected(); };

	modelSelector.clear();

	addAndMakeVisible(promptPresetSelector);
	addAndMakeVisible(modelSelector);
	modelSelector.setTooltip("Select model for this page");

	const bool isLocalMode = audioProcessor.getUseLocalModel();
	auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);
	for (int i = 0; i < modelsForMode.size(); ++i)
	{
		modelSelector.addItem(modelsForMode[i], i + 1);
	}

	int trackNum = trackId.retainCharacters("0123456789").getIntValue();
	if (trackNum >= 1 && trackNum <= modelsForMode.size())
	{
		modelSelector.setSelectedId(trackNum, juce::dontSendNotification);
	}
	else
	{
		modelSelector.setSelectedId(1, juce::dontSendNotification);
	}

	updateModelUI();

	modelSelector.onChange = [this]
	{
		auto *t = getTrack();
		if (!t)
			return;
		auto selectedModel = modelSelector.getText();

		t->getCurrentPage().selectedModel = selectedModel;
		juce::StringArray prompts = audioProcessor.getAvailablePromptsForModel(selectedModel);
		updatePromptPresets(prompts);
		updateModelUI();
		if (onModelChanged)
			onModelChanged(trackId);
	};

	setupIconButtons();

	addAndMakeVisible(intervalKnob);
	intervalKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	intervalKnob.setRange(1, 10, 1);
	intervalKnob.setSize(40, 40);
	intervalKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	intervalKnob.setTooltip("Beat repeat duration: 4 Beats, 2 Beats, 1 Beat, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128");
	intervalKnob.onValueChange = [this]() { onIntervalChanged(); };

	addAndMakeVisible(intervalLabel);
	intervalLabel.setJustificationType(juce::Justification::centred);
	intervalLabel.setFont(juce::FontOptions(9.0f));
	intervalLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
	{
		pageButtons[i].setVisible(true);
	}

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
	auto setupToggleButton = [](IconButton &btn)
	{
		btn.setClickingTogglesState(true);
		btn.setHasAccentBar(true);
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::buttonPrimary);
	};
	auto setupActionButton = [](IconButton &btn)
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
	previewButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
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

	intervalKnob.setEnabled(hasAudio);
	intervalLabel.setEnabled(hasAudio);
}

void TrackComponent::updateBeatRepeatButtonColor()
{
	auto *t = getTrack();
	if (!t)
		return;
	beatRepeatButton.setToggleState(t->randomRetriggerEnabled.load(), juce::dontSendNotification);
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

	if (t->randomRetriggerInterval.load() != value)
	{
		t->randomRetriggerInterval = value;

		if (t->beatRepeatActive.load())
		{
			double hostBpm = audioProcessor.getHostBpm();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double startPosition = t->beatRepeatStartPosition.load();
			double repeatDuration = audioProcessor.getSequencerManager().calculateRetriggerInterval(value, hostBpm);
			double repeatDurationSamples = repeatDuration * t->getCurrentPage().sampleRate;

			const double GATE_SAMPLES = 64.0;
			double newEnd = startPosition + repeatDurationSamples - GATE_SAMPLES;

			double maxSamples = t->getCurrentPage().numSamples;
			if (newEnd > maxSamples)
				newEnd = maxSamples;
			if (newEnd <= startPosition)
				newEnd = startPosition + 1.0;

			t->beatRepeatEndPosition.store(newEnd);

			double currentPos = t->readPosition.load();
			if (currentPos >= newEnd)
			{
				t->readPosition.store(startPosition);
				t->brFadeInPending.store(64);
			}
		}
	}

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

void TrackComponent::statusCallback(const juce::String &message)
{
	if (onStatusMessage)
	{
		onStatusMessage(message);
	}
}

void TrackComponent::loadPromptPresets()
{
	juce::Component::SafePointer<TrackComponent> safeThis(this);

	juce::MessageManager::callAsync(
	    [safeThis]() mutable
	    {
		    if (safeThis == nullptr)
			    return;
		    auto *t = safeThis->getTrack();
		    if (!t)
			    return;

		    safeThis->promptPresetSelector.clear(juce::dontSendNotification);

		    auto &audioProcessor = safeThis->audioProcessor;
		    juce::String currentModel = t->getCurrentPage().selectedModel;
		    juce::StringArray allPrompts = audioProcessor.getAvailablePromptsForModel(currentModel);

		    safeThis->promptPresets = allPrompts;

		    for (int i = 0; i < allPrompts.size(); ++i)
		    {
			    safeThis->promptPresetSelector.addItem(allPrompts[i], i + 1);
		    }

		    bool selected = false;
		    const auto &selectedPrompt = t->getCurrentPage().selectedPrompt;

		    if (selectedPrompt.isNotEmpty())
		    {
			    int index = allPrompts.indexOf(selectedPrompt);
			    if (index >= 0)
			    {
				    safeThis->promptPresetSelector.setSelectedId(index + 1, juce::dontSendNotification);
				    selected = true;
			    }
		    }

		    if (!selected && allPrompts.size() > 0)
		    {
			    safeThis->promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
			    t->getCurrentPage().setSelectedPrompt(allPrompts[0]);
		    }
	    });
}

void TrackComponent::updatePromptPresets(const juce::StringArray &presets, const juce::String &selectedPrompt)
{
	auto *t = getTrack();
	if (!t)
		return;
	juce::String currentSelection = promptPresetSelector.getText();
	promptPresets = presets;
	promptPresets.sort(true);
	promptPresetSelector.clear();

	for (int i = 0; i < promptPresets.size(); ++i)
		promptPresetSelector.addItem(promptPresets[i], i + 1);

	int index = promptPresets.indexOf(currentSelection);
	if (index >= 0 && selectedPrompt.isEmpty())
		promptPresetSelector.setSelectedId(index + 1, juce::dontSendNotification);
	else if (promptPresets.size() > 0)
	{
		if (t->getCurrentPage().selectedPrompt.isNotEmpty())
		{
			bool found = false;
			if (selectedPrompt.isNotEmpty() && t->getCurrentPage().selectedPrompt != selectedPrompt)
			{
				t->getCurrentPage().selectedPrompt = selectedPrompt;
			}

			for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
			{
				if (promptPresetSelector.getItemText(i) == t->getCurrentPage().selectedPrompt)
				{
					promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
					found = true;
					break;
				}
			}

			if (!found)
			{
				promptPresetSelector.addItem(t->getCurrentPage().selectedPrompt, promptPresets.size() + 1);
				promptPresetSelector.setSelectedId(promptPresets.size() + 1, juce::dontSendNotification);
			}
		}

		onTrackPresetSelected();
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
	juce::String newPrompt = promptPresetSelector.getText();

	auto &currentPage = t->getCurrentPage();
	currentPage.setSelectedPrompt(newPrompt);

	if (onTrackPromptChanged)
	{
		onTrackPromptChanged(trackId, newPrompt);
	}
}

void TrackComponent::updateTrackInfo()
{
	auto *t = getTrack();
	if (!t)
		return;

	if (!t->getCurrentPage().prompt.isEmpty())
	{
		float effectiveBpm = calculateEffectiveBpm();
		float originalBpm = t->getCurrentPage().originalBpm;

		juce::String bpmInfo = "";
		juce::String stretchIndicator = "";

		switch (t->timeStretchMode)
		{
		case 1:
			bpmInfo = " | Original: " + juce::String(originalBpm, 1);
			break;
		case 2:
			stretchIndicator = (effectiveBpm > originalBpm) ? " +" : (effectiveBpm < originalBpm) ? " -" : " =";
			bpmInfo = " | BPM: " + juce::String(effectiveBpm, 1) + stretchIndicator;
			break;
		case 3:
			stretchIndicator = " =";
			bpmInfo = " | Sync: " + juce::String(effectiveBpm, 1) + stretchIndicator;
			break;
		case 4:
			stretchIndicator = (t->getCurrentPage().bpmOffset > 0)   ? " +"
			                   : (t->getCurrentPage().bpmOffset < 0) ? " -"
			                                                         : "";
			bpmInfo = " | Host+ " + juce::String(t->getCurrentPage().bpmOffset, 1) + stretchIndicator;
			break;
		}

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

	for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
	{
		if (promptPresetSelector.getItemText(i) == promptText)
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
	isDragOver = true;
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
			if (!sampleEntry->originalPrompt.isEmpty())
			{
				if (!sampleEntry->modelName.isEmpty())
				{
					juce::StringArray prompts = audioProcessor.getAvailablePromptsForModel(sampleEntry->modelName);
					updatePromptPresets(prompts, sampleEntry->originalPrompt);
				}
				else
				{
					for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
					{
						if (promptPresetSelector.getItemText(i) == sampleEntry->originalPrompt)
						{
							promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
							t->getCurrentPage().setSelectedPrompt(sampleEntry->originalPrompt);
							break;
						}
					}
				}
			}

			if (!sampleEntry->modelName.isEmpty())
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

	if (t->slotIndex >= 0 && t->slotIndex < ObsidianDataConst::MAX_TRACKS)
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

	auto setupToggleColours = [&](IconButton &btn)
	{
		btn.setColour(juce::TextButton::textColourOffId, modelColour);
		btn.setColour(juce::TextButton::textColourOnId, modelColour);
	};

	setupToggleColours(previewButton);
	setupToggleColours(originalSyncButton);
	setupToggleColours(beatRepeatButton);
	setupToggleColours(randomDurationToggle);

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

	if (entry->modelName.isNotEmpty())
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
		for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
		{
			if (promptPresetSelector.getItemText(i) == entry->text)
			{
				promptPresetSelector.setSelectedItemIndex(i, juce::sendNotification);
				t->getCurrentPage().setSelectedPrompt(entry->text);
				found = true;
				break;
			}
		}

		if (!found)
		{
			promptPresetSelector.addItem(entry->text, promptPresetSelector.getNumItems() + 1);
			promptPresetSelector.setSelectedId(promptPresetSelector.getNumItems(), juce::sendNotification);
			t->getCurrentPage().setSelectedPrompt(entry->text);
		}
	}

	bank->incrementUsage(promptId);

	if (onStatusMessage)
		onStatusMessage("Prompt loaded from bank!");
}

void TrackComponent::wireParameters()
{
	registerSliderParam("AdsrAttack", adsrAttackKnob);
	registerSliderParam("AdsrDecay", adsrDecayKnob);
	registerSliderParam("AdsrSustain", adsrSustainKnob);
	registerSliderParam("AdsrRelease", adsrReleaseKnob);

	registerSliderParam("RetriggerInterval", intervalKnob);
	registerButtonParam("RandomRetrigger", beatRepeatButton);
	registerButtonParam("Generate", generateButton, true);

	registerMidiLearn("AdsrAttack", &adsrAttackKnob);
	registerMidiLearn("AdsrDecay", &adsrDecayKnob);
	registerMidiLearn("AdsrSustain", &adsrSustainKnob);
	registerMidiLearn("AdsrRelease", &adsrReleaseKnob);
	registerMidiLearn("RetriggerInterval", &intervalKnob);
	registerMidiLearn("RandomRetrigger", &beatRepeatButton);
	registerMidiLearn("Generate", &generateButton);
}