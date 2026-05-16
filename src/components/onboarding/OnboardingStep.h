#pragma once
#include "ConceptRowComponent.h"
#include "ObsidianBase.h"
#include "OnboardingStepData.h"
#include <JuceHeader.h>
#include <memory>
#include <vector>

class OnboardingStep : public ObsidianComponent
{
  public:
	OnboardingStep(const OnboardingStepData &data, OnboardingVariant variant);
	~OnboardingStep() override;

	int getPreferredHeight(int width) const;

	void resized() override;

  private:
	OnboardingStepData data;
	OnboardingVariant variant;

	juce::Label headlineLabel;
	juce::Label leadLabel;
	std::vector<std::unique_ptr<ConceptRowComponent>> rowComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnboardingStep)
};