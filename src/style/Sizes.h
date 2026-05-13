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
inline constexpr int SPACER = 4;
inline constexpr int SPACER_MD = 6;
inline constexpr int PADDING = 6;
inline constexpr int MIN_SMALL_BTN_WIDTH = 24;
inline constexpr int MIN_SMALL_BTN_HEIGHT = 24;
inline constexpr int CORNER = 1;
inline constexpr int SAMPLE_DETAIL_HEIGHT = 86;
inline constexpr int SAMPLE_ROW_HEIGHT = 54;
inline constexpr int MIXER_CHANNEL_KNOB = 36;
inline constexpr int COMBO_BOX_BASE_HEIGHT = 20;

inline constexpr int TRACK_BASE_HEIGHT = 80;
inline constexpr int WAVEFORM_HEIGHT = 45;
inline constexpr int SEQUENCER_HEIGHT = 45;
inline constexpr int PAGE_BUTTON_SIZE = 16;
inline constexpr int TAB_BAR_HEIGHT = 20;

inline constexpr int TITLE_PANEL_HEIGHT = 28;
inline constexpr int INFO_PANEL_HEIGHT = 32;

inline constexpr int SCALE_AND_DURATION_HEIGHT = 114;
inline constexpr int CONFIG_AREA_HEIGHT = 138;

inline constexpr float LIST_PANEL_CORNER_SIZE = 1.0f;
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
inline constexpr float TEXT_XXS = 10.0f;
inline constexpr float TEXT_XXXS = 8.0f;

inline constexpr int ACCORDION_HEADER_HEIGHT = 32;
inline constexpr int ACCORDION_ITEM_SPACING = 2;
inline constexpr int ACCORDION_ACCENT_BAR_WIDTH = 4;
inline constexpr int ACCORDION_CHEVRON_AREA_WIDTH = 24;
inline constexpr int ACCORDION_TEXT_LEFT_PADDING = 10;

inline constexpr int ACCORDION_ITEM_MIN_HEIGHT = 50;
inline constexpr int ACCORDION_ITEM_MAX_LINES = 4;

inline constexpr int ACCORDION_LOCK_ICON_SIZE = 12;
inline constexpr int ACCORDION_LOCK_AREA_WIDTH = 18;
} // namespace ObsidianSizes

#endif