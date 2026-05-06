#include "MixerChannel.h"
#include "AiModelDefinitions.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "PluginEditor.h"
#include <string>

MixerChannel::MixerChannel(const juce::String &trackId, DjIaVstProcessor &processor, TrackData *trackData)
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
	setVisible(false);
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
			auto &allParams = audioProcessor.AudioProcessor::getParameters();
			for (int i = 0; i < allParams.size(); ++i)
			{
				auto *param = allParams[i];
				if (param)
				{
					param->removeListener(this);
				}
			}
		}
	}
	catch (...)
	{
	}

	track = nullptr;
}

MixerChannel::~MixerChannel()
{
	cleanup();
}

void MixerChannel::setTrackData(TrackData *trackData)
{
	track = trackData;
	if (track)
	{
		juce::WeakReference<MixerChannel> weakThis(this);
		track->onPlayStateChanged = [weakThis](bool /*isPlaying*/)
		{
			if (weakThis != nullptr)
			{
				juce::MessageManager::callAsync(
				    [weakThis]()
				    {
					    if (weakThis != nullptr && !weakThis->isUpdatingButtons)
					    {
						    weakThis->updateButtonColors();
					    }
				    });
			}
		};
		track->onArmedStateChanged = [weakThis](bool /*isArmed*/)
		{
			if (weakThis != nullptr)
			{
				juce::MessageManager::callAsync(
				    [weakThis]()
				    {
					    if (weakThis != nullptr)
					    {
						    weakThis->isBlinking = true;
						    weakThis->startTimer(300);
					    }
				    });
			}
		};

		track->onArmedToStopStateChanged = [weakThis](bool /*isArmedToStop*/)
		{
			if (weakThis != nullptr)
			{
				juce::MessageManager::callAsync(
				    [weakThis]()
				    {
					    if (weakThis != nullptr)
					    {
						    bool allStepsAreFalse = true;
						    auto &seqData = weakThis->track->getCurrentSequencerData();
						    for (const auto &measure : seqData.steps)
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
							    weakThis->stopTrackImmediatly();
						    }
						    else
						    {
							    weakThis->isBlinking = true;
							    weakThis->startTimer(300);
						    }
					    }
				    });
			}
		};
	}
}

void MixerChannel::removeListener(juce::String name)
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

void MixerChannel::addListener(juce::String name)
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

void MixerChannel::parameterValueChanged(int parameterIndex, float newValue)
{
	if (!track || track->slotIndex == -1)
		return;

	juce::String slotPrefix = "Slot " + juce::String(track->slotIndex + 1);
	auto &allParams = audioProcessor.AudioProcessor::getParameters();

	if (parameterIndex >= 0 && parameterIndex < allParams.size())
	{
		auto *param = allParams[parameterIndex];
		juce::String paramName = param->getName(256);

		if (juce::MessageManager::getInstance()->isThisTheMessageThread())
		{
			juce::Timer::callAfterDelay(50, [this, paramName, slotPrefix, newValue]()
			                            { updateUIFromParameter(paramName, slotPrefix, newValue); });
		}
		else
		{
			juce::MessageManager::callAsync(
			    [this, paramName, slotPrefix, newValue]()
			    {
				    juce::Timer::callAfterDelay(50, [this, paramName, slotPrefix, newValue]()
				                                { updateUIFromParameter(paramName, slotPrefix, newValue); });
			    });
		}
	}
}

void MixerChannel::updateUIFromParameter(const juce::String &paramName, const juce::String &slotPrefix, float newValue)
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
			playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
			stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
		}
		else if (newValue > 0.5 && !track->isCurrentlyPlaying.load())
		{
			bool allStepsAreFalse = true;
			auto &seqData = track->getCurrentSequencerData();
			for (const auto &measure : seqData.steps)
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
			playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playArmed);
			stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::stopActive);
		}
	}
}

void MixerChannel::parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/)
{
}

void MixerChannel::setSliderParameter(juce::String name, juce::Slider &slider)
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
	}
}

void MixerChannel::setButtonParameter(juce::String name, juce::Button &button)
{
	auto &currentPage = track->getCurrentPage();
	if (!track || track->slotIndex == -1 || currentPage.numSamples <= 0)
		return;
	if (this == nullptr)
		return;
	juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + name;
	try
	{
		auto *param = audioProcessor.getParameters().getParameter(paramName);
		if (param != nullptr)
		{
			bool state = button.getToggleState();
			param->setValueNotifyingHost(state ? 1.0f : 0.0f);
			updateButtonColors();
		}
	}
	catch (...)
	{
	}
}

void MixerChannel::addEventListeners()
{
	volumeSlider.onValueChange = [this]() { setSliderParameter("Volume", volumeSlider); };
	pitchKnob.onValueChange = [this]() { setSliderParameter("Pitch", pitchKnob); };
	fineKnob.onValueChange = [this]() { setSliderParameter("Fine", fineKnob); };
	panKnob.onValueChange = [this]() { setSliderParameter("Pan", panKnob); };
	playButton.onClick = [this]()
	{
		auto &currentPage = track->getCurrentPage();
		if (track && currentPage.numSamples > 0)
		{
			bool allStepsAreFalse = true;
			auto &seqData = track->getCurrentSequencerData();
			for (const auto &measure : seqData.steps)
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
		auto &currentPage = track->getCurrentPage();
		if (track && currentPage.numSamples > 0)
		{
			bool allStepsAreFalse = true;
			auto &seqData = track->getCurrentSequencerData();
			for (const auto &measure : seqData.steps)
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
	playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
	stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
	playButton.repaint();
	stopButton.repaint();
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

	if (juce::MessageManager::getInstanceWithoutCreating() == nullptr)
		return;

	juce::MessageManager::callAsync(
	    [safeThis = juce::Component::SafePointer<MixerChannel>(this)]()
	    {
		    if (safeThis != nullptr && !safeThis->isDestroyed.load())
		    {
			    safeThis->updateVUMeter();
			    safeThis->repaint();
		    }
	    });
}

void MixerChannel::updateFromTrackData()
{
	if (!track || track->slotIndex == -1)
		return;

	trackNameLabel.setText(track->trackName, juce::dontSendNotification);
	auto &params = audioProcessor.getParameterTreeState();
	juce::String slotPrefix = "slot" + juce::String(track->slotIndex + 1);

	if (auto *volumeParam = params.getParameter(slotPrefix + "Volume"))
	{
		volumeSlider.setValue(volumeParam->getValue(), juce::dontSendNotification);
	}

	if (auto *pitchParam = params.getParameter(slotPrefix + "Pitch"))
	{
		float normalizedPitch = pitchParam->getValue();
		float denormalizedPitch = normalizedPitch * 24.0f - 12.0f;
		pitchKnob.setValue(denormalizedPitch, juce::dontSendNotification);
	}

	if (auto *fineParam = params.getParameter(slotPrefix + "Fine"))
	{
		float normalizedFine = fineParam->getValue();
		float denormalizedFine = normalizedFine * 100.0f - 50.0f;
		fineKnob.setValue(denormalizedFine, juce::dontSendNotification);
	}

	if (auto *panParam = params.getParameter(slotPrefix + "Pan"))
	{
		float normalizedPan = panParam->getValue();
		float denormalizedPan = normalizedPan * 2.0f - 1.0f;
		panKnob.setValue(denormalizedPan, juce::dontSendNotification);
	}

	muteButton.setToggleState(track->isMuted.load(), juce::dontSendNotification);
	soloButton.setToggleState(track->isSolo.load(), juce::dontSendNotification);

	updateButtonColors();
	updateModelUI();
}

void MixerChannel::updateModelUI()
{
	if (!track)
		return;

	auto &currentPage = track->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	volumeSlider.setColour(juce::Slider::thumbColourId, modelColour);

	pitchKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	fineKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	panKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void MixerChannel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

	if (hasSamplePending && !isGenerating)
	{
		g.setColour(ColourPalette::samplePending.withAlpha(0.15f));
		g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
	}

	juce::Colour borderColour;
	float borderWidth;

	if (isGenerating)
	{
		auto &currentPage = track->getCurrentPage();
		auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
		borderColour = blinkState ? modelColour.brighter(0.4f) : modelColour.darker(0.4f);
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
		borderColour = ColourPalette::sliderTrack;
		borderWidth = 1.0f;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, borderWidth);
}

void MixerChannel::resized()
{
	auto area = getLocalBounds().reduced(4);
	const int width = area.getWidth();

	trackNameLabel.setBounds(area.removeFromTop(16));
	area.removeFromTop(3);

	auto bottomRow2 = area.removeFromBottom(24);
	muteButton.setBounds(bottomRow2.removeFromLeft(width / 2 - 2).reduced(1));
	soloButton.setBounds(bottomRow2.removeFromLeft(width / 2 - 2).reduced(1));

	auto bottomRow1 = area.removeFromBottom(24);
	playButton.setBounds(bottomRow1.removeFromLeft(width / 2 - 2).reduced(1));
	stopButton.setBounds(bottomRow1.removeFromLeft(width / 2 - 2).reduced(1));

	area.removeFromBottom(3);

	const int knobColumnWidth = juce::jmin(60, width * 2 / 5);
	auto knobsColumn = area.removeFromRight(knobColumnWidth);
	area.removeFromRight(8);

	int sliderBottom = area.getBottom();
	int sliderTop = area.getY();
	sliderBounds = juce::Rectangle<int>(area.getX(), sliderTop, area.getWidth() * 3 / 4, sliderBottom - sliderTop);
	volumeSlider.setBounds(sliderBounds);

	int meterTotalWidth = 12;

	int customHeight = sliderBounds.getHeight() - 12;
	int customY = sliderBounds.getY() + 8;

	vuMeter.setBounds(sliderBounds.getRight() + 3, customY, meterTotalWidth, customHeight);

	const int knobSectionH = 42;
	auto placeKnobSection = [&](juce::Rectangle<int> secArea, juce::Label &label, juce::Slider &knob)
	{
		label.setBounds(secArea.removeFromTop(11));
		knob.setBounds(secArea.reduced(1));
	};
	knobsColumn.removeFromTop(5);
	placeKnobSection(knobsColumn.removeFromTop(knobSectionH), pitchLabel, pitchKnob);
	placeKnobSection(knobsColumn.removeFromTop(knobSectionH), fineLabel, fineKnob);
	placeKnobSection(knobsColumn.removeFromTop(knobSectionH), panLabel, panKnob);
}

void MixerChannel::updateVUMeter()
{
	if (track)
	{
		auto &currentPage = track->getCurrentPage();
		vuMeter.updateMeter(&currentPage.audioBuffer, track->readPosition.load(), track->volume.load(),
		                    track->isPlaying.load());
	}
	else
	{
		vuMeter.updateMeter(nullptr, 0.0, 0.0f, false);
	}
}

void MixerChannel::setSelected(bool selected)
{
	isSelected = selected;
	repaint();
}

void MixerChannel::setupUI()
{
	addAndMakeVisible(trackNameLabel);
	trackNameLabel.setText("Track", juce::dontSendNotification);
	trackNameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	trackNameLabel.setJustificationType(juce::Justification::centred);
	trackNameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
	trackNameLabel.setEditable(false, true);
	trackNameLabel.onTextChange = [this]()
	{
		if (onTrackRenamed)
			onTrackRenamed(trackNameLabel.getText());
	};

	addAndMakeVisible(playButton);
	playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
	playButton.loadIconToggled(BinaryData::pause_svg, BinaryData::pause_svgSize);
	playButton.setHasToggledIcon(true);
	playButton.setLabelText("");
	playButton.setClickingTogglesState(true);

	addAndMakeVisible(stopButton);
	stopButton.loadIcon(BinaryData::square_svg, BinaryData::square_svgSize);
	stopButton.setLabelText("");
	stopButton.setClickingTogglesState(false);

	addAndMakeVisible(muteButton);
	muteButton.loadIcon(BinaryData::volume_svg, BinaryData::volume_svgSize);
	muteButton.loadIconToggled(BinaryData::mute_svg, BinaryData::mute_svgSize);
	muteButton.setHasToggledIcon(true);
	muteButton.setLabelText("");
	muteButton.setClickingTogglesState(true);

	addAndMakeVisible(soloButton);
	soloButton.loadIcon(BinaryData::headphones_svg, BinaryData::headphones_svgSize);
	soloButton.setLabelText("");
	soloButton.setClickingTogglesState(true);

	playButton.setCompactMode(true);
	stopButton.setCompactMode(true);
	muteButton.setCompactMode(true);
	soloButton.setCompactMode(true);

	addAndMakeVisible(volumeSlider);
	volumeSlider.setRange(0.0, 1.0, 0.01);
	volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
	volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	volumeSlider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
	volumeSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	volumeSlider.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	volumeSlider.getProperties().set(CustomLookAndFeel::getDrawTicksPropertyId(), 9);
	volumeSlider.getProperties().set(CustomLookAndFeel::getDrawTicksSmallPropertyId(), true);

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

	addAndMakeVisible(vuMeter);

	playButton.setTooltip("Arm/disarm track for playback");
	stopButton.setTooltip("Stop track playback");
	muteButton.setTooltip("Mute this track");
	soloButton.setTooltip("Solo this track");
	volumeSlider.setTooltip("Track volume level");
	pitchKnob.setTooltip("Pitch adjustment (-12 to +12 semitones)");
	fineKnob.setTooltip("Fine pitch adjustment (-50 to +50 cents)");
	panKnob.setTooltip("Pan position (left/right balance)");
}

void MixerChannel::setTrackName(const juce::String &name)
{
	trackNameLabel.setText(name, juce::dontSendNotification);
}

void MixerChannel::updateButtonColors()
{
	if (!track)
		return;

	bool isArmed = track->isArmed.load();
	bool isPlaying = track->isCurrentlyPlaying.load();
	bool isMuted = track->isMuted.load();
	bool isSolo = track->isSolo.load();

	playButton.setToggleState(isArmed || isPlaying, juce::dontSendNotification);
	if (isPlaying)
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playActive);
	else if (isArmed)
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::playArmed);
	else
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
	playButton.repaint();

	stopButton.setColour(juce::TextButton::buttonColourId,
	                     (isArmed || isPlaying) ? ColourPalette::stopActive : ColourPalette::buttonInactive);
	stopButton.repaint();

	muteButton.setToggleState(isMuted, juce::dontSendNotification);
	muteButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::amber);
	muteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
	muteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
	muteButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);

	muteButton.repaint();

	soloButton.setToggleState(isSolo, juce::dontSendNotification);
	soloButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::emerald);
	soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
	soloButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
	soloButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	soloButton.repaint();
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

void MixerChannel::learn(juce::String param, MidiLearnableBase *component, std::function<void(float)> uiCallback)
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
				    editor->updateLCD();
			    }
		    });
		audioProcessor.getMidiLearnManager().startLearning(parameterName, &audioProcessor, uiCallback, description,
		                                                   component);
	}
}

void MixerChannel::removeMidiMapping(const juce::String &param)
{
	if (track && track->slotIndex != -1)
	{
		juce::String parameterName = "slot" + juce::String(track->slotIndex + 1) + param;
		audioProcessor.getMidiLearnManager().removeMappingForParameter(parameterName);
	}
}

void MixerChannel::setupMidiLearn()
{
	playButton.onMidiLearn = [this]() { learn("Play", &playButton); };
	muteButton.onMidiLearn = [this]() { learn("Mute", &muteButton); };
	soloButton.onMidiLearn = [this]() { learn("Solo", &soloButton); };
	volumeSlider.onMidiLearn = [this]() { learn("Volume", &volumeSlider); };
	pitchKnob.onMidiLearn = [this]() { learn("Pitch", &pitchKnob); };
	fineKnob.onMidiLearn = [this]() { learn("Fine", &fineKnob); };
	panKnob.onMidiLearn = [this]() { learn("Pan", &panKnob); };
	playButton.onMidiRemove = [this]() { removeMidiMapping("Play"); };
	muteButton.onMidiRemove = [this]() { removeMidiMapping("Mute"); };
	soloButton.onMidiRemove = [this]() { removeMidiMapping("Solo"); };
	volumeSlider.onMidiRemove = [this]() { removeMidiMapping("Volume"); };
	pitchKnob.onMidiRemove = [this]() { removeMidiMapping("Pitch"); };
	fineKnob.onMidiRemove = [this]() { removeMidiMapping("Fine"); };
	panKnob.onMidiRemove = [this]() { removeMidiMapping("Pan"); };
}