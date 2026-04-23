#pragma once
#include "components/ObsidianBase.h"

class MixerChannel;
class DjIaVstProcessor;
class MasterChannel;

class MixerPanel : public ObsidianComponent
{
public:
	MixerPanel(DjIaVstProcessor& processor);
	~MixerPanel();

	void updateTrackName(const juce::String& trackId, const juce::String& newName);
	void updateAllMixerComponents();

	void calculateMasterLevel();
	void refreshMixerChannels();
	void refreshAllChannels();

	void trackSelected(const juce::String& trackId);
	void updateModelUI(const juce::String& trackId);
	void paint(juce::Graphics& g) override;
	void resized() override;
	void startGeneratingAnimationForTrack(const juce::String& trackId);
	void stopGeneratingAnimationForTrack(const juce::String& trackId);
	void clearSamplePending(const juce::String& trackId);

	std::function<void(const juce::String& trackId, const juce::String& newName)> onTrackRenamedFromMixer;

private:
	DjIaVstProcessor& audioProcessor;

	std::unique_ptr<MasterChannel> masterChannel;
	float masterVolume = 0.8f;
	float masterPan = 0.0f;

	juce::Viewport channelsViewport;
	juce::Component channelsContainer;
	std::vector<std::unique_ptr<MixerChannel>> mixerChannels;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};