#pragma once
#include "components/ObsidianBase.h"
#include "PluginProcessor.h"
#include "midi/MidiLearnableComponents.h"
#include "components/shared/IconButton.h"
#include "components/mixer/VUMeter.h"

struct StereoLevel
{
	float left;
	float right;
};

class MixerChannel : public ObsidianComponent, public juce::Timer, public juce::AudioProcessorParameter::Listener
{
public:
	MixerChannel(const juce::String& trackId, DjIaVstProcessor& processor, TrackData* trackData);
	~MixerChannel() override;
	juce::String getTrackId() const { return trackId; }
	juce::Label trackNameLabel;
	TrackData* track;
	void setSelected(bool selected);
	void updateFromTrackData();
	void updateModelUI();
	void updateVUMeters();
	void setTrackData(TrackData* trackData);
	void updateButtonColors();
	void cleanup();
	void addEventListeners();
	void startGeneratingAnimation();
	void stopGeneratingAnimation();
	void setSamplePending(bool pending)
	{
		hasSamplePending = pending;
		repaint();
	}
	void setTrackName(const juce::String& name);
	std::function<void(const juce::String&)> onTrackRenamed;

private:
	DjIaVstProcessor& audioProcessor;
	VuMeter vuMeter;
	std::atomic<bool> isDestroyed{ false };
	juce::String trackId;

	bool isGenerating = false;
	bool stopBlinkState = false;
	bool hasSamplePending = false;

	bool isSelected = false;
	int bypassMidiFrames = 0;
	std::atomic<bool> isUpdatingButtons{ false };

	float currentAudioLevel = 0.0f;
	float peakHold = 0.0f;
	int peakHoldTimer = 0;
	std::vector<float> levelHistory;

	bool isBlinking = false;
	bool blinkState = false;

	juce::Rectangle<int> sliderBounds;

	IconButton playButton{ "Play", "PLAY" };
	IconButtonSimple stopButton{ "Stop", "STOP" };
	IconButton muteButton{ "Mute", "MUTE" };
	IconButton soloButton{ "Solo", "SOLO" };

	MidiLearnableSlider volumeSlider;

	MidiLearnableSlider pitchKnob;
	juce::Label pitchLabel;
	MidiLearnableSlider fineKnob;
	juce::Label fineLabel;

	MidiLearnableSlider panKnob;
	juce::Label panLabel;

	float currentAudioLevelLeft = 0.0f;
	float currentAudioLevelRight = 0.0f;
	float peakHoldLeft = 0.0f;
	float peakHoldRight = 0.0f;
	int peakHoldTimerLeft = 0;
	int peakHoldTimerRight = 0;

	std::vector<float> levelHistoryLeft;
	std::vector<float> levelHistoryRight;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void updateVUMeter();
	StereoLevel calculateInstantLevel();
	void timerCallback() override;
	void setupMidiLearn();
	void setupUI();
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void learn(juce::String param, MidiLearnableBase* component, std::function<void(float)> uiCallback = nullptr);
	void removeListener(juce::String name);
	void addListener(juce::String name);
	void setSliderParameter(juce::String name, juce::Slider& slider);
	void setButtonParameter(juce::String name, juce::Button& button);
	void updateUIFromParameter(const juce::String& paramName,
		const juce::String& slotPrefix,
		float newValue);
	void removeMidiMapping(const juce::String& param);
	void stopTrackImmediatly();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel);
	JUCE_DECLARE_WEAK_REFERENCEABLE(MixerChannel);
};