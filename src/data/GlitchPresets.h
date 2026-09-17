#pragma once
#include "GlitchTypes.h"
#include <JuceHeader.h>

struct TrackData;

struct GlitchPreset
{
	const char *name;
	const char *sequences[8];
};

const GlitchPreset *getFactoryGlitchPresets();
int getNumFactoryGlitchPresets();

GlitchEffectType glitchEffectFromChar(char c);
char charFromGlitchEffect(GlitchEffectType type);

void applyGlitchSequencesFromStrings(TrackData &track, const juce::String sequences[8]);
void applyFactoryGlitchPreset(TrackData &track, int presetIndex);