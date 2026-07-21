#pragma once
#include "DjIaClient.h"
#include <JuceHeader.h>
#include <memory>

class DjIaVstProcessor;

struct LocalGenerationResult
{
	bool success = false;
	std::vector<uint8_t> audioData;
	float actualDuration = 0.0f;
	juce::String errorMessage;
};

class LocalGenerationEngine
{
  public:
	virtual ~LocalGenerationEngine() = default;
	virtual LocalGenerationResult generate(const DjIaClient::LoopRequest &request, double hostSampleRate) = 0;
};

std::unique_ptr<LocalGenerationEngine> createLocalGenerationEngine(DjIaVstProcessor &processor);