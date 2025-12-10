#pragma once
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "MidiLearnableComponents.h"

class MasterChannel : public juce::Component, public juce::AudioProcessorParameter::Listener
{
public:
	MasterChannel(DjIaVstProcessor& processor);
	~MasterChannel();
	void drawMasterVUMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const;
	void drawPeakHoldLine(int numSegments, juce::Rectangle<float>& vuArea, float segmentHeight, juce::Graphics& g) const;
	void drawMasterClipping(juce::Rectangle<float>& vuArea, juce::Graphics& g) const;
	void drawMasterChanelSegments(juce::Rectangle<float>& vuArea, int i, float segmentHeight, int numSegments, juce::Graphics& g) const;
	void setRealAudioLevel(float level);
	void updateMasterLevels();

	std::function<void(float)> onMasterVolumeChanged;
	std::function<void(float)> onMasterPanChanged;
	std::function<void(float, float, float)> onMasterEQChanged;

private:
	DjIaVstProcessor& audioProcessor;

	MidiLearnableSlider masterVolumeSlider;
	MidiLearnableSlider masterPanKnob;
	MidiLearnableSlider highKnob, midKnob, lowKnob;

	std::atomic<bool> isDestroyed{ false };

	float realAudioLevel = 0.0f;
	bool hasRealAudio = false;

	juce::Label masterLabel;
	juce::Label highLabel, midLabel, lowLabel, panLabel;

	float masterLevel = 0.0f;
	float masterPeakHold = 0.0f;
	int masterPeakHoldTimer = 0;
	bool isClipping = false;

	void setupUI();
	void paint(juce::Graphics& g) override;
	void resized() override;
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void setupMidiLearn();
	void removeMidiMapping(const juce::String& param);
	void learn(juce::String param, juce::String description, MidiLearnableBase* component, std::function<void(float)> uiCallback = nullptr);
	void removeListener(juce::String name);
	void addListener(juce::String name);
	void addEventListeners();
	void setSliderParameter(juce::String name, juce::Slider& slider);
	void updateUIFromParameter(const juce::String& paramName, float newValue);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannel)
};
