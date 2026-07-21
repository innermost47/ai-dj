#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class TrackRecapPanel : public ObsidianComponent
{
  public:
	TrackRecapPanel(DjIaVstProcessor &processor);
	~TrackRecapPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	int getPreferredHeight() const;

  private:
	void handleVBlank();
	void paintTrackCard(juce::Graphics &g, juce::Rectangle<int> bounds, int trackIndex);

	DjIaVstProcessor &audioProcessor;

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	std::map<juce::String, int> lastActivePages;
	std::map<juce::String, juce::String> lastPrompts;

	static constexpr int CARD_HEIGHT = 86;
	static constexpr int CARD_SPACING = 3;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRecapPanel)
};