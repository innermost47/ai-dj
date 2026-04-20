#include "MixerPanel.h"
#include "ColourPalette.h"
#include "MixerChannel.h"
#include "MasterChannel.h"
#include "PluginProcessor.h"

MixerPanel::MixerPanel(DjIaVstProcessor& processor) : audioProcessor(processor)
{
	masterChannel = std::make_unique<MasterChannel>(audioProcessor);
	addAndMakeVisible(*masterChannel);

	addAndMakeVisible(channelsViewport);
	channelsViewport.setViewedComponent(&channelsContainer, false);
	channelsViewport.setScrollBarsShown(false, true);

	refreshMixerChannels();
}

MixerPanel::~MixerPanel()
{
}

void MixerPanel::updateTrackName(const juce::String& trackId, const juce::String& newName)
{
	for (auto& channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->trackNameLabel.setText(newName, juce::dontSendNotification);
			break;
		}
	}
}

void MixerPanel::updateAllMixerComponents()
{
	for (auto& channel : mixerChannels)
	{
		channel->updateVUMeters();
	}
	calculateMasterLevel();
	masterChannel->updateMasterLevels();
}

float MixerPanel::getMasterVolume() const
{
	return masterVolume;
}

float MixerPanel::getMasterPan() const
{
	return masterPan;
}

void MixerPanel::calculateMasterLevel()
{
	float maxPeakLeft = 0.0f;
	float maxPeakRight = 0.0f;

	for (auto& channel : mixerChannels)
	{
		float channelPeakLeft = channel->getCurrentAudioLevelLeft();
		float channelPeakRight = channel->getCurrentAudioLevelRight();

		maxPeakLeft = std::max(maxPeakLeft, channelPeakLeft);
		maxPeakRight = std::max(maxPeakRight, channelPeakRight);
	}

	auto dbToLinear = [](float normalized) -> float
		{
			float db = -60.0f + normalized * 60.0f;
			return ::powf(10.0f, db / 20.0f);
		};

	auto linearToDb = [](float linear) -> float
		{
			if (linear <= 0.00001f)
				return -100.0f;
			return 20.0f * ::log10f(linear);
		};

	auto dbToNormalized = [](float db) -> float
		{
			return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
		};

	float linearLeft = dbToLinear(maxPeakLeft);
	float linearRight = dbToLinear(maxPeakRight);

	linearLeft *= masterVolume;
	linearRight *= masterVolume;

	masterChannel->setRealAudioLevelStereo(
		dbToNormalized(linearToDb(linearLeft)),
		dbToNormalized(linearToDb(linearRight)));
}

void MixerPanel::refreshMixerChannels()
{
	for (auto& mixerChannel : mixerChannels)
		if (mixerChannel)
			mixerChannel->cleanup();

	channelsContainer.removeAllChildren();
	mixerChannels.clear();

	auto trackIds = audioProcessor.getAllTrackIds();
	std::sort(trackIds.begin(), trackIds.end(),
		[this](const juce::String& a, const juce::String& b)
		{
			TrackData* trackA = audioProcessor.getTrack(a);
			TrackData* trackB = audioProcessor.getTrack(b);
			if (!trackA || !trackB) return false;
			return trackA->slotIndex < trackB->slotIndex;
		});

	for (const auto& trackId : trackIds)
	{
		TrackData* trackData = audioProcessor.getTrack(trackId);
		if (!trackData) continue;
		auto mixerChannel = std::make_unique<MixerChannel>(
			trackId, audioProcessor, static_cast<TrackData*>(trackData));
		channelsContainer.addAndMakeVisible(mixerChannel.get());
		mixerChannels.push_back(std::move(mixerChannel));
	}

	for (auto& channel : mixerChannels)
	{
		if (audioProcessor.getGeneratingTrackId() == channel->getTrackId()
			&& audioProcessor.getIsGenerating())
			channel->startGeneratingAnimation();
	}

	channelsContainer.setVisible(true);
	resized();
}

void MixerPanel::paint(juce::Graphics& g)
{
	int masterX = getWidth() - 100;
	g.setColour(ColourPalette::backgroundLight);
	g.drawLine(static_cast<float>(masterX - 5), 10.0f,
		static_cast<float>(masterX - 5),
		static_cast<float>(getHeight() - 10), 2.0f);
}

void MixerPanel::resized()
{
	auto area = getLocalBounds();
	auto masterArea = area.removeFromRight(120);
	area.removeFromRight(10);
	channelsViewport.setBounds(area);

	const int channelHeight = getHeight() - 10;
	const int channelSpacing = 5;
	const int numChannels = (int)mixerChannels.size();

	const int availableWidth = area.getWidth() - 10;
	const int minChannelWidth = 60;
	const int maxChannelWidth = 120;

	const int channelWidth = numChannels > 0
		? juce::jlimit(minChannelWidth, maxChannelWidth,
			(availableWidth - (numChannels - 1) * channelSpacing) / numChannels)
		: minChannelWidth;

	const int totalChannelsWidth = numChannels * channelWidth
		+ (numChannels - 1) * channelSpacing
		+ 10;

	channelsContainer.setSize(totalChannelsWidth, channelHeight);

	masterChannel->setBounds(masterArea.getX() + 5,
		masterArea.getY(),
		masterArea.getWidth() - 10,
		channelHeight);

	int xPos = 5;
	for (auto& channel : mixerChannels)
	{
		channel->setBounds(xPos, 0, channelWidth, channelHeight);
		xPos += channelWidth + channelSpacing;
	}
}

void MixerPanel::trackAdded(const juce::String& /*trackId*/)
{
	refreshMixerChannels();
	resized();
}

void MixerPanel::trackRemoved(const juce::String& /*trackId*/)
{
	refreshMixerChannels();
	resized();
}

void MixerPanel::refreshAllChannels()
{
	for (auto& mixerChannel : mixerChannels)
	{
		if (mixerChannel && mixerChannel->track)
		{
			mixerChannel->cleanup();
			mixerChannel->addEventListeners();
			mixerChannel->updateFromTrackData();
		}
	}
}

void MixerPanel::trackSelected(const juce::String& trackId)
{
	for (auto& channel : mixerChannels)
	{
		bool isThisTrackSelected = (channel->getTrackId() == trackId);
		channel->setSelected(isThisTrackSelected);
	}
}

void MixerPanel::startGeneratingAnimationForTrack(const juce::String& trackId)
{
	for (auto& channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->startGeneratingAnimation();
			break;
		}
	}
}

void MixerPanel::clearSamplePending(const juce::String& trackId)
{
	for (auto& channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->setSamplePending(false);
			break;
		}
	}
}

void MixerPanel::stopGeneratingAnimationForTrack(const juce::String& trackId)
{
	for (auto& channel : mixerChannels)
	{
		if (channel->getTrackId() == trackId)
		{
			channel->stopGeneratingAnimation();
			channel->setSamplePending(true);
			break;
		}
	}
}