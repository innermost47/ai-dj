#include "UILayoutManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

UILayoutManager::UILayoutManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UILayoutManager::layoutConfigSection(juce::Rectangle<int> area, int spacing)
{
	constexpr int numCtrl = 5;
	const int totalCtrlSpacing = (numCtrl - 1) * spacing;
	const int ctrlBtnW = juce::jmax(24, (area.getWidth() - totalCtrlSpacing) / numCtrl);

	editor.configButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.helpButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.openMidiEditorButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.bypassSequencerButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.bypassLLMButton.setBounds(area);
}

void UILayoutManager::layoutTracksGrid()
{
	const int spacing = 5;
	const int minCellW = 420;
	const int minTotalWidth = TRACK_COLS * minCellW + spacing * (TRACK_COLS - 1);

	auto viewportBounds = editor.tracksViewport.getBounds();
	if (viewportBounds.isEmpty())
		return;

	const int scrollbarAllowance = editor.tracksViewport.isVerticalScrollBarShown() ? 12 : 0;
	const int scrollbarBottomAllowance = editor.tracksViewport.isHorizontalScrollBarShown() ? 4 : 0;
	const int availableWidth = viewportBounds.getWidth() - scrollbarAllowance;
	const int totalWidth = juce::jmax(minTotalWidth, availableWidth);
	const int cellW = (totalWidth - spacing * (TRACK_COLS - 1)) / TRACK_COLS;
	const int totalHeight = TRACK_CELL_H * TRACK_ROWS + spacing * (TRACK_ROWS - 1) + scrollbarBottomAllowance;
	editor.tracksContainer.setSize(totalWidth, totalHeight);

	for (auto &comp : editor.uiTrackManager->getTrackComponents())
	{
		TrackData *trackData = editor.audioProcessor.getTrack(comp->getTrackId());
		if (trackData == nullptr)
			continue;

		int slot = trackData->slotIndex;
		if (slot < 0 || slot >= 8)
			continue;

		int col = (trackData->getDeckSide() == TrackData::DeckSide::A) ? 0 : 1;
		int row = slot % 4;

		int x = col * (cellW + spacing);
		int y = row * (TRACK_CELL_H + spacing);

		comp->setBounds(x, y, cellW, TRACK_CELL_H);
	}
}

void UILayoutManager::resized()
{
	static bool resizing = false;
	if (resizing)
		return;
	resizing = true;
	const int spacing = 4;
	const int padding = 6;
	const int panelWidth = 290;
	auto fullBounds = editor.getLocalBounds();

	const int configBarHeight = 36;
	const int bankWidth = (editor.leftPanelWrapper && editor.leftPanelWrapper->isVisible())
	                          ? juce::jmax(panelWidth, fullBounds.getWidth() / 6)
	                          : 0;

	if (editor.leftPanelWrapper && editor.leftPanelWrapper->isVisible())
	{
		auto leftCol = fullBounds.removeFromLeft(bankWidth);

		auto configArea = leftCol.removeFromTop(configBarHeight).reduced(padding, 4);
		layoutConfigSection(configArea, spacing);

		editor.leftPanelWrapper->setBounds(leftCol);
	}
	else
	{
		auto configArea = fullBounds.removeFromTop(configBarHeight).reduced(padding, 4);
		auto configZone = configArea.removeFromLeft(panelWidth);
		layoutConfigSection(configZone, spacing);
	}

	fullBounds.removeFromLeft(padding);
	fullBounds.removeFromRight(padding);
	fullBounds.removeFromTop(6);
	auto area = fullBounds;
	const int totalHeight = area.getHeight();
	const int maxMixerHeight = 240;
	const int minMixerHeight = 240;
	int mixerHeight = juce::jlimit(minMixerHeight, maxMixerHeight, static_cast<int>(totalHeight * 0.28f));
	int tracksHeight = totalHeight - mixerHeight - spacing;
	auto tracksArea = area.removeFromTop(tracksHeight);
	const int rightPanelWidth = panelWidth;

	if (editor.rightPanelWrapper)
	{
		tracksArea.removeFromRight(padding);
		auto rightCol = tracksArea.removeFromRight(rightPanelWidth);
		editor.rightPanelWrapper->setBounds(rightCol);
	}
	auto tracksColArea = tracksArea;

	tracksColArea.removeFromRight(padding);
	editor.tracksViewport.setBounds(tracksColArea);
	editor.tracksViewport.setViewedComponent(&editor.tracksContainer, false);
	const int totalContentHeight = TRACK_CELL_H * TRACK_ROWS + spacing * (TRACK_ROWS - 1);
	const int totalContentWidth = TRACK_COLS * 420 + spacing * (TRACK_COLS - 1);
	bool needsHorizontal = totalContentWidth > tracksArea.getWidth();
	bool needsVertical = totalContentHeight + (needsHorizontal ? 12 : 0) > tracksArea.getHeight();
	editor.tracksViewport.setScrollBarsShown(needsVertical, needsHorizontal);
	layoutTracksGrid();

	area.removeFromTop(spacing);

	auto bottomRow = area;
	const int minMixerWidth = 1300;

	if (editor.mixerPanel)
	{
		int contentWidth = juce::jmax(minMixerWidth, bottomRow.getWidth());
		bool needsHorizontalScroll = (contentWidth > bottomRow.getWidth());
		int scrollbarH = needsHorizontalScroll ? (editor.mixerViewport.getScrollBarThickness() + 6) : 6;
		editor.mixerViewport.setBounds(bottomRow);
		editor.mixerPanel->setSize(contentWidth, bottomRow.getHeight() - scrollbarH);
		editor.mixerViewport.setScrollBarsShown(false, needsHorizontalScroll);
		editor.mixerViewport.setVisible(true);
	}
	resizing = false;
	editor.audioProcessor.setWindowSize(editor.getWidth(), editor.getHeight());
}
