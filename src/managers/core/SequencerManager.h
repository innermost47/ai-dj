#pragma once
#include "TrackManager.h"
#include <JuceHeader.h>
#include <atomic>

class DjIaVstProcessor;

class SequencerManager
{
  public:
	SequencerManager(DjIaVstProcessor &processor, TrackManager &trackManager);
	~SequencerManager() = default;

	void handleSequencerPlayState(bool hostIsPlaying);
	void updateSequencers(bool hostIsPlaying, int numSamples);
	void addSequencerMidiMessage(const juce::MidiMessage &message);
	void handleAdvanceStep(TrackData *track, bool hostIsPlaying);
	void triggerSequencerStep(TrackData *track);
	void handlePageChange(const juce::String &parameterID);
	void handleSequenceChange(int slotNum, int targetSequence);
	void checkBeatRepeatWithSampleCounter();
	void executePendingAction(TrackData *track);
	void flushMidiBuffer(juce::MidiBuffer &destination, int numSamples);

	void setBypass(bool bypass)
	{
		bypassSequencer.store(bypass);
	}
	bool isBypassed() const
	{
		return bypassSequencer.load();
	}

	double calculateRetriggerInterval(int intervalValue, double hostBpm) const;

	std::function<void(const juce::String &trackId, int pageIndex)> onPageChanged;
	std::function<void(const juce::String &trackId)> onSequencerUpdateNeeded;
	std::atomic<int64_t> internalSampleCounter{0};

  private:
	DjIaVstProcessor &audioProcessor;
	TrackManager &trackManager;

	juce::MidiBuffer sequencerMidiBuffer;
	juce::CriticalSection sequencerMidiLock;

	std::atomic<bool> bypassSequencer{false};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerManager)
};