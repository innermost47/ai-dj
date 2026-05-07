#include "UILayoutManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

UILayoutManager::UILayoutManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UILayoutManager::layoutPromptSection(juce::Rectangle<int> area, int spacing, int controlsZoneW)
{
	const int itemH = 28;
	const int vPad = (area.getHeight() - itemH) / 2;
	area = area.reduced(0, vPad);

	constexpr int numCtrl = 8;
	const int totalCtrlSpacing = numCtrl * spacing;
	const int ctrlBtnW = juce::jmax(24, (controlsZoneW - totalCtrlSpacing) / numCtrl);

	editor.configButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.toggleBankButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.helpButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.openMidiEditorButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.loadSampleButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.autoLoadButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.bypassSequencerButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	editor.bypassLLMButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	if (editor.sampleBankPanel && editor.sampleBankPanel->isVisible())
	{
		area.removeFromLeft(2);
	}

	const int genBtnW = 50;
	const int saveBtnW = 34;

	area.removeFromRight(2);
	editor.generateButton.setBounds(area.removeFromRight(genBtnW));
	area.removeFromRight(spacing);
	editor.savePresetButton.setBounds(area.removeFromRight(saveBtnW));
	area.removeFromRight(spacing);

	const int idealKeyW = 180;
	const int idealDurW = 100;
	const int idealPresetW = 600;
	const int idealPromptW = 600;

	const int minKeyW = 120;
	const int minDurW = 80;
	const int minPresetW = 280;
	const int minPromptW = 280;

	const int remaining = area.getWidth();
	const int idealTotal = idealKeyW + idealDurW + idealPresetW + idealPromptW + spacing * 3;

	int keyW, durW, presetW, promptW;

	if (remaining >= idealTotal)
	{
		keyW = idealKeyW;
		durW = idealDurW;
		const int extra = remaining - idealTotal;
		presetW = idealPresetW + extra / 2;
		promptW = remaining - keyW - durW - presetW - spacing * 3;
	}
	else
	{
		const float scale = static_cast<float>(remaining) / static_cast<float>(idealTotal);
		keyW = juce::jmax(minKeyW, static_cast<int>(idealKeyW * scale));
		durW = juce::jmax(minDurW, static_cast<int>(idealDurW * scale));
		presetW = juce::jmax(minPresetW, static_cast<int>(idealPresetW * scale));
		promptW = juce::jmax(minPromptW, remaining - keyW - durW - presetW - spacing * 3);
	}

	editor.keySelector.setBounds(area.removeFromRight(keyW));
	area.removeFromRight(spacing);
	editor.durationSelector.setBounds(area.removeFromRight(durW));
	area.removeFromRight(spacing);
	editor.promptPresetSelector.setBounds(area.removeFromRight(presetW));
	area.removeFromRight(spacing);

	editor.promptInput.setBounds(area);
}

void UILayoutManager::layoutTracksGrid()
{
	const int spacing = 5;
	const int minCellW = 600;
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
	auto fullBounds = editor.getLocalBounds();

	const int bannerHeight = 40;

	const int bankWidth = (editor.sampleBankPanel && editor.sampleBankPanel->isVisible())
	                          ? juce::jmax(290, fullBounds.getWidth() / 6)
	                          : 0;

	auto headerArea = fullBounds.removeFromTop(bannerHeight);
	headerArea.reduce(padding, 0);

	const int ctrlZoneW = 290;

	layoutPromptSection(headerArea, spacing, ctrlZoneW);

	if (editor.sampleBankPanel && editor.sampleBankPanel->isVisible())
	{
		auto bankArea = fullBounds.removeFromLeft(bankWidth);
		editor.sampleBankPanel->setBounds(bankArea);
	}
	fullBounds.removeFromLeft(padding);
	fullBounds.removeFromRight(padding);
	auto area = fullBounds;
	const int totalHeight = area.getHeight();
	const int maxMixerHeight = 220;
	const int minMixerHeight = 220;
	int mixerHeight = juce::jlimit(minMixerHeight, maxMixerHeight, static_cast<int>(totalHeight * 0.28f));
	int tracksHeight = totalHeight - mixerHeight - spacing;
	auto tracksArea = area.removeFromTop(tracksHeight);
	editor.tracksViewport.setBounds(tracksArea);
	editor.tracksViewport.setViewedComponent(&editor.tracksContainer, false);
	const int totalContentHeight = TRACK_CELL_H * TRACK_ROWS + spacing * (TRACK_ROWS - 1);
	const int totalContentWidth = TRACK_COLS * 600 + spacing * (TRACK_COLS - 1);
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
