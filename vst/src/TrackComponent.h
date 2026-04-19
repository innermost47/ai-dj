#pragma once
#include "JuceHeader.h"
#include "TrackManager.h"
#include "MidiLearnableComponents.h"
#include "ColourPalette.h"
#include "DrawingCanvas.h"
#include "IconButton.h"
#include "TrackButtonIcons.h"

class WaveformDisplay;
class SequencerComponent;
class DjIaVstProcessor;

class CustomInfoLabelLookAndFeel : public juce::LookAndFeel_V4
{
public:
	void drawLabel(juce::Graphics& g, juce::Label& label) override
	{
		auto bounds = label.getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(bounds, 4.0f);
		g.setColour(ColourPalette::textAccent.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
		g.setColour(ColourPalette::textAccent);
		g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
		g.drawText(label.getText(), bounds.reduced(8, 2),
			juce::Justification::centredLeft, false);
	}
};

class TrackComponent : public juce::Component, public juce::Timer, public juce::AudioProcessorParameter::Listener, public juce::DragAndDropTarget
{
public:
	TrackComponent(const juce::String& trackId, DjIaVstProcessor& processor);
	~TrackComponent();
	juce::String getTrackId() const
	{
		return trackId;
	}

	std::function<void(const juce::String&)> onDeleteTrack;
	std::function<void(const juce::String&)> onSelectTrack;
	std::function<void(const juce::String&)> onGenerateForTrack;
	std::function<void(const juce::String&, const juce::String&)> onTrackRenamed;
	std::function<void(const juce::String&, const juce::String&)> onTrackPromptChanged;
	std::function<void(const juce::String&)> onStatusMessage;
	std::function<void(const juce::String&, const juce::String&, const juce::StringArray&)> onGenerateWithImage;
	std::function<void(const juce::String&)> onStopPreview;

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragMove(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;

	static const int BASE_HEIGHT = 80;
	static const int WAVEFORM_HEIGHT = 70;
	static const int SEQUENCER_HEIGHT = 100;
	static const int PAGE_BUTTON_SIZE = 20;
	static const int ICON_BUTTON_WIDTH = 42;
	static const int ICON_BUTTON_HEIGHT = 50;
	static const int CLUSTER_GAP = 14;
	static const int INTRA_CLUSTER_GAP = 4;

	IconButton showWaveformButton{ "ShowWaveform", "WAVE" };
	IconButton sequencerToggleButton{ "SequencerToggle", "SEQ" };

	TrackData* getTrack() const { return track; }

	std::function<void(const juce::String&, const juce::String&)> onReorderTrack;
	std::function<void(const juce::String&)> onPreviewTrack;

	void setSelected(bool selected);
	void setTrackData(TrackData* trackData);
	void refreshWaveformDisplay();
	bool isWaveformVisible() const;
	void startGeneratingAnimation();
	void stopGeneratingAnimation();
	void updateFromTrackData();
	void setGenerateButtonEnabled(bool enabled);
	void updateWaveformWithTimeStretch();
	void updatePlaybackPosition(double timeInSeconds);
	void toggleWaveformDisplay();
	void refreshWaveformIfNeeded();
	void toggleSequencerDisplay();
	void updatePromptPresets(const juce::StringArray& presets);
	void setupMidiLearn();
	void updatePromptSelection(const juce::String& promptText);
	void onPageSelected(int pageIndex);
	void performPageChange(int pageIndex);
	void updatePagesDisplay();
	void setSamplePending(bool pending);
	bool isSamplePending() const { return hasSamplePending; }
	void setPreviewPlaying(bool playing);

	juce::String getSelectedModel() const { return modelSelector.getText(); }

	bool isEditingLabel = false;
	bool sequencerVisible = false;

	MidiLearnableComboBox promptPresetSelector;

	juce::Component::SafePointer<juce::DocumentWindow> drawingWindowPtr;

	juce::String trackId;

	IconButton* getGenerateButton() { return &generateButton; }
	juce::Slider* getBpmOffsetSlider() { return &bpmOffsetSlider; }

	SequencerComponent* getSequencer() const { return sequencer.get(); }

	void setCanvasGenerating(bool generating)
	{
		canvasIsGenerating = generating;
		if (drawingWindowPtr != nullptr)
		{
			if (auto* window = drawingWindowPtr.getComponent())
			{
				if (auto* canvas = dynamic_cast<DrawingCanvas*>(window->getContentComponent()))
				{
					canvas->setGenerating(generating);
				}
			}
		}
	}

private:
	class DrawingWindow : public juce::DocumentWindow
	{
	public:
		DrawingWindow(const juce::String& name, DrawingCanvas* canvas)
			: juce::DocumentWindow(name, juce::Colour(0xff2a2a2a),
				juce::DocumentWindow::closeButton)
		{
			setContentOwned(canvas, true);
			centreWithSize(920, 770);
			setResizable(false, false);
			setUsingNativeTitleBar(true);
		}

		void closeButtonPressed() override
		{
			if (onBeforeClose)
				onBeforeClose();
			delete this;
		}

		std::function<void()> onBeforeClose;
	};

	juce::StringArray promptPresets;

	TrackData* track;

	std::unique_ptr<WaveformDisplay> waveformDisplay;
	std::unique_ptr<SequencerComponent> sequencer;
	std::unique_ptr<DrawingCanvas> drawingCanvas;

	DjIaVstProcessor& audioProcessor;

	CustomInfoLabelLookAndFeel customLookAndFeel;

	MidiLearnableButton pageButtons[4];

	IconButton drawButton{ "DrawBtn", "DRAW" };
	IconButton generateButton{ "GenerateBtn", "GEN" };

	IconButton originalSyncButton{ "OriginalSyncBtn", "ORIG" };
	IconButton previewButton{ "PreviewBtn", "PREVIEW" };

	IconButton randomRetriggerButton{ "RandomRetriggerBtn", "REPEAT" };
	IconButton randomDurationToggle{ "RandomDurationBtn", "RND" };

	IconButton deleteButton{ "DeleteBtn", "DELETE" };

	MidiLearnableSlider intervalKnob;

	juce::ComboBox modelSelector;
	juce::StringArray aiModels;

	juce::TextButton trackNumberButton;

	juce::Slider bpmOffsetSlider;

	juce::Label trackNameLabel;
	juce::Label intervalLabel;

	juce::Label infoLabel;
	juce::Label bpmOffsetLabel;

	juce::ComboBox timeStretchModeSelector;

	std::atomic<bool> isDestroyed{ false };

	bool isGenerating = false;
	bool blinkState = false;
	bool isSelected = false;
	bool isDragOver = false;
	bool hasSamplePending = false;
	bool pagesMode = true;
	bool pageBlinkState = false;
	bool canvasIsGenerating = false;
	bool isPreviewPlaying = false;

	juce::TextButton togglePagesButton;

	void setupPagesUI();
	void onTogglePagesMode();
	void loadPageIfNeeded(int pageIndex);
	void loadPageAudioFile(int pageIndex, const juce::File& audioFile);
	void layoutPagesButtons(juce::Rectangle<int> area);
	void calculateHostBasedDisplay();
	void paint(juce::Graphics& g);
	void resized();
	void timerCallback() override;
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void setupUI();
	void setupIconButtons();
	void updateButtonsEnabledState();
	void adjustLoopPointsToTempo();
	void updateTrackInfo();
	void learn(juce::String param, MidiLearnableBase* component, std::function<void(float)> uiCallback = nullptr);
	void removeMidiMapping(const juce::String& param);
	void addListener(juce::String name);
	void removeListener(juce::String name);
	void setButtonParameter(juce::String name);
	void updateUIFromParameter(const juce::String& paramName,
		const juce::String& slotPrefix,
		float newValue);
	void loadPromptPresets();
	void onTrackPresetSelected();
	void toggleOriginalSync();
	void statusCallback(const juce::String& message);
	void onRandomRetriggerToggled();
	void onIntervalChanged();
	void setSliderParameter(juce::String name, juce::Slider& slider);
	void addEventListeners();
	void updateRandomRetriggerButtonColor();
	void updateRandomDurationButtonColor();
	void openDrawingCanvas();
	void updatePreviewButton();
	void updateModelUI();
	void layoutPlaybackCluster(juce::Rectangle<int> area);
	void layoutFxCluster(juce::Rectangle<int> area);

	float calculateEffectiveBpm();

	juce::String getIntervalName(int value);

	JUCE_DECLARE_WEAK_REFERENCEABLE(TrackComponent);
};