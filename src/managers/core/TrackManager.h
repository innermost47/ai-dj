#pragma once
#include "AiModelDefinitions.h"
#include "TrackData.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class TrackManager
{
  public:
	TrackManager(DjIaVstProcessor &processor);

	std::atomic<std::function<void(int slot, TrackData *track)> *> parameterUpdateCallback{nullptr};
	std::function<void(const juce::String &)> onPreviewEnded;
	std::atomic<bool> isInitializing{false};

	juce::String createTrack(const juce::String &name = "Track");

	TrackData *getTrack(const juce::String &trackId);

	TrackData *getTrackBySlot(int slot) const noexcept
	{
		if (slot < 0 || slot >= Obsidian::MAX_TRACKS)
			return nullptr;
		return slotRegistry[(size_t)slot].load(std::memory_order_acquire);
	}

	std::vector<juce::String> getAllTrackIds() const;

	void renderAllTracks(juce::AudioBuffer<float> &outputBuffer,
	                     std::vector<juce::AudioBuffer<float>> &individualOutputs,
	                     juce::AudioBuffer<float> &previewOutput, const float pairPrev[4], const float pairCurrent[4],
	                     float globalPrev, float globalCurrent, int curveMode, double sampleRate, bool useCrossfader);

	void loadAudioFileForPage(TrackData *track, int pageIndex, const juce::File &audioFile);
	void addTrack(const juce::String &trackId, std::unique_ptr<TrackData> track);
	void prepareTrack(TrackData &track);
	void clearAllTracks();
	void forEachTrack(std::function<void(const juce::String &, const TrackData *)> callback) const;
	void forEachTrack(std::function<void(const TrackData *)> callback) const;
	void setSlotUsed(int index, bool used);
	void resetAllSlots();

	bool isSlotUsed(int index) const;

	int findFreeSlot();

	size_t getNumTracks() const;

	void processPerTrackDelays(std::vector<juce::AudioBuffer<float>> &individualOutputs,
	                           juce::AudioBuffer<float> &mainOutput, double hostBpm, DelaySend::TimeDivision division,
	                           float feedback, DelaySend::Mode mode, int numSamples);

	void prepareSends(double sampleRate, int maxBlockSize);
	void updateBeatRepeat(TrackData *track, int value, double hostBpm, double repeatDuration);

	void processPerTrackReverbs(std::vector<juce::AudioBuffer<float>> &individualOutputs,
	                            juce::AudioBuffer<float> &mainOutput, float size, float damping, float width, float mix,
	                            int numSamples);
	void ensureTransientsAnalyzed(TrackPage &page) const;

	struct PlaybackRatioInfo
	{
		double playbackRatio;
		float pitchSemis;
		float fineCents;
		float totalSemis;
	};

	PlaybackRatioInfo getPlaybackRatio(const TrackPage &page) const;

	juce::ScopedLock getTracksLock() const
	{
		return juce::ScopedLock(tracksLock);
	}

	template <typename Fn> void forEachTrackAudio(Fn &&callback)
	{
		for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
			if (auto *t = slotRegistry[(size_t)i].load(std::memory_order_acquire))
				callback(t);
	}

  private:
	DjIaVstProcessor &audioProcessor;
	mutable juce::CriticalSection tracksLock;
	std::map<juce::String, std::unique_ptr<TrackData>> tracks;
	std::vector<juce::String> trackOrder;
	juce::AudioBuffer<float> perTrackFxBuffer;
	juce::AudioBuffer<float> tempMixBuffer;
	juce::AudioBuffer<float> tempIndividualBuffer;
	std::array<std::atomic<TrackData *>, Obsidian::MAX_TRACKS> slotRegistry{};

	struct PageInfo
	{
		const TrackPage &currentPage;
		const juce::AudioSampleBuffer *bufferToUse = nullptr;
		int numSamplesToUse = 0;
		double sampleRateToUse = 0;
		double loopStartToUse = 0;
		double loopEndToUse = 0;
		float originalBpmToUse = 126.0f;
		float adsrAttack = Obsidian::ADSRDefaultValues::ATTACK_DEFAULT;
		float adsrDecay = Obsidian::ADSRDefaultValues::DECAY_DEFAULT;
		float adsrSustain = Obsidian::ADSRDefaultValues::SUSTAIN_DEFAULT;
		float adsrRelease = Obsidian::ADSRDefaultValues::RELEASE_DEFAULT;
	};

	struct TrackInfo
	{
		float volume;
		float pan;
		float leftGain;
		float rightGain;
		double currentPosition;
		double playbackRatio = 1.0;
		float pitchSemis;
		float fineCents;
		float totalSemis;
		double startSample;
		double endSample;
	};

	struct FadeInfo
	{
		bool beatRepeatActive;
		double beatRepeatEnd;
		double beatRepeatStart;
		bool fadeOutArmed;
		double samplesUntilEnd;
		double safetyFadeLength;
		double totalSamplesPerSequence;
	};

	double currentSampleRate = Obsidian::SAMPLERATE;
	int currentMaxBlockSize = Obsidian::MAX_BLOCK_SIZE;
	bool audioPrepared = false;

	std::array<bool, 8> usedSlots{false};

	void renderSingleTrack(TrackData &track, juce::AudioBuffer<float> &mixOutput,
	                       juce::AudioBuffer<float> &individualOutput, int numSamples, double sampleRate) const;
	void handleOutput(juce::AudioSampleBuffer &individualOutput, juce::AudioSampleBuffer &mixOutput, TrackData &track,
	                  double &currentPosition, float volume) const;
	void prepareOutput(float adsrGain, float oldAdsrGain, const float *leftChannel, const float *rightChannel,
	                   double absolutePosition, int bufferSize, juce::AudioSampleBuffer &individualOutput,
	                   TrackData &track, int i, TrackInfo &trackInfo, FadeInfo &fadeInfo) const;

	PageInfo getPageInfo(const TrackPage &page, double sampleRate) const;
	TrackInfo getTrackInfo(const TrackData &track, const TrackPage &page, const PageInfo &pageInfo) const;
	FadeInfo getFadeInfo(TrackData &track, const TrackInfo &trackInfo, const TrackPage &page) const;

	float interpolateLinear(const float *buffer, double position, int bufferSize) const;
	float getADSRGain(double absolutePosition, double startSample, double sectionLength, PageInfo &info) const;

	void applyJumpSmoothing(TrackData &track, float &left, float &right, const float *leftChannel,
	                        const float *rightChannel, int bufferSize, float oldAdsrGain) const;

	static float applyCrossfadeCurve(float xfaderValue, bool isDeckA, int curveMode);

	void publishSlotRegistry();
};