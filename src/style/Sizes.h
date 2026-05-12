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
inline constexpr int PADDING = 6;
inline constexpr int MIN_SMALL_BTN_WIDTH = 24;
inline constexpr int MIN_SMALL_BTN_HEIGHT = 24;
inline constexpr int CORNER = 1;
static constexpr int SAMPLE_DETAIL_HEIGHT = 86;
static constexpr int SAMPLE_ROW_HEIGHT = 54;
static constexpr int MIXER_CHANNEL_KNOB = 36;

static constexpr int TRACK_BASE_HEIGHT = 80;
static constexpr int WAVEFORM_HEIGHT = 45;
static constexpr int SEQUENCER_HEIGHT = 45;
static constexpr int PAGE_BUTTON_SIZE = 16;
static constexpr int TAB_BAR_HEIGHT = 20;

static constexpr int SCALE_AND_DURATION_HEIGHT = 92;

static constexpr float LIST_PANEL_CORNER_SIZE = 4.0f;
inline constexpr float HALF_CORNER = 0.5f;
inline constexpr float BORDER_WIDTH = 1.0f;
inline constexpr float BORDER_WIDTH_XL = 1.2f;
inline constexpr float BORDER_WIDTH_SM = 0.6f;

inline constexpr float MIXER_KNOB_LABEL = 9.0f;
inline constexpr float MIXER_LABEL_NAME = 13.0f;

inline constexpr float SEND_KNOB_LABEL = 12.0f;

inline constexpr float TEXT_XXL = 22.0f;
inline constexpr float TEXT_XL = 18.0f;
inline constexpr float TEXT_TITLE = 16.0f;
inline constexpr float TEXT_SUBTITLE = 16.0f;
inline constexpr float TEXT_REGULAR = 14.0f;
inline constexpr float TEXT_INFO = 13.0f;
inline constexpr float TEXT_SMALL = 12.0f;
inline constexpr float TEXT_XS = 11.0f;
} // namespace ObsidianSizes

#endif