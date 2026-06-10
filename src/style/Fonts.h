#pragma once
#include <JuceHeader.h>
#ifndef OBSIDIAN_FONTS_H
#define OBSIDIAN_FONTS_H
namespace Obsidian
{
inline const juce::Typeface::Ptr &notoBold()
{
	static const juce::Typeface::Ptr tf =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::notobold_ttf, BinaryData::notobold_ttfSize);
	return tf;
}
inline const juce::Typeface::Ptr &notoItalic()
{
	static const juce::Typeface::Ptr tf =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::notoitalic_ttf, BinaryData::notoitalic_ttfSize);
	return tf;
}
inline const juce::Typeface::Ptr &notoRegular()
{
	static const juce::Typeface::Ptr tf =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::notoregular_ttf, BinaryData::notoregular_ttfSize);
	return tf;
}
inline const juce::Typeface::Ptr &michroma()
{
	static const juce::Typeface::Ptr tf =
	    juce::Typeface::createSystemTypefaceFor(BinaryData::michroma_ttf, BinaryData::michroma_ttfSize);
	return tf;
}
inline static void applyFontSize(juce::Label &l, float newSize)
{
	l.setFont(l.getFont().withHeight(newSize));
}
} // namespace Obsidian
#endif