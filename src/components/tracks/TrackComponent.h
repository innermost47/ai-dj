#pragma once
#include "DrawingCanvas.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBase.h"
#include "TrackManager.h"
#include <JuceHeader.h>

class WaveformDisplay;
class SequencerComponent;
class DjIaVstProcessor;

class CustomInfoLabelLookAndFeel : public juce::LookAndFeel_V4
{
  public:
	void drawLabel(juce::Graphics &g, juce::Label &label) override
	{
		auto bounds = label.getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(bounds, 4.0f);
		g.setColour(ColourPalette::textAccent.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
		g.setColour(ColourPalette::textAccent);
		g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
		g.drawText(label.getText(), bounds.reduced(8, 2), juce::Justification::centredLeft, false);
	}
};

class TrackComponent : public ObsidianComponent,
                       public juce::Timer,
                       public juce::AudioProcessorParameter::Listener,
                       public juce::DragAndDropTarget
{
  public:
	TrackComponent(const juce::String &trackId, DjIaVstProcessor &processor);
	~TrackComponent();
	const juce::String &getTrackId() const
	{
		return trackId;
	}

	std::function<void(const juce::String &)> onDeleteTrack;
	std::function<void(const juce::String &)> onGenerateForTrack;
	std::function<void(const juce::String &, const juce::String &)> onTrackRenamed;
	std::function<void(const juce::String &, const juce::String &)> onTrackPromptChanged;
	std::function<void(const juce::String &)> onStatusMessage;
	std::function<void(const juce::String &, const juce::String &, const juce::StringArray &)> onGenerateWithImage;
	std::function<void(const juce::String &)> onStopPreview;
	std::function<void(const juce::String &trackId)> onModelChanged;

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
	void itemDragMove(const SourceDetails &dragSourceDetails) override;
	void itemDragExit(const SourceDetails &dragSourceDetails) override;
	void itemDropped(const SourceDetails &dragSourceDetails) override;

	static const int BASE_HEIGHT = 80;
	static const int WAVEFORM_HEIGHT = 45;
	static const int SEQUENCER_HEIGHT = 45;
	static const int PAGE_BUTTON_SIZE = 16;
	static const int ICON_BUTTON_WIDTH = 42;
	static const int ICON_BUTTON_HEIGHT = 50;
	static const int CLUSTER_GAP = 4;
	static const int INTRA_CLUSTER_GAP = 1;

	TrackData *getTrack() const
	{
		return track;
	}

	std::function<void(const juce::String &)> onPreviewTrack;

	juce::ComboBox modelSelector;

	void setTrackData(TrackData *trackData);
	void refreshWaveformDisplay();
	bool isWaveformVisible() const;
	void startGeneratingAnimation();
	void stopGeneratingAnimation();
	void updateFromTrackData();
	void setGenerateButtonEnabled(bool enabled);
	void updateWaveformWithTimeStretch();
	void updatePlaybackPosition(double timeInSeconds);
	void refreshWaveformIfNeeded();
	void updatePromptPresets(const juce::StringArray &presets);
	void setupMidiLearn();
	void updatePromptSelection(const juce::String &promptText);
	void onPageSelected(int pageIndex);
	void performPageChange(int pageIndex);
	void updatePagesDisplay();
	void setSamplePending(bool pending);
	void setPreviewPlaying(bool playing);
	void syncTrackName(const juce::String &name);
	void loadPromptPresets();
	void removeListener(juce::String name);
	void detachWaveformTrack();

	bool isEditingLabel = false;
	bool sequencerVisible = false;

	MidiLearnableComboBox promptPresetSelector;

	juce::Component::SafePointer<juce::DocumentWindow> drawingWindowPtr;

	juce::String trackId;

	SequencerComponent *getSequencer() const
	{
		return sequencer.get();
	}

	void setCanvasGenerating(bool generating)
	{
		canvasIsGenerating = generating;
		if (drawingCanvasPtr != nullptr)
		{
			drawingCanvasPtr->setGenerating(generating);
		}
	}

  private:
	class BorderOverlay : public juce::Component
	{
	  public:
		BorderOverlay()
		{
			setInterceptsMouseClicks(false, false);
			setOpaque(false);
		}

		void setVisualState(bool generating, bool samplePending, bool selected, bool dragOver, bool blink,
		                    juce::Colour modelColour)
		{
			if (generating == isGenerating && samplePending == hasSamplePending && selected == isSelected &&
			    dragOver == isDragOver && blink == blinkState && modelColour == accentColour)
				return;

			isGenerating = generating;
			hasSamplePending = samplePending;
			isSelected = selected;
			isDragOver = dragOver;
			blinkState = blink;
			accentColour = modelColour;
			repaint();
		}

		void paint(juce::Graphics &g) override
		{
			auto bounds = getLocalBounds().toFloat();

			juce::Colour bgColour;
			bool fillBg = true;

			if (isDragOver)
				bgColour = ColourPalette::buttonSuccess.withAlpha(0.4f);
			else if (hasSamplePending && !isGenerating)
				bgColour = ColourPalette::samplePending.withAlpha(0.15f);
			else
				fillBg = false;

			if (fillBg)
			{
				g.setColour(bgColour);
				g.fillRoundedRectangle(bounds, 6.0f);
			}

			juce::Colour borderColour;
			float borderWidth;

			if (isGenerating)
			{
				borderColour = blinkState ? accentColour.brighter(0.4f) : accentColour.darker(0.4f);
				borderWidth = 3.0f;
			}
			else if (hasSamplePending)
			{
				borderColour = ColourPalette::samplePending;
				borderWidth = 2.0f;
			}
			else if (isSelected)
			{
				borderColour = ColourPalette::trackSelected;
				borderWidth = 2.0f;
			}
			else
			{
				borderColour = ColourPalette::backgroundLight;
				borderWidth = 1.0f;
			}

			g.setColour(borderColour);
			g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, borderWidth);
		}

	  private:
		bool isGenerating = false;
		bool hasSamplePending = false;
		bool isSelected = false;
		bool isDragOver = false;
		bool blinkState = false;
		juce::Colour accentColour{ColourPalette::buttonPrimary};
	};

	struct PageButtonState
	{
		bool isActive = false;
		bool isPending = false;
		bool hasAudio = false;
		bool blinkState = false;
		juce::Colour modelColour{ColourPalette::buttonPrimary};

		bool operator==(const PageButtonState &other) const
		{
			return isActive == other.isActive && isPending == other.isPending && hasAudio == other.hasAudio &&
			       blinkState == other.blinkState && modelColour == other.modelColour;
		}
		bool operator!=(const PageButtonState &other) const
		{
			return !(*this == other);
		}
	};

	std::array<PageButtonState, 4> lastPageStates;
	int lastWaveformNumSamples = 0;
	juce::Colour cachedModelColour{ColourPalette::buttonPrimary};

	BorderOverlay borderOverlay;

	juce::Component::SafePointer<DrawingCanvas> drawingCanvasPtr;

	juce::StringArray promptPresets;

	TrackData *track;

	std::unique_ptr<WaveformDisplay> waveformDisplay;
	std::unique_ptr<SequencerComponent> sequencer;
	std::unique_ptr<DrawingCanvas> drawingCanvas;

	DjIaVstProcessor &audioProcessor;

	CustomInfoLabelLookAndFeel customLookAndFeel;

	MidiLearnableButton pageButtons[4];

	IconButton drawButton{"DrawBtn", "DRAW"};
	IconButton generateButton{"GenerateBtn", "GEN"};

	IconButton originalSyncButton{"OriginalSyncBtn", "ORIG"};
	IconButton previewButton{"PreviewBtn", "PREVIEW"};

	IconButton randomRetriggerButton{"RandomRetriggerBtn", "REPEAT"};
	IconButton randomDurationToggle{"RandomDurationBtn", "RND"};

	MidiLearnableSlider intervalKnob;
	MidiLearnableSlider adsrAttackKnob;
	MidiLearnableSlider adsrDecayKnob;
	MidiLearnableSlider adsrSustainKnob;
	MidiLearnableSlider adsrReleaseKnob;
	juce::Label adsrAttackLabel, adsrDecayLabel, adsrSustainLabel, adsrReleaseLabel;

	juce::StringArray aiModels;

	juce::Label intervalLabel;

	juce::Label infoLabel;

	std::atomic<bool> isDestroyed{false};

	bool isGenerating = false;
	bool blinkState = false;
	bool isSelected = false;
	bool isDragOver = false;
	bool hasSamplePending = false;
	bool pageBlinkState = false;
	bool canvasIsGenerating = false;
	bool isPreviewPlaying = false;
	bool canvasModalOpen = false;
	bool isDraggingPrompt = false;

	juce::TextButton togglePagesButton;

	void setupPagesUI();
	void loadPageIfNeeded(int pageIndex);
	void loadPageAudioFile(int pageIndex, const juce::File &audioFile);
	void layoutPagesButtons(juce::Rectangle<int> area);
	void calculateHostBasedDisplay();
	void paint(juce::Graphics &g);
	void resized();
	void timerCallback() override;
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void setupUI();
	void setupIconButtons();
	void updateButtonsEnabledState();
	void updateTrackInfo();
	void learn(juce::String param, MidiLearnableBase *component, std::function<void(float)> uiCallback = nullptr);
	void removeMidiMapping(const juce::String &param);
	void addListener(juce::String name);
	void setButtonParameter(juce::String name);
	void updateUIFromParameter(const juce::String &paramName, const juce::String &slotPrefix, float newValue);
	void onTrackPresetSelected();
	void toggleOriginalSync();
	void statusCallback(const juce::String &message);
	void onRandomRetriggerToggled();
	void onIntervalChanged();
	void setSliderParameter(juce::String name, juce::Slider &slider);
	void addEventListeners();
	void updateRandomRetriggerButtonColor();
	void updateRandomDurationButtonColor();
	void openDrawingCanvas();
	void updatePreviewButton();
	void updateModelUI();
	void syncBorderOverlay();
	juce::Colour getCurrentModelColour() const;
	void setupAdsrKnobs();
	void updateAdsrKnobsFromPage();
	void syncAdsrToWaveform();
	void applyPromptFromBank(const juce::String &promptId);

	float calculateEffectiveBpm();

	juce::String getIntervalName(int value);

	JUCE_DECLARE_WEAK_REFERENCEABLE(TrackComponent);
};