#include "ModulationPanel.h"
#include "PluginProcessor.h"
#include "TrackData.h"

ModulationPanel::ModulationPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	addAndMakeVisible(helpLabel);
	helpLabel.setText("MODULATORS - tempo-synced LFOs. Pick a target, set the amount, keep tweaking the knob.",
	                  juce::dontSendNotification);
	helpLabel.setJustificationType(juce::Justification::centredLeft);
	helpLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary.withAlpha(0.6f));
	helpLabel.setFont(juce::FontOptions(9.5f));

	for (int i = 0; i < kNumModSlots; ++i)
	{
		auto row = std::make_unique<ModulatorRow>(processor, i);
		row->onContentChanged = [this]()
		{
			refreshTargetAvailability();
			if (onContentChanged)
				onContentChanged();
		};
		addAndMakeVisible(*row);
		modulatorRows[i] = std::move(row);
	}

	auto trackIds = audioProcessor.getAllTrackIds();
	if (!trackIds.empty())
		showTrack(trackIds[0]);

	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

ModulationPanel::~ModulationPanel()
{
	vBlankAttachment.reset();
}

void ModulationPanel::showTrack(const juce::String &trackId)
{
	auto *t = audioProcessor.getTrack(trackId);
	if (!t)
		return;

	activeTrackId = trackId;
	activeTrack = t;
	refreshRowsFromTrack();
}

void ModulationPanel::refreshRowsFromTrack()
{
	if (!activeTrack)
		return;

	for (auto &row : modulatorRows)
		row->setTrackData(activeTrack);

	if (onContentChanged)
		onContentChanged();
}

void ModulationPanel::refreshAfterStateLoad(const juce::String &trackId)
{
	activeTrack = nullptr;
	activeTrackId.clear();
	showTrack(trackId);
}

void ModulationPanel::handleVBlank()
{
	if (!activeTrack || !isShowing())
		return;

	for (auto &row : modulatorRows)
		row->updateModulationDisplay();
}

void ModulationPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_04));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
}

void ModulationPanel::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	helpLabel.setBounds(area.removeFromTop(12).reduced(4, 0));
	area.removeFromTop(Obsidian::GAP_2);

	for (auto &row : modulatorRows)
	{
		row->setBounds(area.removeFromTop(row->getRequiredHeight()));
		area.removeFromTop(Obsidian::GAP_4);
	}
}

int ModulationPanel::getPreferredHeight() const
{
	int total = 4 + 12 + Obsidian::GAP_2;

	for (auto &row : modulatorRows)
		total += row->getRequiredHeight() + Obsidian::GAP_4;

	return total;
}

void ModulationPanel::updateModelUI()
{
	for (auto &row : modulatorRows)
		row->updateModelUI();
}

void ModulationPanel::refreshTargetAvailability()
{
	for (auto &row : modulatorRows)
		row->refreshTargetAvailability();
}