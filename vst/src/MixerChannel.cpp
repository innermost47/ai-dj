#include "JuceHeader.h"
#include "MixerChannel.h"
#include <string>
#include "PluginEditor.h"
#include "ColourPalette.h"

MixerChannel::MixerChannel(const juce::String& trackId, DjIaVstProcessor& processor, TrackData* trackData)
	: trackId(trackId), audioProcessor(processor), track(nullptr)
{
	setupUI();
	setTrackData(trackData);
	addEventListeners();
	updateFromTrackData();
	setupMidiLearn();
}

void MixerChannel::cleanup()
{
	isDestroyed.store(true);

	isGenerating = false;
	isBlinking = false;
	blinkState = false;
	stopBlinkState = false;

	volumeSlider.onValueChange = nullptr;
	pitchKnob.onValueChange = nullptr;
	fineKnob.onValueChange = nullptr;
	panKnob.onValueChange = nullptr;

	playButton.onClick = nullptr;
	stopButton.onClick = nullptr;
	muteButton.onClick = nullptr;
	soloButton.onClick = nullptr;

	playButton.onMidiLearn = nullptr;
	muteButton.onMidiLearn = nullptr;
	soloButton.onMidiLearn = nullptr;
	volumeSlider.onMidiLearn = nullptr;
	pitchKnob.onMidiLearn = nullptr;
	fineKnob.onMidiLearn = nullptr;
	panKnob.onMidiLearn = nullptr;

	playButton.onMidiRemove = nullptr;
	muteButton.onMidiRemove = nullptr;
	soloButton.onMidiRemove = nullptr;
	volumeSlider.onMidiRemove = nullptr;
	pitchKnob.onMidiRemove = nullptr;
	fineKnob.onMidiRemove = nullptr;
	panKnob.onMidiRemove = nullptr;

	stopTimer();

	try
	{
		if (track && track->slotIndex != -1)
		{
			removeListener("Volume");
			removeListener("Play");
			removeListener("Stop");
			removeListener("Mute");
			removeListener("Solo");
			removeListener("Pitch");
			removeListener("Fine");
			removeListener("Pan");
		}
		else
		{
			auto& allParams = audioProcessor.AudioProcessor::getParameters();
			for (int i = 0; i < allParams.size(); ++i)
			{
				auto* param = allParams[i];
				if (param)
				{
					param->removeListener(this);
				}
			}
		}
	}
	catch (...)
	{
		DBG("Exception during listener cleanup in destructor");
	}

	track = nullptr;
}

MixerChannel::~MixerChannel()
{
	cleanup();
}

void MixerChannel::setTrackData(TrackData* trackData)
{
	track = trackData;
	if (track)
	{
		juce::WeakReference<MixerChannel> weakThis(this);
		track->onPlayStateChanged = [weakThis](bool /*isPlaying*/)
			{
				if (weakThis != nullptr)
				{
					juce::MessageManager::callAsync([weakThis]()
						{
							if (weakThis != nullptr && !weakThis->isUpdatingButtons) {
								weakThis->updateButtonColors();
							} });
				}
			};
		track->onArmedStateChanged = [weakThis](bool /*isArmed*/)
			{
				if (weakThis != nullptr)
				{
					juce::MessageManager::callAsync([weakThis]()
						{
							if (weakThis != nullptr) {
								weakThis->isBlinking = true;
								weakThis->startTimer(300);
							} });
				}
			};

		track->onArmedToStopStateChanged = [weakThis](bool /*isArmedToStop*/)
			{
				if (weakThis != nullptr)
				{
					juce::MessageManager::callAsync([weakThis]()
						{
							if (weakThis != nullptr) {
								bool allStepsAreFalse = true;
								for (const auto& measure : weakThis->track->sequencerData.steps) {
									for (bool step : measure) {
										if (step) {
											allStepsAreFalse = false;
											break;
										}
									}
									if (!allStepsAreFalse) break;
								}
								if (allStepsAreFalse) {
									weakThis->stopTrackImmediatly();
								}
								else {
									weakThis->isBlinking = true;
									weakThis->startTimer(300);
								}
							} });
				}
			};
	}
}

void MixerChannel::removeListener(juce::String name)
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

void MixerChannel::addListener(juce::String name)
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

void MixerChannel::parameterValueChanged(int parameterIndex, float newValue)
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

void MixerChannel::updateUIFromParameter(const juce::String& paramName,
	const juce::String& slotPrefix,
	float newValue)
{
	if (isDestroyed.load())
		return;

	if (paramName == slotPrefix + " Volume")
	{
		if (!volumeSlider.isMouseButtonDown())
			volumeSlider.setValue(newValue, juce::dontSendNotification);
	}
	else if (paramName == slotPrefix + " Pan")
	{
		if (!panKnob.isMouseButtonDown())
		{
			float denormalizedPan = newValue * 2.0f - 1.0f;
			panKnob.setValue(denormalizedPan, juce::dontSendNotification);
		}
	}
	else if (paramName == slotPrefix + " Pitch")
	{
		if (!pitchKnob.isMouseButtonDown())
		{
			float denormalizedPitch = newValue * 24.0f - 12.0f;
			pitchKnob.setValue(denormalizedPitch, juce::dontSendNotification);
		}
	}
	else if (paramName == slotPrefix + " Fine")
	{
		if (!fineKnob.isMouseButtonDown())
		{
			float denormalizedFine = newValue * 100.0f - 50.0f;
			fineKnob.setValue(denormalizedFine, juce::dontSendNotification);
		}
	}
	else if (paramName == slotPrefix + " Mute")
	{
		bool isMuted = newValue > 0.5f;
		muteButton.setToggleState(isMuted, juce::dontSendNotification);
	}
	else if (paramName == slotPrefix + " Solo")
	{
		bool isSolo = newValue > 0.5f;
		soloButton.setToggleState(isSolo, juce::dontSendNotification);
	}
	else if (paramName == slotPrefix + " Play")
	{
		if (newValue < 0.5 && !track->isCurrentlyPlaying.load())
		{
			playButton.setToggleState(false, juce::dontSendNotification);
			playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
			playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
			stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
		}
		else if (newValue > 0.5 && !track->isCurrentlyPlaying.load())
		{
			bool allStepsAreFalse = true;
			for (const auto& measure : track->sequencerData.steps)
			{
				for (bool step : measure)
				{
					if (step)
					{
						allStepsAreFalse = false;
						break;
					}
				}
				if (!allStepsAreFalse)
					break;
			}
			if (allStepsAreFalse)
			{
				track->isArmedToStop = false;
				track->pendingAction = TrackData::PendingAction::None;
			}
			playButton.setToggleState(true, juce::dontSendNotification);
			playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
			playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playArmed);
			stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::stopActive);
		}
	}
}

void MixerChannel::parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/)
{
}

void MixerChannel::setSliderParameter(juce::String name, juce::Slider& slider)
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
				if (name == "Pitch")
				{
					value = (value + 12.0f) / 24.0f;
				}
				else if (name == "Pan")
				{
					value = (value + 1.0f) / 2.0f;
				}
				else if (name == "Fine")
				{
					value = (value + 50.0f) / 100.0f;
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

void MixerChannel::setButtonParameter(juce::String name, juce::Button& button)
{
	if (!track || track->slotIndex == -1 || track->numSamples <= 0)
		return;
	if (this == nullptr)
		return;
	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	try
	{
		auto* param = audioProcessor.getParameters().getParameter(paramName);
		if (param != nullptr)
		{
			bool state = button.getToggleState();
			param->setValueNotifyingHost(state ? 1.0f : 0.0f);
			updateButtonColors();
		}
	}
	catch (...)
	{
		DBG("Exception in setButtonParameter for " << paramName);
	}
}

void MixerChannel::addEventListeners()
{
	volumeSlider.onValueChange = [this]()
		{
			setSliderParameter("Volume", volumeSlider);
		};
	pitchKnob.onValueChange = [this]()
		{
			setSliderParameter("Pitch", pitchKnob);
		};
	fineKnob.onValueChange = [this]()
		{
			setSliderParameter("Fine", fineKnob);
		};
	panKnob.onValueChange = [this]()
		{
			setSliderParameter("Pan", panKnob);
		};
	playButton.onClick = [this]()
		{
			if (track && track->numSamples > 0)
			{
				bool allStepsAreFalse = true;
				for (const auto& measure : track->sequencerData.steps)
				{
					for (bool step : measure)
					{
						if (step)
						{
							allStepsAreFalse = false;
							break;
						}
					}
					if (!allStepsAreFalse)
						break;
				}
				if (!track->isCurrentlyPlaying.load())
				{
					bool shouldArm = playButton.getToggleState();
					if (shouldArm)
					{
						track->isArmed = true;
					}
					else
					{
						track->pendingAction = TrackData::PendingAction::None;
						track->isArmed = false;
					}
				}
				else if (track->isCurrentlyPlaying.load() && !allStepsAreFalse)
				{
					track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
					track->isArmed = false;
					track->isArmedToStop = true;
					playButton.setToggleState(false, juce::dontSendNotification);
					isBlinking = true;
					startTimer(300);
				}
				else if (allStepsAreFalse)
				{
					stopTrackImmediatly();
					return;
				}
				setButtonParameter("Play", playButton);
			}
		};

	stopButton.onClick = [this]()
		{
			if (track && track->numSamples > 0)
			{
				bool allStepsAreFalse = true;
				for (const auto& measure : track->sequencerData.steps)
				{
					for (bool step : measure)
					{
						if (step)
						{
							allStepsAreFalse = false;
							break;
						}
					}
					if (!allStepsAreFalse)
						break;
				}
				if (track->isCurrentlyPlaying.load() && !track->isArmedToStop.load() && !allStepsAreFalse)
				{
					track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
					track->isArmed = false;
					track->isArmedToStop = true;
					playButton.setToggleState(false, juce::dontSendNotification);
					isBlinking = true;
					startTimer(300);
				}
				else if (!track->isCurrentlyPlaying.load() || allStepsAreFalse)
				{
					stopTrackImmediatly();
					return;
				}
				setButtonParameter("Stop", stopButton);
			}
		};
	muteButton.onClick = [this]()
		{
			if (track)
			{
				track->isMuted = muteButton.getToggleState();
			}
			setButtonParameter("Mute", muteButton);
		};
	soloButton.onClick = [this]()
		{
			if (track)
			{
				bool newSoloState = soloButton.getToggleState();
				track->isSolo = newSoloState;
			}
			setButtonParameter("Solo", soloButton);
		};
	pitchKnob.setDoubleClickReturnValue(true, 0.0);
	fineKnob.setDoubleClickReturnValue(true, 0.0);
	panKnob.setDoubleClickReturnValue(true, 0.0);
	volumeSlider.setDoubleClickReturnValue(true, 0.8);

	addListener("Volume");
	addListener("Play");
	addListener("Stop");
	addListener("Mute");
	addListener("Solo");
	addListener("Pitch");
	addListener("Fine");
	addListener("Pan");
}

void MixerChannel::stopTrackImmediatly()
{
	track->pendingAction = TrackData::PendingAction::None;
	track->isArmed = false;
	track->isArmedToStop = false;
	track->isPlaying = false;
	track->isCurrentlyPlaying = false;
	playButton.setToggleState(false, juce::dontSendNotification);
	playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
	stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
}

void MixerChannel::timerCallback()
{
	bool shouldContinueTimer = false;

	if (isGenerating)
	{
		blinkState = !blinkState;
		repaint();
		shouldContinueTimer = true;
	}

	if (isBlinking && track && track->isArmedToStop)
	{
		stopBlinkState = !stopBlinkState;
		stopButton.setColour(juce::TextButton::buttonColourId,
			stopBlinkState ? ColourPalette::buttonDangerLight : ColourPalette::buttonDangerDark);
		shouldContinueTimer = true;
	}
	else if (isBlinking)
	{
		isBlinking = false;
		updateButtonColors();
	}

	if (!shouldContinueTimer)
	{
		stopTimer();
	}
}

void MixerChannel::updateVUMeters()
{
	if (isDestroyed.load())
		return;

	updateVUMeter();

	juce::MessageManager::callAsync([this]()
		{
			if (!isDestroyed.load()) {
				repaint();
			} });
}

void MixerChannel::updateFromTrackData()
{
	if (!track || track->slotIndex == -1)
		return;

	trackNameLabel.setText(track->trackName, juce::dontSendNotification);
	auto& params = audioProcessor.getParameterTreeState();
	juce::String slotPrefix = "slot" + juce::String(track->slotIndex + 1);

	if (auto* volumeParam = params.getParameter(slotPrefix + "Volume"))
	{
		volumeSlider.setValue(volumeParam->getValue(), juce::dontSendNotification);
	}

	if (auto* pitchParam = params.getParameter(slotPrefix + "Pitch"))
	{
		float normalizedPitch = pitchParam->getValue();
		float denormalizedPitch = normalizedPitch * 24.0f - 12.0f;
		pitchKnob.setValue(denormalizedPitch, juce::dontSendNotification);
	}

	if (auto* fineParam = params.getParameter(slotPrefix + "Fine"))
	{
		float normalizedFine = fineParam->getValue();
		float denormalizedFine = normalizedFine * 100.0f - 50.0f;
		fineKnob.setValue(denormalizedFine, juce::dontSendNotification);
	}

	if (auto* panParam = params.getParameter(slotPrefix + "Pan"))
	{
		float normalizedPan = panParam->getValue();
		float denormalizedPan = normalizedPan * 2.0f - 1.0f;
		panKnob.setValue(denormalizedPan, juce::dontSendNotification);
	}

	muteButton.setToggleState(track->isMuted.load(), juce::dontSendNotification);
	soloButton.setToggleState(track->isSolo.load(), juce::dontSendNotification);

	updateButtonColors();
}

void MixerChannel::paint(juce::Graphics& g)
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
		bgColour = ColourPalette::backgroundDark;
	}

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

	juce::Colour borderColour;
	float borderWidth;

	if (isGenerating)
	{
		borderColour = ColourPalette::playArmed;
		borderWidth = 3.0f;
	}
	else if (isSelected)
	{
		borderColour = ColourPalette::trackSelected;
		borderWidth = 2.0f;
	}
	else
	{
		borderColour = ColourPalette::sliderTrack;
		borderWidth = 1.0f;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, borderWidth);

	if (isSelected && !isGenerating)
	{
		g.setColour(borderColour.withAlpha(0.3f));
		g.drawRoundedRectangle(bounds.toFloat().reduced(1), 10.0f, 1.0f);
	}

	drawVUMeter(g, bounds);
}

void MixerChannel::drawVUMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
	auto vuArea = juce::Rectangle<float>(
		static_cast<float>(bounds.getWidth() - 10),
		110.0f,
		6.0f,
		static_cast<float>(bounds.getHeight() - 120));
	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(vuArea, 2.0f);
	g.setColour(ColourPalette::backgroundLight);
	g.drawRoundedRectangle(vuArea, 2.0f, 0.5f);

	if (!track)
		return;

	float currentLevel = getCurrentAudioLevel();
	float peakLevel = getPeakLevel();
	int numSegments = 20;
	float segmentHeight = (vuArea.getHeight() - 4) / numSegments;

	for (int i = 0; i < numSegments; ++i)
	{
		fillMeters(vuArea, i, segmentHeight, numSegments, currentLevel, g);
	}

	if (peakLevel > 0.0f)
	{
		int peakSegment = (int)(peakLevel * numSegments);
		if (peakSegment < numSegments)
		{
			float peakY = vuArea.getBottom() - 2 - (peakSegment + 1) * segmentHeight;
			juce::Rectangle<float> peakRect(
				vuArea.getX() + 1, peakY, vuArea.getWidth() - 2, 2);
			g.setColour(ColourPalette::vuPeak);
			g.fillRect(peakRect);
		}
	}
	if (peakLevel >= 0.95f)
	{
		auto clipRect = juce::Rectangle<float>(vuArea.getX(), vuArea.getY() - 8, vuArea.getWidth(), 4);
		g.setColour(ColourPalette::vuClipping);
		g.fillRoundedRectangle(clipRect, 2.0f);
	}
}

void MixerChannel::fillMeters(juce::Rectangle<float>& vuArea, int i, float segmentHeight, int numSegments, float currentLevel, juce::Graphics& g)
{
	float segmentY = vuArea.getBottom() - 2 - (i + 1) * segmentHeight;
	float segmentLevel = (float)i / numSegments;

	juce::Rectangle<float> segmentRect(
		vuArea.getX() + 1, segmentY, vuArea.getWidth() - 2, segmentHeight - 1);

	juce::Colour segmentColour;
	if (segmentLevel < 0.7f)
		segmentColour = ColourPalette::vuGreen;
	else if (segmentLevel < 0.9f)
		segmentColour = ColourPalette::vuOrange;
	else
		segmentColour = ColourPalette::vuRed;

	if (currentLevel >= segmentLevel)
	{
		g.setColour(segmentColour);
		g.fillRoundedRectangle(segmentRect, 1.0f);
	}
	else
	{
		g.setColour(segmentColour.withAlpha(0.1f));
		g.fillRoundedRectangle(segmentRect, 1.0f);
	}
}

void MixerChannel::resized()
{
	auto area = getLocalBounds().reduced(4);
	int width = area.getWidth();

	trackNameLabel.setBounds(area.removeFromTop(20));
	area.removeFromTop(5);

	auto transportArea = area.removeFromTop(60);
	auto topRow = transportArea.removeFromTop(30);
	auto bottomRow = transportArea.removeFromTop(30);

	playButton.setBounds(topRow.removeFromLeft(width / 2 - 2).reduced(2));
	stopButton.setBounds(topRow.removeFromLeft(width / 2 - 2).reduced(2));
	muteButton.setBounds(bottomRow.removeFromLeft(width / 2 - 2).reduced(2));
	soloButton.setBounds(bottomRow.removeFromLeft(width / 2 - 2).reduced(2));

	area.removeFromTop(5);

	auto volumeArea = area.removeFromTop(320);
	volumeSlider.setBounds(volumeArea.reduced(width / 4, 0));

	area.removeFromTop(5);

	auto knobsArea = area.removeFromTop(170);

	auto pitchArea = knobsArea.removeFromTop(50);
	pitchLabel.setBounds(pitchArea.removeFromTop(12));
	pitchKnob.setBounds(pitchArea.reduced(2));

	auto fineArea = knobsArea.removeFromTop(50);
	fineLabel.setBounds(fineArea.removeFromTop(12));
	fineKnob.setBounds(fineArea.reduced(2));

	area.removeFromTop(5);
	auto panArea = knobsArea.removeFromTop(50);
	panLabel.setBounds(panArea.removeFromTop(12));
	panKnob.setBounds(panArea.reduced(2));
}

void MixerChannel::updateVUMeter()
{
	if (!track || !track->isPlaying.load())
	{
		currentAudioLevel *= 0.95f;
		if (peakHoldTimer > 0)
		{
			peakHoldTimer--;
			if (peakHoldTimer == 0)
				peakHold *= 0.9f;
		}
		return;
	}

	float instantLevel = calculateInstantLevel();

	levelHistory.push_back(instantLevel);
	if (levelHistory.size() > 5)
		levelHistory.erase(levelHistory.begin());

	float smoothedLevel = 0.0f;
	for (float level : levelHistory)
		smoothedLevel += level;
	smoothedLevel /= levelHistory.size();

	if (smoothedLevel > currentAudioLevel)
	{
		currentAudioLevel = smoothedLevel;
	}
	else
	{
		currentAudioLevel = currentAudioLevel * 0.85f + smoothedLevel * 0.15f;
	}

	if (currentAudioLevel > peakHold)
	{
		peakHold = currentAudioLevel;
		peakHoldTimer = 30;
	}
}

float MixerChannel::calculateInstantLevel()
{
	if (!track || track->numSamples == 0)
		return 0.0f;

	double readPos = track->readPosition.load();
	int sampleIndex = (int)(readPos);

	if (sampleIndex >= 0 && sampleIndex < track->numSamples)
	{
		float level = 0.0f;
		int samples = std::min(32, track->numSamples - sampleIndex);

		for (int i = 0; i < samples; ++i)
		{
			for (int ch = 0; ch < track->audioBuffer.getNumChannels(); ++ch)
			{
				float sample = track->audioBuffer.getSample(ch, sampleIndex + i);
				level += std::abs(sample);
			}
		}

		level /= (samples * track->audioBuffer.getNumChannels());
		level *= track->volume.load();

		return juce::jlimit(0.0f, 1.0f, level * 3.0f);
	}

	return 0.0f;
}

void MixerChannel::setSelected(bool selected)
{
	isSelected = selected;
	repaint();
}

void MixerChannel::setCurrentLevel(float level)
{
	currentAudioLevel = level;
}

void MixerChannel::setupUI()
{
	addAndMakeVisible(trackNameLabel);
	trackNameLabel.setText("Track", juce::dontSendNotification);
	trackNameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	trackNameLabel.setJustificationType(juce::Justification::centred);
	trackNameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));

	addAndMakeVisible(playButton);
	playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	playButton.setClickingTogglesState(true);

	addAndMakeVisible(stopButton);
	stopButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xA0"));
	stopButton.setClickingTogglesState(false);

	addAndMakeVisible(muteButton);
	muteButton.setButtonText(juce::String::fromUTF8("\xE2\x9C\x95"));
	muteButton.setClickingTogglesState(true);

	addAndMakeVisible(soloButton);
	soloButton.setButtonText(juce::String::fromUTF8("\xE2\x97\x8F"));
	soloButton.setClickingTogglesState(true);

	addAndMakeVisible(volumeSlider);
	volumeSlider.setRange(0.0, 1.0, 0.01);
	volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
	volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	volumeSlider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
	volumeSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	volumeSlider.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(pitchKnob);
	pitchKnob.setRange(-12.0, 12.0, 0.01);
	pitchKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	pitchKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	pitchKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	pitchKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	pitchKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(pitchLabel);
	pitchLabel.setText("PITCH", juce::dontSendNotification);
	pitchLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	pitchLabel.setJustificationType(juce::Justification::centred);
	pitchLabel.setFont(juce::FontOptions(9.0f));

	addAndMakeVisible(fineKnob);
	fineKnob.setRange(-50.0, 50.0, 1.0);
	fineKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	fineKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	fineKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	fineKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	fineKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(fineLabel);
	fineLabel.setText("FINE", juce::dontSendNotification);
	fineLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	fineLabel.setJustificationType(juce::Justification::centred);
	fineLabel.setFont(juce::FontOptions(9.0f));

	addAndMakeVisible(panKnob);
	panKnob.setRange(-1.0, 1.0, 0.01);
	panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	panKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	panKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	panKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(panLabel);
	panLabel.setText("PAN", juce::dontSendNotification);
	panLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	panLabel.setJustificationType(juce::Justification::centred);
	panLabel.setFont(juce::FontOptions(9.0f));

	playButton.setTooltip("Arm/disarm track for playback");
	stopButton.setTooltip("Stop track playback");
	muteButton.setTooltip("Mute this track");
	soloButton.setTooltip("Solo this track");
	volumeSlider.setTooltip("Track volume level");
	pitchKnob.setTooltip("Pitch adjustment (-12 to +12 semitones)");
	fineKnob.setTooltip("Fine pitch adjustment (-50 to +50 cents)");
	panKnob.setTooltip("Pan position (left/right balance)");
}

void MixerChannel::updateButtonColors()
{
	if (!track)
	{
		return;
	}

	bool isArmed = track->isArmed.load();
	bool isPlaying = track->isCurrentlyPlaying.load();
	bool isMuted = track->isMuted.load();
	bool isSolo = track->isSolo.load();

	playButton.setToggleState(isArmed || isPlaying, juce::dontSendNotification);

	if (isPlaying)
	{
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playActive);
		playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	}
	else if (isArmed)
	{
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playArmed);
		playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	}
	else
	{
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
		playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	}

	muteButton.setToggleState(isMuted, juce::dontSendNotification);
	muteButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::muteActive);
	muteButton.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
	muteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
	muteButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);

	soloButton.setToggleState(isSolo, juce::dontSendNotification);
	soloButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::soloActive);
	soloButton.setColour(juce::TextButton::textColourOnId, ColourPalette::soloText);
	soloButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
	soloButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);

	stopButton.setColour(juce::TextButton::buttonColourId,
		(isArmed || isPlaying) ? ColourPalette::stopActive : ColourPalette::buttonInactive);
}

void MixerChannel::startGeneratingAnimation()
{
	isGenerating = true;
	blinkState = false;

	if (!isTimerRunning())
	{
		startTimer(200);
	}
}

void MixerChannel::stopGeneratingAnimation()
{
	isGenerating = false;
	blinkState = false;

	if (!isBlinking)
	{
		stopTimer();
	}

	repaint();
}

void MixerChannel::learn(juce::String param, MidiLearnableBase* component, std::function<void(float)> uiCallback)
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
					.startLearning(parameterName, &audioProcessor, uiCallback, description, component);
	}
}

void MixerChannel::removeMidiMapping(const juce::String& param)
{
	if (track && track->slotIndex != -1)
	{
		juce::String parameterName = "slot" + juce::String(track->slotIndex + 1) + param;
		audioProcessor.getMidiLearnManager().removeMappingForParameter(parameterName);
	}
}

void MixerChannel::setupMidiLearn()
{
	playButton.onMidiLearn = [this]()
		{
			learn("Play", &playButton);
		};
	muteButton.onMidiLearn = [this]()
		{
			learn("Mute", &muteButton);
		};
	soloButton.onMidiLearn = [this]()
		{
			learn("Solo", &soloButton);
		};
	volumeSlider.onMidiLearn = [this]()
		{
			learn("Volume", &volumeSlider);
		};
	pitchKnob.onMidiLearn = [this]()
		{
			learn("Pitch", &pitchKnob);
		};
	fineKnob.onMidiLearn = [this]()
		{
			learn("Fine", &fineKnob);
		};
	panKnob.onMidiLearn = [this]()
		{
			learn("Pan", &panKnob);
		};
	playButton.onMidiRemove = [this]()
		{
			removeMidiMapping("Play");
		};

	muteButton.onMidiRemove = [this]()
		{
			removeMidiMapping("Mute");
		};

	soloButton.onMidiRemove = [this]()
		{
			removeMidiMapping("Solo");
		};

	volumeSlider.onMidiRemove = [this]()
		{
			removeMidiMapping("Volume");
		};

	pitchKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("Pitch");
		};

	fineKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("Fine");
		};

	panKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("Pan");
		};
}