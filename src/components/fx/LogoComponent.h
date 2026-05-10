#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class LogoComponent : public ObsidianComponent
{
  public:
	LogoComponent();

	void paint(juce::Graphics &g) override;

  private:
	juce::Image logoImage;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogoComponent)
};