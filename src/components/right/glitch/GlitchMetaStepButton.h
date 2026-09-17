#pragma once
#include "DataConst.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class GlitchMetaStepButton : public ObsidianComponent
{
  public:
	GlitchMetaStepButton();

	void setValue(int newValue);
	int getValue() const
	{
		return value;
	}

	void setPlaybackStep(bool isCurrent);
	void setAccent(bool onBeat);

	std::function<void(int)> onValueChanged;

	void mouseDown(const juce::MouseEvent &e) override;
	void paint(juce::Graphics &g) override;

  private:
	int value = 0;
	bool isPlaybackStep = false;
	bool accent = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchMetaStepButton)
};