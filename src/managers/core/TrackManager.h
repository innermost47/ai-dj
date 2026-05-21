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
	                     int timeSignatureNumerator, int timeSignatureDenominator, double sampleRate);

	void loadAudioFileForPage(TrackData *track, int pageIndex, const juce::File &audioFile);
	void addTrack(const std::string &trackId, std::unique_ptr<TrackData> track);
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

	void processPerTrackReverbs(std::vector<juce::AudioBuffer<float>> &individualOutputs,
	                            juce::AudioBuffer<float> &mainOutput, float size, float damping, float width, float mix,
	                            int numSamples);

  private:
	mutable juce::CriticalSection tracksLock;
	std::map<std::string, std::unique_ptr<TrackData>> tracks;
	std::vector<std::string> trackOrder;
	juce::AudioBuffer<float> perTrackFxBuffer;

	double currentSampleRate = ObsidianDataConst::SAMPLERATE;
	int currentMaxBlockSize = ObsidianDataConst::MAX_BLOCK_SIZE;
	bool audioPrepared = false;

	std::array<bool, 8> usedSlots{false};

	void renderSingleTrack(TrackData &track, juce::AudioBuffer<float> &mixOutput,
	                       juce::AudioBuffer<float> &individualOutput, juce::AudioBuffer<float> & /* previewOutput */,
	                       int numSamples, int /* trackIndex */, double hostBpm, int timeSignatureNumerator,
	                       int timeSignatureDenominator, double sampleRate) const;

	float interpolateLinear(const float *buffer, double position, int bufferSize) const;

	static float applyCrossfadeCurve(float xfaderValue, bool isDeckA, int curveMode);
};