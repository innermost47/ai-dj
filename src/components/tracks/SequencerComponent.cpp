#include "SequencerComponent.h"
#include "PluginProcessor.h"

SequencerComponent::SequencerComponent(const juce::String &trackId, DjIaVstProcessor &processor)
    : trackId(trackId), audioProcessor(processor)
{
	setupUI();
	updateFromTrackData();
	setupSequenceButtons();
}

SequencerComponent::~SequencerComponent()
{
	setVisible(false);

	for (int i = 0; i < 4; ++i)
		removeChildComponent(&measureButtons[i]);
	removeChildComponent(&prevMeasureButton);
	removeChildComponent(&nextMeasureButton);
	removeChildComponent(&measureLabel);
	removeChildComponent(&currentPlayingMeasureLabel);
	for (int i = 0; i < 8; ++i)
		removeChildComponent(&sequenceButtons[i]);

	prevMeasureButton.setLookAndFeel(nullptr);
	nextMeasureButton.setLookAndFeel(nullptr);
	currentPlayingMeasureLabel.setLookAndFeel(nullptr);

	for (int i = 0; i < 8; ++i)
		sequenceButtons[i].setLookAndFeel(nullptr);
}

void SequencerComponent::setupUI()
{
	for (int i = 0; i < 4; ++i)
	{
		measureButtons[i].setButtonText(juce::String(i + 1));
		measureButtons[i].setClickingTogglesState(false);
		measureButtons[i].setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
		measureButtons[i].setColour(juce::TextButton::buttonOnColourId, accentColour);
		measureButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		measureButtons[i].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		measureButtons[i].setTooltip("Set pattern length to " + juce::String(i + 1) + " measure(s)");
		measureButtons[i].onClick = [this, i]()
		{
			isEditing = true;
			setNumMeasures(i + 1);
			updateMeasureButtonsDisplay();
			juce::Timer::callAfterDelay(500, [this]() { isEditing = false; });
		};
		addAndMakeVisible(measureButtons[i]);
	}
	updateMeasureButtonsDisplay();

	addAndMakeVisible(prevMeasureButton);
	prevMeasureButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	prevMeasureButton.setColour(juce::TextButton::textColourOffId, accentColour);
	prevMeasureButton.onClick = [this]()
	{
		isEditing = true;
		if (currentMeasure > 0)
		{
			setCurrentMeasure(currentMeasure - 1);
		}
		juce::Timer::callAfterDelay(500, [this]() { isEditing = false; });
	};

	addAndMakeVisible(nextMeasureButton);
	nextMeasureButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	nextMeasureButton.setColour(juce::TextButton::textColourOffId, accentColour);
	nextMeasureButton.onClick = [this]()
	{
		isEditing = true;
		if (currentMeasure < numMeasures - 1)
		{
			setCurrentMeasure(currentMeasure + 1);
		}
		juce::Timer::callAfterDelay(500, [this]() { isEditing = false; });
	};
	prevMeasureButton.loadIcon(BinaryData::left_svg, BinaryData::left_svgSize);
	nextMeasureButton.loadIcon(BinaryData::right_svg, BinaryData::right_svgSize);
	prevMeasureButton.setShowBackground(false);
	nextMeasureButton.setShowBackground(false);
	addAndMakeVisible(measureLabel);
	measureLabel.setText("1/1", juce::dontSendNotification);
	measureLabel.setJustificationType(juce::Justification::centred);
	measureLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);

	addAndMakeVisible(currentPlayingMeasureLabel);
	currentPlayingMeasureLabel.setText("M 1", juce::dontSendNotification);
	currentPlayingMeasureLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	currentPlayingMeasureLabel.setColour(juce::Label::backgroundColourId, ColourPalette::lightGrey.withAlpha(0.1f));
	currentPlayingMeasureLabel.setJustificationType(juce::Justification::centred);
	currentPlayingMeasureLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

	prevMeasureButton.setTooltip("Previous measure - Navigate to edit earlier patterns");
	nextMeasureButton.setTooltip("Next measure - Navigate to edit later patterns");

	currentPlayingMeasureLabel.setLookAndFeel(&roundedLabelLF);
}

void SequencerComponent::updateMeasureButtonsDisplay()
{
	for (int i = 0; i < 4; ++i)
	{
		bool active = (i + 1 == numMeasures);
		measureButtons[i].setColour(juce::TextButton::buttonColourId,
		                            active ? accentColour : ColourPalette::backgroundDeep);
		measureButtons[i].setColour(
		    juce::TextButton::textColourOffId,
		    active ? (accentColour.getBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white)
		           : ColourPalette::textSecondary);
		measureButtons[i].repaint();
	}
}

void SequencerComponent::setupSequenceButtons()
{
	TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
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

		sequenceButtons[i].setColour(juce::TextButton::buttonColourId, accentColour.withAlpha(0.2f));
		sequenceButtons[i].setColour(juce::TextButton::buttonOnColourId, accentColour);
		sequenceButtons[i].setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		sequenceButtons[i].setColour(juce::TextButton::textColourOnId,
		                             accentColour.getBrightness() > 0.6f ? juce::Colours::black : juce::Colours::white);

		sequenceButtons[i].onClick = [this, i]() { onSequenceSelected(i); };

		sequenceButtons[i].onMidiLearn = [this, i]()
		{
			TrackData *t = audioProcessor.getTrackManager().getTrack(trackId);
			if (t && t->slotIndex != -1)
			{
				juce::String paramName = "slot" + juce::String(t->slotIndex + 1) + "Seq" + juce::String(i + 1);
				juce::String description =
				    "Slot " + juce::String(t->slotIndex + 1) + " Sequence " + juce::String(i + 1);
				audioProcessor.getMidiLearnManager().startLearning(paramName, &audioProcessor, nullptr, description,
				                                                   &sequenceButtons[i]);
			}
		};

		sequenceButtons[i].onMidiRemove = [this, i]()
		{
			TrackData *t = audioProcessor.getTrackManager().getTrack(trackId);
			if (t && t->slotIndex != -1)
			{
				juce::String paramName = "slot" + juce::String(t->slotIndex + 1) + "Seq" + juce::String(i + 1);
				audioProcessor.getMidiLearnManager().removeMappingForParameter(paramName);
			}
		};

		addAndMakeVisible(sequenceButtons[i]);
	}

	updateSequenceButtonsDisplay();
}

void SequencerComponent::updateSequenceButtonsDisplay()
{
	TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
	if (!track)
		return;

	auto &currentPage = track->getCurrentPage();
	int currentSeq = currentPage.currentSequenceIndex;

	for (int i = 0; i < 8; ++i)
	{
		sequenceButtons[i].setToggleState(i == currentSeq, juce::dontSendNotification);
	}
}

void SequencerComponent::layoutSequenceButtons(juce::Rectangle<int> area)
{
	int numButtons = 8;
	int totalSpacing = (numButtons - 1) * 2;
	int buttonWidth = (area.getWidth() - totalSpacing) / numButtons;

	for (int i = 0; i < 8; ++i)
	{
		sequenceButtons[i].setBounds(area.removeFromLeft(buttonWidth));
		if (i < 7)
			area.removeFromLeft(2);
	}
}

void SequencerComponent::onSequenceSelected(int seqIndex)
{
	TrackData *track = audioProcessor.getTrackManager().getTrack(trackId);
	if (!track || seqIndex < 0 || seqIndex >= 8)
		return;

	auto &currentPage = track->getCurrentPage();
	currentPage.currentSequenceIndex = seqIndex;

	updateSequenceButtonsDisplay();
	updateFromTrackData();
	repaint();

	if (track->slotIndex != -1)
	{
		juce::String paramName = "slot" + juce::String(track->slotIndex + 1) + "Seq" + juce::String(seqIndex + 1);
		auto *param = audioProcessor.getParameterTreeState().getParameter(paramName);
		if (param)
		{
			param->setValueNotifyingHost(1.0f);
		}
	}
}

void SequencerComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	g.setColour(accentColour.withAlpha(0.08f));
	g.fillRoundedRectangle(bounds.toFloat(), ObsidianSizes::CORNER);

	juce::Colour beatColour = ColourPalette::sequencerBeat;
	juce::Colour subBeatColour = ColourPalette::sequencerSubBeat;

	TrackData *track = audioProcessor.getTrack(trackId);
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

	auto &seqData = track->getCurrentSequencerData();
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
			float pulseIntensity = 0.7f + 0.3f * std::sin(juce::Time::getMillisecondCounter() * 0.01f);

			if (seqData.steps[safeMeasure][i])
			{
				stepColour = accentColour.brighter(0.6f).withAlpha(pulseIntensity);
				borderColour = accentColour.brighter(0.9f);
			}
			else
			{
				stepColour = accentColour.withAlpha(0.35f * pulseIntensity);
				borderColour = accentColour.withAlpha(0.7f);
			}
		}

		g.setColour(stepColour);

		const float pillRadius = stepBounds.toFloat().getHeight() * 0.5f;
		g.fillRoundedRectangle(stepBounds.toFloat(), pillRadius);
		g.setColour(borderColour);
		g.drawRoundedRectangle(stepBounds.toFloat(), pillRadius, isVisible ? 0.8f : 0.4f);
	}
}

void SequencerComponent::setAccentColour(juce::Colour colour)
{
	accentColour = colour;

	for (int i = 0; i < 8; ++i)
	{
		sequenceButtons[i].setColour(juce::TextButton::buttonOnColourId, colour);
		sequenceButtons[i].setColour(juce::TextButton::buttonColourId, colour.withAlpha(0.1f));
	}

	prevMeasureButton.setColour(juce::TextButton::textColourOffId, colour);
	nextMeasureButton.setColour(juce::TextButton::textColourOffId, colour);
	prevMeasureButton.repaint();
	nextMeasureButton.repaint();

	updateMeasureButtonsDisplay();

	currentPlayingMeasureLabel.setColour(juce::Label::backgroundColourId, colour.brighter(0.2f));
	currentPlayingMeasureLabel.setColour(juce::Label::textColourId,
	                                     colour.getBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white);

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

	int stepHeight = 12;

	int totalUsedWidth = totalSteps * stepWidth + (totalSteps - 1) * marginPixels;
	int startX = (componentWidth - totalUsedWidth) / 2;
	int availableHeight = componentHeight - 25;
	int startY = (availableHeight - stepHeight) / 2;

	int x = startX + step * (stepWidth + marginPixels);
	int y = startY;

	return juce::Rectangle<int>(x, y, stepWidth, stepHeight);
}

void SequencerComponent::mouseDown(const juce::MouseEvent &event)
{
	int totalSteps = getTotalStepsForCurrentSignature();

	for (int i = 0; i < totalSteps; ++i)
	{
		if (getStepBounds(i).contains(event.getPosition()))
		{
			isEditing = true;
			toggleStep(i);
			repaint();
			juce::Timer::callAfterDelay(50, [this]() { isEditing = false; });
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
	TrackData *track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto &seqData = track->getCurrentSequencerData();
		int safeMeasure = juce::jlimit(0, MAX_MEASURES - 1, currentMeasure);

		seqData.steps[safeMeasure][step] = !seqData.steps[safeMeasure][step];
		seqData.velocities[safeMeasure][step] = 0.8f;
	}
}

void SequencerComponent::resized()
{
	auto bounds = getLocalBounds();
	bounds.removeFromLeft(13);
	bounds.removeFromRight(11);

	auto controlsArea = bounds.removeFromBottom(22);
	controlsArea.removeFromBottom(2);
	controlsArea = controlsArea.reduced(2, 0);

	prevMeasureButton.setBounds(controlsArea.removeFromLeft(18));
	measureLabel.setBounds(controlsArea.removeFromLeft(32));
	nextMeasureButton.setBounds(controlsArea.removeFromLeft(18));
	controlsArea.removeFromLeft(6);

	for (int i = 0; i < 4; ++i)
	{
		measureButtons[i].setBounds(controlsArea.removeFromLeft(18));
		if (i < 3)
			controlsArea.removeFromLeft(2);
	}
	controlsArea.removeFromLeft(10);

	currentPlayingMeasureLabel.setBounds(controlsArea.removeFromLeft(28));
	controlsArea.removeFromLeft(10);

	layoutSequenceButtons(controlsArea);
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

	TrackData *track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto &seqData = track->getCurrentSequencerData();
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

	TrackData *track = audioProcessor.getTrack(trackId);
	if (track)
	{
		auto &seqData = track->getCurrentSequencerData();

		int totalSteps = getTotalStepsForCurrentSignature();
		currentStep = juce::jlimit(0, totalSteps - 1, seqData.currentStep);
		isPlaying = track->isCurrentlyPlaying;
		numMeasures = seqData.numMeasures;
		updateMeasureButtonsDisplay();
		measureLabel.setText(juce::String(currentMeasure + 1) + "/" + juce::String(numMeasures),
		                     juce::dontSendNotification);

		if (isPlaying)
		{
			int playingMeasure = seqData.currentMeasure + 1;
			currentPlayingMeasureLabel.setText("M " + juce::String(playingMeasure), juce::dontSendNotification);
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