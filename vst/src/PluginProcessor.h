#pragma once
#include "JuceHeader.h"
#include "TrackManager.h"
#include "DjIaClient.h"
#include "MidiLearnManager.h"
#include "ObsidianEngine.h"
#include "SimpleEQ.h"
#include "SampleBank.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <atomic>

class DjIaVstEditor;
class TrackComponent;

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

	juce::AudioProcessorEditor* createEditor() override;
	juce::AudioFormatManager sharedFormatManager;

	TrackManager trackManager;

	MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }

	DjIaClient& getApiClient() { return apiClient; }

	SampleBank* getSampleBank() { return sampleBank.get(); }

	TrackData* getCurrentTrack() { return trackManager.getTrack(selectedTrackId); }
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

	juce::String getGlobalKey() const { return globalKey; }
	juce::String getGlobalPrompt() const { return globalPrompt; }
	juce::String getLocalModelsPath() const { return localModelsPath; }
	juce::String getSelectedTrackId() const { return selectedTrackId; }
	juce::String createNewTrack(const juce::String& name = "Track");
	juce::String getGeneratingTrackId() const { return generatingTrackId; }
	juce::String getServerUrl() const { return serverUrl; }
	juce::String getApiKey() const { return apiKey; }
	juce::String getLastPrompt() const { return lastPrompt; }
	juce::String getLastKey() const { return lastKey; }

	static juce::String getModelsDirectory()
	{
		return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
			.getChildFile("OBSIDIAN-Neural")
			.getChildFile("stable-audio")
			.getFullPathName();
	}

	juce::StringArray getBuiltInPrompts() const;
	juce::StringArray getCustomKeywords() const { return customKeywords; }
	juce::StringArray getCustomPrompts() const;

	const juce::String getName() const override { return "OBSIDIAN-Neural"; }
	const juce::String getProgramName(int) override { return {}; }

	std::vector<juce::String> getGlobalStems() const { return globalStems; }
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
	void handlePreviewPlaying(juce::AudioSampleBuffer& buffer);
	void checkIfUIUpdateNeeded(juce::MidiBuffer& midiMessages);
	void applyMasterEffects(juce::AudioSampleBuffer& mainOutput);
	void copyTracksToIndividualOutputs(juce::AudioSampleBuffer& buffer);
	void clearOutputBuffers(juce::AudioSampleBuffer& buffer);
	void resizeIndividualsBuffers(juce::AudioSampleBuffer& buffer);
	void getDawInformations(juce::AudioPlayHead* currentPlayHead, bool& hostIsPlaying, double& hostBpm, double& hostPpqPosition);
	void setDrumsEnabled(bool enabled) { drumsEnabled = enabled; }
	void setBassEnabled(bool enabled) { bassEnabled = enabled; }
	void setOtherEnabled(bool enabled) { otherEnabled = enabled; }
	void setVocalsEnabled(bool enabled) { vocalsEnabled = enabled; }
	void setGuitarEnabled(bool enabled) { guitarEnabled = enabled; }
	void setPianoEnabled(bool enabled) { pianoEnabled = enabled; }
	void setLastBpm(double bpm) { lastBpm = bpm; }
	void setLastDuration(double duration) { lastDuration = duration; }
	void setLastKeyIndex(int index) { lastKeyIndex = index; }
	void setCurrentProgram(int) override {}
	void changeProgramName(int, const juce::String&) override {}
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	void updateUI();
	void addCustomPromptsToIndexedPrompts(juce::ValueTree& promptsState, juce::Array<std::pair<int, juce::String>>& indexedPrompts);
	void loadCustomPromptsByCountProperty(juce::ValueTree& promptsState);
	void setMasterVolume(float volume) { masterVolume = volume; }
	void setMasterPan(float pan) { masterPan = pan; }
	void setMasterEQ(float high, float mid, float low)
	{
		masterHighEQ = high;
		masterMidEQ = mid;
		masterLowEQ = low;
	}
	void deleteTrack(const juce::String& trackId);
	void selectTrack(const juce::String& trackId);
	void reorderTracks(const juce::String& fromTrackId, const juce::String& toTrackId);
	void generateLoop(const DjIaClient::LoopRequest& request, const juce::String& targetTrackId = "");
	void startNotePlaybackForTrack(const juce::String& trackId, int noteNumber, double hostBpm = 126.0);
	void setApiKey(const juce::String& key);
	void setServerUrl(const juce::String& url);
	void setLastPrompt(const juce::String& prompt) { lastPrompt = prompt; }
	void setLastPresetIndex(int index) { lastPresetIndex = index; }
	void setHostBpmEnabled(bool enabled) { hostBpmEnabled = enabled; }
	void updateAllWaveformsAfterLoad();
	void setAutoLoadEnabled(bool enabled);
	void setGeneratingTrackId(const juce::String& trackId) { generatingTrackId = trackId; }
	void handleSampleParams(int slot, TrackData* track);
	void loadGlobalConfig();
	void saveGlobalConfig();
	void removeCustomPrompt(const juce::String& prompt);
	void editCustomPrompt(const juce::String& oldPrompt, const juce::String& newPrompt);
	void handleSequencerPlayState(bool hostIsPlaying);
	void addSequencerMidiMessage(const juce::MidiMessage& message);
	void setRequestTimeout(int requestTimeoutMS);
	void prepareToPlay(double newSampleRate, int samplesPerBlock);
	void setGlobalKey(const juce::String& key) { globalKey = key; }
	void setGlobalPrompt(const juce::String& prompt) { globalPrompt = prompt; }
	void setGlobalDuration(int duration) { globalDuration = duration; }
	void previewTrack(const juce::String& trackId);
	void setGlobalStems(const std::vector<juce::String>& stems) { globalStems = stems; }
	void setUseLocalModel(bool useLocal) { useLocalModel = useLocal; }
	void loadSampleFromBank(const juce::String& sampleId, const juce::String& trackId);
	void loadAudioFileAsync(const juce::String& trackId, const juce::File& audioData);
	void stopSamplePreview();
	void setLocalModelsPath(const juce::String& path) { localModelsPath = path; }
	void generateSampleWithImage(const juce::String& trackId, const juce::String& base64Image, const juce::StringArray& keywords);
	void generateLoopWithImage(const DjIaClient::LoopRequest& request, const juce::String& trackId, int timeoutMS);
	void setGlobalBpm(float bpm) { globalBpm = bpm; }
	void setCanLoad(bool load) { canLoad = load; }
	void setBypassSequencer(bool bypass) { bypassSequencer.store(bypass); }
	void selectNextTrack();
	void selectPreviousTrack();
	void triggerGlobalGeneration();
	void syncSelectedTrackWithGlobalPrompt();
	void setCreditsRemaining(int credits) { creditsRemaining = credits; }
	void clearCustomPrompts();
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
	float getMasterVolume() const { return masterVolume.load(); }
	float getMasterPan() const { return masterPan.load(); }

	double getTailLengthSeconds() const override { return 0.0; }
	double getLastDuration() const { return lastDuration; }
	double getLastBpm() const { return lastBpm; }
	double getHostBpm() const;
	double calculateRetriggerInterval(int intervalValue, double hostBpm) const;

	bool getUseLocalModel() const { return useLocalModel; }
	bool getIsGenerating() const { return isGenerating; }
	bool hasSampleWaiting() const { return hasUnloadedSample.load(); }
	bool getHostBpmEnabled() const { return hostBpmEnabled; }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	bool hasEditor() const override { return true; }
	bool getPianoEnabled() const { return pianoEnabled; }
	bool getGuitarEnabled() const { return guitarEnabled; }
	bool getVocalsEnabled() const { return vocalsEnabled; }
	bool getOtherEnabled() const { return otherEnabled; }
	bool getBassEnabled() const { return bassEnabled; }
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	bool getDrumsEnabled() const { return drumsEnabled; }
	bool getBypassSequencer() const { return bypassSequencer.load(); }
	bool isSamplePreviewing() const { return isPreviewPlaying.load(); }
	bool previewSampleFromBank(const juce::String& sampleId);
	bool isStateReady() const { return stateLoaded; }
	bool getAutoLoadEnabled() const { return autoLoadEnabled.load(); }

	bool canGenerateStandard = true;

	int getSamplesPerBlock() const { return currentBlockSize; };
	int getRequestTimeout() const { return requestTimeoutMS; };
	int getGlobalDuration() const { return globalDuration; }
	int getLastKeyIndex() const { return lastKeyIndex; }
	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	int getCreditsRemaining() const { return creditsRemaining; }
	int getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
	int getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }
	int getLastPresetIndex() const { return lastPresetIndex; }

	int creditsRemaining = 0;

private:
	DjIaVstEditor* currentEditor = nullptr;
	SimpleEQ masterEQ;
	MidiLearnManager midiLearnManager;
	DjIaClient apiClient;
	GenerationListener* generationListener = nullptr;
	juce::String projectId;
	bool migrationCompleted = false;
	std::unique_ptr<SampleBank> sampleBank;
	juce::StringArray customKeywords;

	std::atomic<float>* nextTrackParam = nullptr;
	std::atomic<float>* prevTrackParam = nullptr;
	std::atomic<int64_t> internalSampleCounter{ 0 };
	std::atomic<double> lastHostBpmForQuantization{ 120.0 };

	std::atomic<bool> isPreviewPlaying{ false };
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

	juce::CriticalSection apiLock;
	juce::CriticalSection sequencerMidiLock;

	juce::File pendingAudioFile;

	juce::MidiBuffer sequencerMidiBuffer;

	juce::AudioProcessorValueTreeState parameters;
	juce::String serverUrl = "";
	juce::String apiKey;
	juce::String lastPrompt = "";
	juce::String lastKey = "C Aeolian";
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
		"slot8Volume", "slot8Pan", "slot8Pitch", "slot8Fine", "slot8BpmOffset" };

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
	std::atomic<float> pendingDetectedBpm{ -1.0f };

	static juce::File getGlobalConfigFile()
	{
		return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
			.getChildFile("OBSIDIAN-Neural")
			.getChildFile("global_config.json");
	}

	void processIncomingAudio(bool hostIsPlaying);
	void clearPendingAudio();
	void processMidiMessages(juce::MidiBuffer& midiMessages, bool hostIsPlaying, double hostBpm);
	void playTrack(const juce::MidiMessage& message, double hostBpm);
	void handlePlayAndStop(bool hostIsPlaying);
	void updateTimeStretchRatios(double hostBpm);
	void updateMasterEQ();
	void processAudioBPMAndSync(TrackData* track);
	void loadAudioToStagingBuffer(std::unique_ptr<juce::AudioFormatReader>& reader, TrackData* track);
	void checkAndSwapStagingBuffers();
	void performAtomicSwap(TrackData* track, const juce::String& trackId);
	void updateWaveformDisplay(const juce::String& trackId);
	void performTrackDeletion(const juce::String& trackId);
	void reassignTrackOutputsAndMidi();
	void stopNotePlaybackForTrack(int noteNumber);
	void updateSequencers(bool hostIsPlaying);
	void handleAdvanceStep(TrackData* track, bool hostIsPlaying);
	void triggerSequencerStep(TrackData* track);
	void saveBufferToFile(const juce::AudioBuffer<float>& buffer,
		const juce::File& outputFile,
		double sampleRate);
	void executePendingAction(TrackData* track) const;
	void handleGenerate();
	void notifyGenerationComplete(const juce::String& trackId, const juce::String& message);
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

	juce::File getTrackPageAudioFile(const juce::String& trackId, int pageIndex);

	TrackComponent* findTrackComponentByName(const juce::String& trackName, DjIaVstEditor* editor);

	juce::Button* findGenerateButtonInTrack(TrackComponent* trackComponent);

	juce::Slider* findBpmOffsetSliderInTrack(TrackComponent* trackComponent);

	juce::File getTrackAudioFile(const juce::String& trackId);

	void handleGenerationComplete(const juce::String& trackId,
		const DjIaClient::LoopRequest& originalRequest,
		const ObsidianEngine::LoopResponse& response);

	juce::File createTempAudioFile(const std::vector<float>& audioData, float duration);
	void performMigrationIfNeeded();
	void updateTrackPathsAfterMigration();
	void checkBeatRepeatWithSampleCounter();
	void generateLoopFromGlobalSettings();
	void clearMasterChannel(juce::AudioSampleBuffer& mainOutput);
	void handlePageChange(const juce::String& parameterID);
	void reEnableCanvasGenerate();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DjIaVstProcessor);
};