#pragma once
#include "BpmDetector.h"
#include "DummySynth.h"
#include "GenerationManager.h"
#include "SimpleEQ.h"
#include "TrackManager.h"
#include <JuceHeader.h>
#include <atomic>
#include <vector>

class DjIaVstProcessor;

class AudioManager
{
  public:
	AudioManager(DjIaVstProcessor &processor, TrackManager &trackManager, GenerationManager &generationManager);
	~AudioManager() = default;

	struct PreprocessResult
	{
		juce::File stretchedFile;
		juce::File originalFile;
		bool hasOriginalVersion = false;
		float originalBpm = 126.0f;
		bool success = false;
		bool stagingFilled = false;
	};

	struct GenerationFiles
	{
		juce::File stretched;
		juce::File original;
	};

	PreprocessResult preprocessAudioFile(const juce::File &rawFile, float serverSnappedBpm,
	                                     const juce::String &trackId);

	void prepareToPlay(double sampleRate, int samplesPerBlock);
	void releaseResources();
	void initDummySynth();
	void initBuffers(int numTracks);
	void processIncomingAudio(bool hostIsPlaying);
	void applyMasterEffects(juce::AudioSampleBuffer &mainOutput);
	void copyToIndividualOutputs(juce::AudioSampleBuffer &buffer);
	void clearOutputBuffers(juce::AudioSampleBuffer &buffer);
	void resizeIndividualBuffers(juce::AudioSampleBuffer &buffer);
	void checkAndSwapStagingBuffers();
	void performAtomicSwap(TrackData *track, const juce::String &trackId);
	void updateWaveformDisplay(const juce::String &trackId);
	void processAudioBPMAndSync(TrackData *track, float sampleBpm = -1.0f);
	void updateMasterEQ();
	void saveBufferToFile(const juce::AudioBuffer<float> &buffer, const juce::File &outputFile, double sampleRate);
	void loadAudioFileForPageSwitch(const juce::String &trackId, int pageIndex, const juce::File &audioFile);
	void loadSampleToBankPage(const juce::String &trackId, int pageIndex, const juce::File &sampleFile,
	                          const juce::String &sampleId, float sampleBpm, double fileSampleRate);
	void loadSampleFromBank(const juce::String &sampleId, const juce::String &trackId);

	juce::File getExportDirectory();
	juce::File exportSampleForDragDrop(const juce::File &originalFile);
	juce::File getTrackPageAudioFile(const juce::String &trackId, int pageIndex);

	bool isSamplePreviewing() const
	{
		return previewActive.load();
	}
	void stopSamplePreview();
	bool previewSampleFromBank(const juce::String &sampleId);
	void stopTrackPreview(const juce::String &trackId);

	float getPeakLevelLeft() const
	{
		return meterAccumPeakLeft;
	}
	float getPeakLevelRight() const
	{
		return meterAccumPeakRight;
	}
	void setPeakLevels(float l, float r)
	{
		meterAccumPeakLeft = l;
		meterAccumPeakRight = r;
	}
	std::vector<juce::AudioBuffer<float>> &getIndividualOutputBuffers()
	{
		return individualOutputBuffers;
	}

	int getBlockSize() const
	{
		return blockSize;
	}

	void renderPreviewToOutput(juce::AudioBuffer<float> &previewBus, juce::AudioBuffer<float> &mainOutput,
	                           int numSamples, double currentSampleRate, bool previewBusIsEffectivelyEnabled);

	void computeAndSetPeakLevels(const juce::AudioBuffer<float> &buffer);

  private:
	DjIaVstProcessor &audioProcessor;
	TrackManager &trackManager;
	GenerationManager &generationManager;

	SimpleEQ masterEQ;

	juce::Synthesiser synth;

	double hostSampleRate = 44100.0;
	int blockSize = Obsidian::MAX_BLOCK_SIZE;

	std::atomic<bool> previewActive{false};
	juce::AudioBuffer<float> previewBuffer;
	std::atomic<double> previewPosition{0.0};
	std::atomic<double> previewSampleRate{44100.0};
	juce::CriticalSection previewLock;
	juce::String currentPreviewTrackId;

	float meterAccumPeakLeft{0.0f};
	float meterAccumPeakRight{0.0f};
	int meterSampleCounter{0};
	int meterUpdateInterval{2400};

	std::vector<juce::AudioBuffer<float>> individualOutputBuffers;

	std::unique_ptr<BpmDetector> bpmDetector = createBpmDetector();

	float smoothedMasterVol = 1.0f;
	float smoothedMasterPan = 0.0f;

	struct AudioData
	{
		int cleanedSize;
		int firstValidSample;
	};

	AudioData getAudioTrimmed(int outputSamples, int numChannels, juce::AudioBuffer<float> &finalStretchedAudio);
	GenerationFiles createGenerationFiles(const juce::String &trackId, int pageIndex);

	float getBPM(double hostBpm, juce::AudioBuffer<float> &cleanedRawBuffer, const int cleanedNumSamples,
	             const double sampleRate);
	juce::AudioBuffer<float> stretchAndTrim(const juce::AudioBuffer<float> &inputBuffer, double ratio,
	                                        double sampleRate);

	void registerGeneratedSample(const juce::String &trackId, const juce::File &stretchedFile,
	                             const juce::File &originalFile);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioManager)
};