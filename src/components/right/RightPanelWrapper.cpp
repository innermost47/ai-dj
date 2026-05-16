#include "RightPanelWrapper.h"
#include "ConfigComponent.h"
#include "LCDScreen.h"
#include "MasterChannel.h"
#include "MasterWaveformDisplay.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SendsPanel.h"
#include "StandaloneTransport.h"
#include "StandaloneTransportComponent.h"
#include "TrackEffectsPanel.h"
#include "TrackRecapPanel.h"

RightPanelWrapper::RightPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	masterChannel = std::make_unique<MasterChannel>(processor);
	trackRecap = std::make_unique<TrackRecapPanel>(processor);
	trackEffects = std::make_unique<TrackEffectsPanel>(processor);
	sendsPanel = std::make_unique<SendsPanel>(processor);
	configComponent = std::make_unique<ConfigComponent>(processor, editor);

	scrollContent.addAndMakeVisible(*trackRecap);
	scrollContent.addAndMakeVisible(*trackEffects);

	addAndMakeVisible(*masterChannel);
	addAndMakeVisible(*configComponent);

	contentViewport.setViewedComponent(&scrollContent, false);
	contentViewport.setScrollBarsShown(true, false);
	contentViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
	addAndMakeVisible(contentViewport);

	addAndMakeVisible(*sendsPanel);
}

RightPanelWrapper::~RightPanelWrapper() = default;

void RightPanelWrapper::paint(juce::Graphics &g)
{
	paintBaseBackgroundWithLeftBorder(g);
}

void RightPanelWrapper::resized()
{
	using FlexBox = juce::FlexBox;
	using FlexItem = juce::FlexItem;

	FlexBox bottomRow;
	bottomRow.flexDirection = FlexBox::Direction::row;

	if (masterChannel != nullptr)
		bottomRow.items.add(FlexItem(*masterChannel).withFlex(0.3f));

	bottomRow.items.add(FlexItem(*configComponent).withFlex(0.4f));

	FlexBox mainStack;
	mainStack.flexDirection = FlexBox::Direction::column;
	mainStack.flexWrap = FlexBox::Wrap::noWrap;

	juce::Component *topComp = juce::JUCEApplicationBase::isStandaloneApp()
	                               ? (juce::Component *)standaloneTransport.get()
	                               : (juce::Component *)masterWaveform;

	if (topComp != nullptr)
		mainStack.items.add(
		    FlexItem(*topComp).withFlex(0.4f).withMargin(FlexItem::Margin(0, 0, ObsidianSizes::GAP_4, 0)));

	mainStack.items.add(
	    FlexItem(*lcdScreen).withFlex(0.2f).withMargin(FlexItem::Margin(0, 0, ObsidianSizes::GAP_4, 0)));

	mainStack.items.add(
	    FlexItem(contentViewport).withFlex(1.25f).withMargin(FlexItem::Margin(0, 0, ObsidianSizes::GAP_8, 0)));

	mainStack.items.add(
	    FlexItem(*sendsPanel).withFlex(0.7f).withMargin(FlexItem::Margin(0, 0, ObsidianSizes::GAP_4, 0)));

	mainStack.items.add(FlexItem(bottomRow).withFlex(1.0f));

	mainStack.performLayout(getLocalBounds().reduced(ObsidianSizes::PADDING));

	const int viewportW = contentViewport.getWidth() - contentViewport.getScrollBarThickness();
	const int recapH = trackRecap->getPreferredHeight();
	const int effectsH = 400;

	int y = 0;
	trackRecap->setBounds(0, y, viewportW, recapH);
	y += recapH + ObsidianSizes::GAP;

	trackEffects->setBounds(0, y, viewportW, effectsH);
	y += effectsH;

	scrollContent.setSize(viewportW, y);
}

void RightPanelWrapper::updateComponents()
{
	calculateMasterLevel();
	if (masterChannel)
		masterChannel->updateMasterLevels();
}

void RightPanelWrapper::calculateMasterLevel()
{
	auto linearToDb = [](float linear) -> float
	{
		if (linear <= 0.00001f)
			return -100.0f;
		return 20.0f * ::log10f(linear);
	};

	auto dbToNormalized = [](float db) -> float { return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f); };

	float linearLeft = audioProcessor.getAudioManager().getPeakLevelLeft();
	float linearRight = audioProcessor.getAudioManager().getPeakLevelRight();

	masterChannel->setRealAudioLevelStereo(dbToNormalized(linearToDb(linearLeft)),
	                                       dbToNormalized(linearToDb(linearRight)));
}

void RightPanelWrapper::setStandaloneTransport(StandaloneTransport *transport)
{
	if (transport)
	{
		standaloneTransport = std::make_unique<StandaloneTransportComponent>(*transport, audioProcessor);
		addAndMakeVisible(*standaloneTransport);
		if (masterWaveform)
			masterWaveform->setVisible(false);
		resized();
	}
}

void RightPanelWrapper::setMasterWaveform(MasterWaveformDisplay *wf)
{
	masterWaveform = wf;
	if (masterWaveform)
		addAndMakeVisible(*masterWaveform);
	resized();
}

void RightPanelWrapper::setLCDScreen(LCDScreen *lcd)
{
	lcdScreen = lcd;
	if (lcdScreen)
		addAndMakeVisible(*lcdScreen);
	resized();
}
