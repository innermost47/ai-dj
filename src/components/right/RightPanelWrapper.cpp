#include "RightPanelWrapper.h"
#include "ConfigComponent.h"
#include "GlitchSequencerPanel.h"
#include "LCDScreen.h"
#include "MasterChannel.h"
#include "ModulationPanel.h"
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
	glitchSequencerPanel = std::make_unique<GlitchSequencerPanel>(processor, editor);
	sendsPanel = std::make_unique<SendsPanel>(processor);
	configComponent = std::make_unique<ConfigComponent>(processor, editor);
	trackSelectorBar = std::make_unique<TrackSelectorBar>(processor, true);
	modulationPanel = std::make_unique<ModulationPanel>(processor, editor);

	modulationPanel->onContentChanged = [this]() { layoutScrollContent(); };
	trackEffects->onContentChanged = [this]() { layoutScrollContent(); };
	glitchSequencerPanel->onContentChanged = [this]() { layoutScrollContent(); };

	setupUI();
}

RightPanelWrapper::~RightPanelWrapper() = default;

void RightPanelWrapper::setupUI()
{
	scrollContent.addAndMakeVisible(*trackRecap);
	scrollContent.addAndMakeVisible(*trackEffects);
	scrollContent.addAndMakeVisible(*glitchSequencerPanel);
	scrollContent.addAndMakeVisible(*modulationPanel);

	setupTabButton(fxTabButton, [this]() { setActiveTab(0); });
	setupTabButton(glitchTabButton, [this]() { setActiveTab(1); });
	setupTabButton(modTabButton, [this]() { setActiveTab(2); });
	setupTabButton(infoTabButton, [this]() { setActiveTab(3); });

	fxTabButton.loadIcon(BinaryData::sliders_svg, BinaryData::sliders_svgSize);
	glitchTabButton.loadIcon(BinaryData::grid_svg, BinaryData::grid_svgSize);
	modTabButton.loadIcon(BinaryData::waveform_svg, BinaryData::waveform_svgSize);
	infoTabButton.loadIcon(BinaryData::info_svg, BinaryData::info_svgSize);

	fxTabButton.setCompactMode(true);
	glitchTabButton.setCompactMode(true);
	modTabButton.setCompactMode(true);
	infoTabButton.setCompactMode(true);

	addAndMakeVisible(*trackSelectorBar);
	trackSelectorBar->onTrackSelected = [this](const juce::String &trackId) { selectTrack(trackId); };
	trackSelectorBar->onMasterSelected = [this]()
	{
		stashScrollPosition();
		selectedTrackId.clear();
		trackEffects->showMaster();
		layoutScrollContent();
		restoreScrollPosition();
	};

	contentViewport.setViewedComponent(&scrollContent, false);
	contentViewport.setScrollBarsShown(true, false);
	contentViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
	addAndMakeVisible(contentViewport);

	addAndMakeVisible(*masterChannel);
	addAndMakeVisible(*configComponent);
	addAndMakeVisible(*sendsPanel);

	auto trackIds = audioProcessor.getAllTrackIds();
	if (!trackIds.empty())
		selectTrack(trackIds[0]);

	setActiveTab(0);
}

void RightPanelWrapper::selectTrack(const juce::String &trackId)
{
	auto *t = audioProcessor.getTrack(trackId);
	if (!t)
		return;

	stashScrollPosition();
	selectedTrackId = trackId;
	trackSelectorBar->selectTrack(trackId);
	trackEffects->showTrack(trackId);
	glitchSequencerPanel->showTrack(trackId);
	modulationPanel->showTrack(trackId);

	if (!t->isSelected.load())
		editor.uiTrackManager->updateSelectedTrack(trackId);

	layoutScrollContent();
	restoreScrollPosition();
}

void RightPanelWrapper::refreshAfterStateLoad()
{
	trackSelectorBar->refresh();

	auto trackIds = audioProcessor.getAllTrackIds();
	if (trackIds.empty())
		return;

	juce::String targetId;

	for (const auto &id : trackIds)
		if (auto *t = audioProcessor.getTrack(id))
			if (t->isSelected.load())
			{
				targetId = id;
				break;
			}

	if (targetId.isEmpty())
		targetId = trackIds[0];

	selectedTrackId = targetId;
	trackSelectorBar->selectTrack(targetId);
	trackEffects->showTrack(targetId);
	glitchSequencerPanel->refreshAfterStateLoad(targetId);
	modulationPanel->refreshAfterStateLoad(targetId);

	resized();
}

void RightPanelWrapper::paint(juce::Graphics &g)
{
	paintBaseBackgroundWithLeftBorder(g);
}

void RightPanelWrapper::setActiveTab(int tab)
{
	stashScrollPosition();

	activeTab = tab;
	fxTabButton.setToggleState(tab == 0, juce::dontSendNotification);
	glitchTabButton.setToggleState(tab == 1, juce::dontSendNotification);
	modTabButton.setToggleState(tab == 2, juce::dontSendNotification);
	infoTabButton.setToggleState(tab == 3, juce::dontSendNotification);

	trackEffects->setVisible(tab == 0);
	glitchSequencerPanel->setVisible(tab == 1);
	modulationPanel->setVisible(tab == 2);
	trackRecap->setVisible(tab == 3);

	trackSelectorBar->setVisible(tab == 0 || tab == 1 || tab == 2);
	trackSelectorBar->setMasterButtonVisible(tab == 0);

	if ((tab == 1 || tab == 2) && selectedTrackId.isEmpty())
	{
		auto trackIds = audioProcessor.getAllTrackIds();
		if (!trackIds.empty())
			selectTrack(trackIds[0]);
	}

	resized();

	restoreScrollPosition();
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
	                               : (juce::Component *)nullptr;

	float topCompH = .4f;
	float lcdScreenH = .2f;
	float mainStackH = 1.25f;
	float sendsPanelH = .7f;
	float bottomRowH = 1.f;

	if (topComp == nullptr)
		mainStackH = 1.65f;

	if (topComp != nullptr)
		mainStack.items.add(
		    FlexItem(*topComp).withFlex(topCompH).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	if (lcdScreen != nullptr)
		mainStack.items.add(
		    FlexItem(*lcdScreen).withFlex(lcdScreenH).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	mainStack.items.add(
	    FlexItem(contentViewport).withFlex(mainStackH).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_8, 0)));

	if (sendsPanel != nullptr)
		mainStack.items.add(
		    FlexItem(*sendsPanel).withFlex(sendsPanelH).withMargin(FlexItem::Margin(0, 0, Obsidian::GAP_4, 0)));

	mainStack.items.add(FlexItem(bottomRow).withFlex(bottomRowH));

	mainStack.performLayout(getLocalBounds().reduced(Obsidian::PADDING));

	{
		auto vpBounds = contentViewport.getBounds();

		auto tabBar = vpBounds.removeFromTop(Obsidian::TAB_BAR_HEIGHT);
		const int tabW = (tabBar.getWidth() - Obsidian::SPACER_MD * 3) / 4;
		fxTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		glitchTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		modTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		infoTabButton.setBounds(tabBar);

		vpBounds.removeFromTop(4);

		if (activeTab == 0 || activeTab == 1 || activeTab == 2)
		{
			trackSelectorBar->setBounds(vpBounds.removeFromTop(26));
			vpBounds.removeFromTop(Obsidian::GAP_4);
		}

		contentViewport.setBounds(vpBounds);
	}

	layoutScrollContent();
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
		resized();
	}
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
	o->setProperty("selectedTrackId", selectedTrackId);

	const juce::String activeKey = scrollKey(activeTab, selectedTrackId);
	bool activeWritten = false;

	juce::Array<juce::var> entries;
	auto addEntry = [&entries](const juce::String &k, int y)
	{
		juce::DynamicObject::Ptr e = new juce::DynamicObject();
		e->setProperty("k", k);
		e->setProperty("y", y);
		entries.add(juce::var(e.get()));
	};

	for (const auto &kv : scrollPositions)
	{
		const bool isActive = (kv.first == activeKey);
		addEntry(kv.first, isActive ? contentViewport.getViewPositionY() : kv.second);
		activeWritten |= isActive;
	}

	if (!activeWritten && activeTab >= 0)
		addEntry(activeKey, contentViewport.getViewPositionY());

	o->setProperty("scrolls", juce::var(entries));
	return juce::var(o.get());
}

void RightPanelWrapper::restoreUIState(const juce::var &state)
{
	if (!state.isObject())
		return;
	auto *o = state.getDynamicObject();
	if (!o)
		return;

	isRestoringState = true;

	scrollPositions.clear();
	if (auto *arr = o->getProperty("scrolls").getArray())
		for (const auto &v : *arr)
			if (auto *e = v.getDynamicObject())
			{
				const juce::String k = e->getProperty("k").toString();
				if (k.isNotEmpty())
					scrollPositions[k] = juce::jmax(0, (int)e->getProperty("y"));
			}

	if (o->hasProperty("selectedTrackId"))
	{
		juce::String id = o->getProperty("selectedTrackId").toString();
		if (id.isNotEmpty() && audioProcessor.getTrack(id) != nullptr)
			selectTrack(id);
	}

	if (o->hasProperty("activeTab"))
		setActiveTab((int)o->getProperty("activeTab"));

	isRestoringState = false;
	restoreScrollPosition();
}

void RightPanelWrapper::layoutScrollContent()
{
	const int viewportW = contentViewport.getWidth() - contentViewport.getScrollBarThickness();
	if (viewportW <= 0)
		return;

	const auto savedPos = contentViewport.getViewPosition();

	int y = 0;
	if (activeTab == 0)
	{
		const int h = trackEffects->getPreferredHeight();
		trackEffects->setBounds(0, y, viewportW, h);
		y += h;
	}
	else if (activeTab == 1)
	{
		const int h = glitchSequencerPanel->getPreferredHeight();
		glitchSequencerPanel->setBounds(0, y, viewportW, h);
		y += h;
	}
	else if (activeTab == 2)
	{
		const int h = modulationPanel->getPreferredHeight();
		modulationPanel->setBounds(0, y, viewportW, h);
		y += h;
	}
	else
	{
		const int h = trackRecap->getPreferredHeight();
		trackRecap->setBounds(0, y, viewportW, h);
		y += h;
	}

	scrollContent.setSize(viewportW, y);
	contentViewport.setViewPosition(savedPos);
}

juce::String RightPanelWrapper::scrollKey(int tab, const juce::String &trackId) const
{
	if (tab == 3)
		return "3";
	return juce::String(tab) + "|" + trackId;
}

void RightPanelWrapper::stashScrollPosition()
{
	if (activeTab < 0 || isRestoringState)
		return;
	scrollPositions[scrollKey(activeTab, selectedTrackId)] = contentViewport.getViewPositionY();
}

void RightPanelWrapper::restoreScrollPosition()
{
	const auto it = scrollPositions.find(scrollKey(activeTab, selectedTrackId));
	contentViewport.setViewPosition(0, it != scrollPositions.end() ? it->second : 0);
}