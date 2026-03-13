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

	static int ccFeedbackPlay(int slot) { return 20 + slot; }
	static int ccFeedbackGenerate(int slot) { return 30 + slot; }
	static int ccFeedbackPage(int slot) { return 40 + slot; }
};