#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_DATA_H
#define OBSIDIAN_DATA_H

namespace ObsidianDataConst
{
inline constexpr int MAX_STEPS_PER_MEASURE = 32;
inline constexpr int MAX_MEASURES = 4;
inline constexpr int MAX_PAGES = 4;
inline constexpr int MAX_TRACKS = 8;
inline constexpr int MAX_SEQUENCES = 8;
inline constexpr int MAX_CROSSFADER_PAIR = 4;
inline constexpr int MAX_BLOCK_SIZE = 512;

inline constexpr double SAMPLERATE = 48000.0;
} // namespace ObsidianDataConst

#endif
