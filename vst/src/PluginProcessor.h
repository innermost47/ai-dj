#pragma once
#include "JuceHeader.h"
#include "core/TrackManager.h"
#include "core/StateManager.h"
#include "core/GenerationManager.h"
#include "engines/DjIaClient.h"
#include "midi/MidiLearnManager.h"
#include "engines/ObsidianEngine.h"
#include "dsp/SimpleEQ.h"
#include "components/bank/SampleBank.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <atomic>

class DjIaVstEditor;
class TrackComponent;
class StateManager;

class DjIaVstProcessor : public juce::AudioProcessor,
	public juce::AudioProcessorValueTreeState::Listener,
	public juce::Timer,
	public juce::AsyncUpdater
{
public:
	struct GenerationListener
	{
		virtual ~GenerationListener() = default;
		virtual void onGenerationComplete(const juce::String& trackId, const juce::String& message) = 0;
	};

	DjIaVstProcessor();
	~DjIaVstProcessor() override;

	std::function<void()> onUIUpdateNeeded;
	std::function<void(double)> onHostBpmChanged = nullptr;
	std::function<void(const float* l, const float* r, int n, double ppqPosition)> onMasterOutput;
	std::atomic<bool> isShuttingDown{ false };

	juce::AudioProcessorEditor* createEditor() override;
	juce::AudioFormatManager sharedFormatManager;

	TrackManager trackManager;
	StateManager stateManager;
	GenerationManager generationManager;

	MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }

	DjIaClient& getApiClient() { return apiClient; }

	SampleBank* getSampleBank() { return sampleBank.get(); }

	TrackData* getTrack(const juce::String& trackId) { return trackManager.getTrack(trackId); }

	const DjIaClient& getApiClient() const { return apiClient; }

	DjIaClient::LoopRequest createGlobalLoopRequest() const
	{
		DjIaClient::LoopRequest request;
		request.prompt = globalPrompt;
		request.bpm = globalBpm;
		request.key = globalKey;
		request.generationDuration = static_cast<float>(globalDuration);
		return request;
	}

	juce::ValueTree pendingMidiMappings;

	juce::AudioProcessorValueTreeState& getParameterTreeState() { return parameters; }
	juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

	std::atomic<bool> needsUIUpdate{ false };
	bool updateCheckDone = false;

	const juce::String& getGlobalKey() const { return globalKey; }
	const juce::String& getGlobalPrompt() const { return globalPrompt; }
	const juce::String& getSelectedTrackId() const { return selectedTrackId; }
	const juce::String& getGeneratingTrackId() const { return generatingTrackId; }
	const juce::String& getServerUrl() const { return serverUrl; }
	const juce::String& getApiKey() const { return apiKey; }

	const juce::StringArray& getBuiltInPrompts() const;
	const juce::StringArray& getCustomKeywords() const { return customKeywords; }
	const juce::StringArray& getCustomPrompts() const;

	const juce::String getName() const override { return "OBSIDIAN-Neural"; }
	const juce::String getProgramName(int) override { return {}; }

	std::vector<juce::String> getAllTrackIds() const { return trackManager.getAllTrackIds(); }

	juce::File getExportDirectory();
	juce::File exportSampleForDragDrop(const juce::File& originalFile);

	void timerCallback() override;
	void setGenerationListener(GenerationListener* listener) { generationListener = listener; }
	void initDummySynth();
	void initTracks();
	void loadParameters();
	void cleanProcessor();
	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void releaseResources() override;
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	void checkIfUIUpdateNeeded(juce::MidiBuffer& midiMessages);
	void applyMasterEffects(juce::AudioSampleBuffer& mainOutput);
	void copyTracksToIndividualOutputs(juce::AudioSampleBuffer& buffer);
	void clearOutputBuffers(juce::AudioSampleBuffer& buffer);
	void resizeIndividualsBuffers(juce::AudioSampleBuffer& buffer);
	void getDawInformations(juce::AudioPlayHead* currentPlayHead, bool& hostIsPlaying, double& hostBpm, double& hostPpqPosition);
	void setLastDuration(double duration) { lastDuration = duration; }
	void setLastKeyIndex(int index) { lastKeyIndex = index; }
	void setCurrentProgram(int) override {}
	void changeProgramName(int, const juce::String&) override {}
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	void selectTrack(const juce::String& trackId);
	void reorderTracks(const juce::String& fromTrackId, const juce::String& toTrackId);
	void generateLoop(const DjIaClient::LoopRequest& request, const juce::String& targetTrackId = "");
	void startNotePlaybackForTrack(const juce::String& trackId, int noteNumber, double hostBpm = 126.0);
	void setApiKey(const juce::String& key);
	void setServerUrl(const juce::String& url);
	void setLastPrompt(const juce::String& prompt) { lastPrompt = prompt; }
	void setLastPresetIndex(int index) { lastPresetIndex = index; }
	void setAutoLoadEnabled(bool enabled);
	void setBypassLLM(bool bypassed);
	void setGeneratingTrackId(const juce::String& trackId) { generatingTrackId = trackId; }
	void handleSampleParams(int slot, TrackData* track);
	void loadGlobalConfig();
	void saveGlobalConfig();
	void removeCustomPrompt(const juce::String& prompt);
	void editCustomPrompt(const juce::String& oldPrompt, const juce::String& newPrompt);
	void handleSequencerPlayState(bool hostIsPlaying);
	void addSequencerMidiMessage(const juce::MidiMessage& message);
	void setRequestTimeout(int requestTimeoutMS);
	void setGlobalKey(const juce::String& key) { globalKey = key; }
	void setGlobalPrompt(const juce::String& prompt) { globalPrompt = prompt; }
	void setGlobalDuration(int duration) { globalDuration = duration; }
	void previewTrack(const juce::String& trackId);
	void setUseLocalModel(bool useLocal) { useLocalModel = useLocal; }
	void loadSampleFromBank(const juce::String& sampleId, const juce::String& trackId);
	void loadAudioFileAsync(const juce::String& trackId, const juce::File& audioData);
	void stopSamplePreview();
	void generateSampleWithImage(const juce::String& trackId, const juce::String& base64Image, const juce::StringArray& keywords);
	void generateLoopWithImage(const DjIaClient::LoopRequest& request, const juce::String& trackId, int timeoutMS);
	void setBypassSequencer(bool bypass) { bypassSequencer.store(bypass); }
	void selectNextTrack();
	void selectPreviousTrack();
	void triggerGlobalGeneration();
	void syncSelectedTrackWithGlobalPrompt();
	void setCreditsRemaining(int credits) { creditsRemaining = credits; }
	void reloadTrackWithVersion(const juce::String& trackId, bool useOriginal);
	void setIsGenerating(bool generating) { isGenerating = generating; }
	void addCustomPrompt(const juce::String& prompt);
	void loadPendingSample();
	void stopTrackPreview(const juce::String& trackId);
	void addCustomKeyword(const juce::String& keyword)
	{
		if (!customKeywords.contains(keyword))
		{
			customKeywords.add(keyword);
			saveGlobalConfig();
		}
	}
	void setCustomKeywords(const juce::StringArray& keywords)
	{
		customKeywords = keywords;
		saveGlobalConfig();
	}
	void setMidiIndicatorCallback(std::function<void(const juce::String&)> callback)
	{
		midiIndicatorCallback = callback;
	}

	float getGlobalBpm() const
	{
		float hostBpm = static_cast<float>(getHostBpm());
		return hostBpm > 0 ? hostBpm : globalBpm;
	}
	double getTailLengthSeconds() const override { return 0.0; }
	double getHostBpm() const;
	double calculateRetriggerInterval(int intervalValue, double hostBpm) const;

	bool getOnboardingDone() const { return onboardingDone; }
	void setOnboardingDone(bool v) { onboardingDone = v; }

	bool getUseLocalModel() const { return useLocalModel; }
	bool getIsGenerating() const { return isGenerating; }
	bool hasSampleWaiting() const { return hasUnloadedSample.load(); }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	bool hasEditor() const override { return true; }
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	bool getBypassSequencer() const { return bypassSequencer.load(); }
	bool isSamplePreviewing() const { return isPreviewPlaying.load(); }
	bool previewSampleFromBank(const juce::String& sampleId);
	bool isStateReady() const { return stateLoaded; }
	void setStateReady(bool ready) { stateLoaded = ready; }
	bool getAutoLoadEnabled() const { return autoLoadEnabled.load(); }
	bool getBypassLLM() const { return bypassLLM.load(); }

	bool canGenerateStandard = true;

	int getRequestTimeout() const { return requestTimeoutMS; };
	int getGlobalDuration() const { return globalDuration; }
	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	int getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
	int getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }
	int getLastPresetIndex() const { return lastPresetIndex; }

	int creditsRemaining = 0;

	juce::StringArray promptPresets = {
		"Acidic 303 bassline",
		"Ambient flute psychedelic",
		"Dark atmospheric pad",
		"Deep rolling bass",
		"Distorted noise chops",
		"Drum and bass rhythm",
		"Dub kick rhythm",
		"Glitchy percussion loop",
		"Hardcore kick pattern",
		"Industrial noise texture",
		"Techno kick rhythm",
		"Vintage analog lead",
	};

	void  setCrossfaderValue(float v) { crossfaderValue.store(juce::jlimit(0.0f, 1.0f, v)); }
	float getCrossfaderValue()  const { return crossfaderValue.load(); }
	void  setCrossfadeMode(int m) { crossfadeMode.store(m); }
	int   getCrossfadeMode()    const { return crossfadeMode.load(); }

	void setWindowSize(int w, int h) { savedWindowWidth = w; savedWindowHeight = h; }
	void setBankVisible(bool visible) { savedBankVisible = visible; }
	int getSavedWindowWidth() const { return savedWindowWidth; }
	int getSavedWindowHeight() const { return savedWindowHeight; }
	bool getSavedBankVisible() const { return savedBankVisible; }

	void sendMidiFeedback(int cc, int value);
	void sendMidiFeedback(int cc, int value, int channel);

	void setPairCrossfaderValue(int pairIndex, float value);
	float getPairCrossfaderValue(int pairIndex) const;
	void setGlobalCrossfaderValue(float value);
	float getGlobalCrossfaderValue() const;
	void setCrossfaderCurveMode(int mode);
	int getCrossfaderCurveMode() const;

	bool getIsLoadingState() const {
		return isLoadingState.load();
	}

	void setIsLoadingState(bool isLoading) {
		isLoadingState.store(isLoading);
	}

	float getPeakLevelLeft() const { return peakLevelLeft.load(); }
	float getPeakLevelRight() const { return peakLevelRight.load(); }
	void setPeakLevels(float left, float right) { peakLevelLeft.store(left); peakLevelRight.store(right); }

	float getAttackParam(int index) const {
		if (index >= 0 && index < 8 && slotAdsrAttackParams[index] != nullptr) {
			return slotAdsrAttackParams[index]->load();
		}
		return 0.0f;
	}

	float getDecayParam(int index) const {
		if (index >= 0 && index < 8 && slotAdsrDecayParams[index] != nullptr) {
			return slotAdsrDecayParams[index]->load();
		}
		return 0.0f;
	}

	float getSustainParam(int index) const {
		if (index >= 0 && index < 8 && slotAdsrSustainParams[index] != nullptr) {
			return slotAdsrSustainParams[index]->load();
		}
		return 0.0f;
	}

	float getReleaseParam(int index) const {
		if (index >= 0 && index < 8 && slotAdsrReleaseParams[index] != nullptr) {
			return slotAdsrReleaseParams[index]->load();
		}
		return 0.0f;
	}

	float getPitchParam(int index) const {
		if (index >= 0 && index < 8 && slotPitchParams[index] != nullptr) {
			return slotPitchParams[index]->load();
		}
		return 0.0f;
	}

	float getFineParam(int index) const {
		if (index >= 0 && index < 8 && slotFineParams[index] != nullptr) {
			return slotFineParams[index]->load();
		}
		return 0.0f;
	}

	float getVolumeParam(int index) const {
		if (index >= 0 && index < 8 && slotVolumeParams[index] != nullptr) {
			return slotVolumeParams[index]->load();
		}
		return 0.0f;
	}

	void setPairCrossfaderPrevious(int index, float value) {
		if (index >= 0 && index < 4) {
			pairCrossfaderPrevious[index] = value;
		}
	}

	float getPairCrossfaderParam(int index) const {
		if (index >= 0 && index < 4) {
			return pairCrossfaderParams[index]->load();
		}
		return 0.5f;
	}

	float getSlotGenerateParam(int index) const {
		if (index >= 0 && index < 8) {
			return slotGenerateParams[index]->load();
		}
		return 0.0f;
	}

	void setGlobalCrossfaderPrevious(float value) {
		globalCrossfaderPrevious = value;
	}

	float getGlobalCrossfaderParam() const {
		if (globalCrossfaderParam != nullptr) {
			return globalCrossfaderParam->load();
		}
		return 0.5f;
	}

	void setSelectedTrackId(const juce::String& value) {
		selectedTrackId = value;
	}

	void setGlobalBpm(float bpm) {
		globalBpm = bpm;
	}

	void setMigrationCompleted(bool completed) { migrationCompleted = completed; }

	void setProjectId(const juce::String& id) { projectId = id; }

	const juce::String& getProjectId() const { return projectId; }
	const juce::StringArray& getFloatParamIds() const { return floatParamIds; }
	const juce::StringArray& getBooleanParamIds() const { return booleanParamIds; }
	const juce::String& getLastKey() const { return lastKey; }
	const juce::String& getLastPrompt() const {
		return lastPrompt;
	}

	void setLastKey(const juce::String& key) { lastKey = key; }
	void setLastBpm(double bpm) { lastBpm = bpm; }
	void setHostBpmEnabled(bool enabled) { hostBpmEnabled = enabled; }

	double getLastBpm() const { return lastBpm; }
	bool isHostBpmEnabled() const { return hostBpmEnabled; }
	double getLastDuration() const { return lastDuration; }
	int getLastKeyIndex() const { return lastKeyIndex; }

	void performMigrationIfNeeded();

	void setPendingTrackId(const juce::String& trackId) {
		pendingTrackId = trackId;
	}

	void setPendingAudioFile(const juce::File& audioFile) {
		pendingAudioFile = audioFile;
	}

	void setHasPendingAudioData(bool value) {
		hasPendingAudioData.store(value);
	}

	void setWaitingForMidiToLoad(bool value) {
		waitingForMidiToLoad.store(value);
	}

	void setTrackIdWaitingForLoad(const juce::String& trackId) {
		trackIdWaitingForLoad = trackId;
	}

	void setCorrectMidiNoteReceived(bool value) {
		correctMidiNoteReceived.store(value);
	}

	void clearTrackIdWaitingForLoad() {
		trackIdWaitingForLoad.clear();
	}

	void clearPendingTrackId() {
		pendingTrackId.clear();
	}

	double getHostSampleRate() {
		return hostSampleRate;
	}

	void setPendingDetectedBpm(bool value) {
		pendingDetectedBpm.store(value);
	}

	void setLastGeneratedTrackId(const juce::String& trackId) {
		lastGeneratedTrackId = trackId;
	}

	void setPendingMessage(const juce::String& message) {
		pendingMessage = message;
	}

	void setHasPendingNotification(bool value) {
		hasPendingNotification = value;
	}

private:
	DjIaVstEditor* currentEditor = nullptr;
	SimpleEQ masterEQ;
	MidiLearnManager midiLearnManager;
	enum class CrossfadeMode { Linear, EqualPower, DJCurve };
	std::atomic<float> crossfaderValue{ 0.5f };
	std::atomic<int>   crossfadeMode{ 0 };

	std::atomic<float> peakLevelLeft{ 0.0f }, peakLevelRight{ 0.0f };
	float pairCrossfaderPrevious[4]{ 0.5f, 0.5f, 0.5f, 0.5f };
	float globalCrossfaderPrevious = 0.5f;
	int savedWindowWidth = 1620;
	int savedWindowHeight = 840;
	bool savedBankVisible = true;
	DjIaClient apiClient;
	GenerationListener* generationListener = nullptr;
	juce::String projectId;
	bool migrationCompleted = false;
	std::unique_ptr<SampleBank> sampleBank;
	juce::StringArray customKeywords;
	juce::MidiBuffer feedbackMidiBuffer;
	juce::CriticalSection feedbackMidiLock;

	std::atomic<bool> isLoadingState{ false };

	std::atomic<float>* nextTrackParam = nullptr;
	std::atomic<float>* prevTrackParam = nullptr;
	std::atomic<int64_t> internalSampleCounter{ 0 };
	std::atomic<double> lastHostBpmForQuantization{ 120.0 };

	std::atomic<bool> isPreviewPlaying{ false };
	std::atomic<bool> bypassLLM{ false };
	juce::AudioBuffer<float> previewBuffer;
	std::atomic<double> previewPosition{ 0.0 };
	std::atomic<double> previewSampleRate{ 44100.0 };
	juce::CriticalSection previewLock;

	std::atomic<bool> isLoadingFromBank{ false };
	juce::String currentBankLoadTrackId;
	juce::String currentPreviewTrackId;

	std::future<void> sampleBankInitFuture;
	std::atomic<bool> sampleBankReady{ false };

	bool useLocalModel = false;
	juce::String localModelsPath = "";

	juce::Synthesiser synth;

	static juce::AudioProcessor::BusesProperties createBusLayout();
	static const int MAX_TRACKS = 8;

	juce::StringArray customPrompts;

	double lastBpm = 126.0;
	double lastDuration = 6.0;
	double hostSampleRate;

	float smoothedMasterVol = 1.0f;
	float smoothedMasterPan = 0.0f;

	bool hostBpmEnabled = true;
	bool drumsEnabled = false;
	bool bassEnabled = false;
	bool otherEnabled = false;
	bool vocalsEnabled = false;
	bool guitarEnabled = false;
	bool pianoEnabled = false;
	bool isGenerating = false;
	bool onboardingDone = false;

	int lastKeyIndex = 1;
	int lastPresetIndex = -1;
	int currentBlockSize = 512;
	int requestTimeoutMS = 360000;
	std::atomic<int> timeSignatureNumerator{ 4 };
	std::atomic<int> timeSignatureDenominator{ 4 };

	juce::String globalPrompt;
	float globalBpm = 110.0f;
	juce::String globalKey = "C Minor";
	int globalDuration = 6;
	std::vector<juce::String> globalStems = {};

	juce::String pendingMessage;
	bool hasPendingNotification = false;

	void handleAsyncUpdate() override;

	std::unique_ptr<ObsidianEngine> obsidianEngine;

	struct PendingRequest
	{
		int trackId;
		ObsidianEngine::LoopRequest request;
		juce::Time requestTime;
	};

	std::queue<PendingRequest> pendingRequests;
	std::mutex requestsMutex;

	juce::CriticalSection sequencerMidiLock;

	juce::File pendingAudioFile;

	juce::MidiBuffer sequencerMidiBuffer;

	juce::AudioProcessorValueTreeState parameters;
	juce::String serverUrl = "";
	juce::String apiKey;
	juce::String lastPrompt = "";
	juce::String lastKey = "C Minor";
	juce::String trackIdWaitingForLoad;
	juce::String pendingTrackId;
	juce::String lastGeneratedTrackId;
	juce::String selectedTrackId;
	juce::String generatingTrackId = "";

	juce::StringArray booleanParamIds = {
		"generate", "play",
		"slot1Mute", "slot1Solo", "slot1Play", "slot1Stop", "slot1Generate", "slot1RandomRetrigger",
		"slot2Mute", "slot2Solo", "slot2Play", "slot2Stop", "slot2Generate", "slot2RandomRetrigger",
		"slot3Mute", "slot3Solo", "slot3Play", "slot3Stop", "slot3Generate", "slot3RandomRetrigger",
		"slot4Mute", "slot4Solo", "slot4Play", "slot4Stop", "slot4Generate", "slot4RandomRetrigger",
		"slot5Mute", "slot5Solo", "slot5Play", "slot5Stop", "slot5Generate", "slot5RandomRetrigger",
		"slot6Mute", "slot6Solo", "slot6Play", "slot6Stop", "slot6Generate", "slot6RandomRetrigger",
		"slot7Mute", "slot7Solo", "slot7Play", "slot7Stop", "slot7Generate", "slot7RandomRetrigger",
		"slot8Mute", "slot8Solo", "slot8Play", "slot8Stop", "slot8Generate", "slot8RandomRetrigger",
		"nextTrack", "prevTrack",
		"slot1PageA", "slot1PageB", "slot1PageC", "slot1PageD",
		"slot2PageA", "slot2PageB", "slot2PageC", "slot2PageD",
		"slot3PageA", "slot3PageB", "slot3PageC", "slot3PageD",
		"slot4PageA", "slot4PageB", "slot4PageC", "slot4PageD",
		"slot5PageA", "slot5PageB", "slot5PageC", "slot5PageD",
		"slot6PageA", "slot6PageB", "slot6PageC", "slot6PageD",
		"slot7PageA", "slot7PageB", "slot7PageC", "slot7PageD",
		"slot8PageA", "slot8PageB", "slot8PageC", "slot8PageD" };

	juce::StringArray floatParamIds = {
		"bpm", "masterVolume", "masterPan", "masterHigh", "masterMid", "masterLow",
		"slot1Volume", "slot1Pan", "slot1Pitch", "slot1Fine", "slot1BpmOffset",
		"slot2Volume", "slot2Pan", "slot2Pitch", "slot2Fine", "slot2BpmOffset",
		"slot3Volume", "slot3Pan", "slot3Pitch", "slot3Fine", "slot3BpmOffset",
		"slot4Volume", "slot4Pan", "slot4Pitch", "slot4Fine", "slot4BpmOffset",
		"slot5Volume", "slot5Pan", "slot5Pitch", "slot5Fine", "slot5BpmOffset",
		"slot6Volume", "slot6Pan", "slot6Pitch", "slot6Fine", "slot6BpmOffset",
		"slot7Volume", "slot7Pan", "slot7Pitch", "slot7Fine", "slot7BpmOffset",
		"slot8Volume", "slot8Pan", "slot8Pitch", "slot8Fine", "slot8BpmOffset",
		"globalCrossfader",
		"pairCrossfader1", "pairCrossfader2", "pairCrossfader3", "pairCrossfader4",
		"crossfaderCurveMode"
	};

	juce::CriticalSection filesToDeleteLock;

	std::function<void(const juce::String&)> midiIndicatorCallback;

	std::atomic<double> cachedHostBpm{ 126.0 };

	std::vector<juce::AudioBuffer<float>> individualOutputBuffers;

	std::unordered_map<int, juce::String> playingTracks;

	std::atomic<int> currentNoteNumber{ -1 };

	std::atomic<bool> hasPendingAudioData{ false };
	std::atomic<bool> autoLoadEnabled;
	std::atomic<bool> hasUnloadedSample{ false };
	std::atomic<bool> waitingForMidiToLoad{ false };
	std::atomic<bool> isNotePlaying{ false };
	std::atomic<bool> correctMidiNoteReceived{ false };
	std::atomic<bool> stateLoaded{ false };
	std::atomic<bool> canLoad{ false };
	std::atomic<bool> bypassSequencer{ false };

	std::atomic<float>* generateParam = nullptr;
	std::atomic<float>* playParam = nullptr;
	std::atomic<float> masterVolume{ 0.8f };
	std::atomic<float> masterPan{ 0.0f };
	std::atomic<float> masterHighEQ{ 0.0f };
	std::atomic<float> masterMidEQ{ 0.0f };
	std::atomic<float> masterLowEQ{ 0.0f };
	std::atomic<float>* masterVolumeParam = nullptr;
	std::atomic<float>* masterPanParam = nullptr;
	std::atomic<float>* masterHighParam = nullptr;
	std::atomic<float>* masterMidParam = nullptr;
	std::atomic<float>* masterLowParam = nullptr;
	std::atomic<float>* slotVolumeParams[8] = { nullptr };
	std::atomic<float>* slotPanParams[8] = { nullptr };
	std::atomic<float>* slotMuteParams[8] = { nullptr };
	std::atomic<float>* slotSoloParams[8] = { nullptr };
	std::atomic<float>* slotPlayParams[8] = { nullptr };
	std::atomic<float>* slotStopParams[8] = { nullptr };
	std::atomic<float>* slotGenerateParams[8] = { nullptr };
	std::atomic<float>* slotPitchParams[8] = { nullptr };
	std::atomic<float>* slotFineParams[8] = { nullptr };
	std::atomic<float>* slotBpmOffsetParams[8] = { nullptr };
	std::atomic<float>* slotRandomRetriggerParams[8];
	std::atomic<float>* slotRetriggerIntervalParams[8];
	std::atomic<float>* slotAdsrAttackParams[8] = {};
	std::atomic<float>* slotAdsrDecayParams[8] = {};
	std::atomic<float>* slotAdsrSustainParams[8] = {};
	std::atomic<float>* slotAdsrReleaseParams[8] = {};
	std::atomic<float>* globalCrossfaderParam = nullptr;
	std::atomic<float>* pairCrossfaderParams[4] = { nullptr, nullptr, nullptr, nullptr };
	std::atomic<float>* crossfaderCurveModeParam = nullptr;

	std::atomic<float> pendingDetectedBpm{ -1.0f };

	static juce::File getGlobalConfigFile()
	{
		return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
			.getChildFile("OBSIDIAN-Neural")
			.getChildFile("global_config.json");
	}

	void processIncomingAudio(bool hostIsPlaying);
	void processMidiMessages(juce::MidiBuffer& midiMessages, bool hostIsPlaying, double hostBpm);
	void playTrack(const juce::MidiMessage& message, double hostBpm);
	void prepareToPlay(double newSampleRate, int samplesPerBlock);
	void handlePlayAndStop(bool hostIsPlaying);
	void updateTimeStretchRatios(double hostBpm);
	void updateMasterEQ();
	void processAudioBPMAndSync(TrackData* track);
	void loadAudioToStagingBuffer(std::unique_ptr<juce::AudioFormatReader>& reader, TrackData* track);
	void checkAndSwapStagingBuffers();
	void performAtomicSwap(TrackData* track, const juce::String& trackId);
	void updateWaveformDisplay(const juce::String& trackId);
	void stopNotePlaybackForTrack(int noteNumber);
	void updateSequencers(bool hostIsPlaying);
	void handleAdvanceStep(TrackData* track, bool hostIsPlaying);
	void triggerSequencerStep(TrackData* track);
	void saveBufferToFile(const juce::AudioBuffer<float>& buffer,
		const juce::File& outputFile,
		double sampleRate);
	void executePendingAction(TrackData* track);
	void generateLoopFromMidi(const juce::String& trackId);
	void updateMidiIndicatorWithActiveNotes(double hostBpm, const juce::Array<int>& triggeredNotes);
	void generateLoopAPI(const DjIaClient::LoopRequest& request, const juce::String& trackId);
	void generateLoopLocal(const DjIaClient::LoopRequest& request, const juce::String& trackId);
	void saveOriginalAndStretchedBuffers(const juce::AudioBuffer<float>& originalBuffer,
		const juce::AudioBuffer<float>& stretchedBuffer,
		const juce::String& trackId,
		double sampleRate);
	void loadAudioFileForSwitch(const juce::String& trackId, const juce::File& audioFile);
	void loadSampleToBankPage(const juce::String& trackId, int pageIndex, const juce::File& sampleFile, const juce::String& sampleId);
	void loadAudioFileForPageSwitch(const juce::String& trackId, int pageIndex, const juce::File& audioFile);
	void notifyPageChangedFeedback(int slotNumber, int pageIndex);
	void sendFullStateFeedback();

	juce::File getTrackPageAudioFile(const juce::String& trackId, int pageIndex);

	juce::File getTrackAudioFile(const juce::String& trackId);

	void updateTrackPathsAfterMigration();
	void checkBeatRepeatWithSampleCounter();
	void handlePageChange(const juce::String& parameterID);
	void handleSequenceChange(const juce::String& parameterID);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DjIaVstProcessor);
};