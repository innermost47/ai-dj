#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

namespace AiModelDefinitions
{

static const juce::String LOCAL_MODEL_NAME = "stable-audio-open-small-tflite";

inline const juce::StringArray &getAvailableModels()
{
	static const juce::StringArray models = {"stable-audio-open-1.0",
	                                         "foundation-1",
	                                         "audialab-edm-elements",
	                                         "rc-infinite-pianos",
	                                         "rc-vocal-textures",
	                                         "sao-instrumental",
	                                         "stablebeat",
	                                         "gluten-v1",
	                                         "stable-audio-open-small-tflite"};
	return models;
}

inline juce::StringArray getModelsForMode(bool isLocalMode)
{
	auto &all = getAvailableModels();
	if (isLocalMode)
		return {LOCAL_MODEL_NAME};

	juce::StringArray filtered;
	for (auto &m : all)
		if (m != LOCAL_MODEL_NAME)
			filtered.add(m);
	return filtered;
}

inline juce::Colour getColourForModel(const juce::String &modelName)
{
	if (modelName.isEmpty())
		return ColourPalette::modelStableAudio;
	auto &models = getAvailableModels();
	if (modelName == models[0])
		return ColourPalette::modelStableAudio;
	if (modelName == models[1])
		return ColourPalette::modelFoundation;
	if (modelName == models[2])
		return ColourPalette::modelEdm;
	if (modelName == models[3])
		return ColourPalette::modelPianos;
	if (modelName == models[4])
		return ColourPalette::modelVocals;
	if (modelName == models[5])
		return ColourPalette::modelInstrumental;
	if (modelName == models[6])
		return ColourPalette::modelBeats;
	if (modelName == models[7])
		return ColourPalette::modelGluten;
	if (modelName == models[8])
		return ColourPalette::modelStableAudioTflite;
	return ColourPalette::modelFoundation;
}
} // namespace AiModelDefinitions