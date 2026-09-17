#include "GlitchMetaSequenceRow.h"
#include "PluginProcessor.h"
#include "TrackData.h"

GlitchMetaSequenceRow::GlitchMetaSequenceRow(DjIaVstProcessor &processor) : ObsidianBaseMidiComponent(processor)
{
	addAndMakeVisible(bypassButton);
	bypassButton.loadIcon(BinaryData::power_svg, BinaryData::power_svgSize);
	bypassButton.setClickingTogglesState(true);
	bypassButton.setShowBackground(false);
	bypassButton.setCustomIconColour(ColourPalette::textSecondary.withAlpha(Obsidian::ALPHA_06));
	bypassButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	bypassButton.setTooltip("Enable the sequence chainer");

	addAndMakeVisible(titleLabel);
	titleLabel.setText("CHAIN", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredLeft);
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	Obsidian::applyFontSize(titleLabel, Obsidian::MIXER_KNOB_LABEL);

	for (int i = 0; i < GlitchMetaSequence::MAX_STEPS; ++i)
	{
		auto btn = std::make_unique<GlitchMetaStepButton>();
		btn->onValueChanged = [this, i](int v)
		{
			if (track)
				track->glitchMetaSequence.setStep(i, v);
		};
		addAndMakeVisible(*btn);
		stepButtons[i] = std::move(btn);
	}
}

GlitchMetaSequenceRow::~GlitchMetaSequenceRow()
{
	markForDestruction();
	clearAllBindings();
}

void GlitchMetaSequenceRow::setTrackData(TrackData *trackData)
{
	clearAllBindings();
	track = trackData;

	subscribeToParam("GlitchChainActive");
	registerMidiLearn("GlitchChainActive", &bypassButton);

	bypassButton.onClick = [this]()
	{
		auto *p = getProcessor().getParameterTreeState().getParameter(fullParamId("GlitchChainActive"));
		if (!p)
			return;
		bool wantOn = bypassButton.getToggleState();
		p->beginChangeGesture();
		p->setValueNotifyingHost(wantOn ? 1.0f : 0.0f);
		p->endChangeGesture();
	};

	refreshFromTrack();
}

void GlitchMetaSequenceRow::onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue)
{
	if (paramSuffix != "GlitchChainActive")
		return;

	bypassButton.setToggleState(normalizedValue > 0.5f, juce::dontSendNotification);

	if (onBypassChanged)
		onBypassChanged();
}

void GlitchMetaSequenceRow::refreshFromTrack()
{
	if (!track)
		return;

	const bool chainOn = getProcessor().getParameterManager().getGlitchChainActive(track->slotIndex);
	bypassButton.setToggleState(chainOn, juce::dontSendNotification);

	auto &meta = track->glitchMetaSequence;
	const int n = meta.getNumSteps();
	for (int i = 0; i < GlitchMetaSequence::MAX_STEPS; ++i)
	{
		const bool visible = i < n;
		stepButtons[i]->setVisible(visible);
		if (visible)
			stepButtons[i]->setValue(meta.getStep(i));
	}

	resized();
}

void GlitchMetaSequenceRow::setCurrentPlaybackStep(int stepIndex)
{
	for (int i = 0; i < GlitchMetaSequence::MAX_STEPS; ++i)
		stepButtons[i]->setPlaybackStep(i == stepIndex);
}

bool GlitchMetaSequenceRow::isChainActive() const
{
	return track != nullptr && !track->glitchMetaSequence.isBypassed();
}

void GlitchMetaSequenceRow::paint(juce::Graphics &g)
{
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_02));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), Obsidian::LIST_PANEL_CORNER_SIZE);
}

void GlitchMetaSequenceRow::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	bypassButton.setBounds(area.removeFromLeft(18));
	area.removeFromLeft(Obsidian::GAP_2);
	titleLabel.setBounds(area.removeFromLeft(40));
	area.removeFromLeft(Obsidian::GAP_4);

	if (!track)
		return;

	const int n = track->glitchMetaSequence.getNumSteps();
	if (n <= 0)
		return;

	const int colWidth = area.getWidth() / n;
	for (int i = 0; i < n; ++i)
	{
		stepButtons[i]->setAccent((i % 4) == 0);
		stepButtons[i]->setBounds(area.removeFromLeft(colWidth).reduced(1));
	}
}