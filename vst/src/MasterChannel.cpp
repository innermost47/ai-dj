#pragma once
#include "MasterChannel.h"
#include "PluginEditor.h"
#include "ColourPalette.h"

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
	masterLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

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
	g.setColour(ColourPalette::backgroundMid);
	g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
	g.setColour(ColourPalette::playArmed);
	g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, 2.0f);
	drawMasterVUMeter(g, bounds);
}

void MasterChannel::resized()
{
	auto area = getLocalBounds().reduced(4);
	int width = area.getWidth();

	masterLabel.setBounds(area.removeFromTop(20));
	area.removeFromTop(10);

	auto eqArea = area.removeFromTop(100);
	int knobSize = 35;

	auto highRow = eqArea.removeFromTop(50);
	highLabel.setBounds((width - 50) / 2, highRow.getY(), 50, 12);
	highKnob.setBounds((width - knobSize) / 2, highRow.getY() + 15, knobSize, knobSize);

	eqArea.removeFromTop(5);

	auto bottomRow = eqArea.removeFromTop(50);
	int spacing = width / 4;

	midLabel.setBounds(spacing - 25, bottomRow.getY(), 50, 12);
	midKnob.setBounds(spacing - knobSize / 2, bottomRow.getY() + 15, knobSize, knobSize);

	lowLabel.setBounds(width - spacing - 25, bottomRow.getY(), 50, 12);
	lowKnob.setBounds(width - spacing - knobSize / 2, bottomRow.getY() + 15, knobSize, knobSize);

	auto volumeArea = area.removeFromTop(372);
	int faderWidth = width / 3;
	int centerX = (width - faderWidth) / 2;
	masterVolumeSlider.setBounds(centerX, volumeArea.getY() + 5, faderWidth, volumeArea.getHeight() - 10);

	area.removeFromTop(5);

	auto panArea = area.removeFromTop(60);
	auto vuZone = panArea.removeFromRight(10);
	auto knobZone = panArea;
	panLabel.setBounds(knobZone.removeFromTop(12));
	masterPanKnob.setBounds(knobZone.reduced(2));
}

void MasterChannel::drawMasterVUMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const
{
	float width = static_cast<float>(bounds.getWidth());
	float height = static_cast<float>(bounds.getHeight());
	auto vuArea = juce::Rectangle<float>(width - 12.0f, 40.0f, 6.0f, height - 80.0f);
	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(vuArea, 2.0f);
	g.setColour(ColourPalette::playArmed);
	g.drawRoundedRectangle(vuArea, 2.0f, 1.0f);

	int numSegments = 25;
	float segmentHeight = (vuArea.getHeight() - 4) / numSegments;

	for (int i = 0; i < numSegments; ++i)
	{
		drawMasterChanelSegments(vuArea, i, segmentHeight, numSegments, g);
	}

	if (masterPeakHold > 0.0f)
	{
		drawPeakHoldLine(numSegments, vuArea, segmentHeight, g);
	}

	if (masterPeakHold >= 0.98f)
	{
		drawMasterClipping(vuArea, g);
	}
}

void MasterChannel::drawPeakHoldLine(int numSegments, juce::Rectangle<float>& vuArea, float segmentHeight, juce::Graphics& g) const
{
	int peakSegment = (int)(masterPeakHold * numSegments);
	if (peakSegment < numSegments)
	{
		float peakY = vuArea.getBottom() - 2 - (peakSegment + 1) * segmentHeight;
		juce::Rectangle<float> peakRect(vuArea.getX() + 1, peakY, vuArea.getWidth() - 2, 3);
		g.setColour(ColourPalette::vuPeak);
		g.fillRect(peakRect);
	}
}

void MasterChannel::drawMasterClipping(juce::Rectangle<float>& vuArea, juce::Graphics& g) const
{
	auto clipRect = juce::Rectangle<float>(vuArea.getX() - 2, vuArea.getY() - 12, vuArea.getWidth() + 4, 8);
	g.setColour(isClipping && (juce::Time::getCurrentTime().toMilliseconds() % 500 < 250)
		? ColourPalette::buttonDangerLight
		: ColourPalette::buttonDangerDark);
	g.fillRoundedRectangle(clipRect, 4.0f);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
	g.drawText("CLIP", clipRect, juce::Justification::centred);
}

void MasterChannel::drawMasterChanelSegments(juce::Rectangle<float>& vuArea, int i, float segmentHeight, int numSegments, juce::Graphics& g) const
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

	if (masterLevel >= segmentLevel)
	{
		g.setColour(segmentColour);
		g.fillRoundedRectangle(segmentRect, 1.0f);
	}
	else
	{
		g.setColour(segmentColour.withAlpha(0.05f));
		g.fillRoundedRectangle(segmentRect, 1.0f);
	}
}

void MasterChannel::setRealAudioLevel(float level)
{
	realAudioLevel = juce::jlimit(0.0f, 1.0f, level);
	hasRealAudio = true;
}

void MasterChannel::updateMasterLevels()
{
	float instantLevel;

	if (hasRealAudio)
	{
		instantLevel = realAudioLevel;
	}
	else
	{
		static float phase = 0.0f;
		phase += 0.05f;
		instantLevel = (std::sin(phase) * 0.3f + 0.3f) * 0.5f;
	}

	if (instantLevel > masterLevel)
	{
		masterLevel = masterLevel * 0.7f + instantLevel * 0.3f;
	}
	else
	{
		masterLevel = masterLevel * 0.95f + instantLevel * 0.05f;
	}

	if (masterLevel > masterPeakHold)
	{
		masterPeakHold = masterLevel;
		masterPeakHoldTimer = 60;
	}
	else if (masterPeakHoldTimer > 0)
	{
		masterPeakHoldTimer--;
	}
	else
	{
		masterPeakHold *= 0.98f;
	}

	isClipping = (masterPeakHold >= 0.95f);

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
				} });
				audioProcessor.getMidiLearnManager()
					.startLearning(param, &audioProcessor, uiCallback, description, component);
	}
}

void MasterChannel::removeMidiMapping(const juce::String& param)
{

	audioProcessor.getMidiLearnManager().removeMappingForParameter(param);
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