#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class TrackEffectsPanel : public ObsidianComponent
{
  public:
	TrackEffectsPanel(DjIaVstProcessor &processor);
	~TrackEffectsPanel() override = default;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void setActiveTrackId(const juce::String &trackId);

  private:
	DjIaVstProcessor &audioProcessor;
	juce::String activeTrackId;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackEffectsPanel)
};