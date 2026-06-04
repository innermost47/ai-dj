#pragma once
#include "Compressor.h"
#include "Console6Channel.h"
#include "DataConst.h"
#include "DelaySend.h"
#include "DjIaClient.h"
#include "Equalizer.h"
#include "LowpassHighpassFilter.h"
#include "ReverbSend.h"
#include <JuceHeader.h>
#include <memory>

struct TrackStretchImpl;

struct SequencerData
{
	bool steps[Obsidian::MAX_MEASURES][Obsidian::MAX_STEPS_PER_MEASURE] = {};
	float velocities[Obsidian::MAX_MEASURES][Obsidian::MAX_STEPS_PER_MEASURE] = {};
	bool isPlaying = false;
	int currentStep = 0;
	int currentMeasure = 0;
	int numMeasures = 1;
	int beatsPerMeasure = 4;
	double stepAccumulator = 0.0;
	double samplesPerStep = 0.0;

	SequencerData()
	{
		steps[0][0] = true;
		velocities[0][0] = 0.8f;
	}
};

struct TrackPage
{
	juce::AudioSampleBuffer audioBuffer;

	juce::String audioFilePath;
	juce::String prompt;
	juce::String selectedPrompt;
	juce::String generationPrompt;
	juce::String generationKey;
	juce::String canvasData;
	juce::String canvasState;
	juce::String selectedModel;
	juce::String savedModelBeforeLocal;

	juce::StringArray selectedKeywords;

	int numSamples = 0;
	int generationDuration = 6;

	double sampleRate = Obsidian::SAMPLERATE;
	double loopStart = 0.0;
	double loopEnd = 4.0;

	std::atomic<float> fineOffset{0.0f};
	std::atomic<float> pitchSemitones{0.0f};
	std::atomic<float> gain{0.0f};

	float stagingOriginalBpm = 126.0f;
	float bpm = 126.0f;
	float originalBpm = 126.0f;
	float generationBpm;

	std::atomic<bool> useOriginalFile{false};
	std::atomic<bool> hasOriginalVersion{false};
	std::atomic<bool> isLoaded{false};
	std::atomic<bool> isLoading{false};
	std::atomic<bool> loopPointsLocked{false};

	std::atomic<float> adsrAttack{0.0f};
	std::atomic<float> adsrDecay{4.0f};
	std::atomic<float> adsrSustain{1.0f};
	std::atomic<float> adsrRelease{0.0f};

	TrackPage() : generationBpm(126.0f)
	{
	}

	TrackPage(const TrackPage &other)
	    : audioBuffer(other.audioBuffer), audioFilePath(other.audioFilePath), prompt(other.prompt),
	      selectedPrompt(other.selectedPrompt), generationPrompt(other.generationPrompt),
	      generationKey(other.generationKey), selectedModel(other.selectedModel), numSamples(other.numSamples),
	      generationDuration(other.generationDuration), sampleRate(other.sampleRate), loopStart(other.loopStart),
	      loopEnd(other.loopEnd), stagingOriginalBpm(other.stagingOriginalBpm), bpm(other.bpm),
	      originalBpm(other.originalBpm), generationBpm(other.generationBpm),
	      savedModelBeforeLocal(other.savedModelBeforeLocal)
	{
		useOriginalFile = other.useOriginalFile.load();
		hasOriginalVersion = other.hasOriginalVersion.load();
		isLoaded = other.isLoaded.load();
		isLoading = other.isLoading.load();
		loopPointsLocked = other.loopPointsLocked.load();
		adsrAttack.store(other.adsrAttack.load());
		adsrDecay.store(other.adsrDecay.load());
		adsrSustain.store(other.adsrSustain.load());
		adsrRelease.store(other.adsrRelease.load());
		pitchSemitones.store(other.pitchSemitones.load());
		gain.store(other.gain.load());
	}

	void reset()
	{
		audioBuffer.setSize(0, 0);
		audioFilePath.clear();
		numSamples = 0;
		sampleRate = Obsidian::SAMPLERATE;
		originalBpm = 126.0f;
		prompt.clear();
		selectedPrompt.clear();
		generationPrompt.clear();
		selectedModel.clear();
		generationBpm = 126.0f;
		generationKey.clear();
		generationDuration = 6;
		loopStart = 0.0;
		loopEnd = 4.0;
		useOriginalFile = false;
		hasOriginalVersion = false;
		isLoaded = false;
		isLoading = false;
		loopPointsLocked = false;
		adsrAttack.store(0.0f);
		adsrDecay.store(4.0f);
		adsrSustain.store(1.0f);
		adsrRelease.store(0.0f);
		pitchSemitones.store(0.0);
		gain.store(0.0);
		savedModelBeforeLocal.clear();
	}

	SequencerData sequences[8];
	int currentSequenceIndex = 0;

	SequencerData &getCurrentSequence()
	{
		return sequences[currentSequenceIndex];
	}

	void setSelectedPrompt(juce::String newPrompt)
	{
		selectedPrompt = newPrompt;
	}

	const SequencerData &getCurrentSequence() const
	{
		return sequences[currentSequenceIndex];
	}
};

struct TrackData
{
	TrackData();
	~TrackData();

	TrackPage pages[4];

	juce::AudioSampleBuffer stagingBuffer;
	juce::AudioBuffer<float> originalStagingBuffer;

	Console6Channel consoleChannel;

	DelaySend delaySendProcessor;
	ReverbSend reverbSendProcessor;
	LowpassHighpassFilter lowpassHighpassFilter;
	Equalizer equalizer;
	Compressor compressor;

	juce::String trackId;
	juce::String trackName;
	juce::String style;
	juce::String currentSampleId;
	std::atomic<float> delaySend{0.0f};
	std::atomic<float> reverbSend{0.0f};

	std::atomic<bool> showWaveform{true};
	std::atomic<bool> isLoadingFromBank{false};
	std::atomic<bool> showSequencer{true};
	std::atomic<bool> isVersionSwitch{false};
	std::atomic<bool> preservedLoopLocked{false};
	std::atomic<bool> hasSamplePending{false};

	std::atomic<float> lastFeedbackDelaySend{-1.0f};

	int slotIndex = -1;

	enum class DeckSide
	{
		A,
		B
	};

	std::atomic<int> currentPageIndex{0};
	int midiNote = 60;
	int customStepCounter = 0;

	std::unique_ptr<TrackStretchImpl> stretchImpl;

	int stretchConfiguredChannels = 0;
	float stretchConfiguredSampleRate = 0.0f;
	juce::AudioBuffer<float> pitchInputBuffer;
	juce::AudioBuffer<float> pitchOutputBuffer;
	int lastRenderedPageIndex = -1;
	bool wasPitchActiveLastBlock = false;
	int pitchTransitionCounter = 0;
	bool pitchTransitionToActive = false;
	int pitchTransitionLength = 4096;

	float meterAccumPeakLeft{0.0f};
	float meterAccumPeakRight{0.0f};
	int meterSampleCounter{0};
	int meterUpdateInterval{2400};

	std::atomic<double> theoreticalPosition{0.0};

	double timeStretchRatio = 1.0;
	double preservedLoopStart = 0.0;
	double preservedLoopEnd = 4.0;
	double lastPpqPosition = -1.0;

	std::atomic<bool> skipBpmSync{false};
	std::atomic<bool> preprocessHasOriginal{false};
	std::atomic<float> preprocessOriginalBpm{126.0f};
	std::atomic<bool> isPlaying{false};
	std::atomic<bool> isArmed{false};
	std::atomic<bool> isArmedToStop{false};
	std::atomic<bool> isCurrentlyPlaying{false};
	std::atomic<bool> hasStagingData{false};
	std::atomic<bool> swapRequested{false};
	std::atomic<bool> isEnabled{true};
	std::atomic<bool> isSolo{false};
	std::atomic<bool> isMuted{false};
	std::atomic<bool> nextHasOriginalVersion{false};
	std::atomic<bool> randomRetriggerActive{false};
	std::atomic<bool> beatRepeatActive{false};
	std::atomic<bool> randomRetriggerEnabled{false};
	std::atomic<bool> beatRepeatPending{false};
	std::atomic<bool> beatRepeatStopPending{false};
	std::atomic<bool> randomRetriggerDurationEnabled{false};
	std::atomic<bool> pageChangePending{false};
	std::atomic<bool> lastFeedbackBeatRepeat{false};
	std::atomic<bool> isPreviewMode{false};
	std::atomic<bool> previewEndPending{false};
	std::atomic<bool> skipDiskReload{false};
	std::atomic<bool> isInGeneratingProcess{false};

	std::atomic<double> stagingSampleRate{Obsidian::SAMPLERATE};
	std::atomic<double> readPosition{0.0};
	std::atomic<double> beatRepeatStartPosition{0.0};
	std::atomic<double> beatRepeatEndPosition{0.0};
	std::atomic<double> beatRepeatDuration{0.25};
	std::atomic<double> originalReadPosition{0.0};
	std::atomic<double> lastRetriggerTime{-1.0};
	std::atomic<double> nextRetriggerTime{0.0};
	std::atomic<double> lastBeatTime{-1.0};

	std::atomic<float> lastFeedbackVolume{-1.0f};
	std::atomic<float> lastFeedbackPan{-999.0f};
	std::atomic<float> lastFeedbackPitch{-999.0f};
	std::atomic<float> lastFeedbackFine{-999.0f};
	std::atomic<int> brFadeInPending{0};
	std::atomic<int> stagingNumSamples{0};
	std::atomic<int> randomRetriggerInterval{Obsidian::RNDM_RTRGR_INTRVL};
	std::atomic<int> pendingPageIndex{-1};
	std::atomic<int> stagingTargetPageIndex{-1};

	std::atomic<float> volume{0.8f};
	std::atomic<float> pan{0.0f};
	std::atomic<float> audioLevelLeft{0.0f};
	std::atomic<float> audioLevelRight{0.0f};

	std::atomic<int64_t> pendingBeatNumber{-1};
	std::atomic<int64_t> pendingStopBeatNumber{-1};

	std::function<void(bool)> onPlayStateChanged;
	std::function<void(bool)> onArmedStateChanged;
	std::function<void(bool)> onArmedToStopStateChanged;
	std::function<void()> onPageChanged;

	enum class PendingAction
	{
		None,
		StartOnNextMeasure,
		StopOnNextMeasure
	};

	PendingAction pendingAction = PendingAction::None;

	SequencerData &getCurrentSequencerData()
	{
		return getCurrentPage().getCurrentSequence();
	}

	const SequencerData &getCurrentSequencerData() const
	{
		return getCurrentPage().getCurrentSequence();
	}

	TrackPage &getCurrentPage()
	{
		int idx = juce::jlimit(0, 3, currentPageIndex.load());
		return pages[idx];
	}

	const TrackPage &getCurrentPage() const
	{
		int idx = juce::jlimit(0, 3, currentPageIndex.load());
		return pages[idx];
	}

	DjIaClient::LoopRequest createLoopRequest() const
	{
		DjIaClient::LoopRequest request;
		const auto &currentPage = getCurrentPage();
		request.prompt =
		    !currentPage.selectedPrompt.isEmpty() ? currentPage.selectedPrompt : currentPage.generationPrompt;
		request.bpm = currentPage.generationBpm;
		request.key = currentPage.generationKey;
		request.generationDuration = static_cast<float>(currentPage.generationDuration);
		request.model = currentPage.selectedModel;
		return request;
	}

	void updateFromRequest(const DjIaClient::LoopRequest &request);
	void reset();
	void setPlaying(bool playing);
	void setArmed(bool armed);
	void setArmedToStop(bool armedToStop);
	void setStop();
	bool allSequencerStepsAreFalse() const;

	DeckSide getDeckSide() const;
	int getPairIndex() const;
	int getPartnerSlotIndex() const;
	bool isDeckA() const;
	bool isDeckB() const;

	void setCurrentPage(int pageIndex);

  private:
	JUCE_DECLARE_WEAK_REFERENCEABLE(TrackData)
};