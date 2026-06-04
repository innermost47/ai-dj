#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_DATA_H
#define OBSIDIAN_DATA_H

namespace Obsidian
{
inline constexpr int MAX_STEPS_PER_MEASURE = 32;
inline constexpr int MAX_MEASURES = 4;
inline constexpr int MAX_PAGES = 4;
inline constexpr int MAX_TRACKS = 8;
inline constexpr int MAX_SEQUENCES = 8;
inline constexpr int MAX_CROSSFADER_PAIR = 4;
inline constexpr int MAX_BLOCK_SIZE = 512;
inline constexpr int RNDM_RTRGR_INTRVL = 3;

inline constexpr double SAMPLERATE = 48000.0;

enum RadioGroupIDs
{
	FilterTypeGroup = 1,
	DelayDivisionGroup = 2,
	DelayModeGroup = 3,
	TrackFXSelector = 4
};

enum eqBands
{
	subBass = 0,
	bass = 1,
	lowMid = 2,
	mid = 3,
	highMid = 4,
	presence = 5,
	high = 6,
	air = 7
};

enum filterType
{
	lowShelf = 0,
	peakFilter = 1,
	highShelf = 2,
};

enum distorsionChain
{
	filter = 0,
	preGain = 1,
	waveshaper = 2,
	postGain = 3
};

enum distorsionType
{
	soft = 0,
	hard = 1,
	sigm = 2,
	arc = 3,
	fold = 4,
	crush = 5
};

} // namespace Obsidian
#endif
