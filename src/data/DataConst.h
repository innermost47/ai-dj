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

inline constexpr float COMPRESSOR_THRESHOLD = -12.f;
inline constexpr float COMPRESSOR_RATIO = 4.f;
inline constexpr float COMPRESSOR_ATTACK = 10.f;
inline constexpr float COMPRESSOR_RELEASE = 100.f;
inline constexpr float COMPRESSOR_MAKEUP_GAIN = 1.f;

inline constexpr float DISTORTION_PRE = 0.f;
inline constexpr float DISTORTION_POST = 0.f;
inline constexpr float DISTORTION_CUT = 1000.f;

inline constexpr float EQ_BANDS_GAIN = 1.f;
inline constexpr float EQ_SUB_BAS_FRQ = 40.f;
inline constexpr float EQ_BASS_FRQ = 120.f;
inline constexpr float EQ_LOW_MID_FRQ = 350.f;
inline constexpr float EQ_MID_FRQ = 1000.f;
inline constexpr float EQ_HI_MID_FRQ = 3000.f;
inline constexpr float EQ_PRESENCE_FRQ = 5000.f;
inline constexpr float EQ_HI_FRQ = 8000.f;
inline constexpr float EQ_AIR_FRQ = 15000.f;
inline constexpr float EQ_BASE_RESONANCE = 0.707f;

inline constexpr bool COMPRESSOR_BYPASSED = false;
inline constexpr bool LIMITER_BYPASSED = false;
inline constexpr bool EQ_BYPASSED = false;
inline constexpr bool FILTER_BYPASSED = false;
inline constexpr bool DISTORTION_BYPASSED = true;

enum RadioGroupIDs
{
	FilterTypeGroup = 1,
	DelayDivisionGroup = 2,
	DelayModeGroup = 3,
	TrackFXSelector = 4,
	DistortionType = 5
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

enum distortionChain
{
	filter = 0,
	preGain = 1,
	waveshaper = 2,
	postGain = 3
};

enum compressorChain
{
	compressor = 0,
	makeUpGain = 1
};

enum distortionType
{
	soft = 0,
	hard = 1,
	tube = 2,
	fold = 3,
	diode = 4,
	cubic = 5
};

} // namespace Obsidian
#endif
