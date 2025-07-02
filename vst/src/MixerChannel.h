/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

#pragma once
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "MidiLearnableComponents.h"

class MixerChannel : public juce::Component, public juce::Timer, public juce::AudioProcessorParameter::Listener
{
public:
	MixerChannel(const juce::String& trackId, DjIaVstProcessor& processor, TrackData* trackData);
	~MixerChannel() override;
	juce::String getTrackId() const { return trackId; }
	juce::Label trackNameLabel;

	float getCurrentAudioLevel() const { return currentAudioLevel; }
	float getPeakLevel() const { return peakHold; }
	void setSelected(bool selected);
	void updateFromTrackData();
	void updateVUMeters();
	void setTrackData(TrackData* trackData);
	void updateButtonColors();
	void cleanup();

private:
	DjIaVstProcessor& audioProcessor;
	std::atomic<bool> isDestroyed{ false };
	juce::String trackId;
	TrackData* track;
	bool isSelected = false;
	int bypassMidiFrames = 0;
	std::atomic<bool> isUpdatingButtons{ false };
	std::atomic<float>* speedParam = nullptr;

	float currentAudioLevel = 0.0f;
	float peakHold = 0.0f;
	int peakHoldTimer = 0;
	std::vector<float> levelHistory;

	bool isBlinking = false;
	bool blinkState = false;

	MidiLearnableButton playButton;
	juce::TextButton stopButton;
	MidiLearnableButton muteButton;
	MidiLearnableButton soloButton;

	MidiLearnableSlider volumeSlider;

	MidiLearnableSlider pitchKnob;
	juce::Label pitchLabel;
	MidiLearnableSlider fineKnob;
	juce::Label fineLabel;

	MidiLearnableSlider panKnob;
	juce::Label panLabel;

	MidiLearnableSlider speedKnob;
	juce::Label speedLabel;

	void paint(juce::Graphics& g) override;
	void drawVUMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
	void fillMeters(juce::Rectangle<float>& vuArea, int i, float segmentHeight, int numSegments, float currentLevel, juce::Graphics& g);
	void resized() override;
	void updateVUMeter();
	float calculateInstantLevel();
	void setCurrentLevel(float level);
	void timerCallback() override;
	void setupMidiLearn();
	void setupUI();
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
	void addEventListeners();
	void learn(juce::String param, std::function<void(float)> uiCallback = nullptr);
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