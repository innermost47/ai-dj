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
	juce::FlexBox box;
	box.flexDirection = juce::FlexBox::Direction::row;
	box.flexWrap = juce::FlexBox::Wrap::wrap;
	box.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
	box.alignItems = juce::FlexBox::AlignItems::center;

	box.items.add(juce::FlexItem(editor.configButton)
	                  .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                  .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                  .withFlex(1)
	                  .withMargin(ObsidianSizes::SPACER_SM));
	box.items.add(juce::FlexItem(editor.helpButton)
	                  .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                  .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                  .withFlex(1)
	                  .withMargin(ObsidianSizes::SPACER_SM));
	box.items.add(juce::FlexItem(editor.openMidiEditorButton)
	                  .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                  .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                  .withFlex(1)
	                  .withMargin(ObsidianSizes::SPACER_SM));
	box.items.add(juce::FlexItem(editor.bypassSequencerButton)
	                  .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                  .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                  .withFlex(1)
	                  .withMargin(ObsidianSizes::SPACER_SM));
	box.items.add(juce::FlexItem(editor.bypassLLMButton)
	                  .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                  .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                  .withFlex(1)
	                  .withMargin(ObsidianSizes::SPACER_SM));

	box.performLayout(getLocalBounds());
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

	grid.templateRows = {Track(Fr(1)), Track(Fr(32))};
	grid.templateColumns = {Track(Fr(1))};
	grid.columnGap = GridPx(ObsidianSizes::GAP);
	grid.rowGap = GridPx(ObsidianSizes::GAP);

	GridItem configRow(configContainer.get());
	GridItem bankPanel(leftPanelWrapper);

	grid.items = {configRow, bankPanel};
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
	grid.columnGap = GridPx(ObsidianSizes::GAP);
	grid.rowGap = GridPx(ObsidianSizes::GAP);

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

	grid.performLayout(getLocalBounds().reduced(0, ObsidianSizes::GAP));
}

TracksAndFXContainer::TracksAndFXContainer(TracksContainer &tc, RightPanelWrapper &rw)
    : tracksContainer(tc), rightPanelWrapper(rw) {};

void TracksAndFXContainer::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid;

	grid.templateRows = {Track(Fr(1))};
	grid.templateColumns = {Track(Fr(15)), Track(Fr(4))};
	grid.columnGap = GridPx(ObsidianSizes::GAP);

	GridItem tracks(tracksContainer);
	GridItem rightPanel(rightPanelWrapper);

	grid.items = {tracks, rightPanel};
	grid.performLayout(getBounds());
}

MainContainer::MainContainer(TracksAndFXContainer &tfc, MixerPanel &mp)
    : tracksAndFXContainer(tfc), mixerPanel(mp) {

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
	grid.columnGap = GridPx(ObsidianSizes::GAP);

	GridItem top(tracksAndFXContainer);
	GridItem mixer(mixerPanel);

	grid.items = {top, mixer};
	grid.performLayout(getBounds());
}

UILayoutManager::UILayoutManager(DjIaVstProcessor &processor, DjIaVstEditor &editor, MixerPanel &mixerPanel)
    : audioProcessor(processor), editor(editor), mixerPanel(mixerPanel)
{
	leftPanelWrapper = std::make_unique<LeftPanelWrapper>(audioProcessor, editor);
	leftContainer = std::make_unique<LeftContainer>(editor, *leftPanelWrapper);
	tracksContainer = std::make_unique<TracksContainer>(editor);
	rightPanelWrapper = std::make_unique<RightPanelWrapper>(audioProcessor);
	tracksAndFXContainer = std::make_unique<TracksAndFXContainer>(*tracksContainer, *rightPanelWrapper);
	mainContainer = std::make_unique<MainContainer>(*tracksAndFXContainer, mixerPanel);

	editor.addAndMakeVisible(*leftContainer);
	editor.addAndMakeVisible(*tracksContainer);
	editor.addAndMakeVisible(*rightPanelWrapper);
	editor.addAndMakeVisible(mixerPanel);
}

void UILayoutManager::resized()
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	using GridItem = juce::GridItem;
	using Grid = juce::Grid;
	using GridPx = juce::Grid::Px;

	Grid grid;

	grid.templateRows = {Track(Fr(1))};
	grid.templateColumns = {Track(Fr(1)), Track(Fr(5))};
	grid.columnGap = GridPx(ObsidianSizes::GAP);

	GridItem leftPanel(leftContainer.get());
	GridItem main(mainContainer.get());

	grid.items = {leftPanel, main};
	grid.performLayout(editor.getBounds());
}
