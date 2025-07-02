/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

#include "TrackComponent.h"
#include "WaveformDisplay.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "PluginEditor.h"
#include "ColourPalette.h"

TrackComponent::TrackComponent(const juce::String& trackId, DjIaVstProcessor& processor)
	: trackId(trackId), track(nullptr), audioProcessor(processor)
{
	setupUI();
	loadPromptPresets();
	addListener("Generate");
	setupMidiLearn();
}

TrackComponent::~TrackComponent()
{
	removeListener("Generate");
	isDestroyed.store(true);
	stopTimer();
	track = nullptr;
}

void TrackComponent::setTrackData(TrackData* trackData)
{
	track = trackData;
	updateFromTrackData();
}

bool TrackComponent::isWaveformVisible() const
{
	return showWaveformButton.getToggleState() && waveformDisplay && waveformDisplay->isVisible();
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

void TrackComponent::setButtonParameter(juce::String name, juce::Button& button)
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
				bool state = button.getToggleState();
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

		bool hasPitchChange = std::abs(track->fineOffset) > 0.001f || std::abs(track->bpmOffset) > 0.001f;

		float displayRatio;

		if (hasPitchChange)
		{
			displayRatio = static_cast<float>(track->cachedPlaybackRatio.load());
		}
		else
		{
			displayRatio = track->speed.load();
		}

		waveformDisplay->setSpeedRatio(displayRatio);

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
						track->loopStart = start;
						track->loopEnd = end;
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
	if (!track)
		return;

	showWaveformButton.setToggleState(track->showWaveform, juce::dontSendNotification);
	sequencerToggleButton.setToggleState(track->showSequencer, juce::dontSendNotification);
	originalSyncButton.setToggleState(track->useOriginalFile.load(), juce::dontSendNotification);
	originalSyncButton.setEnabled(track->hasOriginalVersion.load());
	bool useOriginal = track->useOriginalFile.load();
	originalSyncButton.setButtonText(useOriginal ?
		juce::String::fromUTF8("\xE2\x97\x8F") :
		juce::String::fromUTF8("\xE2\x97\x8B"));

	trackNameLabel.setText(track->trackName, juce::dontSendNotification);
	juce::String noteName = juce::MidiMessage::getMidiNoteName(track->midiNote, true, true, 3);
	trackNumberLabel.setText(noteName, juce::dontSendNotification);

	bpmOffsetSlider.setValue(track->bpmOffset, juce::dontSendNotification);
	trackNumberLabel.setColour(juce::Label::backgroundColourId, ColourPalette::getTrackColour(track->slotIndex));
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

	if (isGenerating && blinkState)
	{
		bgColour = ColourPalette::playArmed.withAlpha(0.3f);
	}
	else if (isSelected)
	{
		bgColour = ColourPalette::trackSelected.withAlpha(0.1f);
	}
	else
	{
		bgColour = ColourPalette::backgroundDark.withAlpha(0.8f);
	}

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

	juce::Colour borderColour = isGenerating ? ColourPalette::playArmed : (isSelected ? ColourPalette::trackSelected : ColourPalette::backgroundLight);

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f,
		isGenerating ? 3.0f : (isSelected ? 2.0f : 1.0f));

	if (isSelected)
	{
		g.setColour(borderColour.withAlpha(0.3f));
		g.drawRoundedRectangle(bounds.toFloat().expanded(1), 8.0f, 1.0f);
	}
}

void TrackComponent::resized()
{
	auto area = getLocalBounds().reduced(6);

	auto trackNumberArea = area.removeFromLeft(40);
	trackNumberLabel.setBounds(trackNumberArea);
	area.removeFromLeft(5);

	auto headerArea = area.removeFromTop(30);

	selectButton.setBounds(headerArea.removeFromLeft(35));
	headerArea.removeFromLeft(5);
	trackNameLabel.setBounds(headerArea.removeFromLeft(55));
	promptPresetSelector.setBounds(headerArea.removeFromLeft(150).reduced(2));
	headerArea.removeFromLeft(5);
	infoLabel.setBounds(headerArea.removeFromLeft(150));

	headerArea.removeFromLeft(10);

	deleteButton.setBounds(headerArea.removeFromRight(35));
	headerArea.removeFromRight(5);
	generateButton.setBounds(headerArea.removeFromRight(35));
	headerArea.removeFromRight(5);
	originalSyncButton.setBounds(headerArea.removeFromRight(35));
	headerArea.removeFromRight(5);
	previewButton.setBounds(headerArea.removeFromRight(35));
	headerArea.removeFromRight(5);
	showWaveformButton.setBounds(headerArea.removeFromRight(35));
	headerArea.removeFromRight(5);
	sequencerToggleButton.setBounds(headerArea.removeFromRight(35));

	if (waveformDisplay && showWaveformButton.getToggleState())
	{
		area.removeFromTop(10);
		waveformDisplay->setBounds(area.removeFromTop(WAVEFORM_HEIGHT));
		waveformDisplay->setVisible(true);
	}
	else if (waveformDisplay)
	{
		waveformDisplay->setVisible(false);
	}

	if (sequencer && sequencerVisible && sequencerToggleButton.getToggleState())
	{
		area.removeFromTop(5);
		sequencer->setBounds(area.removeFromTop(SEQUENCER_HEIGHT));
		sequencer->setVisible(true);
	}
	else if (sequencer)
	{
		sequencer->setVisible(false);
	}
}

void TrackComponent::startGeneratingAnimation()
{
	isGenerating = true;
	startTimer(200);
}

void TrackComponent::stopGeneratingAnimation()
{
	isGenerating = false;
	stopTimer();
	if (waveformDisplay && showWaveformButton.getToggleState() && track && track->numSamples > 0)
	{
		waveformDisplay->setAudioData(track->audioBuffer, track->sampleRate);
		waveformDisplay->setLoopPoints(track->loopStart, track->loopEnd);
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
}

void TrackComponent::refreshWaveformDisplay()
{
	if (waveformDisplay && track && track->numSamples > 0)
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
	addAndMakeVisible(selectButton);
	selectButton.setButtonText(juce::String::fromUTF8("\xE2\x97\x89"));
	selectButton.setTooltip("Select this track");
	selectButton.onClick = [this]()
		{
			if (onSelectTrack)
				onSelectTrack(trackId);
		};

	addAndMakeVisible(deleteButton);
	deleteButton.setButtonText(juce::String::fromUTF8("\xE2\x9C\x95"));
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.setTooltip("Delete this track");
	deleteButton.onClick = [this]()
		{
			if (onDeleteTrack)
				onDeleteTrack(trackId);
		};

	addAndMakeVisible(generateButton);
	generateButton.setButtonText(juce::String::fromUTF8("\xE2\x9C\x93"));
	generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	generateButton.setTooltip("Generate new sample for this track");
	generateButton.onClick = [this]()
		{
			if (onGenerateForTrack)
			{
				if (track)
				{
					track->selectedPrompt = promptPresetSelector.getText();
					track->generationBpm = audioProcessor.getGlobalBpm();
					track->generationKey = audioProcessor.getGlobalKey();
					track->generationDuration = audioProcessor.getGlobalDuration();

					if (audioProcessor.isGlobalStemEnabled("drums"))
						track->preferredStems.push_back("drums");
					if (audioProcessor.isGlobalStemEnabled("bass"))
						track->preferredStems.push_back("bass");
					if (audioProcessor.isGlobalStemEnabled("other"))
						track->preferredStems.push_back("other");
					if (audioProcessor.isGlobalStemEnabled("vocals"))
						track->preferredStems.push_back("vocals");
					if (audioProcessor.isGlobalStemEnabled("guitar"))
						track->preferredStems.push_back("guitar");
					if (audioProcessor.isGlobalStemEnabled("piano"))
						track->preferredStems.push_back("piano");
				}
				onGenerateForTrack(trackId);
				setButtonParameter("Generate", generateButton);
			}
		};

	addAndMakeVisible(sequencerToggleButton);
	sequencerToggleButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xA6"));
	sequencerToggleButton.setClickingTogglesState(true);
	sequencerToggleButton.setTooltip("Show/hide step sequencer");
	sequencerToggleButton.onClick = [this]()
		{
			track->showSequencer = sequencerToggleButton.getToggleState();
			toggleSequencerDisplay();
		};

	addAndMakeVisible(originalSyncButton);
	originalSyncButton.setButtonText(juce::String::fromUTF8("\xE2\x97\x8F"));
	originalSyncButton.setClickingTogglesState(true);
	originalSyncButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
	originalSyncButton.setTooltip("Toggle between original and time-stretched version");
	originalSyncButton.onClick = [this]() {
		toggleOriginalSync();
		};

	addAndMakeVisible(infoLabel);
	infoLabel.setText("Empty track - Generate your sample!", juce::dontSendNotification);
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setFont(juce::FontOptions(12.0f));

	addAndMakeVisible(showWaveformButton);
	showWaveformButton.setButtonText(juce::String::fromUTF8("\xE3\x80\x9C"));
	showWaveformButton.setTooltip("Show/hide waveform display");
	showWaveformButton.setClickingTogglesState(true);
	showWaveformButton.onClick = [this]()
		{
			if (track)
			{
				track->showWaveform = showWaveformButton.getToggleState();
				toggleWaveformDisplay();
			}
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

	addAndMakeVisible(trackNumberLabel);
	trackNumberLabel.setJustificationType(juce::Justification::centred);
	trackNumberLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
	trackNumberLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);

	addAndMakeVisible(previewButton);
	previewButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	previewButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
	previewButton.setTooltip("Preview sample (independent of ARM/STOP state)");
	previewButton.onClick = [this]()
		{
			if (track && onPreviewTrack)
				onPreviewTrack(trackId);
		};

	addAndMakeVisible(promptPresetSelector);
	promptPresetSelector.setTooltip("Select prompt for this track");
	promptPresetSelector.onChange = [this]()
		{
			onTrackPresetSelected();
		};
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
	promptPresets = presets;

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
	if (!track) return;

	bool useOriginal = originalSyncButton.getToggleState();
	track->useOriginalFile = useOriginal;
	originalSyncButton.setButtonText(useOriginal ?
		juce::String::fromUTF8("\xE2\x97\x8F") :
		juce::String::fromUTF8("\xE2\x97\x8B"));

	if (track->hasOriginalVersion.load())
	{
		audioProcessor.reloadTrackWithVersion(trackId, useOriginal);
	}
}

void TrackComponent::onTrackPresetSelected()
{
	if (track)
	{
		track->selectedPrompt = promptPresetSelector.getText();
		if (onTrackPromptChanged)
		{
			onTrackPromptChanged(trackId, track->selectedPrompt);
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
		maxWholeBars = std::max(1, maxWholeBars - 1);
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

void TrackComponent::learn(juce::String param, std::function<void(float)> uiCallback)
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
				} });
				audioProcessor.getMidiLearnManager()
					.startLearning(parameterName, &audioProcessor, uiCallback, description);
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
	generateButton.onMidiLearn = [this]()
		{
			learn("Generate");
		};
	generateButton.onMidiRemove = [this]()
		{
			removeMidiMapping("Generate");
		};
}