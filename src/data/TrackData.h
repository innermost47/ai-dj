#pragma once
#include "Console6Channel.h"
#include "DjIaClient.h"
#include <JuceHeader.h>

struct SequencerData
{
	bool steps[4][16] = {};
	float velocities[4][16] = {};
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

	double sampleRate = 48000.0;
	double loopStart = 0.0;
	double loopEnd = 4.0;

	std::atomic<float> fineOffset{0.0f};
	std::atomic<double> bpmOffset{0.0};
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
		bpmOffset.store(other.bpmOffset.load());
	}

	void reset()
	{
		audioBuffer.setSize(0, 0);
		audioFilePath.clear();
		numSamples = 0;
		sampleRate = 48000.0;
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
		bpmOffset.store(0.0);
		savedModelBeforeLocal.clear();
	}

	SequencerData sequences[8];
	int currentSequenceIndex = 0;

	SequencerData &getCurrentSequence()
	{
		return sequences[currentSequenceIndex];
	}

	const SequencerData &getCurrentSequence() const
	{
		return sequences[currentSequenceIndex];
	}
};

struct TrackData
{
	TrackPage pages[4];

	juce::AudioSampleBuffer stagingBuffer;
	juce::AudioBuffer<float> originalStagingBuffer;

	Console6Channel consoleChannel;

	juce::String trackId;
	juce::String trackName;
	juce::String style;
	juce::String currentSampleId;
	std::atomic<float> delaySend{0.0f};
	std::atomic<float> reverbSend{0.0f};

	std::atomic<bool> showWaveform{true};
	std::atomic<bool> showSequencer{true};
	std::atomic<bool> isVersionSwitch{false};
	std::atomic<bool> preservedLoopLocked{false};

	std::atomic<float> lastFeedbackDelaySend{-1.0f};

	int slotIndex = -1;

	enum class DeckSide
	{
		A,
		B
	};

	DeckSide getDeckSide() const
	{
		return (slotIndex >= 0 && slotIndex < 4) ? DeckSide::A : DeckSide::B;
	}

	int getPairIndex() const
	{
		if (slotIndex < 0 || slotIndex >= 8)
			return -1;
		return slotIndex % 4;
	}

	int getPartnerSlotIndex() const
	{
		if (slotIndex < 0 || slotIndex >= 8)
			return -1;
		return (slotIndex < 4) ? slotIndex + 4 : slotIndex - 4;
	}

	bool isDeckA() const
	{
		return getDeckSide() == DeckSide::A;
	}

	bool isDeckB() const
	{
		return getDeckSide() == DeckSide::B;
	}

	std::atomic<int> currentPageIndex{0};
	int timeStretchMode = 4;
	int midiNote = 60;
	int customStepCounter = 0;

	double timeStretchRatio = 1.0;
	double preservedLoopStart = 0.0;
	double preservedLoopEnd = 4.0;
	double lastPpqPosition = -1.0;

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

	std::atomic<double> cachedPlaybackRatio{1.0};
	std::atomic<double> stagingSampleRate{48000.0};
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

	std::atomic<int> stagingNumSamples{0};
	std::atomic<int> randomRetriggerInterval{3};
	std::atomic<int> pendingPageIndex{-1};
	std::atomic<int> stagingTargetPageIndex{-1};

	std::atomic<float> volume{0.8f};
	std::atomic<float> pan{0.0f};

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

	TrackData() : trackId(juce::Uuid().toString()), readPosition(0.0), onPlayStateChanged(nullptr)
	{
		for (int i = 0; i < 4; ++i)
			pages[i].reset();
	}

	~TrackData()
	{
		onPlayStateChanged = nullptr;
		onArmedStateChanged = nullptr;
		onArmedToStopStateChanged = nullptr;
		onPageChanged = nullptr;
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

	void setCurrentPage(int pageIndex)
	{
		if (pageIndex < 0 || pageIndex >= 4)
			return;
		if (currentPageIndex.load() == pageIndex)
			return;

		DBG("[setCurrentPage] slot=" << slotIndex << " from page " << currentPageIndex.load() << " to page "
		                             << pageIndex << " | new page bpmOffset=" << pages[pageIndex].bpmOffset.load()
		                             << " fineOffset=" << pages[pageIndex].fineOffset.load());

		currentPageIndex.store(pageIndex);

		if (onPageChanged)
			onPageChanged();
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

	void updateFromRequest(const DjIaClient::LoopRequest &request)
	{
		auto &currentPage = getCurrentPage();
		currentPage.generationPrompt = request.prompt;
		currentPage.generationBpm = request.bpm;
		currentPage.generationKey = request.key;
		currentPage.generationDuration = static_cast<int>(request.generationDuration);
		currentPage.selectedModel = request.model;
	}

	void reset()
	{
		for (int i = 0; i < 4; ++i)
			pages[i].reset();

		currentPageIndex.store(0);

		readPosition = 0.0;
		isEnabled = true;
		isMuted = false;
		isSolo = false;
		volume = 0.8f;
		pan = 0.0f;
		isVersionSwitch = false;
		preservedLoopStart = 0.0;
		preservedLoopEnd = 4.0;
		preservedLoopLocked = false;
	}

	void setPlaying(bool playing)
	{
		bool wasPlaying = isPlaying.load();
		isPlaying = playing;
		if (wasPlaying != playing && onPlayStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 &&
		    isPlaying.load())
		{
			juce::WeakReference<TrackData> weakThis(this);
			juce::MessageManager::callAsync(
			    [weakThis, playing]()
			    {
				    if (auto *self = weakThis.get())
					    if (self->onPlayStateChanged)
						    self->onPlayStateChanged(playing);
			    });
		}
	}

	void setArmed(bool armed)
	{
		bool wasArmed = isArmed.load();
		isArmed = armed;
		if (wasArmed != armed && onArmedStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 &&
		    isPlaying.load())
		{
			juce::WeakReference<TrackData> weakThis(this);
			juce::MessageManager::callAsync(
			    [weakThis, armed]()
			    {
				    if (auto *self = weakThis.get())
					    if (self->onArmedStateChanged)
						    self->onArmedStateChanged(armed);
			    });
		}
	}

	void setArmedToStop(bool armedToStop)
	{
		isArmedToStop = armedToStop;
		if (onArmedToStopStateChanged && getCurrentPage().audioBuffer.getNumChannels() > 0 && isCurrentlyPlaying.load())
		{
			juce::WeakReference<TrackData> weakThis(this);
			juce::MessageManager::callAsync(
			    [weakThis, armedToStop]()
			    {
				    if (auto *self = weakThis.get())
					    if (self->onArmedToStopStateChanged)
						    self->onArmedToStopStateChanged(armedToStop);
			    });
		}
	}

	void setStop()
	{
		juce::WeakReference<TrackData> weakThis(this);
		juce::MessageManager::callAsync(
		    [weakThis]()
		    {
			    if (auto *self = weakThis.get())
				    if (self->onPlayStateChanged)
					    self->onPlayStateChanged(false);
		    });
	}

  private:
	JUCE_DECLARE_WEAK_REFERENCEABLE(TrackData)
};