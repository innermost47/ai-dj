#pragma once
#include "CrossfaderComponent.h"
#include "LCDScreen.h"
#include "MasterWaveformDisplay.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class MixerChannel;
class DjIaVstProcessor;
class MasterChannel;
class MasterWaveformDisplay;
class LCDScreen;
class MixerPanel : public ObsidianComponent
{
  public:
	MixerPanel(DjIaVstProcessor &processor);
	~MixerPanel();
	void updateTrackName(const juce::String &trackId, const juce::String &newName);
	void updateAllMixerComponents();
	void calculateMasterLevel();
	void refreshMixerChannels();
	void refreshAllChannels();
	void trackSelected(const juce::String &trackId);
	void updateModelUI(const juce::String &trackId);
	void paint(juce::Graphics &g) override;
	void resized() override;
	void startGeneratingAnimationForTrack(const juce::String &trackId);
	void stopGeneratingAnimationForTrack(const juce::String &trackId);
	void clearSamplePending(const juce::String &trackId);
	void setMasterWaveform(MasterWaveformDisplay *wf);
	void setLCDScreen(LCDScreen *lcd);
	std::function<void(const juce::String &trackId, const juce::String &newName)> onTrackRenamedFromMixer;
	CrossfaderComponent *getCrossfader()
	{
		return crossfader.get();
	}

  private:
	DjIaVstProcessor &audioProcessor;
	std::unique_ptr<CrossfaderComponent> crossfader;
	MasterWaveformDisplay *masterWaveform = nullptr;
	LCDScreen *lcdScreen = nullptr;
	std::unique_ptr<MasterChannel> masterChannel;
	float masterVolume = 0.8f;
	float masterPan = 0.0f;
	juce::Viewport deckAViewport;
	juce::Component deckAContainer;
	juce::Viewport deckBViewport;
	juce::Component deckBContainer;
	std::vector<std::unique_ptr<MixerChannel>> mixerChannels;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};