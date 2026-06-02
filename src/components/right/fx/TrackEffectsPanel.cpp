#include "TrackEffectsPanel.h"
#include "FilterComponent.h"
#include "PluginProcessor.h"

TrackEffectsPanel::TrackEffectsPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;

		auto fc = std::make_unique<FilterComponent>(audioProcessor, trackData);
		addAndMakeVisible(*fc);
		filterComponents.push_back(std::move(fc));
	}
}

void TrackEffectsPanel::refresh()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;
		auto it = std::find(trackIds.begin(), trackIds.end(), trackId);
		int idx = static_cast<int>(it - trackIds.begin());
		filterComponents[idx]->setTrackData(trackData);
		filterComponents[idx]->wireParameters();
	}
}

void TrackEffectsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_04));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::LIST_PANEL_CORNER_SIZE, 1.0f);

	auto titleArea = getLocalBounds().reduced(8, 4).removeFromTop(18);
	g.setColour(ColourPalette::textAccent);
	g.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_INFO));
	g.drawText("EFFECTS", titleArea, juce::Justification::centredLeft, false);
}

void TrackEffectsPanel::resized()
{
	auto area = getLocalBounds().reduced(2);

	area.removeFromTop(26);

	juce::FlexBox fb;
	fb.flexDirection = juce::FlexBox::Direction::column;

	for (auto &fc : filterComponents)
	{
		fb.items.add(juce::FlexItem(*fc).withMinWidth(60.0f).withHeight(60.0f).withMargin(
		    juce::FlexItem::Margin(0.f, Obsidian::GAP_4, Obsidian::GAP_4, Obsidian::GAP_4)));
	}
	fb.performLayout(area);
}

void TrackEffectsPanel::updateModelUI(const juce::String &trackId)
{
	for (auto &fc : filterComponents)
	{
		if (fc->getTrackId() == trackId)
		{
			fc->updateModelUI();
			break;
		}
	}
}
