#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_DATA_H
#define OBSIDIAN_DATA_H

namespace ObsidianDataConst
{
static constexpr int MAX_STEPS_PER_MEASURE = 32;
static constexpr int MAX_MEASURES = 4;
static constexpr int MAX_PAGES = 4;
static constexpr int MAX_TRACKS = 8;
static constexpr int MAX_SEQUENCES = 8;
static constexpr int MAX_CROSSFADER_PAIR = 4;
static constexpr int MAX_BLOCK_SIZE = 512;

static constexpr double SAMPLERATE = 48000.0;
} // namespace ObsidianDataConst

#endif
