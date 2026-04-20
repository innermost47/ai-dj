#include "TrackComponent.h"
#include "WaveformDisplay.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "PluginEditor.h"
#include "ColourPalette.h"
#include "AiModelDefinitions.h"

TrackComponent::TrackComponent(const juce::String& trackId, DjIaVstProcessor& processor)
	: trackId(trackId), track(nullptr), audioProcessor(processor)
{
	setupUI();
	loadPromptPresets();
}

TrackComponent::~TrackComponent()
{
	if (track && track->slotIndex != -1)
	{
		removeListener("Generate");
		removeListener("RandomRetrigger");
		removeListener("RetriggerInterval");
	}
	isDestroyed.store(true);
	stopTimer();
	track = nullptr;
}

void TrackComponent::addEventListeners()
{
	addListener("Generate");
	addListener("RandomRetrigger");
	addListener("RetriggerInterval");
}

void TrackComponent::setTrackData(TrackData* trackData)
{
	track = trackData;
	if (track && !track->usePages.load())
	{
		track->migrateToPages();
		pagesMode = true;
	}
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

void TrackComponent::updateUIFromParameter(const juce::String& paramName,
	const juce::String& slotPrefix,
	float newValue)
{
	if (isDestroyed.load())
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
}

void TrackComponent::parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/)
{
}

void TrackComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	if (!track || track->slotIndex == -1)
		return;

	juce::String slotPrefix = "Slot " + juce::String(track->slotIndex + 1);
	auto& allParams = audioProcessor.AudioProcessor::getParameters();

	if (parameterIndex >= 0 && parameterIndex < allParams.size())
	{
		auto* param = allParams[parameterIndex];
		juce::String paramName = param->getName(256);

		if (juce::MessageManager::getInstance()->isThisTheMessageThread())
		{
			juce::Timer::callAfterDelay(50, [this, paramName, slotPrefix, newValue]()
				{ updateUIFromParameter(paramName, slotPrefix, newValue); });
		}
		else
		{
			juce::MessageManager::callAsync([this, paramName, slotPrefix, newValue]()
				{ juce::Timer::callAfterDelay(50, [this, paramName, slotPrefix, newValue]()
					{ updateUIFromParameter(paramName, slotPrefix, newValue); }); });
		}
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
		auto* param = audioProcessor.getParameters().getParameter(paramName);
		if (param != nullptr)
		{
			if (name == "Generate")
			{
				param->setValueNotifyingHost(1.0f);
				juce::Timer::callAfterDelay(100, [param]()
					{ param->setValueNotifyingHost(0.0f); });
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
		DBG("Exception in setButtonParameter for " << paramName);
	}
}

void TrackComponent::calculateHostBasedDisplay()
{
	if (!track || track->numSamples == 0)
		return;

	float effectiveBpm = calculateEffectiveBpm();

	if (waveformDisplay)
	{
		waveformDisplay->setOriginalBpm(track->originalBpm);
		waveformDisplay->setSampleBpm(effectiveBpm);
		if (!track->audioFilePath.isEmpty())
		{
			juce::File audioFile(track->audioFilePath);
			waveformDisplay->setAudioFile(audioFile);
		}
	}
}

void TrackComponent::toggleWaveformDisplay()
{
	if (showWaveformButton.getToggleState())
	{
		if (!waveformDisplay && track != nullptr)
		{
			waveformDisplay = std::make_unique<WaveformDisplay>(audioProcessor, *track);
			waveformDisplay->onLoopPointsChanged = [this](double start, double end)
				{
					if (track)
					{
						if (track->usePages.load())
						{
							auto& currentPage = track->getCurrentPage();
							currentPage.loopStart = start;
							currentPage.loopEnd = end;
							track->syncLegacyProperties();
						}
						else
						{
							track->loopStart = start;
							track->loopEnd = end;
						}

						waveformDisplay->setLoopPoints(start, end);
						if (track->isPlaying.load())
						{
							track->readPosition = 0.0;
						}
					}
				};

			addAndMakeVisible(*waveformDisplay);
		}

		if (track && track->numSamples > 0)
		{
			waveformDisplay->setAudioData(track->audioBuffer, track->sampleRate);
			waveformDisplay->setLoopPoints(track->loopStart, track->loopEnd);
			calculateHostBasedDisplay();
		}

		waveformDisplay->setVisible(true);
	}
	else
	{
		if (waveformDisplay)
		{
			waveformDisplay->setVisible(false);
		}
	}

	bool waveformVisible = showWaveformButton.getToggleState();
	int newHeight = BASE_HEIGHT;
	if (waveformVisible)
		newHeight += WAVEFORM_HEIGHT;
	if (sequencerVisible)
		newHeight += SEQUENCER_HEIGHT;

	setSize(getWidth(), newHeight);

	if (auto* parentViewport = findParentComponentOfClass<juce::Viewport>())
	{
		if (auto* parentContainer = parentViewport->getViewedComponent())
		{
			int totalHeight = 5;
			for (int i = 0; i < parentContainer->getNumChildComponents(); ++i)
			{
				if (auto* trackComp = dynamic_cast<TrackComponent*>(parentContainer->getChildComponent(i)))
				{
					bool hasWaveform = trackComp->showWaveformButton.getToggleState();
					bool hasSequencer = trackComp->sequencerVisible;

					int trackHeight = BASE_HEIGHT;
					if (hasWaveform)
						trackHeight += WAVEFORM_HEIGHT;
					if (hasSequencer)
						trackHeight += SEQUENCER_HEIGHT;

					trackComp->setSize(trackComp->getWidth(), trackHeight);
					trackComp->setBounds(trackComp->getX(), totalHeight, trackComp->getWidth(), trackHeight);
					totalHeight += trackHeight + 5;
				}
			}
			parentContainer->setSize(parentContainer->getWidth(), totalHeight);
			parentContainer->resized();
		}
	}
	resized();
	repaint();
}

void TrackComponent::updatePlaybackPosition(double timeInSeconds)
{
	if (waveformDisplay && showWaveformButton.getToggleState())
	{
		bool isPlaying = track && track->isPlaying.load();
		waveformDisplay->setPlaybackPosition(timeInSeconds, isPlaying);
	}
}

void TrackComponent::updateFromTrackData()
{
	if (track == nullptr)
		return;

	if (track->usePages.load())
	{
		pagesMode = true;
		togglePagesButton.setVisible(false);
		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setVisible(true);
		}
		pageButtons[track->currentPageIndex].setToggleState(true, juce::dontSendNotification);
		updatePagesDisplay();
	}
	else
	{
		pagesMode = false;
		togglePagesButton.setVisible(true);
		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setVisible(false);
		}
	}

	randomDurationToggle.setToggleState(track->randomRetriggerDurationEnabled.load(), juce::dontSendNotification);

	drawButton.setEnabled(!audioProcessor.getUseLocalModel());

	bool hasOriginal = false;
	bool useOriginal = false;

	if (track->usePages.load())
	{
		const auto& currentPage = track->getCurrentPage();
		hasOriginal = currentPage.hasOriginalVersion.load();
		useOriginal = hasOriginal && currentPage.useOriginalFile.load();
	}
	else
	{
		hasOriginal = track->hasOriginalVersion.load();
		if (!hasOriginal)
		{
			track->useOriginalFile = false;
		}
		useOriginal = hasOriginal && track->useOriginalFile.load();
	}

	originalSyncButton.setToggleState(useOriginal, juce::dontSendNotification);

	trackNameLabel.setText(track->trackName, juce::dontSendNotification);
	trackNumberButton.setButtonText(juce::String(track->slotIndex + 1));
	trackNumberButton.setColour(juce::TextButton::buttonColourId,
		ColourPalette::getTrackColour(track->slotIndex));

	bpmOffsetSlider.setValue(track->bpmOffset, juce::dontSendNotification);

	if (!track->selectedPrompt.isEmpty())
	{
		for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
		{
			if (promptPresetSelector.getItemText(i) == track->selectedPrompt)
			{
				promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
				break;
			}
		}
	}

	if (waveformDisplay)
	{
		bool isCurrentlyPlaying = track->isPlaying.load();
		if (track->numSamples > 0 && track->sampleRate > 0)
		{
			double startSample = track->loopStart * track->sampleRate;
			double currentTimeInSection = (startSample + track->readPosition.load()) / track->sampleRate;
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

	juce::String modelToSet = track->usePages.load()
		? track->getCurrentPage().selectedModel
		: track->selectedModel;

	if (modelToSet.isEmpty())
	{
		auto& models = AiModelDefinitions::getAvailableModels();
		modelToSet = models[0];
	}

	modelSelector.setText(modelToSet, juce::dontSendNotification);
	updateModelUI();

	updateRandomRetriggerButtonColor();
	updateRandomDurationButtonColor();

	updateButtonsEnabledState();

	if (track->isPlaying.load() && !isPreviewPlaying)
	{
		previewButton.setEnabled(false);
	}

	updateTrackInfo();
}

float TrackComponent::calculateEffectiveBpm()
{
	if (!track)
		return 126.0f;

	float effectiveBpm = track->originalBpm;

	switch (track->timeStretchMode)
	{
	case 1:
		effectiveBpm = track->originalBpm;
		break;

	case 2:
		effectiveBpm = track->originalBpm + static_cast<float>(track->bpmOffset);
		break;

	case 3:
	{
		double hostBpm = audioProcessor.getHostBpm();
		if (hostBpm > 0.0 && track->originalBpm > 0.0)
		{
			float ratio = (float)hostBpm / track->originalBpm;
			effectiveBpm = track->originalBpm * ratio;
		}
	}
	break;

	case 4:
	{
		double hostBpm = audioProcessor.getHostBpm();
		if (hostBpm > 0.0 && track->originalBpm > 0.0)
		{
			float ratio = (float)hostBpm / track->originalBpm;
			effectiveBpm = track->originalBpm * ratio + static_cast<float>(track->bpmOffset);
		}
	}
	break;
	}

	return juce::jlimit(40.0f, 250.0f, effectiveBpm);
}

void TrackComponent::setSelected(bool selected)
{
	isSelected = selected;
	repaint();
}

void TrackComponent::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();

	juce::Colour bgColour;
	if (isDragOver)
		bgColour = ColourPalette::buttonSuccess.withAlpha(0.4f);
	else if (hasSamplePending && !isGenerating)
		bgColour = ColourPalette::samplePending.withAlpha(0.15f);
	else
		bgColour = ColourPalette::backgroundDark.withAlpha(0.8f);

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

	juce::Colour borderColour;
	float borderWidth;

	if (isGenerating)
	{
		borderColour = blinkState ? ColourPalette::buttonDangerLight : ColourPalette::buttonDangerDark;
		borderWidth = 3.0f;
	}
	else if (hasSamplePending)
	{
		borderColour = ColourPalette::samplePending;
		borderWidth = 2.0f;
	}
	else if (isSelected)
	{
		borderColour = ColourPalette::trackSelected;
		borderWidth = 2.0f;
	}
	else
	{
		borderColour = ColourPalette::backgroundLight;
		borderWidth = 1.0f;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, borderWidth);
}

void TrackComponent::setSamplePending(bool pending)
{
	hasSamplePending = pending;
	repaint();
}

void TrackComponent::layoutPlaybackCluster(juce::Rectangle<int> area)
{
	const int gap = 2;
	int cellW = (area.getWidth() - gap) / 2;
	int cellH = (area.getHeight() - gap) / 2;

	auto topRow = area.removeFromTop(cellH);
	originalSyncButton.setBounds(topRow.removeFromLeft(cellW));
	topRow.removeFromLeft(gap);
	previewButton.setBounds(topRow.removeFromLeft(cellW));

	area.removeFromTop(gap);

	auto bottomRow = area.removeFromTop(cellH);
	showWaveformButton.setBounds(bottomRow.removeFromLeft(cellW));
	bottomRow.removeFromLeft(gap);
	sequencerToggleButton.setBounds(bottomRow.removeFromLeft(cellW));
}

void TrackComponent::layoutFxCluster(juce::Rectangle<int> area)
{
	const int gap = 2;
	int cellW = (area.getWidth() - gap) / 2;
	int topRowHeight = area.getHeight() / 2;

	auto topRow = area.removeFromTop(topRowHeight);
	randomRetriggerButton.setBounds(topRow.removeFromLeft(cellW));
	topRow.removeFromLeft(gap);
	randomDurationToggle.setBounds(topRow.removeFromLeft(cellW));

	area.removeFromTop(gap);

	const int knobDiameter = 22;
	const int labelHeight = 10;

	intervalKnob.setBounds(
		area.getX() + (area.getWidth() - knobDiameter) / 2,
		area.getY(),
		knobDiameter, knobDiameter);

	intervalLabel.setBounds(
		area.getX(),
		area.getY() + knobDiameter + 2,
		area.getWidth(), labelHeight);
}

void TrackComponent::resized()
{
	auto area = getLocalBounds().reduced(6);

	const int leftColumnWidth = 28;
	auto leftColumn = area.removeFromLeft(leftColumnWidth);

	const int smallButtonSize = 22;
	auto noteButtonArea = leftColumn.removeFromTop(smallButtonSize);
	noteButtonArea = noteButtonArea.withSizeKeepingCentre(smallButtonSize, smallButtonSize);
	trackNumberButton.setBounds(noteButtonArea);

	leftColumn.removeFromTop(4);

	{
		const int labelHeight = leftColumn.getHeight();
		const int labelWidth = leftColumnWidth;

		trackNameLabel.setTransform(juce::AffineTransform::identity);
		trackNameLabel.setBounds(leftColumn.getX(), leftColumn.getY(), labelHeight, labelWidth);

		auto transform = juce::AffineTransform::rotation(
			-juce::MathConstants<float>::halfPi,
			(float)(leftColumn.getX() + labelHeight / 2),
			(float)(leftColumn.getY() + labelWidth / 2))
			.translated(
				(float)(-(labelHeight - labelWidth) / 2),
				(float)((labelHeight - labelWidth) / 2));
		trackNameLabel.setTransform(transform);
	}

	area.removeFromLeft(5);

	const int compactHeaderHeight = 36;
	auto headerArea = area.removeFromTop(compactHeaderHeight);

	if (pagesMode)
	{
		auto pagesArea = headerArea.removeFromLeft(48);
		int pagesGridHeight = PAGE_BUTTON_SIZE * 2 + 2;
		int yOffset = (pagesArea.getHeight() - pagesGridHeight) / 2;
		pagesArea.removeFromTop(yOffset);
		pagesArea.setHeight(pagesGridHeight);
		layoutPagesButtons(pagesArea);
		headerArea.removeFromLeft(3);
	}
	else
	{
		auto toggleArea = headerArea.removeFromLeft(25);
		int toggleHeight = 25;
		int yOffset = (toggleArea.getHeight() - toggleHeight) / 2;
		togglePagesButton.setBounds(toggleArea.getX(), toggleArea.getY() + yOffset,
			toggleArea.getWidth(), toggleHeight);
		headerArea.removeFromLeft(3);
	}

	{
		const int selectorsWidth = 140;
		auto selectorsArea = headerArea.removeFromLeft(selectorsWidth);

		const int selectorHeight = 18;
		const int gap = 2;
		const int totalStackHeight = selectorHeight * 2 + gap;
		int yOffset = (selectorsArea.getHeight() - totalStackHeight) / 2;

		promptPresetSelector.setBounds(
			selectorsArea.getX(),
			selectorsArea.getY() + yOffset,
			selectorsArea.getWidth(),
			selectorHeight);

		modelSelector.setBounds(
			selectorsArea.getX(),
			selectorsArea.getY() + yOffset + selectorHeight + gap,
			selectorsArea.getWidth(),
			selectorHeight);
	}

	headerArea.removeFromLeft(8);

	{
		const int createButtonWidth = 36;
		generateButton.setBounds(headerArea.removeFromRight(createButtonWidth));
		headerArea.removeFromRight(INTRA_CLUSTER_GAP);

		drawButton.setBounds(headerArea.removeFromRight(createButtonWidth));
	}
	headerArea.removeFromRight(CLUSTER_GAP);

	{
		const int labelledButtonWidth = 48;
		originalSyncButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
		headerArea.removeFromRight(INTRA_CLUSTER_GAP);
		previewButton.setBounds(headerArea.removeFromRight(labelledButtonWidth));
	}
	headerArea.removeFromRight(CLUSTER_GAP);

	const int iconBtnWidth = 32;
	randomRetriggerButton.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

	randomDurationToggle.setBounds(headerArea.removeFromRight(iconBtnWidth));
	headerArea.removeFromRight(INTRA_CLUSTER_GAP);

	{
		auto knobArea = headerArea.removeFromRight(38);
		const int knobDiameter = 30;
		const int labelHeight = 10;
		const int stackHeight = knobDiameter + 2 + labelHeight;
		int yOffset = (knobArea.getHeight() - stackHeight) / 2;
		intervalKnob.setBounds(
			knobArea.getX() + (knobArea.getWidth() - knobDiameter) / 2,
			knobArea.getY() + yOffset,
			knobDiameter, knobDiameter);
		intervalLabel.setBounds(
			knobArea.getX(),
			knobArea.getY() + yOffset + knobDiameter + 2,
			knobArea.getWidth(), labelHeight);
	}

	if (!waveformDisplay)
	{
		if (track != nullptr)
		{
			waveformDisplay = std::make_unique<WaveformDisplay>(audioProcessor, *track);
			waveformDisplay->onLoopPointsChanged = [this](double start, double end)
				{
					if (track)
					{
						if (track->usePages.load())
						{
							auto& currentPage = track->getCurrentPage();
							currentPage.loopStart = start;
							currentPage.loopEnd = end;
							track->syncLegacyProperties();
						}
						else
						{
							track->loopStart = start;
							track->loopEnd = end;
						}
						waveformDisplay->setLoopPoints(start, end);
						if (track->isPlaying.load())
						{
							track->readPosition = 0.0;
						}
					}
				};
			addAndMakeVisible(*waveformDisplay);
			if (track->numSamples > 0)
			{
				waveformDisplay->setAudioData(track->audioBuffer, track->sampleRate);
				waveformDisplay->setLoopPoints(track->loopStart, track->loopEnd);
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
	}

	if (sequencer)
	{
		area.removeFromTop(5);
		sequencer->setBounds(area.removeFromTop(SEQUENCER_HEIGHT));
		sequencer->setVisible(true);
	}
}

void TrackComponent::openDrawingCanvas()
{
	if (drawingWindowPtr != nullptr)
	{
		drawingWindowPtr->toFront(true);
		return;
	}

	auto* canvas = new DrawingCanvas(audioProcessor);

	auto* window = new DrawingWindow("Draw Image - " + trackNameLabel.getText(), canvas);
	drawingWindowPtr = window;
	window->setVisible(true);

	if (track && track->usePages.load())
	{
		const auto& currentPage = track->getCurrentPage();
		if (!currentPage.canvasState.isEmpty())
		{
			auto state = DrawingCanvas::CanvasState::fromXml(currentPage.canvasState);
			canvas->setState(state);
		}
		else if (!currentPage.canvasData.isEmpty())
		{
			canvas->loadFromBase64(currentPage.canvasData);
		}
	}
	else if (track && !track->canvasState.isEmpty())
	{
		auto state = DrawingCanvas::CanvasState::fromXml(track->canvasState);
		canvas->setState(state);
	}
	else if (track && !track->canvasData.isEmpty())
	{
		canvas->loadFromBase64(track->canvasData);
	}

	canvas->setGenerating(canvasIsGenerating);

	canvas->onGenerate = [this, canvas](const juce::String& base64Image)
		{
			if (track)
			{
				auto canvasState = canvas->getState();
				juce::String stateXml = canvasState.toXml();
				if (track->usePages.load())
				{
					auto& currentPage = track->getCurrentPage();
					currentPage.canvasState = stateXml;
					currentPage.canvasData = base64Image;
					currentPage.selectedKeywords = canvasState.selectedKeywords;
					track->syncLegacyProperties();
				}
				else
				{
					track->canvasState = stateXml;
					track->canvasData = base64Image;
					track->selectedKeywords = canvasState.selectedKeywords;
				}
			}
			if (onGenerateWithImage)
			{
				auto keywords = canvas->getState().selectedKeywords;
				onGenerateWithImage(trackId, base64Image, keywords);
			}
		};

	window->onBeforeClose = [this, canvas]()
		{
			if (track)
			{
				auto canvasState = canvas->getState();
				juce::String stateXml = canvasState.toXml();
				if (track->usePages.load())
				{
					auto& currentPage = track->getCurrentPage();
					currentPage.canvasState = stateXml;
					currentPage.canvasData = canvasState.imageBase64;
					currentPage.selectedKeywords = canvasState.selectedKeywords;
					track->syncLegacyProperties();
				}
				else
				{
					track->canvasState = stateXml;
					track->canvasData = canvasState.imageBase64;
					track->selectedKeywords = canvasState.selectedKeywords;
				}
			}
			drawingWindowPtr = nullptr;
		};
}

void TrackComponent::layoutPagesButtons(juce::Rectangle<int> area)
{
	int buttonSize = PAGE_BUTTON_SIZE;
	int spacing = 2;

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
	const char* pageLabels[4] = { "A", "B", "C", "D" };

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

		pageButtons[i].onClick = [this, i]()
			{ onPageSelected(i); };

		pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
		pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight);
		pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);

		pageButtons[i].onMidiLearn = [this, i]()
			{
				if (track && track->slotIndex != -1)
				{
					const char* pageNames[4] = { "PageA", "PageB", "PageC", "PageD" };
					char pageLetter = 'A' + static_cast<char>(i);
					juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[i];
					juce::String description = "Slot " + juce::String(track->slotIndex + 1) + " Page " + juce::String::charToString(pageLetter);

					statusCallback("Learning MIDI for " + description + "...");

					audioProcessor.getMidiLearnManager().startLearning(paramName, &audioProcessor, nullptr, description, &pageButtons[i]);
				}
			};

		pageButtons[i].onMidiRemove = [this, i]()
			{
				if (track && track->slotIndex != -1)
				{
					const char* pageNames[4] = { "PageA", "PageB", "PageC", "PageD" };
					juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[i];

					char pageLetter = 'A' + static_cast<char>(i);
					statusCallback("MIDI mapping removed for Page " + juce::String::charToString(pageLetter));

					audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName);
				}
			};
	}

	addAndMakeVisible(togglePagesButton);
	togglePagesButton.setButtonText(juce::String::fromUTF8("\xE2\x97\xA8"));
	togglePagesButton.setTooltip("Enable multi-page mode (A/B/C/D)");
	togglePagesButton.onClick = [this]()
		{ onTogglePagesMode(); };
}

void TrackComponent::onTogglePagesMode()
{
	if (!track)
		return;

	pagesMode = !pagesMode;

	if (pagesMode)
	{
		if (!track->usePages.load())
		{
			track->migrateToPages();
		}

		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setVisible(true);
		}
		togglePagesButton.setVisible(false);

		pageButtons[track->currentPageIndex].setToggleState(true, juce::dontSendNotification);
		updatePagesDisplay();

		statusCallback("Pages mode enabled - " + juce::String(4) + " slots available");
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setVisible(false);
		}
		togglePagesButton.setVisible(true);

		statusCallback("Pages mode disabled");
	}

	resized();
	repaint();
}

void TrackComponent::onPageSelected(int pageIndex)
{
	if (!track || !pagesMode || pageIndex < 0 || pageIndex >= 4)
		return;

	if (track->currentPageIndex == pageIndex && !track->pageChangePending.load())
	{
		return;
	}

	if (track->pageChangePending.load() && track->pendingPageIndex.load() == pageIndex)
	{
		track->pageChangePending = false;
		track->pendingPageIndex = -1;
		stopTimer();
		updatePagesDisplay();
		statusCallback("Page change cancelled");
		return;
	}

	if (track->slotIndex != -1)
	{
		const char* pageNames[4] = { "PageA", "PageB", "PageC", "PageD" };
		juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + pageNames[pageIndex];

		auto* param = audioProcessor.getParameterTreeState().getParameter(paramName);
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

	DBG("Switching to page " << (char)('A' + pageIndex) << " for track " << track->trackName);

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

	const auto& newPage = track->getCurrentPage();

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

	if (sequencer)
	{
		sequencer->updateSequenceButtonsDisplay();
		sequencer->updateFromTrackData();
	}

	if (waveformDisplay && showWaveformButton.getToggleState())
	{
		if (newPage.numSamples > 0 && newPage.isLoaded.load())
		{
			waveformDisplay->setAudioData(newPage.audioBuffer, newPage.sampleRate);
			waveformDisplay->setLoopPoints(newPage.loopStart, newPage.loopEnd);
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
	if (!track || !pagesMode)
		return;

	int pendingPage = track->pageChangePending.load() ? track->pendingPageIndex.load() : -1;

	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setToggleState(i == track->currentPageIndex, juce::dontSendNotification);

		if (i == pendingPage)
		{
			continue;
		}

		if (track->pages[i].numSamples > 0)
		{
			pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
			pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
			pageButtons[i].setColour(juce::TextButton::buttonColourId,
				i == track->currentPageIndex ? ColourPalette::buttonDangerLight : ColourPalette::backgroundLight);
			pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight.withAlpha(0.4f));
		}
		else if (track->pages[i].isLoading.load())
		{
			pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
			pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
			pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
			pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight);
		}
		else
		{
			pageButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
			pageButtons[i].setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
			pageButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDark);
			pageButtons[i].setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonDangerLight);
		}
	}
}

void TrackComponent::loadPageIfNeeded(int pageIndex)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto& page = track->pages[pageIndex];
	if (page.isLoaded.load() || page.isLoading.load())
		return;

	page.isLoading = true;
	updatePagesDisplay();

	if (!page.audioFilePath.isEmpty())
	{
		juce::File audioFile(page.audioFilePath);
		if (audioFile.existsAsFile())
		{
			juce::Thread::launch([this, pageIndex, audioFile]()
				{ loadPageAudioFile(pageIndex, audioFile); });
			return;
		}
	}

	page.isLoading = false;
	updatePagesDisplay();
}

void TrackComponent::loadPageAudioFile(int pageIndex, const juce::File& audioFile)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto& page = track->pages[pageIndex];

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

		juce::MessageManager::callAsync([this, pageIndex]()
			{
				if (track && track->currentPageIndex == pageIndex) {
					track->syncLegacyProperties();
					updateFromTrackData();
					if (waveformDisplay && showWaveformButton.getToggleState()) {
						refreshWaveformDisplay();
					}
				}
				updatePagesDisplay(); });
	}
	catch (const std::exception& /*e*/)
	{
		page.isLoading = false;

		juce::MessageManager::callAsync([this]()
			{ updatePagesDisplay(); });
	}
}

void TrackComponent::startGeneratingAnimation()
{
	isGenerating = true;

	if (pagesMode)
	{
		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setEnabled(false);
		}
	}
	togglePagesButton.setEnabled(false);

	if (!isTimerRunning())
	{
		startTimer(200);
	}
}

void TrackComponent::stopGeneratingAnimation()
{
	isGenerating = false;

	if (pagesMode)
	{
		for (int i = 0; i < 4; ++i)
		{
			pageButtons[i].setEnabled(true);
		}
	}
	togglePagesButton.setEnabled(true);

	if (!track || !track->pageChangePending.load())
	{
		stopTimer();
	}

	if (waveformDisplay && showWaveformButton.getToggleState() && track)
	{
		if (track->usePages.load())
		{
			const auto& currentPage = track->getCurrentPage();
			if (currentPage.numSamples > 0)
			{
				waveformDisplay->setAudioData(currentPage.audioBuffer, currentPage.sampleRate);
				waveformDisplay->setLoopPoints(currentPage.loopStart, currentPage.loopEnd);
			}
		}
		else
		{
			if (track->numSamples > 0)
			{
				waveformDisplay->setAudioData(track->audioBuffer, track->sampleRate);
				waveformDisplay->setLoopPoints(track->loopStart, track->loopEnd);
			}
		}
	}

	repaint();
}

void TrackComponent::timerCallback()
{
	if (isGenerating)
	{
		blinkState = !blinkState;
		repaint();
	}

	if (track && track->pageChangePending.load())
	{
		pageBlinkState = !pageBlinkState;
		int pendingPage = track->pendingPageIndex.load();
		if (pendingPage >= 0 && pendingPage < 4)
		{
			pageButtons[pendingPage].setColour(
				juce::TextButton::buttonColourId,
				pageBlinkState ? ColourPalette::buttonDanger : ColourPalette::backgroundDeep);
			pageButtons[pendingPage].repaint();
		}
	}

	if (!isGenerating && (!track || !track->pageChangePending.load()))
	{
		stopTimer();
	}
}

void TrackComponent::refreshWaveformDisplay()
{
	if (!waveformDisplay || !track)
		return;

	if (track->usePages.load())
	{
		const auto& currentPage = track->getCurrentPage();

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
	else
	{
		if (track->numSamples > 0)
		{
			waveformDisplay->setAudioData(track->audioBuffer, track->sampleRate);
			waveformDisplay->setLoopPoints(track->loopStart, track->loopEnd);

			if (!track->audioFilePath.isEmpty())
			{
				juce::File audioFile(track->audioFilePath);
				waveformDisplay->setAudioFile(audioFile);
			}
			calculateHostBasedDisplay();
		}
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
	auto* param = audioProcessor.getParameterTreeState().getParameter(paramName);
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

	auto* param = audioProcessor.getParameterTreeState().getParameter(paramName);
	if (param)
	{
		param->addListener(this);
	}
}

void TrackComponent::setupUI()
{

	addAndMakeVisible(trackNumberButton);
	trackNumberButton.setButtonText("");
	trackNumberButton.setTooltip("Select this track");
	trackNumberButton.onClick = [this]()
		{
			if (onSelectTrack)
				onSelectTrack(trackId);
		};

	addAndMakeVisible(trackNameLabel);
	trackNameLabel.setText(track ? track->trackName : "Track", juce::dontSendNotification);
	trackNameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	trackNameLabel.setEditable(true);
	trackNameLabel.onEditorShow = [this]()
		{
			isEditingLabel = true;
			if (auto* editor = trackNameLabel.getCurrentTextEditor())
			{
				editor->selectAll();
			}
		};
	trackNameLabel.onTextChange = [this]()
		{
			if (track)
			{
				track->trackName = trackNameLabel.getText();
				if (onTrackRenamed)
					onTrackRenamed(trackId, trackNameLabel.getText());
			}
		};
	trackNameLabel.onEditorHide = [this]()
		{
			isEditingLabel = false;
		};
	trackNameLabel.toFront(false);

	addAndMakeVisible(infoLabel);
	infoLabel.setText("Empty track - Generate your sample!", juce::dontSendNotification);
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setFont(juce::FontOptions(12.0f));

	addAndMakeVisible(promptPresetSelector);
	promptPresetSelector.setTooltip("Select prompt for this track");
	promptPresetSelector.onChange = [this]()
		{
			onTrackPresetSelected();
		};

	addAndMakeVisible(modelSelector);
	modelSelector.clear();

	auto& models = AiModelDefinitions::getAvailableModels();
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
				if (track->usePages.load())
				{
					track->getCurrentPage().selectedModel = selectedModel;
					track->syncLegacyProperties();
				}
				else
				{
					track->selectedModel = selectedModel;
				}
			}
			updateModelUI();
		};

	setupIconButtons();

	addAndMakeVisible(intervalKnob);
	intervalKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	intervalKnob.setRange(1, 10, 1);
	intervalKnob.setSize(40, 40);
	intervalKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	intervalKnob.setTooltip("Beat repeat duration: 4 Beats, 2 Beats, 1 Beat, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128");
	intervalKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	intervalKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	intervalKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::sliderTrack);
	intervalKnob.onValueChange = [this]()
		{ onIntervalChanged(); };

	addAndMakeVisible(intervalLabel);
	intervalLabel.setJustificationType(juce::Justification::centred);
	intervalLabel.setFont(juce::FontOptions(9.0f));
	intervalLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);

	togglePagesButton.setVisible(false);
	for (int i = 0; i < 4; ++i)
	{
		pageButtons[i].setVisible(true);
	}

	setupPagesUI();
}

void TrackComponent::setupIconButtons()
{
	auto setupToggleButton = [](IconButton& btn)
		{
			btn.setClickingTogglesState(true);
			btn.setHasAccentBar(true);
			btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
			btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
			btn.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
			btn.setColour(juce::TextButton::textColourOnId, ColourPalette::buttonPrimary);
		};

	auto setupActionButton = [](IconButton& btn)
		{
			btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
			btn.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
		};

	addAndMakeVisible(drawButton);
	drawButton.setIconPath(TrackButtonIcons::draw());
	setupActionButton(drawButton);
	drawButton.setTooltip("Draw a visual prompt to guide AI generation (server mode only)");
	drawButton.onClick = [this]()
		{ openDrawingCanvas(); };

	addAndMakeVisible(generateButton);
	generateButton.setIconPath(TrackButtonIcons::generate());
	generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
	generateButton.setColour(juce::TextButton::textColourOffId, ColourPalette::backgroundDeep);
	generateButton.setTooltip("Generate AI audio with current prompt for this track");
	generateButton.onClick = [this]()
		{
			if (onGenerateForTrack)
			{
				if (track)
				{
					if (track->usePages.load())
					{
						auto& currentPage = track->getCurrentPage();
						currentPage.selectedPrompt = promptPresetSelector.getText();
						currentPage.generationBpm = audioProcessor.getGlobalBpm();
						currentPage.generationKey = audioProcessor.getGlobalKey();
						currentPage.generationDuration = audioProcessor.getGlobalDuration();
						track->syncLegacyProperties();
					}
					else
					{
						track->selectedPrompt = promptPresetSelector.getText();
						track->generationBpm = audioProcessor.getGlobalBpm();
						track->generationKey = audioProcessor.getGlobalKey();
						track->generationDuration = audioProcessor.getGlobalDuration();
					}
				}
				onGenerateForTrack(trackId);
				setButtonParameter("Generate");
			}
		};

	addAndMakeVisible(previewButton);
	previewButton.setIconPath(TrackButtonIcons::play());
	previewButton.setIconPathToggled(TrackButtonIcons::stop());
	previewButton.setHasToggledIcon(true);
	previewButton.setLabelText("PREV");
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
				{
					onPreviewTrack(trackId);
				}
			}
		};

	addAndMakeVisible(originalSyncButton);
	originalSyncButton.setIconPath(TrackButtonIcons::sync());
	originalSyncButton.setLabelText("ORIG");
	setupToggleButton(originalSyncButton);
	originalSyncButton.setTooltip("Play original file (bypass time-stretching). Disabled when no original version exists.");
	originalSyncButton.onClick = [this]()
		{ toggleOriginalSync(); };

	addAndMakeVisible(randomRetriggerButton);
	randomRetriggerButton.setIconPath(TrackButtonIcons::repeat());
	setupToggleButton(randomRetriggerButton);
	randomRetriggerButton.setTooltip("Beat repeat - re-trigger current section at interval while ON");
	randomRetriggerButton.onClick = [this]()
		{ onRandomRetriggerToggled(); };

	addAndMakeVisible(randomDurationToggle);
	randomDurationToggle.setIconPath(TrackButtonIcons::random());
	setupToggleButton(randomDurationToggle);
	randomDurationToggle.setTooltip("Auto-randomize repeat interval on each trigger");
	randomDurationToggle.onClick = [this]()
		{
			if (track)
			{
				track->randomRetriggerDurationEnabled = randomDurationToggle.getToggleState();
				updateRandomDurationButtonColor();
				statusCallback("Auto-random duration: " + juce::String(track->randomRetriggerDurationEnabled.load() ? "ON" : "OFF"));
			}
		};
}

void TrackComponent::updateButtonsEnabledState()
{
	bool hasAudio = track && track->numSamples > 0;

	bool hasOriginal = false;
	if (track)
	{
		if (track->usePages.load())
			hasOriginal = track->getCurrentPage().hasOriginalVersion.load();
		else
			hasOriginal = track->hasOriginalVersion.load();
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
	randomRetriggerButton.setToggleState(track->randomRetriggerEnabled.load(),
		juce::dontSendNotification);
}

void TrackComponent::updateRandomDurationButtonColor()
{
	if (!track)
		return randomDurationToggle.setToggleState(track->randomRetriggerDurationEnabled.load(),
			juce::dontSendNotification);
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
			double repeatDuration = audioProcessor.calculateRetriggerInterval(value, hostBpm);
			double repeatDurationSamples = repeatDuration * track->sampleRate;
			track->beatRepeatEndPosition.store(startPosition + repeatDurationSamples);

			double maxSamples = track->numSamples;
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

void TrackComponent::statusCallback(const juce::String& message)
{
	if (onStatusMessage)
	{
		onStatusMessage(message);
	}
	if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
	{
		editor->statusLabel.setText(message, juce::dontSendNotification);
		editor->updateLCD();
	}
}

void TrackComponent::setSliderParameter(juce::String name, juce::Slider& slider)
{
	if (!track || track->slotIndex == -1)
		return;
	if (this == nullptr)
		return;

	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	try
	{
		auto& parameterTreeState = audioProcessor.getParameterTreeState();
		auto* param = parameterTreeState.getParameter(paramName);

		if (param != nullptr)
		{
			float value = static_cast<float>(slider.getValue());
			if (!std::isnan(value) && !std::isinf(value))
			{
				if (name == "RetriggerInterval")
				{
					value = (value - 1.0f) / 9.0f;
				}
				param->setValueNotifyingHost(value);
			}
		}
	}
	catch (...)
	{
		DBG("Exception in setSliderParameter for " << paramName);
	}
}

void TrackComponent::loadPromptPresets()
{
	promptPresetSelector.clear();
	juce::StringArray allPrompts = audioProcessor.getBuiltInPrompts();
	auto customPrompts = audioProcessor.getCustomPrompts();

	for (const auto& customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);
	promptPresets = allPrompts;

	for (int i = 0; i < allPrompts.size(); ++i)
	{
		promptPresetSelector.addItem(allPrompts[i], i + 1);
	}

	if (track && !track->selectedPrompt.isEmpty())
	{
		int index = allPrompts.indexOf(track->selectedPrompt);
		if (index >= 0)
		{
			promptPresetSelector.setSelectedId(index + 1, juce::dontSendNotification);
		}
	}
	else if (allPrompts.size() > 0)
	{
		promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
	}
}

void TrackComponent::updatePromptPresets(const juce::StringArray& presets)
{
	juce::String currentSelection = promptPresetSelector.getText();
	juce::StringArray sortedPresets = presets;
	sortedPresets.sort(true);
	promptPresets = sortedPresets;
	promptPresetSelector.clear();

	for (int i = 0; i < presets.size(); ++i)
	{
		promptPresetSelector.addItem(presets[i], i + 1);
	}

	int index = presets.indexOf(currentSelection);
	if (index >= 0)
	{
		promptPresetSelector.setSelectedId(index + 1, juce::dontSendNotification);
	}
	else if (presets.size() > 0)
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
	DBG("=== toggleOriginalSync START ===");
	DBG("useOriginal: " << (useOriginal ? "YES" : "NO"));

	if (track->usePages.load())
	{
		auto& currentPage = track->getCurrentPage();
		DBG("Page hasOriginalVersion BEFORE: " << (currentPage.hasOriginalVersion.load() ? "YES" : "NO"));

		if (!currentPage.hasOriginalVersion.load())
		{
			DBG("ERROR: No original version - reverting button");
			originalSyncButton.setToggleState(!useOriginal, juce::dontSendNotification);
			originalSyncButton.setEnabled(false);
			return;
		}
		currentPage.useOriginalFile = useOriginal;
		track->syncLegacyProperties();

		DBG("Page hasOriginalVersion AFTER syncLegacyProperties: " << (currentPage.hasOriginalVersion.load() ? "YES" : "NO"));
	}
	else
	{
		if (!track->hasOriginalVersion.load())
		{
			originalSyncButton.setToggleState(false, juce::dontSendNotification);
			originalSyncButton.setEnabled(false);
			DBG("No original version available for track");
			return;
		}
		track->useOriginalFile = useOriginal;
	}

	originalSyncButton.setButtonText(useOriginal ? juce::String::fromUTF8("\xE2\x97\x8F") : juce::String::fromUTF8("\xE2\x97\x8B"));
	originalSyncButton.setEnabled(false);
	DBG("About to call reloadTrackWithVersion...");
	audioProcessor.reloadTrackWithVersion(trackId, useOriginal);
	juce::Timer::callAfterDelay(500, [this]()
		{
			if (track && track->usePages.load()) {
				const auto& currentPage = track->getCurrentPage();
				if (currentPage.hasOriginalVersion.load()) {
					originalSyncButton.setEnabled(true);
					DBG("Button re-enabled after reload");
				}
				else {
					DBG("Button stays disabled - no original version");
				}
			} });
			DBG("=== toggleOriginalSync END ===");
}

void TrackComponent::onTrackPresetSelected()
{
	if (track)
	{
		juce::String newPrompt = promptPresetSelector.getText();
		if (track->usePages.load())
		{
			auto& currentPage = track->getCurrentPage();
			currentPage.selectedPrompt = newPrompt;
			track->syncLegacyProperties();
		}
		else
		{
			track->selectedPrompt = newPrompt;
		}

		if (onTrackPromptChanged)
		{
			onTrackPromptChanged(trackId, newPrompt);
		}
	}
}

void TrackComponent::adjustLoopPointsToTempo()
{
	if (!track || track->numSamples == 0)
		return;

	float effectiveBpm = calculateEffectiveBpm();
	if (effectiveBpm <= 0)
		return;

	int numerator = audioProcessor.getTimeSignatureNumerator();
	double beatDuration = 60.0 / effectiveBpm;
	double barDuration = beatDuration * numerator;
	double originalDuration = track->numSamples / track->sampleRate;
	double stretchRatio = effectiveBpm / track->originalBpm;
	double effectiveDuration = originalDuration / stretchRatio;

	track->loopStart = 0.0;

	int maxWholeBars = (int)(effectiveDuration / barDuration);
	maxWholeBars = juce::jlimit(1, 8, maxWholeBars);

	track->loopEnd = maxWholeBars * barDuration;

	if (track->loopEnd > effectiveDuration)
	{
		maxWholeBars = (std::max)(1, maxWholeBars - 1);
		track->loopEnd = maxWholeBars * barDuration;
	}
}

void TrackComponent::updateTrackInfo()
{
	if (!track)
		return;

	if (!track->prompt.isEmpty())
	{
		float effectiveBpm = calculateEffectiveBpm();
		float originalBpm = track->originalBpm;

		juce::String bpmInfo = "";
		juce::String stretchIndicator = "";

		switch (track->timeStretchMode)
		{
		case 1:
			bpmInfo = " | Original: " + juce::String(originalBpm, 1);
			break;
		case 2:
			stretchIndicator = (effectiveBpm > originalBpm) ? " +" : (effectiveBpm < originalBpm) ? " -"
				: " =";
			bpmInfo = " | BPM: " + juce::String(effectiveBpm, 1) + stretchIndicator;
			break;
		case 3:
			stretchIndicator = " =";
			bpmInfo = " | Sync: " + juce::String(effectiveBpm, 1) + stretchIndicator;
			break;
		case 4:
			stretchIndicator = (track->bpmOffset > 0) ? " +" : (track->bpmOffset < 0) ? " -"
				: "";
			bpmInfo = " | Host+ " + juce::String(track->bpmOffset, 1) + stretchIndicator;
			break;
		}

		infoLabel.setText(track->prompt.substring(0, 30) + "..." + bpmInfo,
			juce::dontSendNotification);
	}
	repaint();
}

void TrackComponent::refreshWaveformIfNeeded()
{
	if (waveformDisplay && showWaveformButton.getToggleState() && track && track->numSamples > 0)
	{
		static int lastNumSamples = 0;
		if (track->numSamples != lastNumSamples)
		{
			refreshWaveformDisplay();
			lastNumSamples = track->numSamples;
		}
	}
}

void TrackComponent::toggleSequencerDisplay()
{
	sequencerVisible = sequencerToggleButton.getToggleState();

	if (sequencerVisible && !sequencer)
	{
		sequencer = std::make_unique<SequencerComponent>(trackId, audioProcessor);
		addAndMakeVisible(*sequencer);
	}

	if (sequencer)
	{
		sequencer->setVisible(sequencerVisible);
	}

	bool waveformVisible = showWaveformButton.getToggleState();
	int newHeight = BASE_HEIGHT;

	if (waveformVisible)
		newHeight += WAVEFORM_HEIGHT;
	if (sequencerVisible)
		newHeight += SEQUENCER_HEIGHT;

	setSize(getWidth(), newHeight);

	if (auto* parentViewport = findParentComponentOfClass<juce::Viewport>())
	{
		if (auto* parentContainer = parentViewport->getViewedComponent())
		{
			int totalHeight = 5;

			for (int i = 0; i < parentContainer->getNumChildComponents(); ++i)
			{
				if (auto* trackComp = dynamic_cast<TrackComponent*>(parentContainer->getChildComponent(i)))
				{
					bool hasWaveform = trackComp->showWaveformButton.getToggleState();
					bool hasSequencer = trackComp->sequencerVisible;

					int trackHeight = BASE_HEIGHT;
					if (hasWaveform)
						trackHeight += WAVEFORM_HEIGHT;
					if (hasSequencer)
						trackHeight += SEQUENCER_HEIGHT;

					trackComp->setSize(trackComp->getWidth(), trackHeight);
					trackComp->setBounds(trackComp->getX(), totalHeight, trackComp->getWidth(), trackHeight);
					totalHeight += trackHeight + 5;
				}
			}
			parentContainer->setSize(parentContainer->getWidth(), totalHeight);
			parentContainer->resized();
		}
	}
	resized();
}

void TrackComponent::updatePromptSelection(const juce::String& promptText)
{
	if (!track)
		return;

	track->selectedPrompt = promptText;

	for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
	{
		if (promptPresetSelector.getItemText(i) == promptText)
		{
			promptPresetSelector.setSelectedItemIndex(i, juce::sendNotification);
			break;
		}
	}

	repaint();
}

void TrackComponent::learn(juce::String param, MidiLearnableBase* component, std::function<void(float)> uiCallback)
{
	if (audioProcessor.getActiveEditor() && track && track->slotIndex != -1)
	{
		juce::String parameterName = "slot" + juce::String(track->slotIndex + 1) + param;
		juce::String description = "Slot " + juce::String(track->slotIndex + 1) + " " + param;
		juce::MessageManager::callAsync([this, description]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
				{
					editor->statusLabel.setText("Learning MIDI for " + description + "...", juce::dontSendNotification);
					editor->updateLCD();
				} });
				audioProcessor.getMidiLearnManager()
					.startLearning(parameterName, &audioProcessor, uiCallback, description, component);
	}
}

void TrackComponent::removeMidiMapping(const juce::String& param)
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

	generateButton.onMidiLearn = [this]()
		{
			learn("Generate", &generateButton);
		};
	generateButton.onMidiRemove = [this]()
		{
			removeMidiMapping("Generate");
		};

	randomRetriggerButton.onMidiLearn = [this]()
		{
			learn("RandomRetrigger", &randomRetriggerButton);
		};

	randomRetriggerButton.onMidiRemove = [this]()
		{
			removeMidiMapping("RandomRetrigger");
			updateRandomRetriggerButtonColor();
		};

	intervalKnob.onMidiLearn = [this]()
		{
			learn("RetriggerInterval", &intervalKnob);
		};
	intervalKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("RetriggerInterval");
		};

	juce::String paramName = "promptSelector_slot" + juce::String(track->slotIndex + 1);
	auto promptCallback = [this](float value)
		{
			juce::MessageManager::callAsync([this, value]()
				{
					int numItems = promptPresetSelector.getNumItems();
					if (numItems > 0) {
						int selectedIndex = (int)(value * (numItems - 1));
						promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
					} });
		};

	audioProcessor.getMidiLearnManager().registerUICallback(paramName, promptCallback);

	promptPresetSelector.onMidiLearn = [this, paramName, promptCallback]()
		{
			if (audioProcessor.getActiveEditor() && track && track->slotIndex != -1)
			{
				juce::String description = "Slot " + juce::String(track->slotIndex + 1) + " Prompt Selector";
				audioProcessor.getMidiLearnManager().startLearning(
					paramName,
					&audioProcessor,
					promptCallback,
					description,
					&promptPresetSelector);
			}
		};

	promptPresetSelector.onMidiRemove = [this, paramName]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName);
		};
}

bool TrackComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	return dragSourceDetails.description.isString() &&
		dragSourceDetails.description.toString().isNotEmpty();
}

void TrackComponent::itemDragEnter(const SourceDetails& /*dragSourceDetails*/)
{
	isDragOver = true;
	repaint();
}

void TrackComponent::itemDragMove(const SourceDetails& /*dragSourceDetails*/)
{
}

void TrackComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	isDragOver = false;
	repaint();
}

void TrackComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
	isDragOver = false;

	juce::String sampleId = dragSourceDetails.description.toString();
	if (sampleId.isNotEmpty() && track)
	{
		audioProcessor.loadSampleFromBank(sampleId, trackId);

		if (auto* sampleBank = audioProcessor.getSampleBank())
		{
			auto* sampleEntry = sampleBank->getSample(sampleId);
			if (sampleEntry && !sampleEntry->originalPrompt.isEmpty())
			{
				for (int i = 0; i < promptPresetSelector.getNumItems(); ++i)
				{
					if (promptPresetSelector.getItemText(i) == sampleEntry->originalPrompt)
					{
						promptPresetSelector.setSelectedItemIndex(i, juce::dontSendNotification);
						track->selectedPrompt = sampleEntry->originalPrompt;
						break;
					}
				}
			}
		}

		if (onStatusMessage)
		{
			onStatusMessage("Sample loaded from bank!");
		}
	}

	repaint();
}

void TrackComponent::setPreviewPlaying(bool playing)
{
	isPreviewPlaying = playing;
	updatePreviewButton();
	repaint();
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

	juce::String currentModel = modelSelector.getText();

	if (currentModel.isEmpty())
		currentModel = track->selectedModel;

	if (currentModel.isEmpty())
		currentModel = AiModelDefinitions::getAvailableModels()[0];

	auto modelColour = AiModelDefinitions::getColourForModel(currentModel);

	trackNumberButton.setColour(juce::TextButton::buttonColourId, modelColour);
	trackNumberButton.setColour(juce::TextButton::textColourOffId,
		modelColour.getBrightness() > 0.6f ? juce::Colours::black : juce::Colours::white);

	generateButton.setColour(juce::TextButton::buttonColourId, modelColour);
	generateButton.setColour(juce::TextButton::textColourOffId,
		modelColour.getBrightness() > 0.6f ? juce::Colours::black : juce::Colours::white);

	repaint();
}
