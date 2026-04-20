#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"

namespace AiModelDefinitions
{
	inline const juce::StringArray& getAvailableModels()
	{
		static const juce::StringArray models = {
			"stable-audio-open-1.0",
			"foundation-1",
			"audialab-edm-elements",
			"rc-infinite-pianos",
			"rc-vocal-textures",
			"sao-instrumental",
			"stablebeat",
			"gluten-v1"
		};
		return models;
	}

	inline juce::Colour getColourForModel(const juce::String& modelName)
	{
		auto& models = getAvailableModels();
		if (modelName == models[0]) return ColourPalette::modelStableAudio;
		if (modelName == models[1]) return ColourPalette::modelFoundation;
		if (modelName == models[2]) return ColourPalette::modelEdm;
		if (modelName == models[3]) return ColourPalette::modelPianos;
		if (modelName == models[4]) return ColourPalette::modelVocals;
		if (modelName == models[5]) return ColourPalette::modelInstrumental;
		if (modelName == models[6]) return ColourPalette::modelBeats;
		if (modelName == models[7]) return ColourPalette::modelGluten;
		return ColourPalette::modelFoundation;
	}
}