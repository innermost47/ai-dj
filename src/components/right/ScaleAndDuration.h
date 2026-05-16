#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;

class ScaleAndDurationPanel : public ObsidianComponent
{
  public:
	ScaleAndDurationPanel(DjIaVstProcessor &processor);
	~ScaleAndDurationPanel() = default;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void update();

  private:
	DjIaVstProcessor &audioProcessor;
	juce::ComboBox keySelector;
	juce::ComboBox durationSelector;
	juce::Label titleLabel;
	juce::Label helpLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleAndDurationPanel)
};