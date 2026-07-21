#pragma once
#include "ColourPalette.h"
#include "DataConst.h"
#include <JuceHeader.h>

namespace AiModelDefinitions
{

static const juce::String LOCAL_MODEL_NAME = Obsidian::STABLE_AUDIO_OPEN_LOCAL();

inline const juce::StringArray &getAvailableModels()
{
	static const juce::StringArray models = {Obsidian::STABLE_AUDIO_OPEN_V1(),
	                                         Obsidian::STABLE_AUDIO_OPEN_V3_MEDIUM(),
	                                         Obsidian::FOUNDATION_1(),
	                                         Obsidian::AUDIOLAB_EDM(),
	                                         Obsidian::INFINITE_PIANO(),
	                                         Obsidian::RC_VOCAL(),
	                                         Obsidian::SAO_INSTRUMENTAL(),
	                                         Obsidian::STABLEBEAT(),
	                                         Obsidian::GLUTEN_V1(),
	                                         Obsidian::STABLE_AUDIO_OPEN_LOCAL()};
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
		return ColourPalette::modelStableAudio3;
	if (modelName == models[2])
		return ColourPalette::modelFoundation;
	if (modelName == models[3])
		return ColourPalette::modelEdm;
	if (modelName == models[4])
		return ColourPalette::modelPianos;
	if (modelName == models[5])
		return ColourPalette::modelVocals;
	if (modelName == models[6])
		return ColourPalette::modelInstrumental;
	if (modelName == models[7])
		return ColourPalette::modelBeats;
	if (modelName == models[8])
		return ColourPalette::modelGluten;
	if (modelName == models[9])
		return ColourPalette::modelStableAudioTflite;
	return ColourPalette::modelFoundation;
}
} // namespace AiModelDefinitions