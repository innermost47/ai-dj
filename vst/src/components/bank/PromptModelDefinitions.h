#pragma once
#include <JuceHeader.h>
#include <vector>

class PromptModelDefinitions
{
  public:
	struct KeywordGroup
	{
		juce::String label;
		juce::StringArray keywords;
	};

	struct ModelInfo
	{
		juce::String modelName;
		juce::String description;
		std::vector<KeywordGroup> keywordGroups;
		juce::StringArray examples;
	};

	static const std::vector<ModelInfo> &getAllModels();
	static const ModelInfo *getModel(const juce::String &modelName);
};