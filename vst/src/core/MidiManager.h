#pragma once
#include "MidiLearnManager.h"
#include <JuceHeader.h>
#include <atomic>

class DjIaVstProcessor;

class MidiManager
{
  public:
	MidiManager(DjIaVstProcessor &processor, MidiLearnManager &midiLearnManager);
	~MidiManager() = default;

	void sendMidiFeedback(int cc, int value);
	void sendMidiFeedback(int cc, int value, int channel);
	void sendFullStateFeedback();
	void notifyPageChangedFeedback(int slotNumber, int pageIndex);

	void flushFeedbackBuffer(juce::MidiBuffer &destination, int numSamples);

	void processMidiMessages(juce::MidiBuffer &midiMessages, bool hostIsPlaying, double hostBpm);
	void handlePlayAndStop(bool hostIsPlaying);
	void updateMidiIndicatorWithActiveNotes(double hostBpm, const juce::Array<int> &triggeredNotes);

	void setMidiIndicatorCallback(std::function<void(const juce::String &)> callback)
	{
		midiIndicatorCallback = callback;
	}

  private:
	DjIaVstProcessor &audioProcessor;
	MidiLearnManager &midiLearnManager;

	juce::MidiBuffer feedbackMidiBuffer;
	juce::CriticalSection feedbackMidiLock;

	std::function<void(const juce::String &)> midiIndicatorCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiManager)
};