#pragma once
#include "IconButton.h"
#include "ObsidianBase.h"
#include "Shades.h"
#include <JuceHeader.h>
#include <functional>
#include <vector>

class DjIaVstProcessor;

class TrackSelectorBar : public ObsidianComponent
{
  public:
	TrackSelectorBar(DjIaVstProcessor &processor, bool includeMasterButton);

	void refresh();
	void resized() override;
	void selectTrack(const juce::String &trackId);
	void selectMaster();
	void updateModelColour(const juce::String &trackId);
	void setMasterButtonVisible(bool shouldBeVisible);

	std::function<void(const juce::String &trackId)> onTrackSelected;
	std::function<void()> onMasterSelected;

  private:
	DjIaVstProcessor &audioProcessor;
	bool includeMasterButton;
	std::vector<std::unique_ptr<IconButtonSimple>> buttons;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackSelectorBar)
};