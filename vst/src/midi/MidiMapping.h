#pragma once
#include <JuceHeader.h>

class DjIaVstProcessor;

struct MidiMapping
{
	int midiType;
	int midiNumber;
	int midiChannel;
	juce::String parameterName;
	juce::String description;
	DjIaVstProcessor* processor;
	std::function<void(float)> uiCallback;

	static const int feedbackChannel = 1;
	static const int feedbackIdle = 0;
	static const int feedbackPending = 64;
	static const int feedbackActive = 127;
	static const int ccRequestState = 118;

	static int ccFeedbackPlay(int slot) { return 20 + slot; }
	static int ccFeedbackGenerate(int slot) { return 30 + slot; }
	static int ccFeedbackPage(int slot) { return 40 + slot; }
	static int ccFeedbackVolume(int slot) { return 50 + slot; }
	static int ccFeedbackPan(int slot) { return 60 + slot; }
	static int ccFeedbackPitch(int slot) { return 70 + slot; }
	static int ccFeedbackFine(int slot) { return 80 + slot; }
	static int ccFeedbackSeq(int slot) { return 90 + slot; }
	static int ccFeedbackMute(int slot) { return 100 + slot; }
	static int ccFeedbackSolo(int slot) { return 109 + slot; }
	static int ccFeedbackBeatRepeat(int slot) { return 118 + slot; }

	static int volumeToMidi(float v) { return juce::roundToInt(v * 127.0f); }
	static int panToMidi(float p) { return juce::roundToInt((p + 1.0f) / 2.0f * 127.0f); }
	static int pitchToMidi(float p) { return juce::jlimit(0, 127, juce::roundToInt((p + 96.0f) / 192.0f * 127.0f)); }
	static int fineToMidi(float f) { return juce::jlimit(0, 127, juce::roundToInt((f + 100.0f) / 200.0f * 127.0f)); }
};