#include "TrackData.h"
#include "TrackStretchImpl.h"

TrackData::TrackData()
    : stretchImpl(std::make_unique<TrackStretchImpl>()), trackId(juce::Uuid().toString()), readPosition(0.0),
      onPlayStateChanged(nullptr)
{
	for (int i = 0; i < ObsidianDataConst::MAX_PAGES; ++i)
		pages[i].reset();
}

TrackData::~TrackData()
{
	onPlayStateChanged = nullptr;
	onArmedStateChanged = nullptr;
	onArmedToStopStateChanged = nullptr;
	onPageChanged = nullptr;
}