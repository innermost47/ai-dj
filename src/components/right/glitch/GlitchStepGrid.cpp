#include "GlitchStepGrid.h"

GlitchStepGrid::GlitchStepGrid()
{
	for (int i = 0; i < GlitchSequence::MAX_STEPS; ++i)
	{
		auto btn = std::make_unique<GlitchStepButton>();
		btn->setAccentColour(accentColour);

		btn->onEffectChanged = [this, i](GlitchEffectType type)
		{
			if (currentSequence && i < currentSequence->getNumSteps())
			{
				currentSequence->setStep(i, type);
				if (onStepChanged)
					onStepChanged();
			}
		};

		addAndMakeVisible(*btn);
		stepButtons[i] = std::move(btn);
	}
}

void GlitchStepGrid::setSequence(GlitchSequence *sequence)
{
	currentSequence = sequence;
	refreshFromSequence();
}

void GlitchStepGrid::refreshFromSequence()
{
	const int n = currentSequence ? currentSequence->getNumSteps() : 0;
	for (int i = 0; i < GlitchSequence::MAX_STEPS; ++i)
	{
		bool visible = currentSequence != nullptr && i < n;
		stepButtons[i]->setVisible(visible);
		if (visible)
			stepButtons[i]->setEffectType(currentSequence->getStep(i));
	}
	resized();
}

void GlitchStepGrid::setAccentColour(juce::Colour colour)
{
	accentColour = colour;
	for (auto &btn : stepButtons)
		btn->setAccentColour(colour);
}

void GlitchStepGrid::setCurrentPlaybackStep(int stepIndex)
{
	for (int i = 0; i < GlitchSequence::MAX_STEPS; ++i)
		stepButtons[i]->setStepActive(i == stepIndex);
}

void GlitchStepGrid::setStepsPerBar(int newStepsPerBar)
{
	newStepsPerBar = juce::jlimit(1, GlitchSequence::MAX_STEPS, newStepsPerBar);
	if (stepsPerRow == newStepsPerBar)
		return;
	stepsPerRow = newStepsPerBar;
	resized();
}

int GlitchStepGrid::getRequiredHeight() const
{
	int numSteps = currentSequence ? currentSequence->getNumSteps() : 16;
	int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;
	return juce::jmax(1, numRows) * stepHeight;
}

void GlitchStepGrid::setStepsPerBeat(int newStepsPerBeat)
{
	newStepsPerBeat = juce::jlimit(1, GlitchSequence::MAX_STEPS, newStepsPerBeat);
	if (stepsPerBeat == newStepsPerBeat)
		return;
	stepsPerBeat = newStepsPerBeat;
	refreshFromSequence();
}

void GlitchStepGrid::resized()
{
	if (!currentSequence)
		return;
	const int numSteps = currentSequence->getNumSteps();
	if (numSteps <= 0)
		return;

	int numRows = (numSteps + stepsPerRow - 1) / stepsPerRow;

	auto area = getLocalBounds();

	for (int row = 0; row < numRows; ++row)
	{
		auto rowArea = area.removeFromTop(stepHeight);
		int stepsInThisRow = juce::jmin(stepsPerRow, numSteps - row * stepsPerRow);
		int colWidth = rowArea.getWidth() / juce::jmax(1, stepsInThisRow);

		for (int col = 0; col < stepsInThisRow; ++col)
		{
			int stepIndex = row * stepsPerRow + col;
			bool onBeat = (stepIndex % stepsPerBeat) == 0;
			stepButtons[stepIndex]->setAccent(onBeat ? GlitchStepButton::Accent::Beat : GlitchStepButton::Accent::None);
			stepButtons[stepIndex]->setBounds(rowArea.removeFromLeft(colWidth).reduced(1));
		}
	}
}