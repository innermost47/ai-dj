#pragma once
#include "MasterChannel.h"
#include "PluginEditor.h"
#include "style/ColourPalette.h"

MasterChannel::MasterChannel(DjIaVstProcessor& processor) : audioProcessor(processor)
{
	setupUI();
	setupMidiLearn();
	addEventListeners();
}

MasterChannel::~MasterChannel()
{
	isDestroyed.store(true);
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

	removeListener("masterVolume");
	removeListener("masterPan");
	removeListener("masterHigh");
	removeListener("masterMid");
	removeListener("masterLow");
}

void MasterChannel::parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/)
{
}

void MasterChannel::parameterValueChanged(int parameterIndex, float newValue)
{
	auto& allParams = audioProcessor.AudioProcessor::getParameters();

	if (parameterIndex >= 0 && parameterIndex < allParams.size())
	{
		auto* param = allParams[parameterIndex];
		juce::String paramName = param->getName(256);

		if (juce::MessageManager::getInstance()->isThisTheMessageThread())
		{
			juce::Timer::callAfterDelay(50, [this, paramName, newValue]()
				{ updateUIFromParameter(paramName, newValue); });
		}
		else
		{
			juce::MessageManager::callAsync([this, paramName, newValue]()
				{ juce::Timer::callAfterDelay(50, [this, paramName, newValue]()
					{ updateUIFromParameter(paramName, newValue); }); });
		}
	}
}

void MasterChannel::updateUIFromParameter(const juce::String& paramName,
	float newValue)
{
	if (isDestroyed.load())
		return;
	if (paramName == "Master Volume")
	{
		if (!masterVolumeSlider.isMouseButtonDown())
			masterVolumeSlider.setValue(newValue, juce::dontSendNotification);
	}
	else if (paramName == "Master Pan")
	{
		if (!masterPanKnob.isMouseButtonDown())
		{
			float denormalizedValue = newValue * 2.0f - 1.0f;
			masterPanKnob.setValue(denormalizedValue, juce::dontSendNotification);
		}
	}
	else if (paramName == "Master High EQ")
	{
		if (!highKnob.isMouseButtonDown())
		{
			float denormalizedValue = newValue * 24.0f - 12.0f;
			highKnob.setValue(denormalizedValue, juce::dontSendNotification);
		}
	}
	else if (paramName == "Master Mid EQ")
	{
		if (!midKnob.isMouseButtonDown())
		{
			float denormalizedValue = newValue * 24.0f - 12.0f;
			midKnob.setValue(denormalizedValue, juce::dontSendNotification);
		}
	}
	else if (paramName == "Master Low EQ")
	{
		if (!lowKnob.isMouseButtonDown())
		{
			float denormalizedValue = newValue * 24.0f - 12.0f;
			lowKnob.setValue(denormalizedValue, juce::dontSendNotification);
		}
	}
}

void MasterChannel::removeListener(juce::String name)
{
	auto* param = audioProcessor.getParameterTreeState().getParameter(name);
	if (param)
	{
		param->removeListener(this);
	}
}

void MasterChannel::addListener(juce::String name)
{
	auto* param = audioProcessor.getParameterTreeState().getParameter(name);
	if (param)
	{
		param->addListener(this);
	}
}

void MasterChannel::setSliderParameter(juce::String name, juce::Slider& slider)
{
	if (this == nullptr)
		return;

	auto& parameterTreeState = audioProcessor.getParameterTreeState();
	auto* param = parameterTreeState.getParameter(name);

	if (param != nullptr)
	{
		float value = static_cast<float>(slider.getValue());
		if (!std::isnan(value) && !std::isinf(value))
		{
			if (name == "masterHigh" || name == "masterMid" || name == "masterLow")
			{
				value = (value + 12.0f) / 24.0f;
			}
			else if (name == "masterPan")
			{
				value = (value + 1.0f) / 2.0f;
			}
			param->setValueNotifyingHost(value);
		}
	}
}

void MasterChannel::addEventListeners()
{
	masterVolumeSlider.onValueChange = [this]()
		{
			setSliderParameter("masterVolume", masterVolumeSlider);
		};
	masterPanKnob.onValueChange = [this]()
		{
			setSliderParameter("masterPan", masterPanKnob);
		};
	highKnob.onValueChange = [this]()
		{
			setSliderParameter("masterHigh", highKnob);
		};
	midKnob.onValueChange = [this]()
		{
			setSliderParameter("masterMid", midKnob);
		};
	lowKnob.onValueChange = [this]()
		{
			setSliderParameter("masterLow", lowKnob);
		};

	masterVolumeSlider.setDoubleClickReturnValue(true, 0.8);
	masterPanKnob.setDoubleClickReturnValue(true, 0.0);
	highKnob.setDoubleClickReturnValue(true, 0.0);
	midKnob.setDoubleClickReturnValue(true, 0.0);
	lowKnob.setDoubleClickReturnValue(true, 0.0);

	addListener("masterVolume");
	addListener("masterPan");
	addListener("masterHigh");
	addListener("masterMid");
	addListener("masterLow");
}

void MasterChannel::setupUI()
{
	addAndMakeVisible(masterVolumeSlider);
	masterVolumeSlider.setRange(0.0, 1.0, 0.01);
	masterVolumeSlider.setValue(0.8);
	masterVolumeSlider.setSliderStyle(juce::Slider::LinearVertical);
	masterVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	masterVolumeSlider.setColour(juce::Slider::thumbColourId, ColourPalette::playArmed);
	masterVolumeSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	masterVolumeSlider.setColour(juce::Slider::backgroundColourId, ColourPalette::backgroundDeep);
	masterVolumeSlider.getProperties().set(
		CustomLookAndFeel::getDrawTicksPropertyId(), 9);
	masterVolumeSlider.getProperties().set(
		CustomLookAndFeel::getDrawTicksSmallPropertyId(), true);

	addAndMakeVisible(masterPanKnob);
	masterPanKnob.setRange(-1.0, 1.0, 0.01);
	masterPanKnob.setValue(0.0);
	masterPanKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	masterPanKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	masterPanKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	masterPanKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(highKnob);
	highKnob.setRange(-12.0, 12.0, 0.1);
	highKnob.setValue(0.0);
	highKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	highKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	highKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	highKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(midKnob);
	midKnob.setRange(-12.0, 12.0, 0.1);
	midKnob.setValue(0.0);
	midKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	midKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	midKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	midKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(lowKnob);
	lowKnob.setRange(-12.0, 12.0, 0.1);
	lowKnob.setValue(0.0);
	lowKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	lowKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	lowKnob.setColour(juce::Slider::rotarySliderFillColourId, ColourPalette::playArmed);
	lowKnob.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundDeep);

	addAndMakeVisible(masterLabel);
	masterLabel.setText("MASTER", juce::dontSendNotification);
	masterLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	masterLabel.setJustificationType(juce::Justification::centred);
	masterLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));

	addAndMakeVisible(highLabel);
	highLabel.setText("HIGH", juce::dontSendNotification);
	highLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	highLabel.setJustificationType(juce::Justification::centred);
	highLabel.setFont(juce::FontOptions(9.0f));

	addAndMakeVisible(midLabel);
	midLabel.setText("MID", juce::dontSendNotification);
	midLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	midLabel.setJustificationType(juce::Justification::centred);
	midLabel.setFont(juce::FontOptions(9.0f));

	addAndMakeVisible(lowLabel);
	lowLabel.setText("LOW", juce::dontSendNotification);
	lowLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	lowLabel.setJustificationType(juce::Justification::centred);
	lowLabel.setFont(juce::FontOptions(9.0f));

	addAndMakeVisible(panLabel);
	panLabel.setText("PAN", juce::dontSendNotification);
	panLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	panLabel.setJustificationType(juce::Justification::centred);
	panLabel.setFont(juce::FontOptions(9.0f));

	masterVolumeSlider.setTooltip("Master output volume");
	masterPanKnob.setTooltip("Master pan balance");
	highKnob.setTooltip("High frequency EQ (-12dB to +12dB)");
	midKnob.setTooltip("Mid frequency EQ (-12dB to +12dB)");
	lowKnob.setTooltip("Low frequency EQ (-12dB to +12dB)");
}

void MasterChannel::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();
	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
	drawMasterVUMeterStereo(g, bounds);
}

void MasterChannel::drawMasterVUMeterStereo(juce::Graphics& g, juce::Rectangle<int> /* bounds */) const
{
	if (masterVUBounds.isEmpty())
		return;

	float meterWidth = 5.0f;
	float meterSpacing = 2.0f;
	float totalWidth = meterWidth * 2 + meterSpacing;

	float startX = masterVUBounds.getX() + (masterVUBounds.getWidth() - totalWidth) / 2.0f;
	float vuStartY = static_cast<float>(masterVUBounds.getY()) + 4.0f;
	float vuHeight = static_cast<float>(masterVUBounds.getHeight()) - 4.0f;

	auto vuAreaLeft = juce::Rectangle<float>(startX, vuStartY, meterWidth, vuHeight);
	auto vuAreaRight = juce::Rectangle<float>(startX + meterWidth + meterSpacing, vuStartY, meterWidth, vuHeight);

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(vuAreaLeft, 2.0f);
	g.fillRoundedRectangle(vuAreaRight, 2.0f);

	g.setColour(ColourPalette::backgroundLight);
	g.drawRoundedRectangle(vuAreaLeft, 2.0f, 0.5f);
	g.drawRoundedRectangle(vuAreaRight, 2.0f, 0.5f);

	int numSegments = 20;
	float segmentHeight = (vuAreaLeft.getHeight() - 4) / numSegments;

	for (int i = 0; i < numSegments; ++i)
		fillMasterMeterSegment(g, vuAreaLeft, i, segmentHeight, numSegments, masterLevelLeft);

	if (masterPeakHoldLeft > 0.0f)
	{
		int peakSegment = (int)(masterPeakHoldLeft * numSegments);
		if (peakSegment < numSegments)
		{
			float peakY = vuAreaLeft.getBottom() - 2 - (peakSegment + 1) * segmentHeight;
			g.setColour(ColourPalette::vuPeak);
			g.fillRect(juce::Rectangle<float>(vuAreaLeft.getX() + 1, peakY, vuAreaLeft.getWidth() - 2, 2));
		}
	}

	for (int i = 0; i < numSegments; ++i)
		fillMasterMeterSegment(g, vuAreaRight, i, segmentHeight, numSegments, masterLevelRight);

	if (masterPeakHoldRight > 0.0f)
	{
		int peakSegment = (int)(masterPeakHoldRight * numSegments);
		if (peakSegment < numSegments)
		{
			float peakY = vuAreaRight.getBottom() - 2 - (peakSegment + 1) * segmentHeight;
			g.setColour(ColourPalette::vuPeak);
			g.fillRect(juce::Rectangle<float>(vuAreaRight.getX() + 1, peakY, vuAreaRight.getWidth() - 2, 2));
		}
	}

	if (masterLevelLeft >= 0.98f || masterLevelRight >= 0.98f)
	{
		auto clipRect = juce::Rectangle<float>(
			startX - 2, vuStartY - 12, totalWidth + 4, 8);
		g.setColour((juce::Time::getCurrentTime().toMilliseconds() % 500 < 250)
			? ColourPalette::buttonDangerLight
			: ColourPalette::buttonDangerDark);
		g.fillRoundedRectangle(clipRect, 4.0f);
		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
		g.drawText("CLIP", clipRect, juce::Justification::centred);
	}
}

void MasterChannel::fillMasterMeterSegment(juce::Graphics& g, juce::Rectangle<float>& vuArea,
	int i, float segmentHeight, int numSegments,
	float currentLevel) const
{
	float segmentY = vuArea.getBottom() - 2 - (i + 1) * segmentHeight;
	float segmentLevel = (float)i / numSegments;

	juce::Rectangle<float> segmentRect(
		vuArea.getX() + 1, segmentY, vuArea.getWidth() - 2, segmentHeight - 1);

	juce::Colour segmentColour;
	if (segmentLevel < 0.67f)
		segmentColour = ColourPalette::vuGreen;
	else if (segmentLevel < 0.90f)
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

void MasterChannel::resized()
{
	auto area = getLocalBounds().reduced(4);
	int width = area.getWidth();

	masterLabel.setBounds(area.removeFromTop(20));
	area.removeFromTop(5);

	const int colSpacing = 4;
	const int knobColWidth = (width - colSpacing * 2) / 3;

	auto leftCol = area.removeFromLeft(knobColWidth);
	area.removeFromLeft(colSpacing);
	auto rightCol = area.removeFromRight(knobColWidth);
	area.removeFromRight(colSpacing);
	auto centerCol = area;

	const int knobSize = 32;
	const int knobSectionH = 50;

	auto placeEqKnob = [&](juce::Rectangle<int> sec, juce::Label& label, juce::Slider& knob)
		{
			const int labelH = 13;
			const int kSize = juce::jmin(knobSize, sec.getHeight() - labelH - 4);
			int totalH = labelH + 3 + kSize;
			int startY = sec.getY() + (sec.getHeight() - totalH) / 2;
			label.setBounds(sec.getX(), startY, sec.getWidth(), labelH);
			knob.setBounds(sec.withSizeKeepingCentre(kSize, kSize)
				.withY(startY + labelH + 3));
		};
	leftCol.removeFromTop(8);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), highLabel, highKnob);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), midLabel, midKnob);
	placeEqKnob(leftCol.removeFromTop(knobSectionH), lowLabel, lowKnob);

	int faderWidth = juce::jmax(38, centerCol.getWidth() * 3 / 4);
	int centerX = centerCol.getX() + (centerCol.getWidth() - faderWidth) / 2;
	masterVolumeSlider.setBounds(centerX, centerCol.getY() + 8,
		faderWidth, centerCol.getHeight() - 16);

	const int panAreaH = 55;
	auto panArea = rightCol.removeFromBottom(panAreaH);
	auto vuArea = rightCol;

	panLabel.setBounds(panArea.removeFromTop(12).withSizeKeepingCentre(knobColWidth, 12));
	int panKnobSize = juce::jmin(36, panArea.getWidth());
	masterPanKnob.setBounds(panArea.withSizeKeepingCentre(panKnobSize, panKnobSize));

	masterVUBounds = vuArea.reduced(vuArea.getWidth() / 4, 8);
}

void MasterChannel::updateMasterLevels()
{
	float instantLevelLeft = realAudioLevelLeft;
	float instantLevelRight = realAudioLevelRight;

	if (instantLevelLeft > masterLevelLeft)
	{
		masterLevelLeft = instantLevelLeft;
	}
	else
	{
		masterLevelLeft = masterLevelLeft * 0.92f + instantLevelLeft * 0.08f;
	}

	if (masterLevelLeft > masterPeakHoldLeft)
	{
		masterPeakHoldLeft = masterLevelLeft;
		masterPeakHoldTimerLeft = 45;
	}
	else if (masterPeakHoldTimerLeft > 0)
	{
		masterPeakHoldTimerLeft--;
	}
	else
	{
		masterPeakHoldLeft *= 0.98f;
	}

	if (instantLevelRight > masterLevelRight)
	{
		masterLevelRight = instantLevelRight;
	}
	else
	{
		masterLevelRight = masterLevelRight * 0.92f + instantLevelRight * 0.08f;
	}

	if (masterLevelRight > masterPeakHoldRight)
	{
		masterPeakHoldRight = masterLevelRight;
		masterPeakHoldTimerRight = 45;
	}
	else if (masterPeakHoldTimerRight > 0)
	{
		masterPeakHoldTimerRight--;
	}
	else
	{
		masterPeakHoldRight *= 0.98f;
	}

	juce::MessageManager::callAsync([this]()
		{ repaint(); });
}

void MasterChannel::learn(juce::String param, juce::String description, MidiLearnableBase* component, std::function<void(float)> uiCallback)
{
	if (audioProcessor.getActiveEditor())
	{
		juce::MessageManager::callAsync([this, description]()
			{
				if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
				{
					editor->statusLabel.setText("Learning MIDI for " + description + "...", juce::dontSendNotification);
					editor->updateLCD();
				} });
				audioProcessor.getMidiLearnManager()
					.startLearning(param, &audioProcessor, uiCallback, description, component);
	}
}

void MasterChannel::removeMidiMapping(const juce::String& param)
{

	audioProcessor.getMidiLearnManager().removeMappingForParameter(param);
}

void MasterChannel::setRealAudioLevelStereo(float levelLeft, float levelRight)
{
	realAudioLevelLeft = juce::jlimit(0.0f, 1.0f, levelLeft);
	realAudioLevelRight = juce::jlimit(0.0f, 1.0f, levelRight);
	hasRealAudio = true;
}

void MasterChannel::setupMidiLearn()
{
	masterVolumeSlider.onMidiLearn = [this]()
		{
			learn("masterVolume", "Master Volume", &masterVolumeSlider);
		};
	masterPanKnob.onMidiLearn = [this]()
		{
			learn("masterPan", "Master Pan", &masterPanKnob);
		};
	highKnob.onMidiLearn = [this]()
		{
			learn("masterHigh", "Master High EQ", &highKnob);
		};
	midKnob.onMidiLearn = [this]()
		{
			learn("masterMid", "Master Mid EQ", &midKnob);
		};
	lowKnob.onMidiLearn = [this]()
		{
			learn("masterLow", "Master Low EQ", &lowKnob);
		};

	masterVolumeSlider.onMidiRemove = [this]()
		{
			removeMidiMapping("masterVolume");
		};

	masterPanKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("masterPan");
		};

	highKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("masterHigh");
		};

	midKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("masterMid");
		};

	lowKnob.onMidiRemove = [this]()
		{
			removeMidiMapping("masterLow");
		};
}