#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class SendsPanel : public juce::Component
{
  public:
	SendsPanel(DjIaVstProcessor &processor);
	~SendsPanel() override = default;

	void paint(juce::Graphics &g) override;
	void resized() override;

  private:
	DjIaVstProcessor &audioProcessor;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendsPanel)
};