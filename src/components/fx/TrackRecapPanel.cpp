#include "TrackRecapPanel.h"
#include "PluginProcessor.h"
#include "TrackData.h"

TrackRecapPanel::TrackRecapPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	startTimerHz(10);
}

TrackRecapPanel::~TrackRecapPanel()
{
	stopTimer();
}

void TrackRecapPanel::timerCallback()
{
	repaint();
}

void TrackRecapPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(bounds, ObsidianSizes::LIST_PANEL_CORNER_SIZE);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), ObsidianSizes::LIST_PANEL_CORNER_SIZE, 1.0f);

	auto titleArea = getLocalBounds().reduced(8, 4).removeFromTop(18);
	g.setColour(ColourPalette::textAccent);
	g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
	g.drawText("TRACKS", titleArea, juce::Justification::centredLeft, false);

	auto cardsArea = getLocalBounds().reduced(6, 4);
	cardsArea.removeFromTop(22);

	auto trackIds = audioProcessor.getAllTrackIds();

	std::vector<juce::String> sortedIds(trackIds.begin(), trackIds.end());
	std::sort(sortedIds.begin(), sortedIds.end(),
	          [this](const juce::String &a, const juce::String &b)
	          {
		          auto *ta = audioProcessor.getTrack(a);
		          auto *tb = audioProcessor.getTrack(b);
		          if (!ta || !tb)
			          return false;
		          return ta->slotIndex < tb->slotIndex;
	          });

	int y = cardsArea.getY();
	for (size_t i = 0; i < sortedIds.size() && i < 8; ++i)
	{
		auto cardBounds = juce::Rectangle<int>(cardsArea.getX(), y, cardsArea.getWidth(), CARD_HEIGHT);
		paintTrackCard(g, cardBounds, (int)i);
		y += CARD_HEIGHT + CARD_SPACING;
	}
}

void TrackRecapPanel::paintTrackCard(juce::Graphics &g, juce::Rectangle<int> bounds, int trackIndex)
{
	auto trackIds = audioProcessor.getAllTrackIds();

	std::vector<juce::String> sortedIds(trackIds.begin(), trackIds.end());
	std::sort(sortedIds.begin(), sortedIds.end(),
	          [this](const juce::String &a, const juce::String &b)
	          {
		          auto *ta = audioProcessor.getTrack(a);
		          auto *tb = audioProcessor.getTrack(b);
		          if (!ta || !tb)
			          return false;
		          return ta->slotIndex < tb->slotIndex;
	          });

	if (trackIndex >= (int)sortedIds.size())
		return;

	auto *track = audioProcessor.getTrack(sortedIds[trackIndex]);
	if (!track)
		return;

	const auto &currentPage = track->getCurrentPage();
	juce::Colour modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);

	auto bgRect = bounds.toFloat();
	g.setColour(ColourPalette::backgroundDark.withAlpha(0.5f));
	g.fillRoundedRectangle(bgRect, ObsidianSizes::LIST_PANEL_CORNER_SIZE);

	g.setColour(modelColour);
	g.fillRect(bounds.getX(), bounds.getY(), 3, bounds.getHeight());

	auto inner = bounds.reduced(8, 5);
	inner.removeFromLeft(4);

	auto headerLine = inner.removeFromTop(15);

	auto labelArea = headerLine.removeFromLeft(28);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
	g.drawText("T" + juce::String(track->slotIndex + 1), labelArea, juce::Justification::centredLeft, false);

	headerLine.removeFromLeft(4);
	g.setColour(ColourPalette::textSecondary.withAlpha(0.85f));
	g.setFont(juce::FontOptions(10.0f, juce::Font::italic));
	juce::String modelText = currentPage.selectedModel.isEmpty() ? "(no model)" : currentPage.selectedModel;
	g.drawText(modelText, headerLine, juce::Justification::centredLeft, true);

	inner.removeFromTop(3);

	static const char pageLetters[] = {'A', 'B', 'C', 'D'};
	int activePage = track->currentPageIndex.load();

	for (int p = 0; p < 4; ++p)
	{
		if (inner.getHeight() < 13)
			break;

		auto pageLine = inner.removeFromTop(13);
		bool isActive = (p == activePage);
		bool hasContent = (track->pages[p].numSamples > 0 || track->pages[p].selectedPrompt.isNotEmpty());

		auto badgeArea = pageLine.removeFromLeft(14);
		auto badgeRect = badgeArea.withSizeKeepingCentre(11, 11).toFloat();

		juce::Colour modelColourForPage = AiModelDefinitions::getColourForModel(track->pages[p].selectedModel);

		if (isActive)
		{
			g.setColour(modelColourForPage);
			g.fillEllipse(badgeRect);
			g.setColour(juce::Colours::white);
		}
		else
		{
			g.setColour(juce::Colours::transparentBlack);
			g.fillEllipse(badgeRect);
			juce::Colour colour =
			    track->pages[p].numSamples > 0 ? modelColourForPage : ColourPalette::textSecondary.withAlpha(0.3f);
			g.setColour(colour);
			g.drawEllipse(badgeRect, 1.0f);
		}

		g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
		g.drawText(juce::String::charToString(pageLetters[p]), badgeRect.toNearestInt(), juce::Justification::centred,
		           false);

		pageLine.removeFromLeft(4);
		juce::String promptText = track->pages[p].selectedPrompt;
		if (promptText.isEmpty())
			promptText = "(empty)";

		g.setColour(isActive ? ColourPalette::textPrimary
		                     : ColourPalette::textPrimary.withAlpha(hasContent ? 0.65f : 0.35f));
		g.setFont(juce::FontOptions(10.0f, isActive ? juce::Font::bold : juce::Font::plain));
		g.drawText(promptText, pageLine, juce::Justification::centredLeft, true);
	}
}

int TrackRecapPanel::getPreferredHeight() const
{
	auto trackIds = audioProcessor.getAllTrackIds();
	int numTracks = juce::jmin(8, (int)trackIds.size());
	if (numTracks == 0)
		numTracks = 8;

	return 22 + (numTracks * CARD_HEIGHT) + ((numTracks - 1) * CARD_SPACING) + 12;
}

void TrackRecapPanel::resized()
{
}