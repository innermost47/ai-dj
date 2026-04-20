#include "SequencerComponent.h"
#include "PluginProcessor.h"
#include "ColourPalette.h"

SequencerComponent::SequencerComponent(const juce::String& trackId, DjIaVstProcessor& processor)
	: trackId(trackId), audioProcessor(processor)
{
	setupUI();
	updateFromTrackData();
	setupSequenceButtons();
}

SequencerComponent::~SequencerComponent()
{
	prevMeasureButton.setLookAndFeel(nullptr);
	nextMeasureButton.setLookAndFeel(nullptr);
}

void SequencerComponent::setupUI()
{
	addAndMakeVisible(measureSlider);
	measureSlider.setRange(1, 4, 1);
	measureSlider.setValue(1);
	measureSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 30, 20);
	measureSlider.setDoubleClickReturnValue(true, 1);
	measureSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::black);
	measureSlider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
	measureSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	measureSlider.setColour(juce::Slider::textBoxTextColourId, ColourPalette::textPrimary);
	measureSlider.setColour(juce::Slider::textBoxBackgroundColourId, ColourPalette::backgroundDark);
	measureSlider.setColour(juce::Slider::textBoxOutlineColourId, ColourPalette::backgroundDark.darker(0.3f).withAlpha(0.3f));

	measureSlider.setTooltip("Number of measures (1-4) - Extends the pattern length");

	measureSlider.onValueChange = [this]()
		{
			isEditing = true;
			setNumMeasures((int)measureSlider.getValue());
			juce::Timer::callAfterDelay(500, [this]()
				{ isEditing = false; });
		};

	addAndMakeVisible(prevMeasureButton);
	prevMeasureButton.setButtonText("<");
	prevMeasureButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
	prevMeasureButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	prevMeasureButton.onClick = [this]()
		{
			isEditing = true;
			if (currentMeasure > 0)
			{
				setCurrentMeasure(currentMeasure - 1);
			}
			juce::Timer::callAfterDelay(500, [this]()
				{ isEditing = false; });
		};

	addAndMakeVisible(nextMeasureButton);
	nextMeasureButton.setButtonText(">");
	nextMeasureButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
	nextMeasureButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	nextMeasureButton.onClick = [this]()
		{
			isEditing = true;
			if (currentMeasure < numMeasures - 1)
			{
				setCurrentMeasure(currentMeasure + 1);
			}
			juce::Timer::callAfterDelay(500, [this]()
				{ isEditing = false; });
		};
	prevMeasureButton.setLookAndFeel(&flatLF);
	nextMeasureButton.setLookAndFeel(&flatLF);
	addAndMakeVisible(measureLabel);
	measureLabel.setText("1/1", juce::dontSendNotification);
	measureLabel.setJustificationType(juce::Justification::centred);
	measureLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);

	addAndMakeVisible(currentPlayingMeasureLabel);
	currentPlayingMeasureLabel.setText("M 1", juce::dontSendNotification);
	currentPlayingMeasureLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	currentPlayingMeasureLabel.setColour(juce::Label::backgroundColourId, ColourPalette::trackSelected.withAlpha(0.1f));
	currentPlayingMeasureLabel.setJustificationType(juce::Justification::centred);
	currentPlayingMeasureLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

	prevMeasureButton.setTooltip("Previous measure - Navigate to edit earlier patterns");
	nextMeasureButton.setTooltip("Next measure - Navigate to edit later patterns");
}

void SequencerComponent::setupSequenceButtons()
{
	TrackData* track = audioProcessor.trackManager.getTrack(trackId);
	if (!track || track->slotIndex == -1)
		return;

	int groupId = 2000 + track->slotIndex;

	for (int i = 0; i < 8; ++i)
	{
		sequenceButtons[i].setButtonText(juce::String(i + 1));
		sequenceButtons[i].setClickingTogglesState(true);
		sequenceButtons[i].setRadioGroupId(groupId);

		sequenceButtons[i].setTooltip("Select sequence " + juce::String(i + 1) +
			" - Each page has 8 independent sequences you can switch between");

		sequenceButtons[i].setColour(juce::TextButton::buttonColourId,
			ColourPalette::backgroundDeep);
		sequenceButtons[i].setColour(juce::TextButton::buttonOnColourId,
			accentColour);
		sequenceButtons[i].setColour(juce::TextButton::textColourOffId,
			ColourPalette::textSecondary);
		sequenceButtons[i].setColour(juce::TextButton::textColourOnId,
			accentColour.getBrightness() > 0.6f
			? juce::Colours::black
			: juce::Colours::white);

		sequenceButtons[i].onClick = [this, i]()
			{
				onSequenceSelected(i);
			};

		sequenceButtons[i].onMidiLearn = [this, i]()
			{
				TrackData* t = audioProcessor.trackManager.getTrack(trackId);
				if (t && t->slotIndex != -1)
				{
					juce::String paramName = "slot" + juce::String(t->slotIndex + 1) +
						"Seq" + juce::String(i + 1);
					juce::String description = "Slot " + juce::String(t->slotIndex + 1) +
						" Sequence " + juce::String(i + 1);
					audioProcessor.getMidiLearnManager().startLearning(
						paramName, &audioProcessor, nullptr, description, &sequenceButtons[i]);
				}
			};

		sequenceButtons[i].onMidiRemove = [this, i]()
			{
				TrackData* t = audioProcessor.trackManager.getTrack(trackId);
				if (t && t->slotIndex != -1)
				{
					juce::String paramName = "slot" + juce::String(t->slotIndex + 1) +
						"Seq" + juce::String(i + 1);
					audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName);
				}
			};

		addAndMakeVisible(sequenceButtons[i]);
	}

	updateSequenceButtonsDisplay();
}

void SequencerComponent::updateSequenceButtonsDisplay()
{
	TrackData* track = audioProcessor.trackManager.getTrack(trackId);
	if (!track)
		return;

	auto& currentPage = track->getCurrentPage();
	int currentSeq = currentPage.currentSequenceIndex;

	for (int i = 0; i < 8; ++i)
	{
		sequenceButtons[i].setToggleState(i == currentSeq, juce::dontSendNotification);
	}
}

void SequencerComponent::layoutSequenceButtons(juce::Rectangle<int> area)
{
	int totalWidth = area.getWidth() - 8;
	int numButtons = 8;
	int totalSpacing = (numButtons - 1) * 2;
	int availableWidth = totalWidth - totalSpacing;
	int buttonWidth = availableWidth / numButtons;
	int buttonHeight = 28;

	for (int i = 0; i < 8; ++i)
	{
		auto buttonBounds = area.removeFromLeft(buttonWidth).withHeight(buttonHeight);
		sequenceButtons[i].setBounds(buttonBounds);

		if (i < 7)
			area.removeFromLeft(2);
	}
}

void SequencerComponent::onSequenceSelected(int seqIndex)
{
	TrackData* track = audioProcessor.trackManager.getTrack(trackId);
	if (!track || seqIndex < 0 || seqIndex >= 8)
		return;

	auto& currentPage = track->getCurrentPage();
	currentPage.currentSequenceIndex = seqIndex;

	updateSequenceButtonsDisplay();
	updateFromTrackData();
	repaint();

	if (track->slotIndex != -1)
	{
		juce::String paramName = "slot" + juce::String(track->slotIndex + 1) +
			"Seq" + juce::String(seqIndex + 1);
		auto* param = audioProcessor.getParameterTreeState().getParameter(paramName);
		if (param)
		{
			param->setValueNotifyingHost(1.0f);
		}
	}
}

void SequencerComponent::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();

	g.setColour(accentColour.withAlpha(0.08f));
	g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

	juce::Colour beatColour = ColourPalette::sequencerBeat;
	juce::Colour subBeatColour = ColourPalette::sequencerSubBeat;

	TrackData* track = audioProcessor.getTrack(trackId);
	if (!track)
	{
		g.setColour(ColourPalette::textDanger);
		g.drawText("Track not found", getLocalBounds(), juce::Justification::centred);
		return;
	}

	int numerator = audioProcessor.getTimeSignatureNumerator();
	int denominator = audioProcessor.getTimeSignatureDenominator();

	int stepsPerBeat;
	if (denominator == 8)
	{
		stepsPerBeat = 4;
	}
	else if (denominator == 4)
	{
		stepsPerBeat = 4;
	}
	else if (denominator == 2)
	{
		stepsPerBeat = 8;
	}
	else
	{
		stepsPerBeat = 4;
	}

	int totalSteps = getTotalStepsForCurrentSignature();

	auto& seqData = track->getCurrentSequencerData();
	int playingMeasure = seqData.currentMeasure;
	int safeMeasure = juce::jlimit(0, MAX_MEASURES - 1, currentMeasure);

	for (int i = 0; i < totalSteps; ++i)
	{
		auto stepBounds = getStepBounds(i);
		bool isVisible = (i < totalSteps);
		bool isStrongBeat = false;
		bool isBeat = false;

		if (isVisible)
		{
			if (denominator == 8)
			{
				if (numerator == 6)
				{
					isStrongBeat = (i % 12 == 0);
					isBeat = (i % 6 == 0);
				}
				else if (numerator == 9)
				{
					isStrongBeat = (i % 12 == 0);
					isBeat = (i % 4 == 0);
				}
				else
				{
					isStrongBeat = (i % (stepsPerBeat * 2) == 0);
					isBeat = (i % stepsPerBeat == 0);
				}
			}
			else
			{
				isStrongBeat = (i % stepsPerBeat == 0);
				isBeat = (i % (stepsPerBeat / 2) == 0);
			}
		}

		juce::Colour stepColour;
		juce::Colour borderColour;

		if (!isVisible)
		{
			stepColour = ColourPalette::backgroundDeep;
			borderColour = ColourPalette::backgroundMid;
		}
		else if (seqData.steps[safeMeasure][i])
		{
			stepColour = accentColour;
			borderColour = accentColour.brighter(0.4f);
		}
		else
		{
			if (isStrongBeat)
			{
				stepColour = ColourPalette::backgroundLight;
				borderColour = ColourPalette::backgroundLight.brighter(0.3f);
			}
			else if (isBeat)
			{
				stepColour = ColourPalette::sequencerBeat;
				borderColour = ColourPalette::backgroundLight.withAlpha(0.5f);
			}
			else
			{
				stepColour = ColourPalette::sequencerSubBeat;
				borderColour = ColourPalette::backgroundMid.withAlpha(0.8f);
			}
		}

		if (i == currentStep && isPlaying && isVisible && currentMeasure == playingMeasure)
		{
			float pulseIntensity = 0.8f + 0.2f * std::sin(juce::Time::getMillisecondCounter() * 0.01f);
			stepColour = ColourPalette::textPrimary.withAlpha(pulseIntensity);
			borderColour = ColourPalette::textPrimary;
		}

		g.setColour(stepColour);
		g.fillRoundedRectangle(stepBounds.toFloat(), 3.0f);
		g.setColour(borderColour);
		g.drawRoundedRectangle(stepBounds.toFloat(), 3.0f, isVisible ? 1.0f : 0.5f);

		if (isVisible)
		{
			g.setColour(ColourPalette::textPrimary.withAlpha(isStrongBeat ? 0.9f : 0.6f));
			g.setFont(juce::FontOptions(9.0f, isStrongBeat ? juce::Font::bold : juce::Font::plain));
			g.drawText(juce::String(i + 1), stepBounds, juce::Justification::centred);
		}
	}
}

void SequencerComponent::setAccentColour(juce::Colour colour)
{
	accentColour = colour;

	for (int i = 0; i < 8; ++i)
		sequenceButtons[i].setColour(juce::TextButton::buttonOnColourId, colour.brighter(0.8f));

	repaint();
}

juce::Rectangle<int> SequencerComponent::getStepBounds(int step)
{
	int totalSteps = getTotalStepsForCurrentSignature();

	float stepsAreaWidthPercent = 0.98f;
	float marginPercent = 0.005f;

	int componentWidth = getWidth();
	int componentHeight = getHeight();

	int availableWidth = static_cast<int>(componentWidth * stepsAreaWidthPercent);

	int totalMargins = static_cast<int>((totalSteps - 1) * marginPercent * componentWidth);
	int stepWidth = (availableWidth - totalMargins) / totalSteps;
	int marginPixels = static_cast<int>(marginPercent * componentWidth);

	int stepHeight = juce::jmin(stepWidth, 40);

	int totalUsedWidth = totalSteps * stepWidth + (totalSteps - 1) * marginPixels;
	int startX = (componentWidth - totalUsedWidth) / 2;
	int availableHeight = componentHeight - 30 - 10;
	int startY = (availableHeight - stepHeight) / 2;

	int x = startX + step * (stepWidth + marginPixels);
	int y = startY;

	return juce::Rectangle<int>(x, y, stepWidth, stepHeight);
}

void SequencerComponent::mouseDown(const juce::MouseEvent& event)
{
	int totalSteps = getTotalStepsForCurrentSignature();

	for (int i = 0; i < totalSteps; ++i)
	{
		if (getStepBounds(i).contains(event.getPosition()))
		{
			isEditing = true;
			toggleStep(i);
			repaint();
			juce::Timer::callAfterDelay(50, [this]()
				{ isEditing = false; });
			return;
		}
	}
}

int SequencerComponent::getTotalStepsForCurrentSignature() const
{
	int numerator = audioProcessor.getTimeSignatureNumerator();
	int denominator = audioProcessor.getTimeSignatureDenominator();

	int stepsPerBeat;
	if (denominator == 8)
	{
		stepsPerBeat = 2;
	}
	else if (denominator == 4)
	{
		stepsPerBeat = 4;
	}
	else if (denominator == 2)
	{
		stepsPerBeat = 8;
	}
	else
	{
		stepsPerBeat = 4;
	}

	return numerator * stepsPerBeat;
}

void SequencerComponent::toggleStep(int step)
{
	TrackData* track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto& seqData = track->getCurrentSequencerData();
		int safeMeasure = juce::jlimit(0, MAX_MEASURES - 1, currentMeasure);

		seqData.steps[safeMeasure][step] = !seqData.steps[safeMeasure][step];
		seqData.velocities[safeMeasure][step] = 0.8f;
	}
}

void SequencerComponent::setCurrentStep(int step)
{
	int totalSteps = getTotalStepsForCurrentSignature();
	currentStep = step % totalSteps;
	repaint();
}

void SequencerComponent::resized()
{
	int controlsWidth = 250;
	auto bounds = getLocalBounds();
	bounds.removeFromTop(10);
	bounds.removeFromLeft(13);
	auto controlsArea = bounds.removeFromBottom(40);
	controlsArea = controlsArea.reduced(3);
	controlsArea.removeFromBottom(4);
	auto controlArea = controlsArea.removeFromLeft(juce::jmin(controlsWidth, controlsArea.getWidth() / 2));

	auto pageArea = controlArea.removeFromLeft(120);
	prevMeasureButton.setBounds(pageArea.removeFromLeft(25));
	measureLabel.setBounds(pageArea.removeFromLeft(40));
	nextMeasureButton.setBounds(pageArea.removeFromLeft(25));

	if (controlsArea.getWidth() > 50)
	{
		currentPlayingMeasureLabel.setBounds(controlsArea.removeFromLeft(50));
	}

	controlsArea.removeFromLeft(10);

	auto seqButtonsArea = controlsArea;
	layoutSequenceButtons(seqButtonsArea);

	if (controlArea.getWidth() > 120)
	{
		measureSlider.setBounds(controlArea.removeFromLeft(120));
	}
}

void SequencerComponent::setCurrentMeasure(int measure)
{
	currentMeasure = juce::jlimit(0, numMeasures - 1, measure);
	measureLabel.setText(juce::String(currentMeasure + 1) + "/" + juce::String(numMeasures),
		juce::dontSendNotification);
	repaint();
}

void SequencerComponent::setNumMeasures(int measures)
{
	int oldNumMeasures = numMeasures;
	numMeasures = juce::jlimit(1, MAX_MEASURES, measures);

	if (currentMeasure >= numMeasures)
	{
		setCurrentMeasure(numMeasures - 1);
	}

	TrackData* track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto& seqData = track->getCurrentSequencerData();
		seqData.numMeasures = numMeasures;

		if (numMeasures < oldNumMeasures)
		{
			int maxSteps = getTotalStepsForCurrentSignature();
			for (int m = numMeasures; m < oldNumMeasures; ++m)
			{
				for (int s = 0; s < maxSteps; ++s)
				{
					seqData.steps[m][s] = false;
					seqData.velocities[m][s] = 0.8f;
				}
			}
		}
	}

	measureLabel.setText(juce::String(currentMeasure + 1) + "/" + juce::String(numMeasures),
		juce::dontSendNotification);
	repaint();
}

void SequencerComponent::updateFromTrackData()
{
	if (isEditing)
		return;

	TrackData* track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto& seqData = track->getCurrentSequencerData();

		int totalSteps = getTotalStepsForCurrentSignature();
		currentStep = juce::jlimit(0, totalSteps - 1, seqData.currentStep);
		isPlaying = track->isCurrentlyPlaying;
		numMeasures = seqData.numMeasures;
		measureSlider.setValue(seqData.numMeasures);
		measureLabel.setText(juce::String(currentMeasure + 1) + "/" + juce::String(numMeasures),
			juce::dontSendNotification);

		if (isPlaying)
		{
			int playingMeasure = seqData.currentMeasure + 1;
			currentPlayingMeasureLabel.setText("M " + juce::String(playingMeasure),
				juce::dontSendNotification);
		}
		else
		{
			seqData.currentStep = 0;
			seqData.currentMeasure = 0;
			currentPlayingMeasureLabel.setText("M " + juce::String(seqData.currentMeasure + 1),
				juce::dontSendNotification);
		}
		updateSequenceButtonsDisplay();
		repaint();
	}
}