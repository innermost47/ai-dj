#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;
class TrackComponent;

class UITrackManager
{
  public:
	explicit UITrackManager(DjIaVstEditor &editor);
	~UITrackManager();

	void refreshTracks();
	void refreshTrackComponents();
	void updateSelectedTrack();
	void onSampleLoaded(const juce::String &trackId);
	void refreshUIForMode();
	void checkLocalModelsAndNotify();
	void updateUIComponents();
	void detachAllListeners();
	void forceFullRefresh();

	std::vector<std::unique_ptr<TrackComponent>> &getTrackComponents()
	{
		return trackComponents;
	}
	TrackComponent *getTrackComponent(const juce::String &trackId);

  private:
	DjIaVstEditor &editor;
	std::vector<std::unique_ptr<TrackComponent>> trackComponents;
};