#pragma once
#include "OnboardingStepData.h"
#include <JuceHeader.h>
#include <vector>

class DjIaVstEditor;
class UIModalManager;

class OnboardingFlow
{
  public:
	OnboardingFlow(DjIaVstEditor &editor, UIModalManager &modalManager, OnboardingVariant variant);
	~OnboardingFlow();

	void start();
	void showStep(int stepIndex);

  private:
	void finish();
	void skip();

	void buildSteps();

	DjIaVstEditor &editor;
	UIModalManager &modalManager;
	OnboardingVariant variant;

	std::vector<OnboardingStepData> steps;
	int currentStepIndex{0};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnboardingFlow)
};