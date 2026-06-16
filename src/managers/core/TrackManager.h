#pragma once
#include "AiModelDefinitions.h"
#include "TrackData.h"
#include <JuceHeader.h>

class TrackManager
{
  public:
	TrackManager() = default;

	std::atomic<std::function<void(int slot, TrackData *track)> *> parameterUpdateCallback{nullptr};
	std::function<void(const juce::String &)> onPreviewEnded;
	std::atomic<bool> isInitializing{false};

	juce::String createTrack(const juce::String &name = "Track");

	TrackData *getTrack(const juce::String &trackId);

	std::vector<juce::String> getAllTrackIds() const;

	void renderAllTracks(juce::AudioBuffer<float> &outputBuffer,
	                     std::vector<juce::AudioBuffer<float>> &individualOutputs,
	                     juce::AudioBuffer<float> &previewOutput, double hostBpm, const float pairPrev[4],
	                     const float pairCurrent[4], float globalPrev, float globalCurrent, int curveMode,
	                     int timeSignatureNumerator, int timeSignatureDenominator, double sampleRate,
	                     bool useCrossfader);

	void loadAudioFileForPage(TrackData *track, int pageIndex, const juce::File &audioFile);
	void addTrack(const std::string &trackId, std::unique_ptr<TrackData> track);
	void prepareTrack(TrackData &track);
	void clearAllTracks();
	void forEachTrack(std::function<void(const std::string &, const TrackData *)> callback) const;
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

	juce::ScopedLock getTracksLock() const
	{
		return juce::ScopedLock(tracksLock);
	}

  private:
	mutable juce::CriticalSection tracksLock;
	std::map<std::string, std::unique_ptr<TrackData>> tracks;
	std::vector<std::string> trackOrder;
	juce::AudioBuffer<float> perTrackFxBuffer;

	struct PageInfo
	{
		const TrackPage &currentPage;
		const juce::AudioSampleBuffer *bufferToUse = nullptr;
		int numSamplesToUse = 0;
		double sampleRateToUse = 0;
		double loopStartToUse = 0;
		double loopEndToUse = 0;
		float originalBpmToUse = 126.0f;
		float adsrAttack = 0.0f;
		float adsrDecay = 4.0f;
		float adsrSustain = 1.0f;
		float adsrRelease = 0.0f;
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
		const int BR_FADE_DURATION;
		const int BR_FADE_IN_LENGTH;
		double samplesUntilBeatRepeatEnd;
		double endSampleLoop;
		int brFadeInCounter;
		int fadeInCounter;
		double fadeLength;
		float fadeRcp;
		bool fadeOutThisBuffer;
		double samplesUntilLoopEnd;
		double FADE_DURATION;
	};

	double currentSampleRate = Obsidian::SAMPLERATE;
	int currentMaxBlockSize = Obsidian::MAX_BLOCK_SIZE;
	bool audioPrepared = false;

	std::array<bool, 8> usedSlots{false};

	void renderSingleTrack(TrackData &track, juce::AudioBuffer<float> &mixOutput,
	                       juce::AudioBuffer<float> &individualOutput, juce::AudioBuffer<float> & /* previewOutput */,
	                       int numSamples, int /* trackIndex */, double hostBpm, int timeSignatureNumerator,
	                       int timeSignatureDenominator, double sampleRate) const;
	void handleOutput(juce::AudioSampleBuffer &individualOutput, juce::AudioSampleBuffer &mixOutput, TrackData &track,
	                  double &currentPosition, float volume) const;
	void prepareOutput(float adsrGain, float safetyFade, const float *leftChannel, const float *rightChannel,
	                   double absolutePosition, int bufferSize, const bool beatRepeatActive, double endSampleLoop,
	                   juce::AudioSampleBuffer &individualOutput, TrackData &track, int i, TrackInfo &trackInfo) const;

	PageInfo getPageInfo(const TrackPage &page, double sampleRate) const;
	TrackInfo getTrackInfo(const TrackData &track, const TrackPage &page, const PageInfo &pageInfo) const;
	FadeInfo getFadeInfo(TrackData &track, const TrackInfo &trackInfo, const TrackPage &page, const PageInfo &pageInfo,
	                     int timeSignatureNumerator, int timeSignatureDenominator, double hostBpm) const;

	float interpolateLinear(const float *buffer, double position, int bufferSize) const;
	float getADSRGain(double absolutePosition, double startSample, double sectionLength, PageInfo &info) const;
	float prepareSafetyFade(int i, double posInLoop, double loopLength, FadeInfo &fadeInfo) const;

	double getNextStepSampleOn(double stepsPerMeasure, double samplesPerStep, SequencerData &seqData, int numMeasures,
	                           double realPosition) const;

	static float applyCrossfadeCurve(float xfaderValue, bool isDeckA, int curveMode);
};