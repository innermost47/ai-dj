#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class SplashScreen : public ObsidianComponent
{
  public:
	SplashScreen();
	~SplashScreen() override;

	void paint(juce::Graphics &g) override;

  private:
	void handleVBlank();

	juce::Image logoImage;
	float progress = 0.0f;

	double lastFrameTime = 0.0;
	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreen)
};