#pragma once
#include "GlitchStepButton.h"
#include "GlitchTypes.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>
#include <array>

class GlitchStepGrid : public ObsidianComponent
{
  public:
	GlitchStepGrid();

	void resized() override;

	void setSequence(GlitchSequence *sequence);
	void refreshFromSequence();
	void setAccentColour(juce::Colour colour);
	void setCurrentPlaybackStep(int stepIndex);
	void setStepsPerBar(int stepsPerBar);
	void setStepsPerBeat(int newStepsPerBeat);

	int getRequiredHeight() const;

	std::function<void()> onStepChanged;

  private:
	int stepsPerBeat = 4;
	static constexpr int stepHeight = 20;

	std::array<std::unique_ptr<GlitchStepButton>, GlitchSequence::MAX_STEPS> stepButtons;
	GlitchSequence *currentSequence = nullptr;
	juce::Colour accentColour = juce::Colours::orange;
	int stepsPerRow = 16;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchStepGrid)
};