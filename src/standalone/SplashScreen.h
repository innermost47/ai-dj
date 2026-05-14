#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class SplashScreen : public ObsidianComponent
{
  public:
	SplashScreen();
	~SplashScreen() override = default;

	void paint(juce::Graphics &g) override;

  private:
	juce::Image logoImage;
	float rotation = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreen)
};