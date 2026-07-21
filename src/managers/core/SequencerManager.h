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
	void checkReverseWithSampleCounter();
	void executePendingAction(TrackData *track);
	void flushMidiBuffer(juce::MidiBuffer &destination, int numSamples);
	void setupBeatRepeatZone(TrackData *track, double hostBpm) const;

	void setBypass(bool bypass)
	{
		bypassSequencer.store(bypass);
	}
	bool isBypassed() const
	{
		return bypassSequencer.load();
	}
	void setWasPlaying(bool v)
	{
		wasPlaying.store(v);
	}

	double calculateBeatRepeatInterval(int intervalValue, double hostBpm) const;
	double getStartReadPosition(TrackData *track) const;

	std::function<void(const juce::String &trackId, int pageIndex)> onPageChanged;
	std::function<void(const juce::String &trackId)> onSequencerUpdateNeeded;
	std::atomic<int64_t> internalSampleCounter{0};

	double getTransientScatterStartPosition(TrackData *track) const;
	void checkQuantizedToggle(std::atomic<bool> &pending, std::atomic<bool> &stopPending,
	                          std::atomic<int64_t> &pendingBeat, std::atomic<int64_t> &pendingStopBeat,
	                          std::atomic<bool> &active, int64_t currentHalfBeatNumber,
	                          std::function<void()> onActivate, std::function<void()> onDeactivate);
	void checkTransientScatterWithSampleCounter();
	double getEffectiveWindowLength(TrackData *track, const TrackPage &page, bool applyPlaybackRatio = true) const;

  private:
	DjIaVstProcessor &audioProcessor;
	TrackManager &trackManager;

	juce::MidiBuffer sequencerMidiBuffer;
	juce::CriticalSection sequencerMidiLock;

	std::atomic<bool> bypassSequencer{false};
	std::atomic<bool> wasPlaying{false};

	void acquireTheoreticalPosition(TrackData *track) const;
	void releaseTheoreticalPosition(TrackData *track) const;

	int64_t getCurrentHalfBeatNumber(double hostBpm) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerManager)
};