#pragma once
#include <JuceHeader.h>

class ColourPalette
{
public:
	static const juce::Colour track1;
	static const juce::Colour track2;
	static const juce::Colour track3;
	static const juce::Colour track4;
	static const juce::Colour track5;
	static const juce::Colour track6;
	static const juce::Colour track7;
	static const juce::Colour track8;

	static const juce::Colour buttonPrimary;
	static const juce::Colour buttonSecondary;
	static const juce::Colour buttonDanger;
	static const juce::Colour buttonSuccess;
	static const juce::Colour buttonWarning;
	static const juce::Colour buttonDangerLight;
	static const juce::Colour buttonDangerDark;

	static const juce::Colour backgroundDark;
	static const juce::Colour backgroundMid;
	static const juce::Colour backgroundLight;

	static const juce::Colour textPrimary;
	static const juce::Colour textSecondary;
	static const juce::Colour textDanger;
	static const juce::Colour textSuccess;
	static const juce::Colour textWarning;
	static const juce::Colour textAccent;

	static const juce::Colour backgroundDeep;
	static const juce::Colour sliderThumb;
	static const juce::Colour sliderTrack;

	static const juce::Colour vuPeak;
	static const juce::Colour vuClipping;

	static const juce::Colour vuGreen;
	static const juce::Colour vuOrange;
	static const juce::Colour vuRed;

	static const juce::Colour playActive;
	static const juce::Colour playArmed;
	static const juce::Colour muteActive;
	static const juce::Colour soloActive;
	static const juce::Colour soloText;
	static const juce::Colour stopActive;
	static const juce::Colour buttonInactive;

	static const juce::Colour trackSelected;

	static const juce::Colour sequencerAccent;
	static const juce::Colour sequencerBeat;
	static const juce::Colour sequencerSubBeat;

	static const juce::Colour credits;

	static const juce::Colour violet;
	static const juce::Colour emerald;
	static const juce::Colour coral;
	static const juce::Colour slate;
	static const juce::Colour indigo;
	static const juce::Colour teal;
	static const juce::Colour amber;
	static const juce::Colour textInactive;

	static const juce::Colour samplePending;

	static juce::Colour getTrackColour(int trackIndex);
	static juce::Colour withAlpha(const juce::Colour& colour, float alpha);
	static juce::Colour darken(const juce::Colour& colour, float amount = 0.2f);
	static juce::Colour lighten(const juce::Colour& colour, float amount = 0.2f);
};