#include "GlitchSequencerPanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

GlitchSequencerPanel::GlitchSequencerPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor), metaRow(processor)
{
	addAndMakeVisible(legend);

	auto setupHelp = [this](juce::Label &label, const juce::String &text)
	{
		addAndMakeVisible(label);
		label.setText(text, juce::dontSendNotification);
		label.setJustificationType(juce::Justification::centredLeft);
		label.setColour(juce::Label::textColourId, ColourPalette::textSecondary.withAlpha(0.6f));
		label.setFont(juce::FontOptions(9.5f));
	};

	setupHelp(chainHelpLabel, "CHAIN - plays sequences in order, one full cycle each. 0 = keep current.");
	setupHelp(seqHelpLabel, "SEQUENCES - click a step to cycle effects, right-click to clear.");

	addAndMakeVisible(metaRow);
	metaRow.onBypassChanged = [this]() { refreshRowsFromTrack(); };

	for (int i = 0; i < 8; ++i)
	{
		auto row = std::make_unique<GlitchSequenceRow>(processor, i);
		row->onContentChanged = [this]() { notifyContentChanged(); };
		addAndMakeVisible(*row);
		sequenceRows[i] = std::move(row);
	}

	auto trackIds = audioProcessor.getAllTrackIds();
	if (!trackIds.empty())
		showTrack(trackIds[0]);

	addAndMakeVisible(presetLabel);
	presetLabel.setText("PRESET", juce::dontSendNotification);
	presetLabel.setJustificationType(juce::Justification::centredLeft);
	presetLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	Obsidian::applyFontSize(presetLabel, Obsidian::MIXER_KNOB_LABEL);

	addAndMakeVisible(presetSelector);
	presetSelector.setTooltip("Load a glitch preset - fills all 8 sequences and the chain");
	presetSelector.onChange = [this]() { applySelectedPreset(); };

	addAndMakeVisible(savePresetButton);
	savePresetButton.loadIcon(BinaryData::save_svg, BinaryData::save_svgSize);
	savePresetButton.setShowBackground(false);
	savePresetButton.setCustomIconColour(ColourPalette::textSecondary);
	savePresetButton.setTooltip("Save the current sequences as a new preset");
	savePresetButton.onClick = [this]() { saveCurrentAsPreset(); };

	addAndMakeVisible(deletePresetButton);
	deletePresetButton.loadIcon(BinaryData::trash_svg, BinaryData::trash_svgSize);
	deletePresetButton.setShowBackground(false);
	deletePresetButton.setCustomIconColour(ColourPalette::textSecondary);
	deletePresetButton.setTooltip("Delete the selected user preset");
	deletePresetButton.onClick = [this]() { deleteSelectedPreset(); };

	refreshPresetList();

	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

GlitchSequencerPanel::~GlitchSequencerPanel()
{
	vBlankAttachment.reset();
}

void GlitchSequencerPanel::showTrack(const juce::String &trackId)
{
	auto *t = audioProcessor.getTrack(trackId);
	if (!t)
		return;

	activeTrackId = trackId;
	activeTrack = t;
	refreshRowsFromTrack();
	refreshPresetList();
}

void GlitchSequencerPanel::refreshRowsFromTrack()
{
	if (!activeTrack)
		return;

	metaRow.setTrackData(activeTrack);

	const bool chainOn = metaRow.isChainActive();
	for (auto &row : sequenceRows)
	{
		row->setTrackData(activeTrack);
		row->setPowerButtonEnabled(!chainOn);
	}

	notifyContentChanged();
}

void GlitchSequencerPanel::handleVBlank()
{
	if (!activeTrack)
		return;

	const bool sequencerOn = activeTrack->glitchSequencerActive.load();
	const int currentStep = sequencerOn ? activeTrack->lastTriggeredGlitchStep.load() : -1;
	const int activeSeqIdx = activeTrack->currentGlitchSequenceIndex.load();

	for (int i = 0; i < 8; ++i)
		sequenceRows[i]->setCurrentPlaybackStep(i == activeSeqIdx ? currentStep : -1);

	const bool chainOn = metaRow.isChainActive();
	metaRow.setCurrentPlaybackStep(sequencerOn && chainOn ? activeTrack->metaCurrentStep.load() : -1);

	if (chainOn != lastKnownChainState)
	{
		lastKnownChainState = chainOn;
		for (auto &row : sequenceRows)
			row->setPowerButtonEnabled(!chainOn);
	}

	const int numerator = audioProcessor.getTimeSignatureNumerator();
	const int denominator = audioProcessor.getTimeSignatureDenominator();

	if (numerator != lastKnownNumerator || denominator != lastKnownDenominator)
	{
		lastKnownNumerator = numerator;
		lastKnownDenominator = denominator;

		const int stepsPerBar =
		    juce::jlimit(1, GlitchSequence::MAX_STEPS,
		                 static_cast<int>(std::round(numerator * (16.0 / juce::jmax(1, denominator)))));
		const int stepsPerBeat =
		    juce::jlimit(1, GlitchSequence::MAX_STEPS, static_cast<int>(std::round(16.0 / juce::jmax(1, denominator))));

		for (auto &row : sequenceRows)
		{
			row->setStepsPerBeat(stepsPerBeat);
			row->setStepsPerBar(stepsPerBar);
		}

		notifyContentChanged();
	}
}

void GlitchSequencerPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_04));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
}

void GlitchSequencerPanel::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	auto presetRow = area.removeFromTop(20);
	presetLabel.setBounds(presetRow.removeFromLeft(42));
	deletePresetButton.setBounds(presetRow.removeFromRight(18));
	presetRow.removeFromRight(Obsidian::GAP_2);
	savePresetButton.setBounds(presetRow.removeFromRight(18));
	presetRow.removeFromRight(Obsidian::GAP_2);
	presetSelector.setBounds(presetRow.reduced(0, 1));
	area.removeFromTop(Obsidian::GAP_4);

	legend.setBounds(area.removeFromTop(34));
	area.removeFromTop(Obsidian::GAP);

	chainHelpLabel.setBounds(area.removeFromTop(12).reduced(4, 0));
	metaRow.setBounds(area.removeFromTop(metaRow.getRequiredHeight()));
	area.removeFromTop(Obsidian::GAP);

	seqHelpLabel.setBounds(area.removeFromTop(12).reduced(4, 0));
	area.removeFromTop(Obsidian::GAP_2);

	for (auto &row : sequenceRows)
	{
		row->setBounds(area.removeFromTop(row->getRequiredHeight()));
		area.removeFromTop(Obsidian::GAP_4);
	}
}

int GlitchSequencerPanel::getPreferredHeight() const
{
	int total = 4 + 54 + Obsidian::GAP;
	total += 12 + metaRow.getRequiredHeight() + Obsidian::GAP;
	total += 12 + Obsidian::GAP_2;

	for (auto &row : sequenceRows)
		total += row->getRequiredHeight() + Obsidian::GAP_4;

	return total;
}

void GlitchSequencerPanel::refreshAfterStateLoad(const juce::String &trackId)
{
	lastNotifiedHeight = -1;
	activeTrack = nullptr;
	activeTrackId.clear();
	showTrack(trackId);

	lastKnownNumerator = -1;
	lastKnownDenominator = -1;
	lastKnownChainState = !metaRow.isChainActive();
}

void GlitchSequencerPanel::refreshPresetList()
{
	presetSelector.clear(juce::dontSendNotification);
	presetSelector.addItem("None", 1);

	presetSelector.addSectionHeading("Factory");
	for (int i = 0; i < getNumFactoryGlitchPresets(); ++i)
		presetSelector.addItem(getFactoryGlitchPresets()[i].name, i + 2);

	const auto &userPresets = audioProcessor.getUserGlitchPresets();
	if (!userPresets.empty())
	{
		presetSelector.addSectionHeading("User");
		for (int i = 0; i < (int)userPresets.size(); ++i)
			presetSelector.addItem(userPresets[i].name, kUserPresetIdOffset + i);
	}

	if (!activeTrack)
		return;

	const juce::String &current = activeTrack->currentGlitchPresetName;
	if (current.isEmpty())
	{
		presetSelector.setSelectedId(1, juce::dontSendNotification);
		updatePresetButtons();
		return;
	}

	for (int i = 0; i < getNumFactoryGlitchPresets(); ++i)
	{
		if (current == getFactoryGlitchPresets()[i].name)
		{
			presetSelector.setSelectedId(i + 2, juce::dontSendNotification);
			updatePresetButtons();
			return;
		}
	}

	for (int i = 0; i < (int)userPresets.size(); ++i)
	{
		if (current == userPresets[i].name)
		{
			presetSelector.setSelectedId(kUserPresetIdOffset + i, juce::dontSendNotification);
			updatePresetButtons();
			return;
		}
	}

	presetSelector.setSelectedId(1, juce::dontSendNotification);
	updatePresetButtons();
}

void GlitchSequencerPanel::updatePresetButtons()
{
	const int id = presetSelector.getSelectedId();
	deletePresetButton.setEnabled(id >= kUserPresetIdOffset);
	deletePresetButton.setAlpha(id >= kUserPresetIdOffset ? 1.0f : 0.4f);
}

void GlitchSequencerPanel::applySelectedPreset()
{
	if (!activeTrack)
		return;

	const int id = presetSelector.getSelectedId();

	if (id == 1)
	{
		activeTrack->currentGlitchPresetName.clear();
		updatePresetButtons();
		return;
	}

	if (id >= kUserPresetIdOffset)
	{
		const int idx = id - kUserPresetIdOffset;
		const auto &userPresets = audioProcessor.getUserGlitchPresets();
		if (idx < 0 || idx >= (int)userPresets.size())
			return;

		applyGlitchSequencesFromStrings(*activeTrack, userPresets[idx].sequences);
		activeTrack->currentGlitchPresetName = userPresets[idx].name;
	}
	else
	{
		const int idx = id - 2;
		if (idx < 0 || idx >= getNumFactoryGlitchPresets())
			return;

		applyFactoryGlitchPreset(*activeTrack, idx);
		activeTrack->currentGlitchPresetName = getFactoryGlitchPresets()[idx].name;
	}

	audioProcessor.getSequencerManager().getGlitchSequencerEngine().refreshMasksAfterEdit(*activeTrack);

	updatePresetButtons();
	refreshRowsFromTrack();
	metaRow.refreshFromTrack();
}

void GlitchSequencerPanel::saveCurrentAsPreset()
{
	if (!activeTrack)
		return;

	ObsidianAlertManager::showTextInput(
	    this, "Save Glitch Preset", "Preset name:", activeTrack->currentGlitchPresetName, "Save",
	    [this](bool confirmed, const juce::String &name)
	    {
		    if (!confirmed || name.isEmpty() || !activeTrack)
			    return;

		    if (!audioProcessor.addUserGlitchPreset(name, *activeTrack))
		    {
			    editor.uiStatusManager->setStatusWithTimeout("Cannot use a factory preset name", 3000);
			    return;
		    }

		    activeTrack->currentGlitchPresetName = name;
		    refreshPresetList();
		    editor.uiStatusManager->setStatusWithTimeout("Preset saved: " + name, 3000);
	    });
}

void GlitchSequencerPanel::deleteSelectedPreset()
{
	const int id = presetSelector.getSelectedId();
	if (id < kUserPresetIdOffset)
		return;

	const int idx = id - kUserPresetIdOffset;
	const auto &userPresets = audioProcessor.getUserGlitchPresets();
	if (idx < 0 || idx >= (int)userPresets.size())
		return;

	const juce::String name = userPresets[idx].name;

	ObsidianAlertManager::showConfirm(this, "Delete Preset", "Delete \"" + name + "\"?", "Delete", "Cancel",
	                                  [this, name](bool confirmed)
	                                  {
		                                  if (!confirmed)
			                                  return;

		                                  audioProcessor.deleteUserGlitchPreset(name);
		                                  if (activeTrack && activeTrack->currentGlitchPresetName == name)
			                                  activeTrack->currentGlitchPresetName.clear();
		                                  refreshPresetList();
	                                  });
}

void GlitchSequencerPanel::notifyContentChanged()
{
	const int h = getPreferredHeight();
	if (h == lastNotifiedHeight)
	{
		resized();
		return;
	}
	lastNotifiedHeight = h;
	if (onContentChanged)
		onContentChanged();
}