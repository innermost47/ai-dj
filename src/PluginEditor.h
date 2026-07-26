#pragma once
#include "CustomLookAndFeel.h"
#include "IconButton.h"
#include "LCDScreen.h"
#include "MasterWaveformDisplay.h"
#include "MidiLearnableComponents.h"
#include "MixerPanel.h"
#include "ObsidianModal.h"
#include "PluginProcessor.h"
#include "TrackComponent.h"
#include "UIGenerationManager.h"
#include "UILayoutManager.h"
#include "UIMidiManager.h"
#include "UIModalManager.h"
#include "UIPresetManager.h"
#include "UIStatusManager.h"
#include "UITrackManager.h"
#include <JuceHeader.h>

class SequencerComponent;

class DjIaVstEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, public ModalHost
{
  public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	explicit DjIaVstEditor(DjIaVstProcessor &);

	~DjIaVstEditor() override;

	std::unique_ptr<MixerPanel> mixerPanel;
	std::unique_ptr<UILayoutManager> uiLayoutManager;
	std::unique_ptr<UIStatusManager> uiStatusManager;
	std::unique_ptr<UIModalManager> uiModalManager;
	std::unique_ptr<UIGenerationManager> uiGenerationManager;
	std::unique_ptr<UITrackManager> uiTrackManager;
	std::unique_ptr<UIPresetManager> uiPresetManager;
	std::unique_ptr<UIMidiManager> uiMidiManager;
	std::unique_ptr<LCDScreen> lcdScreen;
	std::unique_ptr<MasterWaveformDisplay> masterWaveformDisplay;

	juce::Viewport mainViewport;

	juce::Label statusLabel;

	std::atomic<bool> isBeingDestroyed{false};

	void paint(juce::Graphics &) override;
	void resized() override;
	void handleVBlank();
	void updateUIFromProcessor();
	void refreshMixerChannels();
	void initUI();
	void *getSequencerForTrack(const juce::String &trackId);
	void restoreUICallbacks();
	bool keyStateChanged(bool isKeyDown) override;
	void addModal(std::unique_ptr<ObsidianModalOverlay> overlay) override;
	void removeModal(ObsidianModalOverlay *overlay) override;

	MixerPanel *getMixerPanel()
	{
		return mixerPanel.get();
	}

	DjIaVstProcessor &audioProcessor;
	std::unique_ptr<juce::TooltipWindow> tooltipWindow;

	void visibilityChanged() override;
	void openMidiMappingEditor();
	void setupUI();
	void addEventListeners();
	void finalizeInit();

	bool keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected);
	bool keyPressed(const juce::KeyPress &key) override;

#if JucePlugin_Build_Standalone
	void parentHierarchyChanged() override;
#endif

	void setupScreen();

	float currentScaleFactor = 1.0f;

	int skipFrames = 0;
	double lastHostBpm = 0.0;

	bool mixerVisible = false;
	bool isFullscreen = false;
	bool waitingForState = false;

	std::atomic<bool> isInitialized{false};
	std::atomic<bool> isRefreshingTracks{false};
	std::atomic<bool> canPersistSize{false};

	juce::Typeface::Ptr customFont;
	juce::Label midiIndicator;
	juce::String lastMidiNote;
	juce::Label creditsLabel;

	CustomLookAndFeel customLookAndFeel;

	float getUIScale() const noexcept
	{
		return getWidth() / (float)Obsidian::BASE_PLUGIN_WIDTH;
	}

  private:
	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DjIaVstEditor)
};