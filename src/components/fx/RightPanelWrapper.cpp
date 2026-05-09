#include "RightPanelWrapper.h"
#include "PluginProcessor.h"

RightPanelWrapper::RightPanelWrapper(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	trackRecap = std::make_unique<TrackRecapPanel>(processor);
	trackEffects = std::make_unique<TrackEffectsPanel>(processor);
	sendsPanel = std::make_unique<SendsPanel>(processor);

	scrollContent.addAndMakeVisible(*trackRecap);
	scrollContent.addAndMakeVisible(*trackEffects);

	contentViewport.setViewedComponent(&scrollContent, false);
	contentViewport.setScrollBarsShown(true, false);
	contentViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
	addAndMakeVisible(contentViewport);

	addAndMakeVisible(*sendsPanel);
}

void RightPanelWrapper::paint(juce::Graphics &g)
{
	paintBaseBackground(g);
}

void RightPanelWrapper::resized()
{
	auto area = getLocalBounds().reduced(4);
	const int spacing = 6;

	const int sendsHeight = 160;
	auto sendsArea = area.removeFromBottom(sendsHeight);
	sendsPanel->setBounds(sendsArea);

	area.removeFromBottom(spacing);

	contentViewport.setBounds(area);

	const int viewportW = area.getWidth() - 8;
	const int recapH = trackRecap->getPreferredHeight();
	const int effectsMinH = 200;
	const int effectsH = juce::jmax(effectsMinH, area.getHeight() - recapH - spacing);

	int y = 0;
	trackRecap->setBounds(0, y, viewportW, recapH);
	y += recapH + spacing;

	trackEffects->setBounds(0, y, viewportW, effectsH);
	y += effectsH;

	scrollContent.setSize(viewportW, y);
}