#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBase.h"
#include "PluginProcessor.h"
#include "VuMeter.h"
#include <JuceHeader.h>

class MasterChannel : public ObsidianComponent, public juce::AudioProcessorParameter::Listener
{
  public:
	MasterChannel(DjIaVstProcessor &processor);
	~MasterChannel();
	void setRealAudioLevelStereo(float levelLeft, float levelRight);
	void updateMasterLevels();

	std::function<void(float)> onMasterVolumeChanged;
	std::function<void(float)> onMasterPanChanged;
	std::function<void(float, float, float)> onMasterEQChanged;

  private:
	DjIaVstProcessor &audioProcessor;
	VuMeter vuMeter;
	MidiLearnableSlider masterVolumeSlider;
	MidiLearnableSlider masterPanKnob;
	MidiLearnableSlider highKnob, midKnob, lowKnob;

	juce::Rectangle<int> masterVUBounds;

	std::atomic<bool> isDestroyed{false};

	bool hasRealAudio = false;

	juce::Label masterLabel;
	juce::Label highLabel, midLabel, lowLabel, panLabel;

	float masterLevel = 0.0f;
	float masterPeakHold = 0.0f;
	float masterLevelLeft = 0.0f;
	float masterLevelRight = 0.0f;
	float masterPeakHoldLeft = 0.0f;
	float realAudioLevelLeft = 0.0f;
	float realAudioLevelRight = 0.0f;
	float masterPeakHoldRight = 0.0f;
	float realAudioLevel = 0.0f;

	int masterPeakHoldTimerLeft = 0;
	int masterPeakHoldTimerRight = 0;
	int masterPeakHoldTimer = 0;

	void setupUI();
	void paint(juce::Graphics &g) override;
	void resized() override;
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void setupMidiLearn();
	void removeMidiMapping(const juce::String &param);
	void learn(juce::String param, juce::String description, MidiLearnableBase *component,
	           std::function<void(float)> uiCallback = nullptr);
	void removeListener(juce::String name);
	void addListener(juce::String name);
	void addEventListeners();
	void setSliderParameter(juce::String name, juce::Slider &slider);
	void updateUIFromParameter(const juce::String &paramName, float newValue);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannel)
};
