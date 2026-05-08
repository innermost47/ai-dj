#include "SendsPanel.h"
#include "PluginProcessor.h"

SendsPanel::SendsPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

void SendsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(bounds, 6.0f);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

	g.setColour(ColourPalette::textSecondary.withAlpha(0.5f));
	g.setFont(juce::FontOptions(13.0f, juce::Font::italic));
	g.drawFittedText("Sends: Delay + Reverb (placeholder)", getLocalBounds(), juce::Justification::centred, 1);
}

void SendsPanel::resized()
{
}