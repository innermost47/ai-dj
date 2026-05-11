#include "MixerPanel.h"
#include "MasterChannel.h"
#include "MixerChannel.h"
#include "PluginProcessor.h"

MixerPanel::MixerPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	addAndMakeVisible(deckAViewport);
	deckAViewport.setViewedComponent(&deckAContainer, false);
	deckAViewport.setScrollBarsShown(false, false);

	addAndMakeVisible(deckBViewport);
	deckBViewport.setViewedComponent(&deckBContainer, false);
	deckBViewport.setScrollBarsShown(false, false);

	crossfader = std::make_unique<CrossfaderComponent>(audioProcessor);
	addAndMakeVisible(*crossfader);

	refreshMixerChannels();
}

MixerPanel::~MixerPanel()
{
	for (auto &channel : mixerChannels)
		if (channel)
			channel->setVisible(false);
	mixerChannels.clear();
}

void MixerPanel::updateTrackName(const juce::String &trackId, const juce::String &newName)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->trackNameLabel.setText(newName, juce::dontSendNotification);
			break;
		}
	}
}

void MixerPanel::refreshChannel(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->updateFromTrackData();
			break;
		}
	}
}

void MixerPanel::updateModelUI(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->updateModelUI();
			break;
		}
	}
	if (crossfader)
		crossfader->onModelChanged();
}

void MixerPanel::updateAllMixerComponents()
{
	for (auto &channel : mixerChannels)
	{
		channel->updateVUMeters();
	}
}

void MixerPanel::refreshMixerChannels()
{
	for (auto &ch : mixerChannels)
		if (ch)
			ch->cleanup();

	deckAContainer.removeAllChildren();
	deckBContainer.removeAllChildren();
	mixerChannels.clear();

	auto trackIds = audioProcessor.getAllTrackIds();
	std::sort(trackIds.begin(), trackIds.end(),
	          [this](const juce::String &a, const juce::String &b)
	          {
		          TrackData *ta = audioProcessor.getTrack(a);
		          TrackData *tb = audioProcessor.getTrack(b);
		          if (!ta || !tb)
			          return false;
		          return ta->slotIndex < tb->slotIndex;
	          });

	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;

		auto ch = std::make_unique<MixerChannel>(trackId, audioProcessor, trackData);
		ch->setTrackName(trackData->trackName);
		ch->onTrackRenamed = [this, trackId](const juce::String &newName)
		{
			if (auto *track = audioProcessor.getTrack(trackId))
			{
				track->trackName = newName;
				if (onTrackRenamedFromMixer)
					onTrackRenamedFromMixer(trackId, newName);
			}
		};

		if (trackData->getDeckSide() == TrackData::DeckSide::A)
			deckAContainer.addAndMakeVisible(ch.get());
		else
			deckBContainer.addAndMakeVisible(ch.get());

		mixerChannels.push_back(std::move(ch));
	}

	for (auto &ch : mixerChannels)
	{
		if (audioProcessor.getGeneratingTrackId() == ch->getTrackId() && audioProcessor.getIsGenerating())
			ch->startGeneratingAnimation();
	}

	resized();
}

void MixerPanel::paint(juce::Graphics & /*g*/)
{
}

void MixerPanel::resized()
{
	auto bounds = getLocalBounds().withTrimmedBottom(ObsidianSizes::GAP);

	juce::Grid grid;
	grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1))};
	grid.templateColumns = {juce::Grid::TrackInfo(juce::Grid::Fr(6)), juce::Grid::TrackInfo(juce::Grid::Fr(3)),
	                        juce::Grid::TrackInfo(juce::Grid::Fr(6))};
	grid.columnGap = juce::Grid::Px(ObsidianSizes::GAP);

	grid.items.add(juce::GridItem(deckAViewport));
	if (crossfader)
		grid.items.add(juce::GridItem(*crossfader));
	grid.items.add(juce::GridItem(deckBViewport));

	grid.performLayout(bounds);

	auto layoutWithFlex = [&](juce::Viewport &viewport, juce::Component &container, TrackData::DeckSide side)
	{
		juce::FlexBox fb;
		fb.flexDirection = juce::FlexBox::Direction::row;
		fb.flexWrap = juce::FlexBox::Wrap::noWrap;
		fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
		fb.alignContent = juce::FlexBox::AlignContent::stretch;

		int trackCount = 0;
		for (auto &ch : mixerChannels)
		{
			TrackData *track = audioProcessor.getTrack(ch->getTrackId());
			if (track && track->getDeckSide() == side)
			{
				fb.items.add(
				    juce::FlexItem(*ch).withMinWidth(100.0f).withHeight(static_cast<float>(viewport.getHeight())));
				trackCount++;
			}
		}

		if (trackCount > 0)
		{
			int minTotalWidth = trackCount * 100;
			int finalWidth = std::max(viewport.getWidth(), minTotalWidth);

			container.setBounds(0, 0, finalWidth, viewport.getHeight());
			fb.performLayout(container.getLocalBounds());
		}
	};

	layoutWithFlex(deckAViewport, deckAContainer, TrackData::DeckSide::A);
	layoutWithFlex(deckBViewport, deckBContainer, TrackData::DeckSide::B);
}

void MixerPanel::refreshAllChannels()
{
	for (auto &mixerChannel : mixerChannels)
	{
		if (mixerChannel && mixerChannel->track)
		{
			mixerChannel->cleanup();
			mixerChannel->updateFromTrackData();
		}
	}
}

void MixerPanel::startGeneratingAnimationForTrack(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->startGeneratingAnimation();
			break;
		}
	}
}

void MixerPanel::clearSamplePending(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->setSamplePending(false);
			break;
		}
	}
}

void MixerPanel::stopGeneratingAnimationForTrack(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->stopGeneratingAnimation();
			channel->setSamplePending(true);
			break;
		}
	}
}

void MixerPanel::detachAllTracks()
{
	for (auto &channel : mixerChannels)
		if (channel)
			channel->track = nullptr;
}