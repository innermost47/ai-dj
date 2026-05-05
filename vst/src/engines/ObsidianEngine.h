#pragma once
#include <JuceHeader.h>
#include "StableAudioEngine.h"
#include <nlohmann/json.hpp>

class ObsidianEngine
{
public:
	struct LoopRequest
	{
		juce::String prompt;
		float generationDuration = 10.0f;
		float bpm = 120.0f;
		juce::String key = "C Minor";
		std::vector<juce::String> preferredStems;
	};

	struct LoopResponse
	{
		bool success = false;
		juce::String errorMessage;
		std::vector<float> audioData;
		std::vector<float> leftChannel;
		std::vector<float> rightChannel;
		float actualDuration = 0.0f;
		float bpm = 120.0f;
		float duration = 0.0f;
		juce::String optimizedPrompt;
		std::vector<juce::String> stemsUsed;
	};

	typedef std::function<void(LoopResponse)> GenerationCallback;

	bool initialize();

private:
	std::unique_ptr<StableAudioEngine> stableAudioEngine;
	juce::File appDataDir;
	juce::String currentUserId = "default_user";
};