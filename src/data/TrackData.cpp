#include "TrackData.h"
#include "TrackStretchImpl.h"

TrackData::TrackData()
    : stretchImpl(std::make_unique<TrackStretchImpl>()), trackId(juce::Uuid().toString()), readPosition(0.0),
      onPlayStateChanged(nullptr)
{
	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pages[i].reset();
}

TrackData::~TrackData()
{
	onPlayStateChanged = nullptr;
	onArmedStateChanged = nullptr;
	onArmedToStopStateChanged = nullptr;
	onPageChanged = nullptr;
}

bool TrackData::allSequencerStepsAreFalse() const
{

	auto &seqData = getCurrentSequencerData();
	for (const auto &measure : seqData.steps)
		for (bool step : measure)
			if (step)
				return false;
	return true;
}

void TrackData::setPlaying(bool playing)
{
	bool wasPlaying = isPlaying.load();
	isPlaying = playing;
	if (wasPlaying != playing && onPlayStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 &&
	    isPlaying.load())
	{
		juce::WeakReference<TrackData> weakThis(this);
		juce::MessageManager::callAsync(
		    [weakThis, playing]()
		    {
			    if (auto *self = weakThis.get())
				    if (self->onPlayStateChanged)
					    self->onPlayStateChanged(playing);
		    });
	}
}

void TrackData::setArmed(bool armed)
{
	bool wasArmed = isArmed.load();
	isArmed = armed;
	if (wasArmed != armed && onArmedStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 &&
	    isPlaying.load())
	{
		juce::WeakReference<TrackData> weakThis(this);
		juce::MessageManager::callAsync(
		    [weakThis, armed]()
		    {
			    if (auto *self = weakThis.get())
				    if (self->onArmedStateChanged)
					    self->onArmedStateChanged(armed);
		    });
	}
}

void TrackData::setArmedToStop(bool armedToStop)
{
	isArmedToStop = armedToStop;
	if (onArmedToStopStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 && isCurrentlyPlaying.load())
	{
		juce::WeakReference<TrackData> weakThis(this);
		juce::MessageManager::callAsync(
		    [weakThis, armedToStop]()
		    {
			    if (auto *self = weakThis.get())
				    if (self->onArmedToStopStateChanged)
					    self->onArmedToStopStateChanged(armedToStop);
		    });
	}
}

void TrackData::setStop()
{
	juce::WeakReference<TrackData> weakThis(this);
	juce::MessageManager::callAsync(
	    [weakThis]()
	    {
		    if (auto *self = weakThis.get())
			    if (self->onPlayStateChanged)
				    self->onPlayStateChanged(false);
	    });
}

void TrackData::updateFromRequest(const DjIaClient::LoopRequest &request)
{
	auto &currentPage = getCurrentPage();
	currentPage.generationPrompt = request.prompt;
	currentPage.generationBpm = request.bpm;
	currentPage.generationKey = request.key;
	currentPage.generationDuration = static_cast<int>(request.generationDuration);
	currentPage.selectedModel = request.model;
}

void TrackData::reset()
{
	for (int i = 0; i < Obsidian::MAX_PAGES; ++i)
		pages[i].reset();

	currentPageIndex.store(0);

	readPosition = 0.0;
	isEnabled = true;
	isMuted = false;
	isSolo = false;
	volume = 0.8f;
	pan = 0.0f;
	isVersionSwitch = false;
	preservedLoopStart = 0.0;
	preservedLoopEnd = 4.0;
	preservedLoopLocked = false;
}

void TrackData::setCurrentPage(int pageIndex)
{
	if (pageIndex < 0 || pageIndex >= Obsidian::MAX_PAGES)
		return;
	if (currentPageIndex.load() == pageIndex)
		return;

	currentPageIndex.store(pageIndex);

	if (onPageChanged)
		onPageChanged();
}

TrackData::DeckSide TrackData::getDeckSide() const
{
	return (slotIndex >= 0 && slotIndex < Obsidian::MAX_CROSSFADER_PAIR) ? DeckSide::A : DeckSide::B;
}

int TrackData::getPairIndex() const
{
	if (slotIndex < 0 || slotIndex >= 8)
		return -1;
	return slotIndex % 4;
}

int TrackData::getPartnerSlotIndex() const
{
	if (slotIndex < 0 || slotIndex >= 8)
		return -1;
	return (slotIndex < Obsidian::MAX_CROSSFADER_PAIR) ? slotIndex + Obsidian::MAX_CROSSFADER_PAIR
	                                                   : slotIndex - Obsidian::MAX_CROSSFADER_PAIR;
}

bool TrackData::isDeckA() const
{
	return getDeckSide() == DeckSide::A;
}

bool TrackData::isDeckB() const
{
	return getDeckSide() == DeckSide::B;
}
