#pragma once
#include "CustomLookAndFeel.h"
#include "IconButton.h"
#include "LCDScreen.h"
#include "LeftPanelWrapper.h"
#include "MasterWaveformDisplay.h"
#include "MidiLearnableComponents.h"
#include "MixerPanel.h"
#include "ObsidianModal.h"
#include "PluginProcessor.h"
#include "RightPanelWrapper.h"
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

class DjIaVstEditor : public juce::AudioProcessorEditor,
                      public juce::Timer,
                      public juce::DragAndDropContainer,
                      public ModalHost
{
  public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	explicit DjIaVstEditor(DjIaVstProcessor &);

	~DjIaVstEditor() override;

	juce::Viewport mixerViewport;
	juce::Viewport tracksViewport;
	juce::Component tracksContainer;

	std::unique_ptr<MixerPanel> mixerPanel;
	std::unique_ptr<UILayoutManager> uiLayoutManager;
	std::unique_ptr<UIStatusManager> uiStatusManager;
	std::unique_ptr<UIModalManager> uiModalManager;
	std::unique_ptr<UIGenerationManager> uiGenerationManager;
	std::unique_ptr<UITrackManager> uiTrackManager;
	std::unique_ptr<UIPresetManager> uiPresetManager;
	std::unique_ptr<UIMidiManager> uiMidiManager;
	std::unique_ptr<LeftPanelWrapper> leftPanelWrapper;
	std::unique_ptr<LCDScreen> lcdScreen;
	std::unique_ptr<CustomLookAndFeel> customLookAndFeel;
	std::unique_ptr<MasterWaveformDisplay> masterWaveformDisplay;
	std::unique_ptr<RightPanelWrapper> rightPanelWrapper;

	juce::Label statusLabel;

	std::atomic<bool> isBeingDestroyed{false};

	void paint(juce::Graphics &) override;
	void resized() override;
	void timerCallback() override;
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
	static constexpr int TRACK_CELL_H = 140;
	static constexpr int TRACK_ROWS = 4;
	static constexpr int TRACK_COLS = 2;

	void visibilityChanged() override;
	void openMidiMappingEditor();
	void setupUI();
	void addEventListeners();
	void onAutoLoadToggled();
	void onLoadSampleClicked();
	void updateLoadButtonState();
	void finalizeInit();

	bool keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected);
	bool keyPressed(const juce::KeyPress &key) override;

#if JucePlugin_Build_Standalone
	void parentHierarchyChanged() override;
#endif

	bool mixerVisible = false;
	std::atomic<bool> isInitialized{false};
	std::atomic<bool> isRefreshingTracks{false};

	IconButtonSimple autoLoadButton{"AutoLoad", ""};
	IconButtonSimple loadSampleButton{"LoadSample", ""};
	IconButtonSimple bypassSequencerButton{"BypassSeq", ""};
	IconButtonSimple configButton{"Config", ""};
	IconButtonSimple openMidiEditorButton{"MidiEditor", ""};
	IconButtonSimple helpButton{"Help", ""};
	IconButtonSimple bypassLLMButton{"BypassLLM", ""};

	juce::Typeface::Ptr customFont;
	juce::Label midiIndicator;
	juce::String lastMidiNote;
	juce::Label creditsLabel;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DjIaVstEditor)
};