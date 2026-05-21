#include "MixerChannel.h"
#include "PluginEditor.h"
#include <string>

MixerChannel::MixerChannel(const juce::String &trackId, DjIaVstProcessor &processor, TrackData *trackData)
    : ObsidianBaseMidiComponent(processor), trackId(trackId)
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

	if (auto *t = getTrack())
	{
		t->onPlayStateChanged = nullptr;
		t->onArmedStateChanged = nullptr;
		t->onArmedToStopStateChanged = nullptr;
	}
	track = nullptr;
}

void MixerChannel::setTrackData(TrackData *trackData)
{
	track = trackData;
	auto *t = getTrack();
	if (t && t->slotIndex != -1)
	{
		wireParameters();
		juce::WeakReference<MixerChannel> weakThis(this);
		t->onPlayStateChanged = [weakThis](bool /*isPlaying*/)
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
		t->onArmedStateChanged = [weakThis](bool /*isArmed*/)
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

		t->onArmedToStopStateChanged = [weakThis](bool /*isArmedToStop*/)
		{
			if (weakThis != nullptr)
			{
				juce::MessageManager::callAsync(
				    [weakThis]()
				    {
					    if (weakThis != nullptr)
					    {
						    bool allStepsAreFalse = true;
						    auto &seqData = weakThis->getTrack()->getCurrentSequencerData();
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
	registerSliderParam("Gain", gainKnob);
	registerSliderParam("DelaySend", sendDelayKnob);
	registerSliderParam("ReverbSend", sendReverbKnob);

	registerButtonParam("Play", playButton);
	registerButtonParam("Mute", muteButton);
	registerButtonParam("Solo", soloButton);

	registerMidiLearn("Volume", &volumeSlider);
	registerMidiLearn("Pitch", &pitchKnob);
	registerMidiLearn("Fine", &fineKnob);
	registerMidiLearn("Pan", &panKnob);
	registerMidiLearn("Gain", &gainKnob);
	registerMidiLearn("Mute", &muteButton);
	registerMidiLearn("Solo", &soloButton);
	registerMidiLearn("Play", &playButton);
	registerMidiLearn("DelaySend", &sendDelayKnob);
	registerMidiLearn("ReverbSend", &sendReverbKnob);

	syncSliderRange(pitchKnob, fullParamId("Pitch"));
	syncSliderRange(gainKnob, fullParamId("Gain"));
	syncSliderRange(fineKnob, fullParamId("Fine"));
	syncSliderRange(panKnob, fullParamId("Pan"));
	syncSliderRange(volumeSlider, fullParamId("Volume"));

	sendDelayKnob.setDoubleClickReturnValue(true, 0.0);
	sendReverbKnob.setDoubleClickReturnValue(true, 0.0);
	volumeSlider.setDoubleClickReturnValue(true, 0.8);
	pitchKnob.setDoubleClickReturnValue(true, 0.0);
	gainKnob.setDoubleClickReturnValue(true, 0.0);
	fineKnob.setDoubleClickReturnValue(true, 0.0);
	panKnob.setDoubleClickReturnValue(true, 0.0);
}

void MixerChannel::stopTrackImmediatly()
{
	auto *t = getTrack();
	if (t)
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonInactive);
		stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonInactive);
		playButton.repaint();
		stopButton.repaint();
	}
}

void MixerChannel::timerCallback()
{
	bool shouldContinueTimer = false;

	auto *t = getTrack();
	if (!t)
	{
		stopTimer();
		return;
	}

	if (isGenerating)
	{
		blinkState = !blinkState;
		repaint();
		shouldContinueTimer = true;
	}

	if (isBlinking && t && t->isArmedToStop)
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

	auto *t = getTrack();
	if (!t || t->slotIndex == -1)
		return;

	trackNameLabel.setText(t->trackName, juce::dontSendNotification);

	syncBindingsFromParameters();

	updateButtonColors();
	updateModelUI();
}

void MixerChannel::updateModelUI()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &currentPage = t->getCurrentPage();
	auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
	bool darkText = modelColour.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

	volumeSlider.setColour(juce::Slider::thumbColourId, modelColour);

	pitchKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	fineKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	panKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	gainKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	sendDelayKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);
	sendReverbKnob.setColour(juce::Slider::rotarySliderFillColourId, modelColour);

	repaint();
}

void MixerChannel::paint(juce::Graphics &g)
{
	auto *t = getTrack();
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

	if (isGenerating && t)
	{
		auto &currentPage = t->getCurrentPage();
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

	area.removeFromTop(ObsidianSizes::GAP_2);
	trackNameLabel.setBounds(area.removeFromTop(12));
	area.removeFromTop(ObsidianSizes::GAP_4);

	using FlexBox = juce::FlexBox;
	using FlexItem = juce::FlexItem;

	const float labelH = 0.2f;
	const float knobSize = 0.8f;
	const float volVuHeight = (labelH + knobSize) * 3;
	const float vuMeterWidth = 12.0f;
	const int triggerBtnSize = 18;

	FlexBox bottomRow3;
	bottomRow3.flexDirection = FlexBox::Direction::row;

	bottomRow3.items.add(FlexItem(muteButton).withFlex(1.0f));
	bottomRow3.items.add(FlexItem(soloButton).withFlex(1.0f));

	auto muteSoloArea = area.removeFromBottom(triggerBtnSize);
	bottomRow3.performLayout(muteSoloArea);

	FlexBox bottomRow2;
	bottomRow2.flexDirection = FlexBox::Direction::row;

	bottomRow2.items.add(FlexItem(playButton).withFlex(1.0f));
	bottomRow2.items.add(FlexItem(stopButton).withFlex(1.0f));

	auto playStopArea = area.removeFromBottom(triggerBtnSize);
	bottomRow2.performLayout(playStopArea);

	FlexBox volumeGainColumn;
	volumeGainColumn.flexDirection = FlexBox::Direction::column;
	volumeGainColumn.justifyContent = FlexBox::JustifyContent::center;

	volumeGainColumn.items.add(FlexItem(volumeSlider).withFlex(volVuHeight));
	volumeGainColumn.items.add(FlexItem(gainLabel).withFlex(labelH));
	volumeGainColumn.items.add(FlexItem(gainKnob).withFlex(knobSize));

	FlexBox vuMeterPanColumn;
	vuMeterPanColumn.flexDirection = FlexBox::Direction::column;
	vuMeterPanColumn.justifyContent = FlexBox::JustifyContent::center;

	vuMeterPanColumn.items.add(FlexItem(vuMeterContainer)
	                               .withFlex(volVuHeight)
	                               .withWidth(vuMeterWidth)
	                               .withAlignSelf(FlexItem::AlignSelf::center));
	vuMeterPanColumn.items.add(FlexItem(panLabel).withFlex(labelH));
	vuMeterPanColumn.items.add(FlexItem(panKnob).withFlex(knobSize));

	FlexBox knobsColumn;
	knobsColumn.flexDirection = FlexBox::Direction::column;
	knobsColumn.justifyContent = FlexBox::JustifyContent::center;

	knobsColumn.items.add(FlexItem(sendDelayLabel).withFlex(labelH));
	knobsColumn.items.add(FlexItem(sendDelayKnob).withFlex(knobSize));
	knobsColumn.items.add(FlexItem(sendReverbLabel).withFlex(labelH));
	knobsColumn.items.add(FlexItem(sendReverbKnob).withFlex(knobSize));
	knobsColumn.items.add(FlexItem(pitchLabel).withFlex(labelH));
	knobsColumn.items.add(FlexItem(pitchKnob).withFlex(knobSize));
	knobsColumn.items.add(FlexItem(fineLabel).withFlex(labelH));
	knobsColumn.items.add(FlexItem(fineKnob).withFlex(knobSize));

	FlexBox control;
	control.flexDirection = FlexBox::Direction::row;
	control.justifyContent = FlexBox::JustifyContent::center;
	control.alignContent = FlexBox::AlignContent::center;

	control.items.add(FlexItem(volumeGainColumn).withFlex(1.0f));
	control.items.add(FlexItem(vuMeterPanColumn).withFlex(1.0f));
	control.items.add(FlexItem(knobsColumn).withFlex(1.0f));

	control.performLayout(area);

	auto cb = vuMeterContainer.getLocalBounds();
	vuMeter.setBounds(cb.reduced(0, ObsidianSizes::GAP_4).withX(cb.getX()));
}

void MixerChannel::updateVUMeter()
{
	auto *t = getTrack();
	if (t)
		vuMeter.updateFromRawLevels(t->audioLevelLeft.load(), t->audioLevelRight.load());
	else
		vuMeter.updateFromRawLevels(0.0f, 0.0f);
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

	addAndMakeVisible(gainKnob);
	gainKnob.setRange(-12.0, 12.0, 0.1);
	gainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	gainKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	gainKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	gainKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	gainKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(gainLabel);
	gainLabel.setText("GAIN", juce::dontSendNotification);
	gainLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	gainLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(sendDelayKnob);
	sendDelayKnob.setRange(0.0, 1.0, 0.001);
	sendDelayKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	sendDelayKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	sendDelayKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	sendDelayKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	sendDelayKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(sendReverbKnob);
	sendReverbKnob.setRange(0.0, 1.0, 0.001);
	sendReverbKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	sendReverbKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	sendReverbKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::sliderThumb);
	sendReverbKnob.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	sendReverbKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

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
	ObsidianFonts::applyFontSize(gainLabel, ObsidianSizes::MIXER_KNOB_LABEL);

	addAndMakeVisible(vuMeterContainer);
	vuMeterContainer.addAndMakeVisible(vuMeter);

	playButton.setTooltip("Arm/disarm track for playback");
	stopButton.setTooltip("Stop track playback");
	muteButton.setTooltip("Mute this track");
	soloButton.setTooltip("Solo this track");
	volumeSlider.setTooltip("Track volume level");
	pitchKnob.setTooltip("Pitch adjustment (-12 to +12 semitones)");
	fineKnob.setTooltip("Fine pitch adjustment (-50 to +50 cents)");
	panKnob.setTooltip("Pan position (left/right balance)");
	sendReverbKnob.setTooltip("Reverb send level (post-fader)");
	sendDelayKnob.setTooltip("Delay send level (post-fader)");

	addEventListeners();
}

void MixerChannel::addEventListeners()
{
	trackNameLabel.onTextChange = [this]()
	{
		if (onTrackRenamed)
			onTrackRenamed(trackNameLabel.getText());
	};

	stopButton.onClick = [this]()
	{
		auto *t = getTrack();
		if (!t)
			return;
		applyPlayState(false);

		if (auto *p = getParam("Play"))
			p->setValueNotifyingHost(0.0f);
	};
}

void MixerChannel::applyPlayState(bool shouldArm)
{
	auto *t = getTrack();
	if (!t)
		return;
	juce::ScopedValueSetter<bool> guard(isApplyingPlayState, true);

	auto &currentPage = t->getCurrentPage();
	if (currentPage.numSamples <= 0)
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		return;
	}

	const bool isPlaying = t->isCurrentlyPlaying.load();
	const bool emptySeq = t->allSequencerStepsAreFalse();

	if (!shouldArm && isPlaying && !emptySeq)
	{
		if (t->isArmedToStop.load())
			return;
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
	auto *t = getTrack();
	if (!t)
		return;

	bool isArmed = t->isArmed.load();
	bool isPlaying = t->isCurrentlyPlaying.load();
	bool isMuted = t->isMuted.load();
	bool isSolo = t->isSolo.load();

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
	auto *t = getTrack();
	if (!t)
		return;
	if (isApplyingPlayState)
		return;

	auto &currentPage = t->getCurrentPage();
	if (currentPage.numSamples <= 0)
	{
		return;
	}
	if (paramSuffix == "Play")
	{
		applyPlayState(newValue < 0.5f);
	}
}