#pragma once
#include "PluginProcessor.h"
#include "components/tracks/TrackComponent.h"
#include "components/mixer/MixerPanel.h"
#include "midi/MidiLearnableComponents.h"
#include "components/bank/SampleBankPanel.h"
#include "components/shared/ObsidianModal.h"
#include "style/CustomLookAndFeel.h"
#include "components/mixer/LCDScreen.h"
#include "components/shared/IconButton.h"
#include "components/mixer/MasterWaveformDisplay.h"

class SequencerComponent;

class DjIaVstEditor : public juce::AudioProcessorEditor,
	public juce::Timer,
	public DjIaVstProcessor::GenerationListener,
	public juce::DragAndDropContainer,
	public ModalHost
{
public:
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return createIgnoredAccessibilityHandler(*this);
	}

	explicit DjIaVstEditor(DjIaVstProcessor&);

	~DjIaVstEditor() override;

	std::vector<std::unique_ptr<TrackComponent>> trackComponents;
	std::vector<std::unique_ptr<TrackComponent>>& getTrackComponents()
	{
		return trackComponents;
	}

	std::unique_ptr<MixerPanel> mixerPanel;
	std::unique_ptr<SampleBankPanel> sampleBankPanel;
	juce::Viewport mixerViewport;
	juce::Viewport tracksViewport;
	juce::Component tracksContainer;

	juce::Label statusLabel;

	std::atomic<bool> isBeingDestroyed{ false };

	TrackComponent* getTrackComponent(const juce::String& trackId);

	LCDScreen lcdScreen;

	void paint(juce::Graphics&) override;
	void layoutPromptSection(juce::Rectangle<int> area, int spacing);
	void resized() override;
	void timerCallback() override;
	void refreshTrackComponents();
	void updateUIFromProcessor();
	void refreshTracks();
	void onGenerationComplete(const juce::String& trackId, const juce::String& message) override;
	void refreshMixerChannels();
	void initUI();
	void setStatusWithTimeout(const juce::String& message, int timeoutMs = 2000);
	void* getSequencerForTrack(const juce::String& trackId);
	void stopGenerationUI(const juce::String& trackId, bool success = true, const juce::String& errorMessage = "");
	void startGenerationUI(const juce::String& trackId);
	void restoreUICallbacks();
	void updateSelectedTrack();
	void onGenerateButtonClicked();
	void onSampleLoaded(const juce::String& trackId);
	void reEnableCanvasForTrack();
	void updateLCD();
	void addModal(std::unique_ptr<ObsidianModalOverlay> overlay) override;
	void removeModal(ObsidianModalOverlay* overlay) override;
	bool keyStateChanged(bool isKeyDown) override;
	MixerPanel* getMixerPanel() { return mixerPanel.get(); }

private:
	DjIaVstProcessor& audioProcessor;
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
	void updateMidiIndicator(const juce::String& noteInfo);
	void finalizeInit();
	void updateUIComponents();
	void setAllGenerateButtonsEnabled(bool enabled);
	void showFirstTimeSetup();
	void showConfigDialog();
	void mouseDown(const juce::MouseEvent& event) override;
	void editCustomPromptDialog(const juce::String& selectedPrompt);
	void startGenerationButtonAnimation();
	void stopGenerationButtonAnimation();
	void refreshUIForMode();
	void checkLocalModelsAndNotify();
	void notifyTracksPromptUpdate();
	void generateFromTrackComponent(const juce::String& trackId);
	void refreshCredits();
	void refreshCreditsAsync();
	void showOnboardingStep(int step);
	void showOnboardingTour();
	void checkForUpdates();
	void layoutTracksGrid();
	bool keyMatches(const juce::KeyPress& pressed, const juce::KeyPress& expected);
	bool keyPressed(const juce::KeyPress& key) override;

	juce::StringArray getAllPrompts() const;

	bool mixerVisible = false;
	std::atomic<bool> isGenerating{ false };
	std::atomic<bool> wasGenerating{ false };
	std::atomic<bool> isInitialized{ false };
	std::atomic<bool> isRefreshingTracks{ false };

	IconButtonSimple autoLoadButton{ "AutoLoad", "" };
	IconButtonSimple loadSampleButton{ "LoadSample", "" };
	IconButtonSimple bypassSequencerButton{ "BypassSeq", "" };
	IconButtonSimple configButton{ "Config", "" };
	IconButtonSimple openMidiEditorButton{ "MidiEditor", "" };
	IconButtonSimple helpButton{ "Help", "" };
	IconButtonSimple toggleBankButton{ "ToggleBank", "" };

	juce::String generatingTrackId;
	juce::String originalButtonText;

	juce::Label pluginNameLabel;
	juce::Label developerLabel;
	juce::Label stabilityLabel;
	juce::Typeface::Ptr customFont;
	MidiLearnableComboBox promptPresetSelector;
	IconButtonSimple savePresetButton{ "SavePreset", "SAVE" };
	juce::TextEditor promptInput;
	juce::ComboBox styleSelector;
	juce::Label bpmLabel;
	juce::ComboBox keySelector;
	IconButton generateButton{ "GenerateBtn", "GEN" };
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