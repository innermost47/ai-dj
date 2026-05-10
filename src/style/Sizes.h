#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_SIZES_H
#define OBSIDIAN_SIZES_H

namespace ObsidianSizes
{
inline constexpr int GAP = 6;
inline constexpr int GAP_8 = 8;
inline constexpr int GAP_4 = 4;
inline constexpr int GAP_XL = 12;
inline constexpr int SPACER_XXS = 1;
inline constexpr int SPACER_XS = 2;
inline constexpr int SPACER_SM = 3;
inline constexpr int SPACER_MD = 6;
inline constexpr int MAIN_PADDING = 6;
inline constexpr int MIN_SMALL_BTN_WIDTH = 24;
inline constexpr int MIN_SMALL_BTN_HEIGHT = 24;
inline constexpr int CORNER = 6;
static constexpr int SAMPLE_DETAIL_HEIGHT = 86;
static constexpr int SAMPLE_ROW_HEIGHT = 50;
static constexpr int MIXER_CHANNEL_KNOB = 36;

static constexpr int TRACK_BASE_HEIGHT = 80;
static constexpr int WAVEFORM_HEIGHT = 45;
static constexpr int SEQUENCER_HEIGHT = 45;
static constexpr int PAGE_BUTTON_SIZE = 16;

static constexpr float LIST_PANEL_CORNER_SIZE = 4.0f;
inline constexpr float HALF_CORNER = 3.0f;
inline constexpr float BORDER_WIDTH = 1.0f;
} // namespace ObsidianSizes

#endif