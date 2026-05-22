#include "MasterChannel.h"
#include "PluginEditor.h"

MasterChannel::MasterChannel(DjIaVstProcessor &processor) : ObsidianBaseMidiComponent(processor)
{
	setupUI();
	wireParameters();
}

MasterChannel::~MasterChannel()
{
	markForDestruction();

	masterVolumeSlider.onMidiLearn = nullptr;
	masterPanKnob.onMidiLearn = nullptr;
	highKnob.onMidiLearn = nullptr;
	midKnob.onMidiLearn = nullptr;
	lowKnob.onMidiLearn = nullptr;

	masterVolumeSlider.onMidiRemove = nullptr;
	masterPanKnob.onMidiRemove = nullptr;
	highKnob.onMidiRemove = nullptr;
	midKnob.onMidiRemove = nullptr;
	lowKnob.onMidiRemove = nullptr;
}

void MasterChannel::setupUI()
{
	addAndMakeVisible(masterVolumeSlider);
	masterVolumeSlider.setSliderStyle(juce::Slider::LinearVertical);
	masterVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	masterVolumeSlider.setColour(juce::Slider::thumbColourId, ColourPalette::playArmed);
	masterVolumeSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	masterVolumeSlider.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	masterVolumeSlider.getProperties().set(CustomLookAndFeel::getDrawTicksPropertyId(), 9);
	masterVolumeSlider.getProperties().set(CustomLookAndFeel::getDrawTicksSmallPropertyId(), true);

	addAndMakeVisible(masterPanKnob);
	masterPanKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	masterPanKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	masterPanKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	masterPanKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(highKnob);
	highKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	highKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	highKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	highKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(midKnob);
	midKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	midKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	midKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	midKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(lowKnob);
	lowKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	lowKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	lowKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	lowKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(masterLabel);
	masterLabel.setText("MASTER", juce::dontSendNotification);
	masterLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	masterLabel.setJustificationType(juce::Justification::left);
	masterLabel.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_SUBTITLE));

	addAndMakeVisible(highLabel);
	highLabel.setText("HIGH", juce::dontSendNotification);
	highLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	highLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(midLabel);
	midLabel.setText("MID", juce::dontSendNotification);
	midLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	midLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(lowLabel);
	lowLabel.setText("LOW", juce::dontSendNotification);
	lowLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	lowLabel.setJustificationType(juce::Justification::centred);

	addAndMakeVisible(panLabel);
	panLabel.setText("PAN", juce::dontSendNotification);
	panLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	panLabel.setJustificationType(juce::Justification::centred);

	Obsidian::applyFontSize(panLabel, Obsidian::MIXER_KNOB_LABEL);
	Obsidian::applyFontSize(lowLabel, Obsidian::MIXER_KNOB_LABEL);
	Obsidian::applyFontSize(midLabel, Obsidian::MIXER_KNOB_LABEL);
	Obsidian::applyFontSize(highLabel, Obsidian::MIXER_KNOB_LABEL);

	addAndMakeVisible(vuMeter);

	masterVolumeSlider.setTooltip("Master output volume");
	masterPanKnob.setTooltip("Master pan balance");
	highKnob.setTooltip("High frequency EQ (-12dB to +12dB)");
	midKnob.setTooltip("Mid frequency EQ (-12dB to +12dB)");
	lowKnob.setTooltip("Low frequency EQ (-12dB to +12dB)");
}

void MasterChannel::paint(juce::Graphics &g)
{
	paintBaseRoundedBackgroundMidWithAlpha06(g);
}

void MasterChannel::resized()
{
	auto area = getLocalBounds().reduced(Obsidian::PADDING);
	int width = area.getWidth();

	masterLabel.setBounds(area.removeFromTop(20));

	const int colSpacing = 2;
	const int knobColWidth = (width - colSpacing * 2) / 3;

	auto leftCol = area.removeFromLeft(knobColWidth);
	area.removeFromLeft(colSpacing);
	auto rightCol = area.removeFromRight(knobColWidth);
	area.removeFromRight(colSpacing);
	auto centerCol = area;

	const int knobSize = 32;
	const int knobSectionH = 50;

	auto placeEqKnob = [&](juce::Rectangle<int> sec, juce::Label &label, juce::Slider &knob)
	{
		const int labelH = 13;
		const int kSize = juce::jmin(knobSize, sec.getHeight() - labelH - 4);
		int totalH = labelH + 3 + kSize;
		int startY = sec.getY() + (sec.getHeight() - totalH) / 2;
		label.setBounds(sec.getX(), startY, sec.getWidth(), labelH);
		knob.setBounds(sec.withSizeKeepingCentre(kSize, kSize).withY(startY + labelH + 3));
	};
	leftCol.removeFromTop(8);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), highLabel, highKnob);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), midLabel, midKnob);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), lowLabel, lowKnob);

	int faderWidth = juce::jmax(38, centerCol.getWidth() * 3 / 4);
	int centerX = centerCol.getX() + (centerCol.getWidth() - faderWidth) / 2;
	masterVolumeSlider.setBounds(centerX, centerCol.getY() + 8, faderWidth, centerCol.getHeight() - 8);

	const int panAreaH = 55;
	auto panArea = rightCol.removeFromBottom(panAreaH);
	auto vuArea = rightCol;

	panLabel.setBounds(panArea.removeFromTop(12).withSizeKeepingCentre(knobColWidth, 12));
	int panKnobSize = juce::jmin(36, panArea.getWidth());
	masterPanKnob.setBounds(panArea.withSizeKeepingCentre(panKnobSize, panKnobSize));

	auto vBounds = vuArea.reduced(vuArea.getWidth() / 4, 8);
	float meterTotalWidth = 12.0f;

	float startX = vBounds.getX() + (vBounds.getWidth() - meterTotalWidth) / 2;
	int vuStartY = vBounds.getY() + 6;
	int vuHeight = vBounds.getHeight();

	vuMeter.setBounds(static_cast<int>(startX), vuStartY, static_cast<int>(meterTotalWidth), vuHeight);
}

void MasterChannel::updateMasterLevels()
{
	vuMeter.updateFromRawLevels(realAudioLevelLeft, realAudioLevelRight);
}

void MasterChannel::setRealAudioLevelStereo(float levelLeft, float levelRight)
{
	realAudioLevelLeft = juce::jlimit(0.0f, 1.0f, levelLeft);
	realAudioLevelRight = juce::jlimit(0.0f, 1.0f, levelRight);
	hasRealAudio = true;
}

void MasterChannel::wireParameters()
{
	registerSliderParam("masterVolume", masterVolumeSlider);
	registerSliderParam("masterPan", masterPanKnob);
	registerSliderParam("masterHigh", highKnob);
	registerSliderParam("masterMid", midKnob);
	registerSliderParam("masterLow", lowKnob);

	registerMidiLearn("masterVolume", &masterVolumeSlider);
	registerMidiLearn("masterPan", &masterPanKnob);
	registerMidiLearn("masterHigh", &highKnob);
	registerMidiLearn("masterMid", &midKnob);
	registerMidiLearn("masterLow", &lowKnob);

	syncSliderRange(masterVolumeSlider, fullParamId("masterVolume"));
	syncSliderRange(masterPanKnob, fullParamId("masterPan"));
	syncSliderRange(highKnob, fullParamId("masterHigh"));
	syncSliderRange(midKnob, fullParamId("masterMid"));
	syncSliderRange(lowKnob, fullParamId("masterLow"));
}
