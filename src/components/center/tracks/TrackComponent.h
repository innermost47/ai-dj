#pragma once
#include "DrawingCanvas.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include "TrackManager.h"
#include <JuceHeader.h>

class WaveformDisplay;
class SequencerComponent;
class DjIaVstProcessor;

class TrackComponent : public ObsidianBaseMidiComponent, public juce::Timer, public juce::DragAndDropTarget
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
	std::function<void(const juce::String &trackId)> onSampleDropped;

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
	void itemDragEnter(const SourceDetails &dragSourceDetails) override;
	void itemDragMove(const SourceDetails &dragSourceDetails) override;
	void itemDragExit(const SourceDetails &dragSourceDetails) override;
	void itemDropped(const SourceDetails &dragSourceDetails) override;

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
	void updatePromptPresets(const juce::StringArray &presets, const juce::String &selectedPrompt = "");
	void updatePromptSelection(const juce::String &promptText);
	void onPageSelected(int pageIndex);
	void performPageChange(int pageIndex);
	void updatePagesDisplay();
	void setPreviewPlaying(bool playing);
	void syncTrackName(const juce::String &name);
	void loadPromptPresets();
	void detachWaveformTrack();
	void syncBorderOverlay();

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
				g.fillRoundedRectangle(bounds, Obsidian::CORNER);
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
				borderColour = ColourPalette::lightGrey;
				borderWidth = 2.0f;
			}
			else
			{
				borderColour = ColourPalette::backgroundLight;
				borderWidth = 1.0f;
			}

			g.setColour(borderColour);
			g.drawRoundedRectangle(bounds.reduced(1.0f), Obsidian::CORNER, borderWidth);
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

	std::unique_ptr<WaveformDisplay> waveformDisplay;
	std::unique_ptr<SequencerComponent> sequencer;
	std::unique_ptr<DrawingCanvas> drawingCanvas;

	MidiLearnableButton pageButtons[4];

	IconButtonSimple drawButton{"DrawBtn", "DRAW"};
	IconButton generateButton{"GenerateBtn", "GEN"};

	IconButtonSimple originalSyncButton{"OriginalSyncBtn", "ORIG"};
	IconButtonSimple previewButton{"PreviewBtn", "PREVIEW"};

	IconButton beatRepeatButton{"RandomRetriggerBtn", "REPEAT"};
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
	void setupUI();
	void setupIconButtons();
	void updateButtonsEnabledState();
	void updateTrackInfo();
	void onTrackPresetSelected();
	void toggleOriginalSync();
	void statusCallback(const juce::String &message);
	void onIntervalChanged();
	void updateBeatRepeatButtonColor();
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