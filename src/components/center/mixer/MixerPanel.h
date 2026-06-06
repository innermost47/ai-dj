#pragma once
#include "CrossfaderComponent.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class MixerChannel;
class DjIaVstProcessor;
class DjIaVstEditor;

class MixerPanel : public ObsidianComponent
{
  public:
	MixerPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~MixerPanel();

	void updateTrackName(const juce::String &trackId, const juce::String &newName);
	void updateAllMixerComponents();
	void calculateMasterLevel();
	void refreshMixerChannels();
	void refreshAllChannels();
	void refreshChannel(const juce::String &trackId);
	void updateModelUI(const juce::String &trackId);
	void paint(juce::Graphics &g) override;
	void resized() override;
	void startGeneratingAnimationForTrack(const juce::String &trackId);
	void stopGeneratingAnimationForTrack(const juce::String &trackId);
	void clearSamplePending(const juce::String &trackId);
	void detachAllTracks();
	void trackSelected(const juce::String &trackId);

	std::function<void(const juce::String &trackId, const juce::String &newName)> onTrackRenamedFromMixer;
	CrossfaderComponent *getCrossfader()
	{
		return crossfader.get();
	}

  private:
	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	void setupUI();

	std::unique_ptr<CrossfaderComponent> crossfader;

	juce::Viewport deckAViewport;
	juce::Component deckAContainer;
	juce::Viewport deckBViewport;
	juce::Component deckBContainer;

	std::vector<std::unique_ptr<MixerChannel>> mixerChannels;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};