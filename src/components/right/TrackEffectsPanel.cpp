#include "TrackEffectsPanel.h"
#include "PluginProcessor.h"

TrackEffectsPanel::TrackEffectsPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

void TrackEffectsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(bounds, ObsidianSizes::LIST_PANEL_CORNER_SIZE);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), ObsidianSizes::LIST_PANEL_CORNER_SIZE, 1.0f);

	g.setColour(ColourPalette::textSecondary.withAlpha(0.5f));
	g.setFont(juce::FontOptions(13.0f, juce::Font::italic));
	g.drawFittedText("Track Effects (placeholder)", getLocalBounds(), juce::Justification::centred, 1);
}

void TrackEffectsPanel::resized()
{
}

void TrackEffectsPanel::setActiveTrackId(const juce::String &trackId)
{
	activeTrackId = trackId;
	repaint();
}