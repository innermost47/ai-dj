#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class SplashScreen : public ObsidianComponent, private juce::Timer
{
  public:
	SplashScreen();
	~SplashScreen() override = default;

	void paint(juce::Graphics &g) override;

  private:
	void timerCallback() override;
	juce::Image logoImage;
	float progress = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreen)
};