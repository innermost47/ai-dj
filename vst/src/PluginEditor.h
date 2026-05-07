#pragma once
#include "CustomLookAndFeel.h"
#include "IconButton.h"
#include "LCDScreen.h"
#include "MasterWaveformDisplay.h"
#include "MidiLearnableComponents.h"
#include "MixerPanel.h"
#include "ObsidianModal.h"
#include "PluginProcessor.h"
#include "SampleBankPanel.h"
#include "TrackComponent.h"
#include "UIGenerationManager.h"
#include "UILayoutManager.h"
#include "UIModalManager.h"
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
	std::unique_ptr<SampleBankPanel> sampleBankPanel;
	std::unique_ptr<UILayoutManager> uiLayoutManager;
	std::unique_ptr<UIStatusManager> uiStatusManager;
	std::unique_ptr<UIModalManager> uiModalManager;
	std::unique_ptr<UIGenerationManager> uiGenerationManager;
	std::unique_ptr<UITrackManager> uiTrackManager;

	juce::Label statusLabel;

	std::atomic<bool> isBeingDestroyed{false};

	LCDScreen lcdScreen;

	void paint(juce::Graphics &) override;
	void resized() override;
	void timerCallback() override;
	void updateUIFromProcessor();
	void refreshMixerChannels();
	void initUI();
	void *getSequencerForTrack(const juce::String &trackId);
	void restoreUICallbacks();
	bool keyStateChanged(bool isKeyDown) override;
	void refreshAllPromptLists();
	void addModal(std::unique_ptr<ObsidianModalOverlay> overlay) override;
	void removeModal(ObsidianModalOverlay *overlay) override;
	MixerPanel *getMixerPanel()
	{
		return mixerPanel.get();
	}

	DjIaVstProcessor &audioProcessor;
	CustomLookAndFeel customLookAndFeel;
	MasterWaveformDisplay masterWaveformDisplay;
	std::vector<std::unique_ptr<ObsidianModalOverlay>> activeModals;
	juce::Image logoImage;
	juce::ImageComponent logoComponent;
	juce::Image bannerImage;
	juce::Rectangle<int> bannerArea;
	std::unique_ptr<juce::TooltipWindow> tooltipWindow;
	static constexpr int TRACK_CELL_H = 140;
	static constexpr int TRACK_ROWS = 4;
	static constexpr int TRACK_COLS = 2;
	bool sampleBankVisible = true;
	enum KeyboardLayout
	{
		QWERTY,
		AZERTY,
		QWERTZ
	};
	KeyboardLayout detectKeyboardLayout();

	void visibilityChanged() override;
	void openMidiMappingEditor();
	void setupUI();
	void addEventListeners();
	void loadPromptPresets();
	void onPresetSelected();
	void onSavePreset();
	void onAutoLoadToggled();
	void onLoadSampleClicked();
	void updateLoadButtonState();
	void updateMidiIndicator(const juce::String &noteInfo);
	void finalizeInit();
	void mouseDown(const juce::MouseEvent &event) override;
	void notifyTracksPromptUpdate();
	bool keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected);
	bool keyPressed(const juce::KeyPress &key) override;

	juce::StringArray getAllPrompts() const;

	bool mixerVisible = false;
	std::atomic<bool> isInitialized{false};
	std::atomic<bool> isRefreshingTracks{false};

	IconButtonSimple autoLoadButton{"AutoLoad", ""};
	IconButtonSimple loadSampleButton{"LoadSample", ""};
	IconButtonSimple bypassSequencerButton{"BypassSeq", ""};
	IconButtonSimple configButton{"Config", ""};
	IconButtonSimple openMidiEditorButton{"MidiEditor", ""};
	IconButtonSimple helpButton{"Help", ""};
	IconButtonSimple toggleBankButton{"ToggleBank", ""};
	IconButtonSimple bypassLLMButton{"BypassLLM", ""};

	juce::Label pluginNameLabel;
	juce::Label developerLabel;
	juce::Label stabilityLabel;
	juce::Typeface::Ptr customFont;
	MidiLearnableComboBox promptPresetSelector;
	IconButtonSimple savePresetButton{"SavePreset", "SAVE"};
	juce::TextEditor promptInput;
	juce::ComboBox styleSelector;
	juce::Label bpmLabel;
	juce::ComboBox keySelector;
	IconButton generateButton{"GenerateBtn", "GEN"};
	juce::Label serverUrlLabel;
	juce::TextEditor serverUrlInput;
	juce::Label apiKeyLabel;
	juce::TextEditor apiKeyInput;
	juce::TextButton playButton;
	juce::ComboBox durationSelector;
	juce::Label midiIndicator;
	juce::String lastMidiNote;
	juce::Label tracksLabel;

	juce::Label creditsLabel;

	enum MenuIDs
	{
		newSession = 1,
		saveSession,
		saveSessionAs,
		loadSessionMenu,
		exportSession,
		aboutDjIa = 100,
		showHelp,
		addTrack = 200,
		deleteAllTracks,
		resetTracks
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(DjIaVstEditor)
};