#pragma once
#include "DrawingCanvas.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include "TrackManager.h"
#include <JuceHeader.h>

class WaveformDisplay;
class SequencerComponent;
class DjIaVstProcessor;

class TrackComponent : public ObsidianBaseMidiComponent, public juce::DragAndDropTarget
{
  public:
	TrackComponent(const juce::String &trackId, DjIaVstProcessor &processor);
	~TrackComponent();
	const juce::String &getTrackId() const
	{
		return trackId;
	}

	std::function<void(const juce::String &)> onDeleteTrack;
	std::function<void(const juce::String &)> onSelectTrack;
	std::function<void(const juce::String &)> onGenerateForTrack;
	std::function<void(const juce::String &, const juce::String &)> onTrackRenamed;
	std::function<void(const juce::String &, const juce::String &)> onTrackPromptChanged;
	std::function<void(const juce::String &)> onStatusMessage;
	std::function<void(const juce::String &, const juce::String &, const juce::StringArray &)> onGenerateWithImage;
	std::function<void(const juce::String &)> onStopPreview;
	std::function<void(const juce::String &trackId)> onModelChanged;
	std::function<void(const juce::String &trackId)> onSampleDropped;

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
	void itemDragMove(const SourceDetails &dragSourceDetails) override;
	void itemDragExit(const SourceDetails &dragSourceDetails) override;
	void itemDropped(const SourceDetails &dragSourceDetails) override;

	std::function<void(const juce::String &)> onPreviewTrack;

	juce::ComboBox modelSelector;

	void setTrackData(TrackData *trackData);
	void setSelected(bool s);
	void refreshWaveformDisplay();
	bool isWaveformVisible() const;
	void startGeneratingAnimation();
	void stopGeneratingAnimation();
	void updateFromTrackData();
	void setGenerateButtonEnabled(bool enabled);
	void updateWaveformWithTimeStretch();
	void updatePlaybackPosition(double timeInSeconds);
	void refreshWaveformIfNeeded();
	void updatePromptSelection(const juce::String &promptText);
	void onPageSelected(int pageIndex);
	void performPageChange(int pageIndex);
	void updatePagesDisplay();
	void setPreviewPlaying(bool playing);
	void syncTrackName(const juce::String &name);
	void detachWaveformTrack();
	void syncBorderOverlay();
	void populatePromptPresets(const juce::String &modelName, const juce::String &forceSelectedPrompt = {});
	void mouseDown(const juce::MouseEvent &) override;
	void syncModelSelector();
	void startPagePendingBlink()
	{
		blinkTicking = true;
		blinkCounter = 1;
	}
	void setIsDragOver(bool v)
	{
		isDragOver = v;
	}

	bool isEditingLabel = false;
	bool sequencerVisible = false;

	juce::ComboBox promptPresetSelector;

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
		BorderOverlay();
		void setVisualState(bool generating, bool samplePending, bool selected, bool dragOver, bool blink,
		                    juce::Colour modelColour);
		void paint(juce::Graphics &g) override;
		void triggerFlash()
		{
			flashAmount = 1.0f;
		}

		bool tickFlash()
		{
			if (flashAmount <= 0.01f)
				return false;

			flashAmount *= 0.82f;

			if (flashAmount <= 0.01f)
				flashAmount = 0.0f;

			repaint();
			return flashAmount > 0.0f;
		}

		float flashAmount = 0.0f;

	  private:
		bool isGenerating = false;
		bool hasSamplePending = false;
		bool isSelected = false;
		bool isDragOver = false;
		bool blinkState = false;

		std::unique_ptr<juce::Drawable> blockedIcon;
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

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	BorderOverlay borderOverlay;

	juce::Component::SafePointer<DrawingCanvas> drawingCanvasPtr;

	juce::StringArray promptPresets;

	std::unique_ptr<WaveformDisplay> waveformDisplay;
	std::unique_ptr<SequencerComponent> sequencer;
	std::unique_ptr<DrawingCanvas> drawingCanvas;

	MidiLearnableButton pageButtons[4];

	IconButtonSimple drawButton{"DrawBtn", "DRAW"};
	IconButton generateButton{"GenerateBtn", "GEN"};
	IconButton reverseButton{"ReverseBtn", "REV"};
	IconButton transientScatterButton{"TransientScatterBtn", "SCAT"};

	IconButtonSimple originalSyncButton{"OriginalSyncBtn", "ORIG"};
	IconButtonSimple previewButton{"PreviewBtn", "PREVIEW"};

	IconButton beatRepeatButton{"BeatRepeatActiveBtn", "REPEAT"};
	IconButtonSimple randomDurationToggle{"RandomDurationBtn", "RND"};

	MidiLearnableSlider intervalKnob;
	MidiLearnableSlider adsrAttackKnob;
	MidiLearnableSlider adsrDecayKnob;
	MidiLearnableSlider adsrSustainKnob;
	MidiLearnableSlider adsrReleaseKnob;
	juce::Label adsrAttackLabel, adsrDecayLabel, adsrSustainLabel, adsrReleaseLabel;

	juce::StringArray aiModels;

	juce::Label intervalLabel;

	juce::Label infoLabel;

	int blinkCounter = 0;

	bool blinkTicking = false;
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

	juce::String modelSet;

	void setupPagesUI();
	void loadPageIfNeeded(int pageIndex);
	void loadPageAudioFile(int pageIndex, const juce::File &audioFile);
	void layoutPagesButtons(juce::Rectangle<int> area);
	void calculateHostBasedDisplay();
	void paint(juce::Graphics &g);
	void resized();
	void handleVBlank();
	void setupUI();
	void setupIconButtons();
	void updateButtonsEnabledState();
	void updateTrackInfo();
	void onTrackPresetSelected();
	void toggleOriginalSync();
	void statusCallback(const juce::String &message);
	void onIntervalChanged();
	void updateBeatRepeatButtonState();
	void updateReverseButtonState();
	void updateTransientScatterButtonState();
	void updateRandomDurationButtonColor();
	void openDrawingCanvas();
	void updatePreviewButton();
	void updateModelUI();
	juce::Colour getCurrentModelColour() const;
	void setupAdsrKnobs();
	void updateAdsrKnobsFromPage();
	void syncAdsrToWaveform();
	void applyPromptFromBank(const juce::String &promptId);
	void wireParameters();

	float calculateEffectiveBpm();

	juce::String getIntervalName(int value);
	juce::String getSelectedPromptValue() const;

  protected:
	juce::String getParameterPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "slot" + juce::String(t->slotIndex + 1);
	}

	juce::String getMidiLearnDescriptionPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "Slot " + juce::String(t->slotIndex + 1) + " ";
	}

	void onParameterChangedUI(const juce::String &paramSuffix, float newValue) override;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TrackComponent);
};