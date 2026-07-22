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
	trackEffects = std::make_unique<TrackEffectsPanel>(processor, editor);
	sendsPanel = std::make_unique<SendsPanel>(processor);
	configComponent = std::make_unique<ConfigComponent>(processor, editor);

	trackEffects->onContentChanged = [this]() { resized(); };

	setupUI();
}

RightPanelWrapper::~RightPanelWrapper() = default;

void RightPanelWrapper::setupUI()
{
	scrollContent.addAndMakeVisible(*trackRecap);
	scrollContent.addAndMakeVisible(*trackEffects);

	setupTabButton(fxTabButton, [this]() { setActiveTab(0); });
	setupTabButton(infoTabButton, [this]() { setActiveTab(1); });

	fxTabButton.loadIcon(BinaryData::sliders_svg, BinaryData::sliders_svgSize);
	infoTabButton.loadIcon(BinaryData::info_svg, BinaryData::info_svgSize);

	fxTabButton.setCompactMode(true);
	infoTabButton.setCompactMode(true);

	contentViewport.setViewedComponent(&scrollContent, false);
	contentViewport.setScrollBarsShown(true, false);
	contentViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
	addAndMakeVisible(contentViewport);

	addAndMakeVisible(*masterChannel);
	addAndMakeVisible(*configComponent);
	addAndMakeVisible(*sendsPanel);

	setActiveTab(0);
}

void RightPanelWrapper::paint(juce::Graphics &g)
{
	paintBaseBackgroundWithLeftBorder(g);
}

void RightPanelWrapper::setActiveTab(int tab)
{
	activeTab = tab;
	fxTabButton.setToggleState(tab == 0, juce::dontSendNotification);
	infoTabButton.setToggleState(tab == 1, juce::dontSendNotification);
	trackEffects->setVisible(tab == 0);
	trackRecap->setVisible(tab == 1);
	resized();
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
		mainStack.items.add(FlexItem(*topComp).withFlex(0.4f).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	if (lcdScreen != nullptr)
		mainStack.items.add(FlexItem(*lcdScreen).withFlex(0.2f).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	mainStack.items.add(
	    FlexItem(contentViewport).withFlex(1.25f).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_8, 0)));

	if (sendsPanel != nullptr)
		mainStack.items.add(
		    FlexItem(*sendsPanel).withFlex(0.7f).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	mainStack.items.add(FlexItem(bottomRow).withFlex(1.0f));

	mainStack.performLayout(getLocalBounds().reduced(Obsidian::PADDING));

	{
		auto vpBounds = contentViewport.getBounds();
		auto tabBar = vpBounds.removeFromTop(Obsidian::TAB_BAR_HEIGHT);
		const int tabW = (tabBar.getWidth() - Obsidian::SPACER_MD) / 2;
		fxTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		infoTabButton.setBounds(tabBar.removeFromLeft(tabW));
		vpBounds.removeFromTop(4);
		contentViewport.setBounds(vpBounds);
	}

	const int viewportW = contentViewport.getWidth() - contentViewport.getScrollBarThickness();

	int y = 0;
	if (activeTab == 0)
	{
		const int effectsH = trackEffects->getPreferredHeight();
		trackEffects->setBounds(0, y, viewportW, effectsH);
		y += effectsH;
	}
	else
	{
		const int recapH = trackRecap->getPreferredHeight();
		trackRecap->setBounds(0, y, viewportW, recapH);
		y += recapH;
	}

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

juce::var RightPanelWrapper::saveUIState() const
{
	juce::DynamicObject::Ptr o = new juce::DynamicObject();
	o->setProperty("activeTab", activeTab);
	return juce::var(o.get());
}

void RightPanelWrapper::restoreUIState(const juce::var &state)
{
	if (!state.isObject())
		return;
	auto *o = state.getDynamicObject();
	if (!o)
		return;

	if (o->hasProperty("activeTab"))
		setActiveTab((int)o->getProperty("activeTab"));
}
