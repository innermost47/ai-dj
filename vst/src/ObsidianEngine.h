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
		juce::String key = "C Aeolian";
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

private:
	std::unique_ptr<StableAudioEngine> stableAudioEngine;

	juce::File appDataDir;
	juce::String currentUserId = "default_user";

public:
	bool initialize()
	{
		appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
						 .getChildFile("OBSIDIAN-Neural");
		auto stableAudioDir = appDataDir.getChildFile("stable-audio");

		stableAudioEngine = std::make_unique<StableAudioEngine>();
		if (!stableAudioEngine->initialize(stableAudioDir.getFullPathName()))
		{
			DBG("Failed to initialize Stable Audio engine");
			return false;
		}

		DBG("OBSIDIAN Neural engines ready!");
		return true;
	}

	void generateLoopAsync(const LoopRequest &request, GenerationCallback callback)
	{
		juce::Thread::launch([this, request, callback]()
							 {
				auto response = generateLoop(request);
				juce::MessageManager::callAsync([callback, response]() {
					callback(response);
					}); });
	}

private:
	LoopResponse generateLoop(const LoopRequest &request)
	{
		LoopResponse response;
		try
		{

			juce::String optimizedPrompt = (request.prompt + " " + juce::String(request.bpm) + "bpm " + request.key).toStdString();

			StableAudioEngine::GenerationParams audioParams;
			audioParams.prompt = optimizedPrompt;
			audioParams.duration = request.generationDuration;
			audioParams.numThreads = 4;
			audioParams.seed = -1;

			auto audioResult = stableAudioEngine->generateSample(audioParams);

			if (audioResult.success)
			{
				response.success = true;
				response.audioData = audioResult.audioData;
				response.leftChannel = audioResult.leftChannel;
				response.rightChannel = audioResult.rightChannel;
				response.actualDuration = audioResult.actualDuration;
				response.duration = audioResult.actualDuration;
				response.bpm = request.bpm;
				response.optimizedPrompt = optimizedPrompt;
				response.stemsUsed = request.preferredStems;
				DBG("Generation successful! Duration: " << response.actualDuration << "s");
			}
			else
			{
				response.success = false;
				response.errorMessage = "Audio generation failed: " + audioResult.errorMessage;
			}
		}
		catch (const std::exception &e)
		{
			response.success = false;
			response.errorMessage = juce::String("Generation exception: ") + e.what();
		}
		return response;
	}
};