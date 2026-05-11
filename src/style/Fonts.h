#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_FONTS_H
#define OBSIDIAN_FONTS_H

namespace ObsidianFonts
{
inline static auto NOTO_BOLD =
    juce::Typeface::createSystemTypefaceFor(BinaryData::notobold_ttf, BinaryData::notobold_ttfSize);
inline static auto NOTO_ITALIC =
    juce::Typeface::createSystemTypefaceFor(BinaryData::notoitalic_ttf, BinaryData::notoitalic_ttfSize);
inline static auto NOTO_REGULAR =
    juce::Typeface::createSystemTypefaceFor(BinaryData::notoregular_ttf, BinaryData::notoregular_ttfSize);
inline static auto MICHROMA =
    juce::Typeface::createSystemTypefaceFor(BinaryData::michroma_ttf, BinaryData::michroma_ttfSize);

inline static void applyFontSize(juce::Label &l, float newSize)
{
	l.setFont(l.getFont().withHeight(newSize));
}
} // namespace ObsidianFonts

#endif
