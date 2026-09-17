#pragma once
#include "ObsidianBase.h"
#include "TrackSelectorBar.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class StandaloneTransportComponent;
class StandaloneTransport;
class MasterChannel;
class LCDScreen;
class TrackEffectsPanel;
class ModulationPanel;
class ConfigComponent;
class SendsPanel;
class TrackRecapPanel;
class DjIaVstEditor;
class GlitchSequencerPanel;

class RightPanelWrapper : public ObsidianComponent
{
  public:
	RightPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~RightPanelWrapper() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void setLCDScreen(LCDScreen *lcd);
	void setStandaloneTransport(StandaloneTransport *transport);
	void calculateMasterLevel();
	void updateComponents();
	void restoreUIState(const juce::var &state);
	void refreshAfterStateLoad();
	void selectTrack(const juce::String &trackId);

	juce::var saveUIState() const;

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
	GlitchSequencerPanel *getGlitchSequencerPanel()
	{
		return glitchSequencerPanel.get();
	}
	TrackSelectorBar *getTrackSelectorBar()
	{
		return trackSelectorBar.get();
	}
	ModulationPanel *getModulationPanel()
	{
		return modulationPanel.get();
	}

  private:
	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	std::unique_ptr<TrackRecapPanel> trackRecap;
	std::unique_ptr<TrackEffectsPanel> trackEffects;
	std::unique_ptr<SendsPanel> sendsPanel;
	std::unique_ptr<ConfigComponent> configComponent;
	std::unique_ptr<GlitchSequencerPanel> glitchSequencerPanel;
	std::unique_ptr<TrackSelectorBar> trackSelectorBar;
	std::unique_ptr<ModulationPanel> modulationPanel;

	std::map<juce::String, int> scrollPositions;

	IconButtonSimple fxTabButton{"fx"};
	IconButtonSimple infoTabButton{"info"};
	IconButtonSimple modTabButton{"ModTab", ""};
	IconButtonSimple glitchTabButton{"glitch"};

	juce::String selectedTrackId;

	std::unique_ptr<StandaloneTransportComponent> standaloneTransport;

	LCDScreen *lcdScreen = nullptr;

	std::unique_ptr<MasterChannel> masterChannel;

	float masterVolume = 0.8f;
	float masterPan = 0.0f;

	int activeTab = 0;

	bool isRestoringState = false;

	juce::Component scrollContent;
	juce::Component tabRowContainer;

	juce::Viewport contentViewport;

	juce::String scrollKey(int tab, const juce::String &trackId) const;

	void stashScrollPosition();
	void restoreScrollPosition();
	void setupUI();
	void setActiveTab(int tab);
	void layoutScrollContent();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanelWrapper)
};