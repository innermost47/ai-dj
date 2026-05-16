#include "MixerChannel.h"
#include "PluginEditor.h"
#include <string>

MixerChannel::MixerChannel(const juce::String &trackId, DjIaVstProcessor &processor, TrackData *trackData)
    : trackId(trackId), ObsidianBaseMidiComponent(processor), track(nullptr)
{
	setupUI();
	setTrackData(trackData);
	updateFromTrackData();
}

MixerChannel::~MixerChannel()
{
	markForDestruction();
	cleanup();
}

void MixerChannel::cleanup()
{
	setVisible(false);

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

	stopTimer();

	track = nullptr;
}

void MixerChannel::setTrackData(TrackData *trackData)
{
	track = trackData;
	if (track && track->slotIndex != -1)
	{
		wireParameters();
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

void MixerChannel::wireParameters()
{
	registerSliderParam("Volume", volumeSlider);
	registerSliderParam("Pitch", pitchKnob);
	registerSliderParam("Fine", fineKnob);
	registerSliderParam("Pan", panKnob);
	registerSliderParam("DelaySend", sendDelayKnob);
	registerSliderParam("ReverbSend", sendReverbKnob);

	subscribeToParam("Play");

	registerButtonParam("Mute", muteButton);
	registerButtonParam("Solo", soloButton);

	registerMidiLearn("Volume", &volumeSlider);
	registerMidiLearn("Pitch", &pitchKnob);
	registerMidiLearn("Fine", &fineKnob);
	registerMidiLearn("Pan", &panKnob);
	registerMidiLearn("Mute", &muteButton);
	registerMidiLearn("Solo", &soloButton);
	registerMidiLearn("Play", &playButton);
	registerMidiLearn("DelaySend", &sendDelayKnob);
	registerMidiLearn("ReverbSend", &sendReverbKnob);

	sendDelayKnob.setDoubleClickReturnValue(true, 0.0);
	sendReverbKnob.setDoubleClickReturnValue(true, 0.0);
	volumeSlider.setDoubleClickReturnValue(true, 0.8);
	pitchKnob.setDoubleClickReturnValue(true, 0.0);
	fineKnob.setDoubleClickReturnValue(true, 0.0);
	panKnob.setDoubleClickReturnValue(true, 0.0);
}

void MixerChannel::stopTrackImmediatly()
{
	track->pendingAction = TrackData::PendingAction::None;
	track->isArmed.store(false);
	track->isArmedToStop.store(false);
	track->isPlaying.store(false);
	track->isCurrentlyPlaying.store(false);
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

	if (juce::MessageManager::getInstanceWithoutCreating() == nullptr)
		return;

	juce::MessageManager::callAsync(
	    [safeThis = juce::Component::SafePointer<MixerChannel>(this)]()
	    {
		    if (safeThis != nullptr)
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

	syncBindingsFromParameters();

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
	sendDelayKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	sendReverbKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void MixerChannel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	g.setColour(ColourPalette::backgroundMid.withAlpha(ObsidianShades::ALPHA_06));
	g.fillRoundedRectangle(bounds.toFloat(), ObsidianSizes::CORNER);

	if (hasSamplePending && !isGenerating)
	{
		g.setColour(ColourPalette::samplePending.withAlpha(0.15f));
		g.fillRoundedRectangle(bounds.toFloat(), ObsidianSizes::CORNER);
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
	else
	{
		borderColour = ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER);
		borderWidth = ObsidianSizes::BORDER_WIDTH;
	}

	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), ObsidianSizes::CORNER, borderWidth);
}

void MixerChannel::resized()
{
	auto area = getLocalBounds().reduced(2);
	const int width = area.getWidth();

	area.removeFromTop(2);
	trackNameLabel.setBounds(area.removeFromTop(12));

	auto bottomRow2 = area.removeFromBottom(18);
	int btnW = width / 2 - 2;
	int totalW = btnW * 2 + 2;
	int offsetX = (width - totalW) / 2;
	bottomRow2.removeFromLeft(offsetX);
	muteButton.setBounds(bottomRow2.removeFromLeft(btnW));
	bottomRow2.removeFromLeft(1);
	soloButton.setBounds(bottomRow2.removeFromLeft(btnW));

	auto bottomRow1 = area.removeFromBottom(18);
	bottomRow1.removeFromLeft(offsetX);
	playButton.setBounds(bottomRow1.removeFromLeft(btnW));
	bottomRow1.removeFromLeft(1);
	stopButton.setBounds(bottomRow1.removeFromLeft(btnW));

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

	const int knobSectionH = ObsidianSizes::MIXER_CHANNEL_KNOB;
	auto placeKnobSection = [&](juce::Rectangle<int> secArea, juce::Label &label, juce::Slider &knob)
	{
		label.setBounds(secArea.removeFromTop(6));
		knob.setBounds(secArea.withTrimmedTop(-4));
	};
	knobsColumn.removeFromTop(4);
	placeKnobSection(knobsColumn.removeFromTop(knobSectionH), sendDelayLabel, sendDelayKnob);
	placeKnobSection(knobsColumn.removeFromTop(knobSectionH), sendReverbLabel, sendReverbKnob);
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

void MixerChannel::setupUI()
{
	addAndMakeVisible(trackNameLabel);
	trackNameLabel.setText("Track", juce::dontSendNotification);
	trackNameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	trackNameLabel.setJustificationType(juce::Justification::centred);
	trackNameLabel.setFont(juce::FontOptions(ObsidianSizes::MIXER_LABEL_NAME, juce::Font::bold));
	trackNameLabel.setEditable(false, true);

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

	addAndMakeVisible(sendDelayKnob);
	sendDelayKnob.setRange(0.0, 1.0, 0.001);
	sendDelayKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	sendDelayKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	sendDelayKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	sendDelayKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	sendDelayKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	sendDelayKnob.setTooltip("Delay send level (post-fader)");

	addAndMakeVisible(sendReverbKnob);
	sendReverbKnob.setRange(0.0, 1.0, 0.001);
	sendReverbKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	sendReverbKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	sendReverbKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	sendReverbKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	sendReverbKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);
	sendReverbKnob.setTooltip("Reverb send level (post-fader)");

	addAndMakeVisible(sendDelayLabel);
	sendDelayLabel.setText("DLY", juce::dontSendNotification);
	sendDelayLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	sendDelayLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(sendReverbLabel);
	sendReverbLabel.setText("RVB", juce::dontSendNotification);
	sendReverbLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	sendReverbLabel.setJustificationType(juce::Justification::centred);

	ObsidianFonts::applyFontSize(pitchLabel, ObsidianSizes::MIXER_KNOB_LABEL);
	ObsidianFonts::applyFontSize(sendDelayLabel, ObsidianSizes::MIXER_KNOB_LABEL);
	ObsidianFonts::applyFontSize(sendReverbLabel, ObsidianSizes::MIXER_KNOB_LABEL);
	ObsidianFonts::applyFontSize(panLabel, ObsidianSizes::MIXER_KNOB_LABEL);
	ObsidianFonts::applyFontSize(fineLabel, ObsidianSizes::MIXER_KNOB_LABEL);

	addAndMakeVisible(vuMeter);

	playButton.setTooltip("Arm/disarm track for playback");
	stopButton.setTooltip("Stop track playback");
	muteButton.setTooltip("Mute this track");
	soloButton.setTooltip("Solo this track");
	volumeSlider.setTooltip("Track volume level");
	pitchKnob.setTooltip("Pitch adjustment (-12 to +12 semitones)");
	fineKnob.setTooltip("Fine pitch adjustment (-50 to +50 cents)");
	panKnob.setTooltip("Pan position (left/right balance)");

	addEventListeners();
}

void MixerChannel::addEventListeners()
{
	trackNameLabel.onTextChange = [this]()
	{
		if (onTrackRenamed)
			onTrackRenamed(trackNameLabel.getText());
	};

	playButton.onClick = [this]()
	{
		if (!track)
			return;

		bool currentlyActive = track->isArmed.load() || track->isCurrentlyPlaying.load();
		bool desiredState = !currentlyActive;

		applyPlayState(desiredState);

		if (auto *p = getParam("Play"))
			p->setValueNotifyingHost(desiredState ? 1.0f : 0.0f);
	};

	stopButton.onClick = [this]()
	{
		if (!track)
			return;
		applyPlayState(false);

		if (auto *p = getParam("Play"))
			p->setValueNotifyingHost(0.0f);
	};
}

bool MixerChannel::allSequencerStepsAreFalse() const
{
	if (!track)
		return true;

	auto &seqData = track->getCurrentSequencerData();
	for (const auto &measure : seqData.steps)
		for (bool step : measure)
			if (step)
				return false;
	return true;
}

void MixerChannel::applyPlayState(bool shouldArm)
{
	if (!track)
		return;
	juce::ScopedValueSetter<bool> guard(isApplyingPlayState, true);

	auto &currentPage = track->getCurrentPage();
	if (currentPage.numSamples <= 0)
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		return;
	}

	const bool isPlaying = track->isCurrentlyPlaying.load();
	const bool emptySeq = allSequencerStepsAreFalse();

	if (shouldArm && !isPlaying)
	{
		if (emptySeq)
		{
			track->setArmedToStop(false);
			track->pendingAction = TrackData::PendingAction::None;
		}
		track->setArmed(true);
	}
	else if (!shouldArm && !isPlaying)
	{
		track->pendingAction = TrackData::PendingAction::None;
		track->setArmed(false);
	}
	else if (!shouldArm && isPlaying && !emptySeq)
	{
		if (track->isArmedToStop.load())
			return;
		track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
		track->setArmed(false);
		track->setArmedToStop(true);
		isBlinking = true;
		startTimer(300);
	}
	else if (emptySeq)
	{
		stopTrackImmediatly();
		return;
	}

	updateButtonColors();
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

void MixerChannel::onParameterChangedUI(const juce::String &paramSuffix, float newValue)
{
	if (!track)
		return;
	if (isApplyingPlayState)
		return;

	if (paramSuffix == "Mute")
		track->isMuted = newValue > 0.5f;
	else if (paramSuffix == "Solo")
		track->isSolo = newValue > 0.5f;
	else if (paramSuffix == "Play")
		applyPlayState(newValue > 0.5f);
	else if (paramSuffix == "DelaySend")
	{
		track->delaySend = newValue;
	}
	else if (paramSuffix == "ReverbSend")
	{
		track->reverbSend = newValue;
	}
}