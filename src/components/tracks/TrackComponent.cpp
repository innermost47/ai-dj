#include "TrackComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "WaveformDisplay.h"

TrackComponent::TrackComponent(const juce::String &trackId, DjIaVstProcessor &processor)
    : trackId(trackId), track(nullptr), audioProcessor(processor)
{
	setupUI();
	setupAdsrKnobs();
}

TrackComponent::~TrackComponent()
{
	setVisible(false);
	isDestroyed.store(true);
	stopTimer();

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].onClick = nullptr;
		pageButtons[i].onMidiLearn = nullptr;
		pageButtons[i].onMidiRemove = nullptr;
	}

	sequencer.reset();
	waveformDisplay.reset();
	drawingCanvas.reset();

	if (track && track->slotIndex != -1)
	{
		removeListener("Generate");
		removeListener("RandomRetrigger");
		removeListener("RetriggerInterval");
		removeListener("AdsrAttack");
		removeListener("AdsrDecay");
		removeListener("AdsrSustain");
		removeListener("AdsrRelease");
	}

	track = nullptr;
}

void TrackComponent::addEventListeners()
{
	addListener("Generate");
	addListener("RandomRetrigger");
	addListener("RetriggerInterval");
	addListener("AdsrAttack");
	addListener("AdsrDecay");
	addListener("AdsrSustain");
	addListener("AdsrRelease");
}

void TrackComponent::setTrackData(TrackData *trackData)
{
	track = trackData;
	updateFromTrackData();
	if (track && track->slotIndex != -1)
	{
		addEventListeners();
	}
	setupMidiLearn();
}

bool TrackComponent::isWaveformVisible() const
{
	return waveformDisplay && waveformDisplay->isVisible();
}

void TrackComponent::updateWaveformWithTimeStretch()
{
	calculateHostBasedDisplay();
}

void TrackComponent::updateUIFromParameter(const juce::String &paramName, const juce::String &slotPrefix,
                                           float newValue)
{
	if (isDestroyed.load())
		return;
	if (!track)
		return;
	if (paramName == slotPrefix + " Generate")
	{
		if (newValue > 0.5 && audioProcessor.getIsGenerating())
		{
			return;
		}
	}
	else if (paramName == slotPrefix + " Random Retrigger")
	{
		bool isEnabled = newValue > 0.5f;

		if (track)
		{
			track->randomRetriggerEnabled = isEnabled;
			updateRandomRetriggerButtonColor();
		}
	}
	else if (paramName == slotPrefix + " Retrigger Interval")
	{

		float denormalizedValue = (newValue * 9.0f) + 1.0f;
		intervalKnob.setValue(denormalizedValue, juce::dontSendNotification);

		intervalLabel.setText(getIntervalName((int)denormalizedValue), juce::dontSendNotification);

		if (track)
		{
			track->randomRetriggerInterval = (int)denormalizedValue;
		}
	}
	else if (paramName == slotPrefix + " ADSR Attack")
	{
		float denorm = 0.001f + newValue * (4.0f - 0.001f);
		if (!adsrAttackKnob.isMouseButtonDown())
			adsrAttackKnob.setValue(denorm, juce::dontSendNotification);
		syncAdsrToWaveform();
	}
	else if (paramName == slotPrefix + " ADSR Decay")
	{
		float denorm = 0.001f + newValue * (4.0f - 0.001f);
		if (!adsrDecayKnob.isMouseButtonDown())
			adsrDecayKnob.setValue(denorm, juce::dontSendNotification);
		syncAdsrToWaveform();
	}
	else if (paramName == slotPrefix + " ADSR Sustain")
	{
		if (!adsrSustainKnob.isMouseButtonDown())
			adsrSustainKnob.setValue(newValue, juce::dontSendNotification);
		syncAdsrToWaveform();
	}
	else if (paramName == slotPrefix + " ADSR Release")
	{
		float denorm = 0.001f + newValue * (4.0f - 0.001f);
		if (!adsrReleaseKnob.isMouseButtonDown())
			adsrReleaseKnob.setValue(denorm, juce::dontSendNotification);
		syncAdsrToWaveform();
	}
}

void TrackComponent::parameterGestureChanged(int, bool)
{
}

void TrackComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	if (!track || track->slotIndex == -1)
		return;

	juce::String slotPrefix = "Slot " + juce::String(track->slotIndex + 1);
	auto &allParams = audioProcessor.AudioProcessor::getParameters();

	if (parameterIndex >= 0 && parameterIndex < allParams.size())
	{
		auto *param = allParams[parameterIndex];
		juce::String paramName = param->getName(256);

		juce::MessageManager::callAsync([this, paramName, slotPrefix, newValue]()
		                                { updateUIFromParameter(paramName, slotPrefix, newValue); });
	}
}

void TrackComponent::setButtonParameter(juce::String name)
{
	if (!track || track->slotIndex == -1)
		return;
	if (this == nullptr)
		return;

	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	try
	{
		auto *param = audioProcessor.getParameters().getParameter(paramName);
		if (param != nullptr)
		{
			if (name == "Generate")
			{
				param->setValueNotifyingHost(1.0f);
				juce::Timer::callAfterDelay(100, [param]() { param->setValueNotifyingHost(0.0f); });
			}
			else
			{
				bool state = track ? track->randomRetriggerEnabled.load() : false;
				param->setValueNotifyingHost(state ? 1.0f : 0.0f);
			}
		}
	}
	catch (...)
	{
	}
}

void TrackComponent::calculateHostBasedDisplay()
{
	if (!track)
		return;
	auto &currentPage = track->getCurrentPage();
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
	if (waveformDisplay)
	{
		bool isPlaying = track && track->isPlaying.load();
		waveformDisplay->setPlaybackPosition(timeInSeconds, isPlaying);
	}
}

juce::Colour TrackComponent::getCurrentModelColour() const
{
	juce::String currentModel = modelSelector.getText();
	auto &currentPage = track->getCurrentPage();
	if (currentModel.isEmpty() && track)
		currentModel = currentPage.selectedModel;
	if (currentModel.isEmpty())
		currentModel = AiModelDefinitions::getAvailableModels()[0];
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
	if (track == nullptr)
		return;

	juce::String modelToSet = track->getCurrentPage().selectedModel;

	if (modelToSet.isEmpty())
	{
		auto &models = AiModelDefinitions::getAvailableModels();
		modelToSet = models[0];
	}

	modelSelector.setText(modelToSet, juce::dontSendNotification);
	updateModelUI();

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setVisible(true);
	}
	pageButtons[track->currentPageIndex.load()].setToggleState(true, juce::dontSendNotification);
	updatePagesDisplay();

	randomDurationToggle.setToggleState(track->randomRetriggerDurationEnabled.load(), juce::dontSendNotification);

	drawButton.setEnabled(!audioProcessor.getUseLocalModel());

	bool hasOriginal = false;
	bool useOriginal = false;

	const auto &currentPage = track->getCurrentPage();
	hasOriginal = currentPage.hasOriginalVersion.load();
	useOriginal = hasOriginal && currentPage.useOriginalFile.load();

	originalSyncButton.setToggleState(useOriginal, juce::dontSendNotification);

	if (!currentPage.selectedPrompt.isEmpty())
	{
		for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
		{
			if (promptPresetSelector.getItemText(i) == currentPage.selectedPrompt)
			{
				promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
				break;
			}
		}
	}

	if (waveformDisplay)
	{
		bool isCurrentlyPlaying = track->isPlaying.load();
		if (currentPage.numSamples > 0 && currentPage.sampleRate > 0)
		{
			double startSample = currentPage.loopStart * currentPage.sampleRate;
			double currentTimeInSection = (startSample + track->readPosition.load()) / currentPage.sampleRate;
			calculateHostBasedDisplay();
			waveformDisplay->setPlaybackPosition(currentTimeInSection, isCurrentlyPlaying);
		}
	}

	if (!intervalKnob.isMouseButtonDown())
	{
		int interval = track->randomRetriggerInterval.load();
		intervalKnob.setValue(interval, juce::dontSendNotification);
		intervalLabel.setText(getIntervalName(interval), juce::dontSendNotification);
	}

	updateRandomRetriggerButtonColor();
	updateRandomDurationButtonColor();

	updateButtonsEnabledState();

	if (track->isCurrentlyPlaying.load() && !isPreviewPlaying)
	{
		previewButton.setEnabled(false);
	}

	updateTrackInfo();
	updateAdsrKnobsFromPage();
}

float TrackComponent::calculateEffectiveBpm()
{
	if (!track)
		return 126.0f;
	auto &currentPage = track->getCurrentPage();
	float effectiveBpm = currentPage.originalBpm;
	switch (track->timeStretchMode)
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
	g.setColour(ColourPalette::backgroundDark.withAlpha(ObsidianShades::BACKGROUND_08));
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

	adsrAttackKnob.onValueChange = [this]()
	{
		if (!track)
			return;
		track->getCurrentPage().adsrAttack = (float)adsrAttackKnob.getValue();
		syncAdsrToWaveform();
		setSliderParameter("AdsrAttack", adsrAttackKnob);
	};
	adsrDecayKnob.onValueChange = [this]()
	{
		if (!track)
			return;
		track->getCurrentPage().adsrDecay = (float)adsrDecayKnob.getValue();
		syncAdsrToWaveform();
		setSliderParameter("AdsrDecay", adsrDecayKnob);
	};
	adsrSustainKnob.onValueChange = [this]()
	{
		if (!track)
			return;
		track->getCurrentPage().adsrSustain = (float)adsrSustainKnob.getValue();
		syncAdsrToWaveform();
		setSliderParameter("AdsrSustain", adsrSustainKnob);
	};
	adsrReleaseKnob.onValueChange = [this]()
	{
		if (!track)
			return;
		track->getCurrentPage().adsrRelease = (float)adsrReleaseKnob.getValue();
		syncAdsrToWaveform();
		setSliderParameter("AdsrRelease", adsrReleaseKnob);
	};
}

void TrackComponent::updateAdsrKnobsFromPage()
{
	if (!track)
		return;

	const auto &page = track->getCurrentPage();
	adsrAttackKnob.setValue(page.adsrAttack, juce::dontSendNotification);
	adsrDecayKnob.setValue(page.adsrDecay, juce::dontSendNotification);
	adsrSustainKnob.setValue(page.adsrSustain, juce::dontSendNotification);
	adsrReleaseKnob.setValue(page.adsrRelease, juce::dontSendNotification);

	syncAdsrToWaveform();
}

void TrackComponent::syncAdsrToWaveform()
{
	if (!waveformDisplay || !track)
		return;
	const auto &page = track->getCurrentPage();
	waveformDisplay->setAdsrParams(page.adsrAttack, page.adsrDecay, page.adsrSustain, page.adsrRelease);
}

void TrackComponent::resized()
{
	auto fullBounds = getLocalBounds();
	auto area = fullBounds.reduced(6);
	auto headerArea = area.removeFromTop(32);
	auto &currentPage = track->getCurrentPage();
	auto pagesArea = headerArea.removeFromLeft(38);
	int pagesGridHeight = PAGE_BUTTON_SIZE * 2 + 2;
	int yOffset = (pagesArea.getHeight() - pagesGridHeight) / 2;
	pagesArea.removeFromTop(yOffset);
	pagesArea.setHeight(pagesGridHeight);
	layoutPagesButtons(pagesArea);

	{
		const int selectorsWidth = 82;
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

	headerArea.removeFromLeft(INTRA_CLUSTER_GAP);

	{
		const int createButtonWidth = 34;
		generateButton.setBounds(headerArea.removeFromRight(createButtonWidth));
		headerArea.removeFromRight(6);

		drawButton.setBounds(headerArea.removeFromRight(createButtonWidth));
	}
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

	{
		const int labelledButtonWidth = 36;
		originalSyncButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
		headerArea.removeFromRight(INTRA_CLUSTER_GAP);
		previewButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
	}
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

	const int iconBtnWidth = 34;
	randomRetriggerButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

	randomDurationToggle.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

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

	headerArea.removeFromLeft(INTRA_CLUSTER_GAP);
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
		if (track != nullptr)
		{
			waveformDisplay = std::make_unique<WaveformDisplay>(audioProcessor, track);
			waveformDisplay->onLoopPointsChanged = [this](double start, double end)
			{
				if (track)
				{
					auto &currentPage = track->getCurrentPage();
					const double oldLoopStart = currentPage.loopStart;
					const double sr = currentPage.sampleRate;

					currentPage.loopStart = start;
					currentPage.loopEnd = end;

					waveformDisplay->setLoopPoints(start, end);

					if (track->isPlaying.load())
					{
						const double newStartSample = start * sr;
						const double newEndSample = end * sr;
						const double oldStartSample = oldLoopStart * sr;

						double currentAbs = oldStartSample + track->readPosition.load();
						double newRelative = currentAbs - newStartSample;

						if (currentAbs < newStartSample || currentAbs >= newEndSample)
						{
							track->readPosition.store(0.0);
						}
						else
						{
							track->readPosition.store(newRelative);
						}
					}
				}
			};

			waveformDisplay->onAdsrAttackChanged = [this](float v)
			{
				if (!track)
					return;
				track->getCurrentPage().adsrAttack.store(v);
				if (waveformDisplay)
					waveformDisplay->repaint();
			};

			waveformDisplay->onAdsrDecayChanged = [this](float v)
			{
				if (!track)
					return;
				track->getCurrentPage().adsrDecay.store(v);
				if (waveformDisplay)
					waveformDisplay->repaint();
			};

			waveformDisplay->onAdsrSustainChanged = [this](float v)
			{
				if (!track)
					return;
				track->getCurrentPage().adsrSustain.store(v);
				if (waveformDisplay)
					waveformDisplay->repaint();
			};

			waveformDisplay->onAdsrReleaseChanged = [this](float v)
			{
				if (!track)
					return;
				track->getCurrentPage().adsrRelease.store(v);
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
	}

	if (waveformDisplay)
	{
		area.removeFromTop(8);
		waveformDisplay->setBounds(area.removeFromTop(WAVEFORM_HEIGHT));
		waveformDisplay->setVisible(true);
	}

	if (!sequencer)
	{
		sequencer = std::make_unique<SequencerComponent>(trackId, audioProcessor);
		addAndMakeVisible(*sequencer);
		sequencerVisible = true;

		juce::String currentModel = modelSelector.getText();
		if (currentModel.isEmpty() && track)
			currentModel = currentPage.selectedModel;
		if (currentModel.isEmpty())
			currentModel = AiModelDefinitions::getAvailableModels()[0];
		sequencer->setAccentColour(AiModelDefinitions::getColourForModel(currentModel));
	}

	if (sequencer)
	{
		area.removeFromTop(5);
		sequencer->setBounds(area.removeFromTop(SEQUENCER_HEIGHT));
		sequencer->setVisible(true);
	}

	borderOverlay.setBounds(fullBounds);
	borderOverlay.toFront(false);
}

void TrackComponent::openDrawingCanvas()
{

	if (canvasModalOpen)
		return;
	canvasModalOpen = true;

	auto *canvas = ObsidianAlertManager::showDrawingCanvas(
	    this, audioProcessor, [this](const juce::String &) {},
	    [this](DrawingCanvas *canvas)
	    {
		    if (track && canvas)
		    {
			    auto canvasState = canvas->getState();
			    juce::String stateXml = canvasState.toXml();
			    auto &currentPage = track->getCurrentPage();
			    currentPage.canvasState = stateXml;
			    currentPage.canvasData = canvasState.imageBase64;
			    currentPage.selectedKeywords = canvasState.selectedKeywords;
		    }
		    canvasModalOpen = false;
	    });

	if (canvas == nullptr)
		return;
	drawingCanvasPtr = canvas;

	const auto &currentPage = track->getCurrentPage();
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
		if (track)
		{
			auto canvasState = canvas->getState();
			juce::String stateXml = canvasState.toXml();
			auto &currentPage = track->getCurrentPage();
			currentPage.canvasState = stateXml;
			currentPage.canvasData = base64Image;
			currentPage.selectedKeywords = canvasState.selectedKeywords;
		}
		if (onGenerateWithImage)
		{
			auto keywords = canvas->getState().selectedKeywords;
			onGenerateWithImage(trackId, base64Image, keywords);
		}
	};
}

void TrackComponent::layoutPagesButtons(juce::Rectangle<int> area)
{
	int buttonSize = PAGE_BUTTON_SIZE;
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
	const char *pageLabels[4] = {"A", "B", "C", "D"};

	for (int i = 0; i < 4; ++i)
	{
		addChildComponent(pageButtons[i]);
		pageButtons[i].setButtonText(pageLabels[i]);
		pageButtons[i].setClickingTogglesState(true);

		int groupId = 1000;
		if (track)
		{
			groupId += track->slotIndex;
		}
		pageButtons[i].setRadioGroupId(groupId);

		pageButtons[i].onClick = [this, i]() { onPageSelected(i); };

		pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
		pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight);
		pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);

		pageButtons[i].onMidiLearn = [this, i]()
		{
			if (track && track->slotIndex != -1)
			{
				const char *pageNames[4] = {"PageA", "PageB", "PageC", "PageD"};
				char pageLetter = 'A' + static_cast<char>(i);
				juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[i];
				juce::String description =
				    "Slot " + juce::String(track->slotIndex + 1) + " Page " + juce::String::charToString(pageLetter);

				statusCallback("Learning MIDI for " + description + "...");

				audioProcessor.getMidiLearnManager().startLearning(paramName, &audioProcessor, nullptr, description,
				                                                   &pageButtons[i]);
			}
		};

		pageButtons[i].onMidiRemove = [this, i]()
		{
			if (track && track->slotIndex != -1)
			{
				const char *pageNames[4] = {"PageA", "PageB", "PageC", "PageD"};
				juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[i];

				char pageLetter = 'A' + static_cast<char>(i);
				statusCallback("MIDI mapping removed for Page " + juce::String::charToString(pageLetter));

				audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName);
			}
		};
	}
}

void TrackComponent::onPageSelected(int pageIndex)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	if (track->currentPageIndex.load() == pageIndex && !track->pageChangePending.load())
	{
		pageButtons[pageIndex].setToggleState(true, juce::dontSendNotification);
		return;
	}

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setToggleState(i == track->currentPageIndex.load(), juce::dontSendNotification);
	}

	if (track->pageChangePending.load() && track->pendingPageIndex.load() == pageIndex)
	{
		track->pageChangePending = false;
		track->pendingPageIndex = -1;
		stopTimer();
		lastPageStates[pageIndex] = PageButtonState{};
		updatePagesDisplay();
		statusCallback("Page change cancelled");
		return;
	}

	if (track->slotIndex != -1)
	{
		const char *pageNames[4] = {"PageA", "PageB", "PageC", "PageD"};
		juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[pageIndex];

		auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
		if (param)
		{
			param->setValueNotifyingHost(1.0f);
		}
	}
}

void TrackComponent::performPageChange(int pageIndex)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	if (isPreviewPlaying)
	{
		if (onStopPreview)
			onStopPreview(trackId);
		setPreviewPlaying(false);
	}

	bool wasPlaying = track->isPlaying.load();
	bool wasArmed = track->isArmed.load();
	bool wasArmedToStop = track->isArmedToStop.load();
	bool wasCurrentlyPlaying = track->isCurrentlyPlaying.load();

	track->setCurrentPage(pageIndex);

	track->isPlaying = wasPlaying;
	track->isArmed = wasArmed;
	track->isArmedToStop = wasArmedToStop;
	track->isCurrentlyPlaying = wasCurrentlyPlaying;
	track->readPosition = 0.0;

	const auto &newPage = track->getCurrentPage();

	if (newPage.numSamples == 0 && wasPlaying)
	{
		track->isPlaying = false;
		track->isCurrentlyPlaying = false;
		track->readPosition = 0.0;
		if (track->onPlayStateChanged)
		{
			track->onPlayStateChanged(false);
		}
	}

	track->pageChangePending = false;
	track->pendingPageIndex = -1;

	if (!isGenerating && !track->pageChangePending.load())
	{
		stopTimer();
	}

	updatePagesDisplay();
	updateFromTrackData();
	updateAdsrKnobsFromPage();
	updateModelUI();

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
			waveformDisplay->setAudioData(emptyBuffer, 48000.0);
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
	if (!track)
		return;

	auto modelColour = cachedModelColour;
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	int pendingPage = track->pageChangePending.load() ? track->pendingPageIndex.load() : -1;

	for (int i = 0; i < 4; ++i)
	{
		PageButtonState newState;
		newState.isActive = (i == track->currentPageIndex.load());
		newState.isPending = (i == pendingPage);
		newState.hasAudio = track->pages[i].numSamples > 0;
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
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto &page = track->pages[pageIndex];
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
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto &page = track->pages[pageIndex];

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
		    [this, pageIndex]()
		    {
			    if (track && track->currentPageIndex.load() == pageIndex)
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

	for (int i = 0; i < 4; ++i)
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
	isGenerating = false;

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setEnabled(true);
	}

	if (!track || !track->pageChangePending.load())
	{
		stopTimer();
	}

	if (waveformDisplay && track)
	{
		const auto &currentPage = track->getCurrentPage();
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
	if (isGenerating)
	{
		blinkState = !blinkState;
		syncBorderOverlay();
	}

	if (track && track->pageChangePending.load())
	{
		pageBlinkState = !pageBlinkState;
		updatePagesDisplay();
	}

	if (!isGenerating && (!track || !track->pageChangePending.load()))
		stopTimer();
}

void TrackComponent::refreshWaveformDisplay()
{
	if (!waveformDisplay || !track)
		return;

	const auto &currentPage = track->getCurrentPage();

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
		waveformDisplay->setAudioData(emptyBuffer, 48000.0);
		waveformDisplay->setLoopPoints(0.0, 0.0);
	}
}

void TrackComponent::setGenerateButtonEnabled(bool enabled)
{
	generateButton.setEnabled(enabled);
}

void TrackComponent::removeListener(juce::String name)
{
	if (!track || track->slotIndex == -1)
		return;
	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
	if (param)
	{
		param->removeListener(this);
	}
}

void TrackComponent::addListener(juce::String name)
{
	if (!track || track->slotIndex == -1)
	{
		return;
	}
	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;

	auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
	if (param)
	{
		param->addListener(this);
	}
}

void TrackComponent::setupUI()
{
	addAndMakeVisible(infoLabel);
	infoLabel.setText("Empty track - Generate your sample!", juce::dontSendNotification);
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setFont(juce::FontOptions(12.0f));

	promptPresetSelector.setTooltip("Select prompt for this track");
	promptPresetSelector.onChange = [this]() { onTrackPresetSelected(); };

	modelSelector.clear();

	addAndMakeVisible(promptPresetSelector);
	addAndMakeVisible(modelSelector);

	auto &models = AiModelDefinitions::getAvailableModels();
	for (int i = 0; i < models.size(); ++i)
	{
		modelSelector.addItem(models[i], i + 1);
	}

	int trackNum = trackId.retainCharacters("0123456789").getIntValue();
	if (trackNum >= 1 && trackNum <= models.size())
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
		auto selectedModel = modelSelector.getText();
		if (track != nullptr)
		{
			track->getCurrentPage().selectedModel = selectedModel;
		}
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

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setVisible(true);
	}

	setupPagesUI();

	addAndMakeVisible(borderOverlay);
}

void TrackComponent::syncTrackName(const juce::String &name)
{
	if (track)
		track->trackName = name;
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

	addAndMakeVisible(drawButton);
	drawButton.loadIcon(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	drawButton.setShowBackground(false);
	setupActionButton(drawButton);
	drawButton.setTooltip("Draw a visual prompt to guide AI generation (server mode only)");
	drawButton.onClick = [this]() { openDrawingCanvas(); };

	addAndMakeVisible(generateButton);
	generateButton.loadIcon(BinaryData::zap_svg, BinaryData::zap_svgSize);
	generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
	generateButton.setColour(juce::TextButton::textColourOffId, ColourPalette::backgroundDeep);
	generateButton.setTooltip("Generate AI audio with current prompt for this track");
	generateButton.onClick = [this]()
	{
		if (onGenerateForTrack)
		{
			if (track)
			{
				auto &currentPage = track->getCurrentPage();
				currentPage.selectedPrompt = promptPresetSelector.getText();
				currentPage.generationBpm = audioProcessor.getGlobalBpm();
				currentPage.generationKey = audioProcessor.getGlobalKey();
				currentPage.generationDuration = audioProcessor.getGlobalDuration();
			}
			onGenerateForTrack(trackId);
			setButtonParameter("Generate");
		}
	};

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
		if (track && onPreviewTrack)
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

	addAndMakeVisible(randomRetriggerButton);
	randomRetriggerButton.loadIcon(BinaryData::repeat_svg, BinaryData::repeat_svgSize);
	randomRetriggerButton.setShowBackground(false);
	setupToggleButton(randomRetriggerButton);
	randomRetriggerButton.setTooltip("Beat repeat - re-trigger current section at interval while ON");
	randomRetriggerButton.onClick = [this]() { onRandomRetriggerToggled(); };

	addAndMakeVisible(randomDurationToggle);
	randomDurationToggle.loadIcon(BinaryData::shuffle_svg, BinaryData::shuffle_svgSize);
	randomDurationToggle.setShowBackground(false);
	setupToggleButton(randomDurationToggle);
	randomDurationToggle.setTooltip("Auto-randomize repeat interval on each trigger");
	randomDurationToggle.onClick = [this]()
	{
		if (track)
		{
			track->randomRetriggerDurationEnabled = randomDurationToggle.getToggleState();
			updateRandomDurationButtonColor();
			statusCallback("Auto-random duration: " +
			               juce::String(track->randomRetriggerDurationEnabled.load() ? "ON" : "OFF"));
		}
	};
}

void TrackComponent::updateButtonsEnabledState()
{
	bool hasAudio = track && track->getCurrentPage().numSamples > 0;

	bool hasOriginal = false;
	if (track)
	{
		hasOriginal = track->getCurrentPage().hasOriginalVersion.load();
	}

	previewButton.setEnabled(hasAudio);
	randomRetriggerButton.setEnabled(hasAudio);

	originalSyncButton.setEnabled(hasAudio && hasOriginal);

	randomDurationToggle.setEnabled(hasAudio);

	intervalKnob.setEnabled(hasAudio);
	intervalLabel.setEnabled(hasAudio);
}

void TrackComponent::updateRandomRetriggerButtonColor()
{
	if (!track)
		return;
	randomRetriggerButton.setToggleState(track->randomRetriggerEnabled.load(), juce::dontSendNotification);
}

void TrackComponent::updateRandomDurationButtonColor()
{
	if (!track)
		return;
	randomDurationToggle.setToggleState(track->randomRetriggerDurationEnabled.load(), juce::dontSendNotification);
}

void TrackComponent::onRandomRetriggerToggled()
{
	if (!track)
		return;

	bool isEnabled = !track->randomRetriggerEnabled.load();
	track->randomRetriggerEnabled = isEnabled;

	if (isEnabled)
	{
		track->beatRepeatPending.store(true);
	}
	else
	{
		track->beatRepeatStopPending.store(true);
	}

	updateRandomRetriggerButtonColor();
	statusCallback("Beat Repeat " + juce::String(isEnabled ? "ON" : "OFF"));
	setButtonParameter("RandomRetrigger");
}

void TrackComponent::onIntervalChanged()
{
	if (!track)
		return;

	int value = (int)juce::roundToInt(intervalKnob.getValue());

	if (track->randomRetriggerInterval.load() != value)
	{
		track->randomRetriggerInterval = value;

		if (track->beatRepeatActive.load())
		{
			double hostBpm = audioProcessor.getHostBpm();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double startPosition = track->beatRepeatStartPosition.load();
			double repeatDuration = audioProcessor.getSequencerManager().calculateRetriggerInterval(value, hostBpm);
			double repeatDurationSamples = repeatDuration * track->getCurrentPage().sampleRate;
			track->beatRepeatEndPosition.store(startPosition + repeatDurationSamples);

			double maxSamples = track->getCurrentPage().numSamples;
			if (track->beatRepeatEndPosition.load() > maxSamples)
			{
				track->beatRepeatEndPosition.store(maxSamples);
			}
		}
	}

	juce::String intervalName = getIntervalName(value);
	intervalLabel.setText(intervalName, juce::dontSendNotification);
	statusCallback("Interval: " + intervalName);
	setSliderParameter("RetriggerInterval", intervalKnob);
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
	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		editor->statusLabel.setText(message, juce::dontSendNotification);
		editor->uiStatusManager->updateLCD();
	}
}

void TrackComponent::setSliderParameter(juce::String name, juce::Slider &slider)
{
	if (!track || track->slotIndex == -1)
		return;
	if (this == nullptr)
		return;

	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	try
	{
		auto &parameterTreeState = audioProcessor.getParameterTreeState();
		auto *param = parameterTreeState.getParameter(paramName);

		if (param != nullptr)
		{
			float value = static_cast<float>(slider.getValue());
			if (!std::isnan(value) && !std::isinf(value))
			{
				if (name == "RetriggerInterval")
				{
					value = (value - 1.0f) / 9.0f;
				}
				else if (name == "AdsrAttack" || name == "AdsrDecay" || name == "AdsrRelease")
				{
					value = (value - 0.001f) / (4.0f - 0.001f);
				}
				param->setValueNotifyingHost(value);
			}
		}
	}
	catch (...)
	{
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

		    safeThis->promptPresetSelector.clear(juce::dontSendNotification);

		    if (safeThis->track == nullptr)
			    return;

		    auto &audioProcessor = safeThis->audioProcessor;
		    juce::String currentModel = safeThis->track->getCurrentPage().selectedModel;
		    juce::StringArray allPrompts = audioProcessor.getAvailablePromptsForModel(currentModel);

		    safeThis->promptPresets = allPrompts;

		    for (int i = 0; i < allPrompts.size(); ++i)
		    {
			    safeThis->promptPresetSelector.addItem(allPrompts[i], i + 1);
		    }

		    bool selected = false;
		    const auto &selectedPrompt = safeThis->track->getCurrentPage().selectedPrompt;

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
			    safeThis->track->getCurrentPage().selectedPrompt = allPrompts[0];
		    }
	    });
}

void TrackComponent::updatePromptPresets(const juce::StringArray &presets)
{
	juce::String currentSelection = promptPresetSelector.getText();
	promptPresets = presets;
	promptPresets.sort(true);
	promptPresetSelector.clear();

	for (int i = 0; i < promptPresets.size(); ++i)
		promptPresetSelector.addItem(promptPresets[i], i + 1);

	int index = promptPresets.indexOf(currentSelection);
	if (index >= 0)
		promptPresetSelector.setSelectedId(index + 1, juce::dontSendNotification);
	else if (promptPresets.size() > 0)
	{
		promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
		onTrackPresetSelected();
	}
}

void TrackComponent::toggleOriginalSync()
{
	if (!track)
		return;

	bool useOriginal = originalSyncButton.getToggleState();

	auto &currentPage = track->getCurrentPage();
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
		                            if (track)
		                            {
			                            const auto &currentPage = track->getCurrentPage();
			                            if (currentPage.hasOriginalVersion.load())
			                            {
				                            originalSyncButton.setEnabled(true);
			                            }
		                            }
	                            });
}

void TrackComponent::onTrackPresetSelected()
{
	if (track)
	{
		juce::String newPrompt = promptPresetSelector.getText();

		auto &currentPage = track->getCurrentPage();
		currentPage.selectedPrompt = newPrompt;

		if (onTrackPromptChanged)
		{
			onTrackPromptChanged(trackId, newPrompt);
		}
	}
}

void TrackComponent::updateTrackInfo()
{
	if (!track)
		return;

	if (!track->getCurrentPage().prompt.isEmpty())
	{
		float effectiveBpm = calculateEffectiveBpm();
		float originalBpm = track->getCurrentPage().originalBpm;

		juce::String bpmInfo = "";
		juce::String stretchIndicator = "";

		switch (track->timeStretchMode)
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
			stretchIndicator = (track->getCurrentPage().bpmOffset > 0)   ? " +"
			                   : (track->getCurrentPage().bpmOffset < 0) ? " -"
			                                                             : "";
			bpmInfo = " | Host+ " + juce::String(track->getCurrentPage().bpmOffset, 1) + stretchIndicator;
			break;
		}

		infoLabel.setText(track->getCurrentPage().prompt.substring(0, 30) + "..." + bpmInfo,
		                  juce::dontSendNotification);
	}
}

void TrackComponent::refreshWaveformIfNeeded()
{
	if (waveformDisplay && track && track->getCurrentPage().numSamples > 0)
	{
		if (track->getCurrentPage().numSamples != lastWaveformNumSamples)
		{
			refreshWaveformDisplay();
			lastWaveformNumSamples = track->getCurrentPage().numSamples;
		}
	}
}

void TrackComponent::updatePromptSelection(const juce::String &promptText)
{
	if (!track)
		return;

	track->getCurrentPage().selectedPrompt = promptText;

	for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
	{
		if (promptPresetSelector.getItemText(i) == promptText)
		{
			promptPresetSelector.setSelectedItemIndex(i, juce::sendNotification);
			break;
		}
	}
}

void TrackComponent::learn(juce::String param, MidiLearnableBase *component, std::function<void(float)> uiCallback)
{
	if (audioProcessor.getActiveEditor() && track && track->slotIndex != -1)
	{
		juce::String parameterName = "slot" + juce::String(track->slotIndex + 1) + param;
		juce::String description = "Slot " + juce::String(track->slotIndex + 1) + " " + param;
		juce::MessageManager::callAsync(
		    [this, description]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    editor->statusLabel.setText("Learning MIDI for " + description + "...", juce::dontSendNotification);
				    editor->uiStatusManager->updateLCD();
			    }
		    });
		audioProcessor.getMidiLearnManager().startLearning(parameterName, &audioProcessor, uiCallback, description,
		                                                   component);
	}
}

void TrackComponent::removeMidiMapping(const juce::String &param)
{
	if (track && track->slotIndex != -1)
	{
		juce::String parameterName = "slot" + juce::String(track->slotIndex + 1) + param;
		audioProcessor.getMidiLearnManager().removeMappingForParameter(parameterName);
	}
}

void TrackComponent::setupMidiLearn()
{
	if (!track)
		return;

	generateButton.onMidiLearn = [this]() { learn("Generate", &generateButton); };
	generateButton.onMidiRemove = [this]() { removeMidiMapping("Generate"); };

	randomRetriggerButton.onMidiLearn = [this]() { learn("RandomRetrigger", &randomRetriggerButton); };

	randomRetriggerButton.onMidiRemove = [this]()
	{
		removeMidiMapping("RandomRetrigger");
		updateRandomRetriggerButtonColor();
	};

	intervalKnob.onMidiLearn = [this]() { learn("RetriggerInterval", &intervalKnob); };
	intervalKnob.onMidiRemove = [this]() { removeMidiMapping("RetriggerInterval"); };

	juce::String paramName = "promptSelector_slot" + juce::String(track->slotIndex + 1);
	auto promptCallback = [this](float value)
	{
		juce::MessageManager::callAsync(
		    [this, value]()
		    {
			    int numItems = promptPresetSelector.getNumItems();
			    if (numItems > 0)
			    {
				    int selectedIndex = (int)(value * (numItems - 1));
				    promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
			    }
		    });
	};

	audioProcessor.getMidiLearnManager().registerUICallback(paramName, promptCallback);

	promptPresetSelector.onMidiLearn = [this, paramName, promptCallback]()
	{
		if (audioProcessor.getActiveEditor() && track && track->slotIndex != -1)
		{
			juce::String description = "Slot " + juce::String(track->slotIndex + 1) + " Prompt Selector";
			audioProcessor.getMidiLearnManager().startLearning(paramName, &audioProcessor, promptCallback, description,
			                                                   &promptPresetSelector);
		}
	};

	promptPresetSelector.onMidiRemove = [this, paramName]()
	{ audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName); };

	adsrAttackKnob.onMidiLearn = [this]() { learn("AdsrAttack", &adsrAttackKnob); };
	adsrAttackKnob.onMidiRemove = [this]() { removeMidiMapping("AdsrAttack"); };

	adsrDecayKnob.onMidiLearn = [this]() { learn("AdsrDecay", &adsrDecayKnob); };
	adsrDecayKnob.onMidiRemove = [this]() { removeMidiMapping("AdsrDecay"); };

	adsrSustainKnob.onMidiLearn = [this]() { learn("AdsrSustain", &adsrSustainKnob); };
	adsrSustainKnob.onMidiRemove = [this]() { removeMidiMapping("AdsrSustain"); };

	adsrReleaseKnob.onMidiLearn = [this]() { learn("AdsrRelease", &adsrReleaseKnob); };
	adsrReleaseKnob.onMidiRemove = [this]() { removeMidiMapping("AdsrRelease"); };
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
	isDragOver = false;
	isDraggingPrompt = false;
	syncBorderOverlay();

	juce::String description = dragSourceDetails.description.toString();
	if (description.isEmpty() || !track)
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
				for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
				{
					if (promptPresetSelector.getItemText(i) == sampleEntry->originalPrompt)
					{
						promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
						track->getCurrentPage().selectedPrompt = sampleEntry->originalPrompt;
						break;
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
						track->getCurrentPage().selectedModel = sampleEntry->modelName;
						break;
					}
				}
			}
		}
	}

	if (track->slotIndex >= 0 && track->slotIndex < audioProcessor.getAudioManager().MAX_SLOTS)
	{
		auto &apvts = audioProcessor.getParameterManager().getAPVTS();
		juce::String s = "slot" + juce::String(track->slotIndex + 1);

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
	if (track == nullptr)
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
	setupToggleColours(randomRetriggerButton);
	setupToggleColours(randomDurationToggle);
	setupToggleColours(drawButton);

	if (sequencer)
		sequencer->setAccentColour(modelColour);

	syncBorderOverlay();
	loadPromptPresets();
}

void TrackComponent::detachWaveformTrack()
{
	if (waveformDisplay)
		waveformDisplay->setTrack(nullptr);
}

void TrackComponent::applyPromptFromBank(const juce::String &promptId)
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	auto *entry = bank->getPrompt(promptId);
	if (!entry || !track)
		return;

	if (entry->modelName.isNotEmpty())
	{
		for (int i = 0; i < modelSelector.getNumItems(); ++i)
		{
			if (modelSelector.getItemText(i) == entry->modelName)
			{
				modelSelector.setSelectedItemIndex(i, juce::sendNotification);
				track->getCurrentPage().selectedModel = entry->modelName;
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
				track->getCurrentPage().selectedPrompt = entry->text;
				found = true;
				break;
			}
		}

		if (!found)
		{
			promptPresetSelector.addItem(entry->text, promptPresetSelector.getNumItems() + 1);
			promptPresetSelector.setSelectedId(promptPresetSelector.getNumItems(), juce::sendNotification);
			track->getCurrentPage().selectedPrompt = entry->text;
		}
	}

	bank->incrementUsage(promptId);

	if (onStatusMessage)
		onStatusMessage("Prompt loaded from bank!");
}