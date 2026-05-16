#pragma once
#include <JuceHeader.h>
#include <vector>

enum class OnboardingVariant
{
	Standalone,
	VST
};

struct ConceptRow
{
	juce::String iconName;
	juce::String title;
	juce::String body;
};

struct OnboardingStepData
{
	juce::String title;
	juce::String headline;
	juce::String lead;
	juce::String leadStandaloneOnly;
	juce::String leadVstOnly;
	std::vector<ConceptRow> rows;

	juce::String getLeadForVariant(OnboardingVariant variant) const
	{
		juce::String result = lead;
		if (variant == OnboardingVariant::Standalone && leadStandaloneOnly.isNotEmpty())
			result += "\n\n" + leadStandaloneOnly;
		else if (variant == OnboardingVariant::VST && leadVstOnly.isNotEmpty())
			result += "\n\n" + leadVstOnly;
		return result;
	}
};