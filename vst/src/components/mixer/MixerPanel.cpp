#include "MixerPanel.h"
#include "ColourPalette.h"
#include "MasterChannel.h"
#include "MixerChannel.h"
#include "PluginProcessor.h"

MixerPanel::MixerPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	masterChannel = std::make_unique<MasterChannel>(audioProcessor);
	addAndMakeVisible(*masterChannel);

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

	if (masterChannel)
		masterChannel->setVisible(false);
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
	calculateMasterLevel();
	masterChannel->updateMasterLevels();
}

void MixerPanel::calculateMasterLevel()
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

void MixerPanel::paint(juce::Graphics &g)
{
	juce::Component *centerComp = standaloneTransport ? static_cast<juce::Component *>(standaloneTransport.get())
	                                                  : static_cast<juce::Component *>(masterWaveform);

	if (crossfader)
	{
		auto crossfaderBg = crossfader->getBounds().expanded(4, 2);
		g.setColour(ColourPalette::backgroundDeep.brighter(0.04f));
		g.fillRoundedRectangle(crossfaderBg.toFloat(), 6.0f);
		g.setColour(ColourPalette::sliderTrack);
		g.drawRoundedRectangle(crossfaderBg.toFloat().reduced(0.5f), 6.0f, 1.0f);
	}

	if (masterChannel && centerComp && lcdScreen)
	{
		auto rightBg = centerComp->getBounds()
		                   .getUnion(lcdScreen->getBounds())
		                   .getUnion(masterChannel->getBounds())
		                   .expanded(4, 2);
		g.setColour(ColourPalette::backgroundDeep.brighter(0.04f));
		g.fillRoundedRectangle(rightBg.toFloat(), 6.0f);
		g.setColour(ColourPalette::sliderTrack);
		g.drawRoundedRectangle(rightBg.toFloat().reduced(0.5f), 6.0f, 1.0f);
	}
}

void MixerPanel::setStandaloneTransport(StandaloneTransport *transport)
{
	if (transport)
	{
		standaloneTransport = std::make_unique<StandaloneTransportComponent>(*transport);
		addAndMakeVisible(*standaloneTransport);
		if (masterWaveform)
			masterWaveform->setVisible(false);
		resized();
	}
}

void MixerPanel::setMasterWaveform(MasterWaveformDisplay *wf)
{
	masterWaveform = wf;
	if (masterWaveform != nullptr)
		addAndMakeVisible(*masterWaveform);
	resized();
}

void MixerPanel::setLCDScreen(LCDScreen *lcd)
{
	lcdScreen = lcd;
	if (lcdScreen != nullptr)
		addAndMakeVisible(*lcdScreen);
	resized();
}

void MixerPanel::resized()
{
	auto area = getLocalBounds();
	const int channelH = getHeight();
	const int spacing = 4;
	const int crossfaderWidth = 220;
	const int masterChannelWidth = 120;
	const int waveformWidth = 200;
	const int centerInternalPad = 8;
	const int centerOuterMargin = 4;

	const int rightBlockWidth = masterChannelWidth + waveformWidth + centerInternalPad * 2;
	const int rightBlockFootprint = rightBlockWidth + centerOuterMargin * 2;

	const int centerBlockWidth = crossfaderWidth + centerInternalPad * 2;
	const int centerBlockFootprint = centerBlockWidth + centerOuterMargin * 2;

	const int sideWidth = (area.getWidth() - centerBlockFootprint - rightBlockFootprint) / 2;
	const int channelWidth = juce::jlimit(40, 100, (sideWidth - spacing * 3) / 4);

	auto deckAArea = area.removeFromLeft(channelWidth * 4 + spacing * 3);
	area.removeFromLeft(centerOuterMargin);
	auto centerBlock = area.removeFromLeft(centerBlockWidth);
	area.removeFromLeft(centerOuterMargin);

	auto rightBlock = area.removeFromRight(rightBlockWidth);
	area.removeFromRight(centerOuterMargin);

	auto deckBArea = area;

	centerBlock.reduce(centerInternalPad, 3);
	crossfader->setBounds(centerBlock);

	rightBlock.reduce(centerInternalPad, 3);
	auto mcArea = rightBlock.removeFromLeft(masterChannelWidth);
	rightBlock.removeFromLeft(centerInternalPad);
	auto centerStack = rightBlock;

	masterChannel->setBounds(mcArea);

	if (lcdScreen && (masterWaveform || standaloneTransport))
	{
		centerStack.removeFromTop(6);
		centerStack.removeFromBottom(6);
		const int lcdHeight = juce::jmin(48, centerStack.getHeight() / 3);
		auto lcdArea = centerStack.removeFromBottom(lcdHeight);
		centerStack.removeFromBottom(6);

		if (standaloneTransport)
			standaloneTransport->setBounds(centerStack);
		else if (masterWaveform)
			masterWaveform->setBounds(centerStack);
		lcdScreen->setBounds(lcdArea);
	}
	else if (standaloneTransport)
	{
		standaloneTransport->setBounds(centerStack);
	}
	else if (masterWaveform)
	{
		masterWaveform->setBounds(centerStack);
	}

	deckAViewport.setBounds(deckAArea);
	deckAContainer.setSize(channelWidth * 4 + spacing * 3, channelH);
	deckBViewport.setBounds(deckBArea);
	deckBContainer.setSize(channelWidth * 4 + spacing * 3, channelH);

	int xA = 0, xB = 0;
	for (auto &ch : mixerChannels)
	{
		TrackData *track = audioProcessor.getTrack(ch->getTrackId());
		if (!track)
			continue;
		if (track->getDeckSide() == TrackData::DeckSide::A)
		{
			ch->setBounds(xA, 0, channelWidth, channelH);
			xA += channelWidth + spacing;
		}
		else
		{
			ch->setBounds(xB, 0, channelWidth, channelH);
			xB += channelWidth + spacing;
		}
	}
}

void MixerPanel::refreshAllChannels()
{
	for (auto &mixerChannel : mixerChannels)
	{
		if (mixerChannel && mixerChannel->track)
		{
			mixerChannel->cleanup();
			mixerChannel->addEventListeners();
			mixerChannel->updateFromTrackData();
		}
	}
}

void MixerPanel::trackSelected(const juce::String &trackId)
{
	for (auto &channel : mixerChannels)
	{
		bool isThisTrackSelected = (channel->getTrackId() == trackId);
		channel->setSelected(isThisTrackSelected);
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