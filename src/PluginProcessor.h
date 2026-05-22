#pragma once
#include "AudioManager.h"
#include "Console6Bus.h"
#include "DjIaClient.h"
#include "GenerationManager.h"
#include "MidiLearnManager.h"
#include "MidiManager.h"
#include "ObsidianEngine.h"
#include "ParameterManager.h"
#include "PromptBank.h"
#include "SampleBank.h"
#include "SequencerManager.h"
#include "StateManager.h"
#include "TrackManager.h"
#if JucePlugin_Build_Standalone
#include "StandaloneTransport.h"
#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#endif
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>

class DjIaVstEditor;

#if JucePlugin_Build_Standalone
class StandaloneTransport;
#endif

class DjIaVstProcessor : public juce::AudioProcessor,
                         public juce::AudioProcessorValueTreeState::Listener,
                         public juce::Timer,
                         public juce::AsyncUpdater
{
  public:
	struct GenerationListener
	{
		virtual ~GenerationListener() = default;
		virtual void onGenerationComplete(const juce::String &trackId, const juce::String &message) = 0;
	};

	DjIaVstProcessor();
	~DjIaVstProcessor() override;

	std::function<void()> onUIUpdateNeeded;
	std::function<void(double)> onHostBpmChanged;
	std::function<void(const float *l, const float *r, int n, double ppq)> onMasterOutput;
	std::function<void(const juce::String &)> midiIndicatorCallback;

	juce::ThreadPool threadPool{1};
	std::atomic<bool> heavyInitDone{false};
	std::atomic<bool> isShuttingDown{false};

#if JucePlugin_Build_Standalone
	bool getIsLinkActive() const
	{
		if (link)
		{
			return link->isEnabled();
		}
		return false;
	}
	void setLinkActive(bool active)
	{
		if (link)
		{
			link->enable(active);
			link->enableStartStopSync(active);
		}
	}
	void requestLinkStart();

	void requestLinkStop();

	void setLinkTempo(double bpm);

	void setLinkQuantum(double q);
#endif

	PromptBank *getPromptBank()
	{
		return promptBank.get();
	}

	TrackData *getTrackFromParamId(const juce::String &parameterID);

	TrackManager &getTrackManager()
	{
		return trackManager;
	}
	const TrackManager &getTrackManager() const
	{
		return trackManager;
	}

	StateManager &getStateManager()
	{
		return stateManager;
	}
	const StateManager &getStateManager() const
	{
		return stateManager;
	}

	GenerationManager &getGenerationManager()
	{
		return generationManager;
	}
	const GenerationManager &getGenerationManager() const
	{
		return generationManager;
	}

	ParameterManager &getParameterManager()
	{
		return parameterManager;
	}
	const ParameterManager &getParameterManager() const
	{
		return parameterManager;
	}

	SequencerManager &getSequencerManager()
	{
		return sequencerManager;
	}
	const SequencerManager &getSequencerManager() const
	{
		return sequencerManager;
	}

	AudioManager &getAudioManager()
	{
		return audioManager;
	}
	const AudioManager &getAudioManager() const
	{
		return audioManager;
	}

	MidiLearnManager &getMidiLearnManager()
	{
		return midiLearnManager;
	}
	const MidiLearnManager &getMidiLearnManager() const
	{
		return midiLearnManager;
	}

	MidiManager &getMidiManager()
	{
		return midiManager;
	}
	const MidiManager &getMidiManager() const
	{
		return midiManager;
	}

	DjIaClient &getApiClient()
	{
		return apiClient;
	}
	const DjIaClient &getApiClient() const
	{
		return apiClient;
	}

	SampleBank *getSampleBank()
	{
		return sampleBank.get();
	}

#if JucePlugin_Build_Standalone
	StandaloneTransport *getStandaloneTransport() const
	{
		return standaloneTransport.get();
	}
#endif

	TrackData *getTrack(const juce::String &trackId)
	{
		return trackManager.getTrack(trackId);
	}

	juce::AudioProcessorValueTreeState &getParameterTreeState()
	{
		return parameterManager.getAPVTS();
	}
	juce::AudioProcessorValueTreeState &getParameters()
	{
		return parameterManager.getAPVTS();
	}

	DjIaClient::LoopRequest createGlobalLoopRequest() const;

	const juce::String &getGlobalKey() const
	{
		return globalKey;
	}
	const juce::String &getGlobalPrompt() const
	{
		return globalPrompt;
	}
	const juce::String &getGeneratingTrackId() const
	{
		return generatingTrackId;
	}
	const juce::String &getServerUrl() const
	{
		return serverUrl;
	}
	const juce::String &getApiKey() const
	{
		return apiKey;
	}
	const juce::String &getLastKey() const
	{
		return lastKey;
	}
	const juce::String &getLastPrompt() const
	{
		return lastPrompt;
	}
	const juce::String &getProjectId() const
	{
		return projectId;
	}
	const juce::String &getPendingTrackId() const
	{
		return pendingTrackId;
	}
	const juce::String &getCurrentBankLoadTrackId() const
	{
		return currentBankLoadTrackId;
	}

	std::vector<juce::String> getAllTrackIds() const
	{
		return trackManager.getAllTrackIds();
	}

	const juce::File &getPendingAudioFile()
	{
		return pendingAudioFile;
	}

	float getGlobalBpm() const;
	float getPendingDetectedBpm() const
	{
		return pendingDetectedBpm.load();
	}
	float getPendingSnappedBpm() const
	{
		return pendingSnappedBpm.load();
	}
	float getGlobalCrossfaderValue() const;
	float getPairCrossfaderValue(int pairIndex) const;
	double getHostBpm() const;
	double getLastBpm() const
	{
		return lastBpm;
	}
	double getLastDuration() const
	{
		return lastDuration;
	}
	int getGlobalDuration() const
	{
		return globalDuration;
	}
	int getLastKeyIndex() const
	{
		return lastKeyIndex;
	}
	int getLastPresetIndex() const
	{
		return lastPresetIndex;
	}
	int getRequestTimeout() const
	{
		return requestTimeoutMS;
	}
	int getCrossfaderCurveMode() const;
	int creditsRemaining = 0;

	bool getOnboardingDone() const
	{
		return onboardingDone;
	}
	bool getUseLocalModel() const
	{
		return useLocalModel;
	}
	bool getIsGenerating() const
	{
		return isGenerating;
	}
	bool hasSampleWaiting() const
	{
		return hasUnloadedSample.load();
	}
	bool getAutoLoadEnabled() const
	{
		return autoLoadEnabled.load();
	}
	bool getBypassLLM() const
	{
		return bypassLLM.load();
	}
	bool getBypassSequencer() const
	{
		return sequencerManager.isBypassed();
	}
	bool isStateReady() const
	{
		return stateLoaded;
	}
	bool isHostBpmEnabled() const
	{
		return hostBpmEnabled;
	}
	bool getIsLoadingState() const
	{
		return isLoadingState.load();
	}
	bool getHasPendingAudioData() const
	{
		return hasPendingAudioData.load();
	}
	bool getWaitingForMidiToLoad() const
	{
		return waitingForMidiToLoad.load();
	}
	bool getCorrectMidiNoteReceived() const
	{
		return correctMidiNoteReceived.load();
	}
	bool getCanLoad() const
	{
		return canLoad.load();
	}
	bool getIsLoadingFromBank() const
	{
		return isLoadingFromBank.load();
	}

	bool canGenerateStandard = true;

	int getTimeSignatureNumerator() const
	{
		return timeSignatureNumerator.load();
	}
	int getTimeSignatureDenominator() const
	{
		return timeSignatureDenominator.load();
	}
	juce::StringArray getAvailablePromptsForModel(const juce::String &modelName = "") const;
	const juce::StringArray &getCustomKeywords() const
	{
		return customKeywords;
	}
	const juce::StringArray getCustomPrompts() const;
	const juce::StringArray &getFloatParamIds() const
	{
		return parameterManager.getFloatParamIds();
	}
	const juce::StringArray &getBooleanParamIds() const
	{
		return parameterManager.getBooleanParamIds();
	}

	juce::AudioProcessorEditor *createEditor() override;
	juce::AudioFormatManager sharedFormatManager;
	juce::ValueTree pendingMidiMappings;
	std::atomic<bool> needsUIUpdate{false};
	bool updateCheckDone = false;

	void setGlobalKey(const juce::String &key)
	{
		globalKey = key;
	}
	void setGlobalPrompt(const juce::String &prompt)
	{
		globalPrompt = prompt;
	}
	void setGlobalDuration(int duration)
	{
		globalDuration = duration;
	}
	void setGlobalBpm(float bpm)
	{
		globalBpm = bpm;
	}
	void setGeneratingTrackId(const juce::String &id)
	{
		generatingTrackId = id;
	}
	void setLastKey(const juce::String &key)
	{
		lastKey = key;
	}
	void setLastPrompt(const juce::String &p)
	{
		lastPrompt = p;
	}
	void setCurrentBankLoadTrackId(const juce::String &c)
	{
		currentBankLoadTrackId = c;
	}
	void clearCurrentBankLoadTrackId()
	{
		currentBankLoadTrackId.clear();
	}
	void setLastBpm(double bpm)
	{
		lastBpm = bpm;
	}
	void setLastDuration(double d)
	{
		lastDuration = d;
	}
	void setLastKeyIndex(int i)
	{
		lastKeyIndex = i;
	}
	void setLastPresetIndex(int i)
	{
		lastPresetIndex = i;
	}
	void setHostBpmEnabled(bool e)
	{
		hostBpmEnabled = e;
	}
	void setOnboardingDone(bool v)
	{
		onboardingDone = v;
	}
	void setUseLocalModel(bool v)
	{
		useLocalModel = v;
	}
	void setIsGenerating(bool v)
	{
		isGenerating = v;
	}
	void setStateReady(bool v)
	{
		stateLoaded = v;
	}
	void setIsLoadingState(bool v)
	{
		isLoadingState.store(v);
	}
	void setCreditsRemaining(int v)
	{
		creditsRemaining = v;
	}
	void setRequestTimeout(int ms)
	{
		requestTimeoutMS = ms;
	}
	void setApiKey(const juce::String &key);
	void setServerUrl(const juce::String &url);
	void setAutoLoadEnabled(bool enabled);
	void setBypassLLM(bool bypassed);
	void setCrossfaderCurveMode(int mode);
	void setBypassSequencer(bool bypass)
	{
		sequencerManager.setBypass(bypass);
	}
	void setProjectId(const juce::String &id)
	{
		projectId = id;
	}
	void setWindowSize(int w, int h)
	{
		savedWindowWidth = w;
		savedWindowHeight = h;
	}
	void setPanelVisible(bool v)
	{
		savedPanelVisible = v;
	}
	void setGenerationListener(GenerationListener *l)
	{
		generationListener = l;
	}
	void setMidiIndicatorCallback(std::function<void(const juce::String &)> cb)
	{
		midiIndicatorCallback = cb;
	}
	void setHasUnloadedSample(bool v)
	{
		hasUnloadedSample.store(v);
	}
	void setCanLoad(bool v)
	{
		canLoad.store(v);
	}
	void setIsLoadingFromBank(bool v)
	{
		isLoadingFromBank.store(v);
	}

	int getSavedWindowWidth() const
	{
		return savedWindowWidth;
	}
	int getSavedWindowHeight() const
	{
		return savedWindowHeight;
	}
	bool getSavedPanelVisible() const
	{
		return savedPanelVisible;
	}
	juce::String getPanelStateJson() const
	{
		return panelStateJson;
	}
	void setPanelStateJson(const juce::String &json)
	{
		panelStateJson = json;
		saveGlobalConfig();
	}
	void setPendingTrackId(const juce::String &id)
	{
		pendingTrackId = id;
	}
	void setPendingAudioFile(const juce::File &f)
	{
		pendingAudioFile = f;
	}
	void setHasPendingAudioData(bool v)
	{
		hasPendingAudioData.store(v);
	}
	void setWaitingForMidiToLoad(bool v)
	{
		waitingForMidiToLoad.store(v);
	}
	void setTrackIdWaitingForLoad(const juce::String &id)
	{
		trackIdWaitingForLoad = id;
	}
	void setCorrectMidiNoteReceived(bool v)
	{
		correctMidiNoteReceived.store(v);
	}
	void clearTrackIdWaitingForLoad()
	{
		trackIdWaitingForLoad.clear();
	}
	void clearPendingTrackId()
	{
		pendingTrackId.clear();
	}
	void setPendingDetectedBpm(float v)
	{
		pendingDetectedBpm.store(v);
	}
	void setPendingSnappedBpm(float v)
	{
		pendingSnappedBpm.store(v);
	}
	void setLastGeneratedTrackId(const juce::String &id)
	{
		lastGeneratedTrackId = id;
	}
	void setPendingMessage(const juce::String &msg)
	{
		pendingMessage = msg;
	}
	void setHasPendingNotification(bool v)
	{
		hasPendingNotification = v;
	}

	double getLastHostBpmForQuantization() const
	{
		return lastHostBpmForQuantization.load();
	}

	void addPlayingTrack(int note, const juce::String &trackId)
	{
		playingTracks[note] = trackId;
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

	void initTracks();
	void cleanProcessor();
	void startNotePlaybackForTrack(const juce::String &trackId, int noteNumber, double hostBpm = 126.0);
	void previewTrack(const juce::String &trackId);
	void loadPendingSample();
	void reloadTrackWithVersion(const juce::String &trackId, bool useOriginal);
	void addCustomPrompt(const juce::String &prompt);
	void removeCustomPrompt(const juce::String &prompt);
	void editCustomPrompt(const juce::String &oldPrompt, const juce::String &newPrompt);
	void loadGlobalConfig();
	void saveGlobalConfig();
	void attachPageChangeCallback(TrackData *track);
	double getHostSampleRate()
	{
		return hostSampleRate;
	}
	double getCachedHostBpm()
	{
		return cachedHostBpm.load();
	}

	const juce::String getName() const override
	{
		return "OBSIDIAN-Neural";
	}
	const juce::String getProgramName(int) override
	{
		return {};
	}
	bool acceptsMidi() const override
	{
		return true;
	}
	bool producesMidi() const override
	{
		return false;
	}
	bool isMidiEffect() const override
	{
		return false;
	}
	bool hasEditor() const override
	{
		return true;
	}
	double getTailLengthSeconds() const override
	{
		return 0.0;
	}
	int getNumPrograms() override
	{
		return 1;
	}
	int getCurrentProgram() override
	{
		return 0;
	}
	void setCurrentProgram(int) override
	{
	}
	void changeProgramName(int, const juce::String &) override
	{
	}
	bool isBusesLayoutSupported(const BusesLayout &) const override;
	void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
	void getStateInformation(juce::MemoryBlock &destData) override;
	void setStateInformation(const void *data, int sizeInBytes) override;
	void parameterChanged(const juce::String &parameterID, float newValue) override;
	void timerCallback() override;
	void playTrack(const juce::MidiMessage &message, double hostBpm);
	void stopNotePlaybackForTrack(int noteNumber);

	void setPairCrossfaderPrevious(int index, float value)
	{
		if (index >= 0 && index < Obsidian::MAX_CROSSFADER_PAIR)
			pairCrossfaderPrevious[index] = value;
	}
	void setGlobalCrossfaderPrevious(float value)
	{
		globalCrossfaderPrevious = value;
	}

	void setCrossfaderValue(float v)
	{
		crossfaderValue.store(juce::jlimit(0.0f, 1.0f, v));
	}
	float getCrossfaderValue() const
	{
		return crossfaderValue.load();
	}
	void setCrossfadeMode(int m)
	{
		crossfadeMode.store(m);
	}
	int getCrossfadeMode() const
	{
		return crossfadeMode.load();
	}

	void setCustomKeywords(const juce::StringArray &keywords)
	{
		customKeywords = keywords;
		saveGlobalConfig();
	}

	void addCustomKeyword(const juce::String &keyword)
	{
		if (!customKeywords.contains(keyword))
		{
			customKeywords.add(keyword);
			saveGlobalConfig();
		}
	}

  private:
	DjIaVstEditor *currentEditor = nullptr;
	MidiLearnManager midiLearnManager;
	DjIaClient apiClient;
	GenerationListener *generationListener = nullptr;
#if JucePlugin_Build_Standalone
	std::unique_ptr<StandaloneTransport> standaloneTransport;
#endif
	TrackManager trackManager;
	MidiManager midiManager;
	StateManager stateManager;
	GenerationManager generationManager;
	ParameterManager parameterManager;
	SequencerManager sequencerManager;
	AudioManager audioManager;
	Console6Buss masterConsoleBuss;

	std::unique_ptr<SampleBank> sampleBank;
	std::unique_ptr<ObsidianEngine> obsidianEngine;
	std::unique_ptr<PromptBank> promptBank;

	juce::String panelStateJson;

	juce::String globalPrompt;
	juce::String globalKey = "C Minor";
	juce::String serverUrl;
	juce::String apiKey;
	juce::String lastPrompt;
	juce::String lastKey = "C Minor";
	juce::String generatingTrackId;
	juce::String projectId;
	juce::String pendingTrackId;
	juce::String trackIdWaitingForLoad;
	juce::String lastGeneratedTrackId;
	juce::String pendingMessage;
	juce::String currentBankLoadTrackId;
	juce::String localModelsPath = "";

	float globalBpm = 110.0f;
	float pairCrossfaderPrevious[4]{0.5f, 0.5f, 0.5f, 0.5f};
	float globalCrossfaderPrevious = 0.5f;
	int globalDuration = 6;
	double lastBpm = 126.0;
	double lastDuration = 6.0;
	double hostSampleRate = 44100.0;
	int lastKeyIndex = 1;
	int lastPresetIndex = -1;
	int requestTimeoutMS = 360000;
	int savedWindowWidth = 1620;
	int savedWindowHeight = 840;

	enum class CrossfadeMode
	{
		Linear,
		EqualPower,
		DJCurve
	};
	std::atomic<float> crossfaderValue{0.5f};
	std::atomic<int> crossfadeMode{0};

	bool hostBpmEnabled = true;
	bool useLocalModel = false;
	bool isGenerating = false;
	bool onboardingDone = false;
	bool savedPanelVisible = true;
	bool hasPendingNotification = false;

	juce::StringArray customKeywords;
	juce::StringArray customPrompts;

	std::atomic<int> timeSignatureNumerator{4};
	std::atomic<int> timeSignatureDenominator{4};
	std::atomic<bool> isLoadingState{false};
	std::atomic<bool> hasPendingAudioData{false};
	std::atomic<bool> autoLoadEnabled{false};
	std::atomic<bool> hasUnloadedSample{false};
	std::atomic<bool> waitingForMidiToLoad{false};
	std::atomic<bool> correctMidiNoteReceived{false};
	std::atomic<bool> stateLoaded{false};
	std::atomic<bool> bypassLLM{true};
	std::atomic<bool> isLoadingFromBank{false};
	std::atomic<float> pendingDetectedBpm{-1.0f};
	std::atomic<float> pendingSnappedBpm{-1.0f};

	juce::File pendingAudioFile;

	juce::MidiBuffer feedbackMidiBuffer;
	juce::CriticalSection feedbackMidiLock;

	struct PendingRequest
	{
		int trackId;
		ObsidianEngine::LoopRequest request;
		juce::Time requestTime;
	};
	std::queue<PendingRequest> pendingRequests;
	std::mutex requestsMutex;

	static juce::AudioProcessor::BusesProperties createBusLayout();
	void handleAsyncUpdate() override;
	void checkIfUIUpdateNeeded(juce::MidiBuffer &midiMessages);
	void getDawInformations(juce::AudioPlayHead *playHead, bool &isPlaying, double &bpm, double &ppq);

	std::atomic<float> lastFeedbackDelayFeedback{0.0f};
	std::atomic<float> lastFeedbackReverbSize{0.0f};
	std::atomic<float> lastFeedbackReverbDamping{0.0f};
	std::atomic<float> lastFeedbackReverbWidth{0.0f};
	std::atomic<float> lastFeedbackReverbMix{0.0f};
	std::atomic<int> lastFeedbackDelayDivision{-1};
	std::atomic<int> lastFeedbackDelayMode{-1};

	static juce::File getGlobalConfigFile();

	std::unordered_map<int, juce::String> playingTracks;
	std::atomic<int> currentNoteNumber{-1};
	std::atomic<bool> isNotePlaying{false};
	std::atomic<bool> canLoad{false};
	std::atomic<double> lastHostBpmForQuantization{120.0};
	std::atomic<double> cachedHostBpm{126.0};
	std::uint64_t sample_time = 0;

#if JucePlugin_Build_Standalone
	struct EngineData
	{
		double requested_bpm;
		bool request_start;
		bool request_stop;
		double quantum = 4.0;
		bool startstop_sync;
		JUCE_LEAK_DETECTOR(EngineData)
	};

	EngineData shared_engine_data, lock_free_engine_data;
	std::mutex engine_data_guard;
	std::unique_ptr<ableton::Link> link;
	ableton::link::HostTimeFilter<ableton::link::platform::Clock> host_time_filter;
	std::unique_ptr<ableton::Link::SessionState> session;
	std::chrono::microseconds output_time;

	std::atomic<double> currentBpm{120.0};
	std::atomic<bool> isLinkActive{false};
	std::atomic<bool> isEnableStartStopSync{true};
	std::atomic<bool> isLinkPlaying{false};

	void calculateOutputTime(const double sample_rate, const int buffer_size);
	ableton::Link::SessionState processSessionState(const EngineData &engine_data);
	EngineData pull_engine_data();
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DjIaVstProcessor)
};