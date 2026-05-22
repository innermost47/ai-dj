#include "UILayoutManager.h"
#include "LeftPanelWrapper.h"
#include "MixerPanel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "RightPanelWrapper.h"
#include "Sizes.h"
#include "TrackData.h"

ConfigContainer::ConfigContainer(DjIaVstEditor &editor) : editor(editor) {};

void ConfigContainer::resized()
{
}

LeftContainer::LeftContainer(DjIaVstEditor &editor, LeftPanelWrapper &leftPanelWrapper)
    : editor(editor), leftPanelWrapper(leftPanelWrapper)
{
	configContainer = std::make_unique<ConfigContainer>(editor);
	addAndMakeVisible(leftPanelWrapper);
	addAndMakeVisible(configContainer.get());
};

void LeftContainer::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid;

	grid.templateRows = {Track(Fr(1))};
	grid.templateColumns = {Track(Fr(1))};
	grid.columnGap = GridPx(Obsidian::GAP);
	grid.rowGap = GridPx(Obsidian::GAP);

	GridItem bankPanel(leftPanelWrapper);

	grid.items = {bankPanel};
	grid.performLayout(getBounds());
}

TracksContainer::TracksContainer(DjIaVstEditor &editor) : editor(editor) {};

void TracksContainer::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid{};

	grid.templateRows = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};
	grid.templateColumns = {Track(Fr(1)), Track(Fr(1))};
	grid.columnGap = GridPx(Obsidian::GAP);
	grid.rowGap = GridPx(Obsidian::GAP);

	for (auto &comp : editor.uiTrackManager->getTrackComponents())
	{
		TrackData *trackData = editor.audioProcessor.getTrack(comp->getTrackId());

		if (trackData == nullptr)
			continue;
		int slot = trackData->slotIndex;
		if (slot < 0 || slot >= 8)
			continue;

		int col = (trackData->getDeckSide() == TrackData::DeckSide::A) ? 1 : 2;
		int row = (slot % 4) + 1;

		GridItem item(comp.get());

		item.column = {col, GridItem::Span(1)};
		item.row = {row, GridItem::Span(1)};

		grid.items.add(item);
	}

	grid.performLayout(getLocalBounds().reduced(0, Obsidian::GAP));
}

MainContainer::MainContainer(TracksContainer &tc, MixerPanel &mp) : tracksContainer(tc), mixerPanel(mp)
{
	addAndMakeVisible(tracksContainer);
	addAndMakeVisible(mixerPanel);
};

void MainContainer::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid;

	grid.templateRows = {Track(Fr(8)), Track(Fr(3))};
	grid.templateColumns = {Track(Fr(1))};

	GridItem tracks(tracksContainer);
	GridItem mixer(mixerPanel);

	grid.items = {tracks, mixer};
	grid.performLayout(getBounds());
}

UILayoutManager::UILayoutManager(DjIaVstProcessor &processor, DjIaVstEditor &editor, MixerPanel &mixerPanel)
    : audioProcessor(processor), editor(editor), mixerPanel(mixerPanel)
{
	leftPanelWrapper = std::make_unique<LeftPanelWrapper>(audioProcessor, editor);
	leftContainer = std::make_unique<LeftContainer>(editor, *leftPanelWrapper);
	tracksContainer = std::make_unique<TracksContainer>(editor);
	rightPanelWrapper = std::make_unique<RightPanelWrapper>(audioProcessor, editor);
	mainContainer = std::make_unique<MainContainer>(*tracksContainer, mixerPanel);

	editor.mainViewport.setViewedComponent(mainContainer.get());
	editor.mainViewport.setScrollBarsShown(false, true);

	editor.addAndMakeVisible(*leftContainer);
	editor.addAndMakeVisible(*leftPanelWrapper);
	editor.addAndMakeVisible(*rightPanelWrapper);
	editor.addAndMakeVisible(editor.mainViewport);
}

void UILayoutManager::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid;

	int mainWidth = 4;
	int leftPanelWidth = 1;
	int rightPanelWidth = 1;
	int ratio = 8;

	if (!leftPanelWrapper->getIsExpanded())
	{
		mainWidth = mainWidth * ratio;
		rightPanelWidth = rightPanelWidth * ratio;
	}

	grid.templateRows = {Track(Fr(1))};
	grid.templateColumns = {Track(Fr(leftPanelWidth)), Track(Fr(mainWidth)), Track(Fr(rightPanelWidth))};
	grid.columnGap = GridPx(Obsidian::GAP);

	GridItem leftPanel(leftPanelWrapper.get());
	GridItem main(editor.mainViewport);
	GridItem rightPanel(rightPanelWrapper.get());

	grid.items = {leftPanel, main, rightPanel};
	grid.performLayout(editor.getBounds());

	int minWidth = 1070;

	int contentWidth = std::max(editor.mainViewport.getWidth(), minWidth);
	int contentHeight = editor.mainViewport.getHeight();

	mainContainer->setSize(contentWidth, contentHeight);
	audioProcessor.setWindowSize(editor.getWidth(), editor.getHeight());
}
