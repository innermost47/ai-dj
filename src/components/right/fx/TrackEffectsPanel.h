#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class FilterComponent;

class TrackEffectsPanel : public ObsidianComponent
{
  public:
	TrackEffectsPanel(DjIaVstProcessor &processor);
	~TrackEffectsPanel() override = default;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void updateModelUI(const juce::String &trackId);

  private:
	DjIaVstProcessor &audioProcessor;
	juce::String activeTrackId;
	std::vector<std::unique_ptr<FilterComponent>> filterComponents;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackEffectsPanel)
};