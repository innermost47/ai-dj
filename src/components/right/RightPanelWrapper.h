#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class StandaloneTransportComponent;
class MasterWaveformDisplay;
class StandaloneTransport;
class MasterChannel;
class LCDScreen;
class TrackEffectsPanel;
class ConfigComponent;
class SendsPanel;
class TrackRecapPanel;
class DjIaVstEditor;

class RightPanelWrapper : public ObsidianComponent
{
  public:
	RightPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~RightPanelWrapper() override = default;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void setMasterWaveform(MasterWaveformDisplay *wf);
	void setLCDScreen(LCDScreen *lcd);
	void setStandaloneTransport(StandaloneTransport *transport);
	void calculateMasterLevel();
	void updateComponents();

	StandaloneTransportComponent *getStandaloneTransportComponent()
	{
		return standaloneTransport.get();
	}

	TrackRecapPanel *getTrackRecapPanel()
	{
		return trackRecap.get();
	}
	TrackEffectsPanel *getTrackEffectsPanel()
	{
		return trackEffects.get();
	}
	SendsPanel *getSendsPanel()
	{
		return sendsPanel.get();
	}
	ConfigComponent *getConfigComponent()
	{
		return configComponent.get();
	}

  private:
	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	std::unique_ptr<TrackRecapPanel> trackRecap;
	std::unique_ptr<TrackEffectsPanel> trackEffects;
	std::unique_ptr<SendsPanel> sendsPanel;
	std::unique_ptr<ConfigComponent> configComponent;

	MasterWaveformDisplay *masterWaveform = nullptr;

	std::unique_ptr<StandaloneTransportComponent> standaloneTransport;

	LCDScreen *lcdScreen = nullptr;

	std::unique_ptr<MasterChannel> masterChannel;

	float masterVolume = 0.8f;
	float masterPan = 0.0f;

	juce::Component scrollContent;
	juce::Viewport contentViewport;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanelWrapper)
};