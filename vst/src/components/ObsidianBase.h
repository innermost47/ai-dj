#pragma once
#include <JuceHeader.h>

class ObsidianComponent : public juce::Component
{
public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}
};