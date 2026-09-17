#include "GlitchSequenceRow.h"
#include "DataConst.h"
#include "PluginProcessor.h"
#include "TrackData.h"

GlitchSequenceRow::GlitchSequenceRow(DjIaVstProcessor &processor, int sequenceIndex)
    : ObsidianBaseMidiComponent(processor), index(sequenceIndex)
{
	paramSuffixForThisRow = "GlitchSeq" + juce::String(index + 1) + "Active";

	addAndMakeVisible(activateButton);
	activateButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	activateButton.setClickingTogglesState(true);
	activateButton.setShowBackground(false);
	activateButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	activateButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	activateButton.setTooltip("Activate this glitch sequence (only one active per track)");

	addAndMakeVisible(seqLabel);
	seqLabel.setText("S" + juce::String(index + 1), juce::dontSendNotification);
	seqLabel.setJustificationType(juce::Justification::centredLeft);
	seqLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	Obsidian::applyFontSize(seqLabel, Obsidian::MIXER_KNOB_LABEL);

	stepGrid.onStepChanged = [this]()
	{
		auto *t = getTrack();
		if (!t)
			return;

		getProcessor().getSequencerManager().getGlitchSequencerEngine().refreshMasksAfterEdit(*t);

		if (onContentChanged)
			onContentChanged();
	};

	addAndMakeVisible(stepGrid);
}

GlitchSequenceRow::~GlitchSequenceRow()
{
	markForDestruction();
	clearAllBindings();
}

void GlitchSequenceRow::setTrackData(TrackData *trackData)
{
	clearAllBindings();
	track = trackData;

	subscribeToParam(paramSuffixForThisRow);
	registerMidiLearn(paramSuffixForThisRow, &activateButton);

	activateButton.onClick = [this]()
	{
		auto *p = getProcessor().getParameterTreeState().getParameter(fullParamId(paramSuffixForThisRow));
		if (!p)
			return;
		bool wantOn = activateButton.getToggleState();
		p->beginChangeGesture();
		p->setValueNotifyingHost(wantOn ? 1.0f : 0.0f);
		p->endChangeGesture();
	};

	refreshFromTrack();
}

void GlitchSequenceRow::setStepsPerBeat(int stepsPerBeat)
{
	stepGrid.setStepsPerBeat(stepsPerBeat);
}

void GlitchSequenceRow::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	if (paramSuffix == paramSuffixForThisRow)
		activateButton.setToggleState(normalizedValue > 0.5f, juce::dontSendNotification);
}

void GlitchSequenceRow::refreshFromTrack()
{
	auto *t = getTrack();
	if (!t)
		return;

	auto &seq = t->glitchSequences[index];
	stepGrid.setSequence(&seq);

	bool isActive = t->glitchSequencerActive.load() && t->currentGlitchSequenceIndex.load() == index;
	activateButton.setToggleState(isActive, juce::dontSendNotification);
}

void GlitchSequenceRow::setStepsPerBar(int stepsPerBar)
{
	stepGrid.setStepsPerBar(stepsPerBar);

	auto *t = getTrack();
	if (!t)
		return;

	auto &seq = t->glitchSequences[index];
	seq.setStepLength(1);
	seq.setNumSteps(stepsPerBar);
	stepGrid.refreshFromSequence();

	if (onContentChanged)
		onContentChanged();
}

void GlitchSequenceRow::setCurrentPlaybackStep(int stepIndex)
{
	stepGrid.setCurrentPlaybackStep(stepIndex);
}

void GlitchSequenceRow::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_02));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
}

void GlitchSequenceRow::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	auto headerRow = area.removeFromTop(18);
	activateButton.setBounds(headerRow.removeFromLeft(18));
	headerRow.removeFromLeft(Obsidian::GAP_2);
	seqLabel.setBounds(headerRow.removeFromLeft(22));

	area.removeFromTop(Obsidian::GAP_2);
	stepGrid.setBounds(area.removeFromTop(stepGrid.getRequiredHeight()));
}

void GlitchSequenceRow::setPowerButtonEnabled(bool shouldBeEnabled)
{
	activateButton.setEnabled(shouldBeEnabled);
	activateButton.setAlpha(shouldBeEnabled ? 1.0f : 0.4f);
}

int GlitchSequenceRow::getRequiredHeight() const
{
	return 18 + Obsidian::GAP_2 + stepGrid.getRequiredHeight() + 4;
}