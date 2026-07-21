#pragma once
#include "ObsidianBaseMidiComponent.h"
#include "PluginProcessor.h"
#include "VuMeter.h"
#include <JuceHeader.h>

struct StereoLevel
{
	float left;
	float right;
};

class DjIaVstEditor;

class MixerChannel : public ObsidianBaseMidiComponent
{
  public:
	MixerChannel(const juce::String &trackId, DjIaVstProcessor &processor, TrackData *trackData, DjIaVstEditor &editor);
	~MixerChannel() override;
	juce::String getTrackId() const
	{
		return trackId;
	}
	juce::Label trackNameLabel;

	void updateFromTrackData();
	void updateModelUI();
	void updateVUMeters();
	void setTrackData(TrackData *trackData);
	void updateButtonColors();
	void cleanup();
	void startGeneratingAnimation();
	void stopGeneratingAnimation();
	void setSamplePending()
	{
		hasSamplePending = track->hasSamplePending.load();
		repaint();
	}
	void setTrackName(const juce::String &name);
	void wireParameters();
	void addEventListeners();
	void setSelected(bool selected);
	std::function<void(const juce::String &)> onTrackRenamed;

  private:
	VuMeter vuMeter;
	DjIaVstEditor &editor;

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	juce::String trackId;

	bool isGenerating = false;
	bool stopBlinkState = false;
	bool hasSamplePending = false;
	bool isSelected = false;

	int bypassMidiFrames = 0;
	std::atomic<bool> isUpdatingButtons{false};

	float currentAudioLevel = 0.0f;
	float peakHold = 0.0f;
	int peakHoldTimer = 0;
	std::vector<float> levelHistory;

	bool isBlinking = false;
	bool blinkState = false;

	juce::Rectangle<int> sliderBounds;

	IconButton playButton{"Play", "PLAY"};
	IconButtonSimple stopButton{"Stop", "STOP"};
	IconButton muteButton{"Mute", "MUTE"};
	IconButton soloButton{"Solo", "SOLO"};

	MidiLearnableSlider volumeSlider;

	MidiLearnableSlider pitchKnob;
	juce::Label pitchLabel;
	MidiLearnableSlider fineKnob;
	juce::Label fineLabel;
	MidiLearnableSlider sendDelayKnob;
	juce::Label sendDelayLabel;
	MidiLearnableSlider sendReverbKnob;
	juce::Label sendReverbLabel;

	MidiLearnableSlider panKnob;
	juce::Label panLabel;

	MidiLearnableSlider gainKnob;
	juce::Label gainLabel;

	float currentAudioLevelLeft = 0.0f;
	float currentAudioLevelRight = 0.0f;
	float peakHoldLeft = 0.0f;
	float peakHoldRight = 0.0f;
	int peakHoldTimerLeft = 0;
	int peakHoldTimerRight = 0;
	int lastQuantizedL = -1, lastQuantizedR = -1;
	int blinkCounter = 0;

	bool blinkTicking = false;

	juce::Component vuMeterContainer;

	std::vector<float> levelHistoryLeft;
	std::vector<float> levelHistoryRight;

	bool isApplyingPlayState = false;

	void applyPlayState(bool shouldArm);

	void paint(juce::Graphics &g) override;
	void resized() override;
	void updateVUMeter();
	void handleVBlank();
	void setupUI();
	void stopTrackImmediatly();

  protected:
	juce::String getParameterPrefix() const override
	{
		auto *t = track.get();
		if (!t || t->slotIndex == -1)
			return {};
		return "slot" + juce::String(t->slotIndex + 1);
	}

	juce::String getMidiLearnDescriptionPrefix() const override
	{
		auto *t = track.get();
		if (!t || t->slotIndex == -1)
			return {};
		return "Slot " + juce::String(t->slotIndex + 1) + " ";
	}
	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel);
	JUCE_DECLARE_WEAK_REFERENCEABLE(MixerChannel);
};