#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class TrackRecapPanel : public ObsidianComponent, private juce::Timer
{
  public:
	TrackRecapPanel(DjIaVstProcessor &processor);
	~TrackRecapPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	int getPreferredHeight() const;

  private:
	void timerCallback() override;
	void paintTrackCard(juce::Graphics &g, juce::Rectangle<int> bounds, int trackIndex);

	DjIaVstProcessor &audioProcessor;

	static constexpr int CARD_HEIGHT = 86;
	static constexpr int CARD_SPACING = 3;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRecapPanel)
};