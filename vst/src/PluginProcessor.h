/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2025 Anthony Charretier
 */

#pragma once
#include "JuceHeader.h"
#include "TrackManager.h"
#include "DjIaClient.h"
#include "MidiLearnManager.h"
#include "ObsidianEngine.h"
#include "SimpleEQ.h"
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
	void timerCallback() override;
	std::function<void()> onUIUpdateNeeded;

	DjIaVstProcessor();
	~DjIaVstProcessor() override;

	struct GenerationListener
	{
		virtual ~GenerationListener() = default;
		virtual void onGenerationComplete(const juce::String& trackId, const juce::String& message) = 0;
	};

	void setGenerationListener(GenerationListener* listener) { generationListener = listener; }
	TrackManager trackManager;
	juce::ValueTree pendingMidiMappings;
	juce::AudioProcessorValueTreeState& getParameterTreeState() { return parameters; }
	std::atomic<bool> needsUIUpdate{ false };
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
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	bool getDrumsEnabled() const { return drumsEnabled; }
	void setDrumsEnabled(bool enabled) { drumsEnabled = enabled; }
	bool getBassEnabled() const { return bassEnabled; }
	void setBassEnabled(bool enabled) { bassEnabled = enabled; }
	bool getOtherEnabled() const { return otherEnabled; }
	void setOtherEnabled(bool enabled) { otherEnabled = enabled; }
	bool getVocalsEnabled() const { return vocalsEnabled; }
	void setVocalsEnabled(bool enabled) { vocalsEnabled = enabled; }
	bool getGuitarEnabled() const { return guitarEnabled; }
	void setGuitarEnabled(bool enabled) { guitarEnabled = enabled; }
	bool getPianoEnabled() const { return pianoEnabled; }
	void setPianoEnabled(bool enabled) { pianoEnabled = enabled; }
	double getLastBpm() const { return lastBpm; }
	void setLastBpm(double bpm) { lastBpm = bpm; }
	double getLastDuration() const { return lastDuration; }
	void setLastDuration(double duration) { lastDuration = duration; }
	int getLastKeyIndex() const { return lastKeyIndex; }
	void setLastKeyIndex(int index) { lastKeyIndex = index; }
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override { return true; }
	const juce::String getName() const override { return "OBSIDIAN-Neural"; }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return true; }
	bool isMidiEffect() const override { return false; }
	double getTailLengthSeconds() const override { return 0.0; }
	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int) override {}
	const juce::String getProgramName(int) override { return {}; }
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
	float getMasterVolume() const { return masterVolume.load(); }
	int getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
	int getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }
	float getMasterPan() const { return masterPan.load(); }
	juce::String createNewTrack(const juce::String& name = "Track");
	void deleteTrack(const juce::String& trackId);
	void selectTrack(const juce::String& trackId);
	juce::String getSelectedTrackId() const { return selectedTrackId; }
	void reorderTracks(const juce::String& fromTrackId, const juce::String& toTrackId);
	std::vector<juce::String> getAllTrackIds() const { return trackManager.getAllTrackIds(); }
	TrackData* getCurrentTrack() { return trackManager.getTrack(selectedTrackId); }
	TrackData* getTrack(const juce::String& trackId) { return trackManager.getTrack(trackId); }
	void generateLoop(const DjIaClient::LoopRequest& request, const juce::String& targetTrackId = "");
	void startNotePlaybackForTrack(const juce::String& trackId, int noteNumber, double hostBpm = 126.0);
	void setApiKey(const juce::String& key);
	void setServerUrl(const juce::String& url);
	double getHostBpm() const;
	juce::String getServerUrl() const { return serverUrl; }
	juce::String getApiKey() const { return apiKey; }
	juce::String getLastPrompt() const { return lastPrompt; }
	juce::String getLastKey() const { return lastKey; }
	int getLastPresetIndex() const { return lastPresetIndex; }
	bool getHostBpmEnabled() const { return hostBpmEnabled; }
	void setLastPrompt(const juce::String& prompt) { lastPrompt = prompt; }
	void setLastPresetIndex(int index) { lastPresetIndex = index; }
	void setHostBpmEnabled(bool enabled) { hostBpmEnabled = enabled; }
	void updateAllWaveformsAfterLoad();
	void setAutoLoadEnabled(bool enabled);
	bool getAutoLoadEnabled() const { return autoLoadEnabled.load(); }
	void loadPendingSample();
	bool hasSampleWaiting() const { return hasUnloadedSample.load(); }
	void setMidiIndicatorCallback(std::function<void(const juce::String&)> callback)
	{
		midiIndicatorCallback = callback;
	}
	void reloadTrackWithVersion(const juce::String& trackId, bool useOriginal);
	juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
	void addCustomPrompt(const juce::String& prompt);
	juce::StringArray getCustomPrompts() const;
	void clearCustomPrompts();
	bool getIsGenerating() const { return isGenerating; }
	void setIsGenerating(bool generating) { isGenerating = generating; }
	juce::String getGeneratingTrackId() const { return generatingTrackId; }
	void setGeneratingTrackId(const juce::String& trackId) { generatingTrackId = trackId; }
	bool isStateReady() const { return stateLoaded; }
	MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }
	void handleSampleParams(int slot, TrackData* track);
	void loadGlobalConfig();
	void saveGlobalConfig();
	void removeCustomPrompt(const juce::String& prompt);
	void editCustomPrompt(const juce::String& oldPrompt, const juce::String& newPrompt);
	int getSamplesPerBlock() const { return currentBlockSize; };
	int getRequestTimeout() const { return requestTimeoutMS; };
	void handleSequencerPlayState(bool hostIsPlaying);
	void addSequencerMidiMessage(const juce::MidiMessage& message);
	void setRequestTimeout(int requestTimeoutMS);
	void prepareToPlay(double newSampleRate, int samplesPerBlock);
	std::function<void(double)> onHostBpmChanged = nullptr;
	void setGlobalPrompt(const juce::String& prompt) { globalPrompt = prompt; }
	juce::String getGlobalPrompt() const { return globalPrompt; }

	void setGlobalBpm(float bpm) { globalBpm = bpm; }
	void setCanLoad(bool load) { canLoad = load; }
	float getGlobalBpm() const
	{
		float hostBpm = static_cast<float>(getHostBpm());
		return hostBpm > 0 ? hostBpm : globalBpm;
	}

	void setGlobalKey(const juce::String& key) { globalKey = key; }
	juce::String getGlobalKey() const { return globalKey; }

	void setGlobalDuration(int duration) { globalDuration = duration; }
	int getGlobalDuration() const { return globalDuration; }
	void previewTrack(const juce::String& trackId);
	void setGlobalStems(const std::vector<juce::String>& stems) { globalStems = stems; }
	std::vector<juce::String> getGlobalStems() const { return globalStems; }

	DjIaClient::LoopRequest createGlobalLoopRequest() const
	{
		DjIaClient::LoopRequest request;
		request.prompt = globalPrompt;
		request.bpm = globalBpm;
		request.key = globalKey;
		request.generationDuration = static_cast<float>(globalDuration);
		request.preferredStems = globalStems;
		return request;
	}

	void updateGlobalStem(const juce::String& stem, bool enabled)
	{
		auto it = std::find(globalStems.begin(), globalStems.end(), stem);
		if (enabled && it == globalStems.end())
		{
			globalStems.push_back(stem);
		}
		else if (!enabled && it != globalStems.end())
		{
			globalStems.erase(it);
		}
	}

	bool isGlobalStemEnabled(const juce::String& stem) const
	{
		return std::find(globalStems.begin(), globalStems.end(), stem) != globalStems.end();
	}

	bool getUseLocalModel() const { return useLocalModel; }
	void setUseLocalModel(bool useLocal) { useLocalModel = useLocal; }

	juce::String getLocalModelsPath() const { return localModelsPath; }
	void setLocalModelsPath(const juce::String& path) { localModelsPath = path; }

	static juce::String getModelsDirectory()
	{
		return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
			.getChildFile("OBSIDIAN-Neural")
			.getChildFile("stable-audio")
			.getFullPathName();
	}

	juce::StringArray getBuiltInPrompts() const;

private:
	DjIaVstEditor* currentEditor = nullptr;
	SimpleEQ masterEQ;
	MidiLearnManager midiLearnManager;
	DjIaClient apiClient;
	GenerationListener* generationListener = nullptr;

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

	juce::String globalPrompt = "Techno kick rhythm";
	float globalBpm = 110.0f;
	juce::String globalKey = "C Aeolian";
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
		"slot1Mute", "slot1Solo", "slot1Play", "slot1Stop", "slot1Generate",
		"slot2Mute", "slot2Solo", "slot2Play", "slot2Stop", "slot2Generate",
		"slot3Mute", "slot3Solo", "slot3Play", "slot3Stop", "slot3Generate",
		"slot4Mute", "slot4Solo", "slot4Play", "slot4Stop", "slot4Generate",
		"slot5Mute", "slot5Solo", "slot5Play", "slot5Stop", "slot5Generate",
		"slot6Mute", "slot6Solo", "slot6Play", "slot6Stop", "slot6Generate",
		"slot7Mute", "slot7Solo", "slot7Play", "slot7Stop", "slot7Generate",
		"slot8Mute", "slot8Solo", "slot8Play", "slot8Stop", "slot8Generate" };

	juce::StringArray floatParamIds = {
		"bpm", "masterVolume", "masterPan", "masterHigh", "masterMid", "masterLow",
		"slot1Volume", "slot1Pan", "slot1Pitch", "slot1Fine", "slot1BpmOffset", "slot1Speed",
		"slot2Volume", "slot2Pan", "slot2Pitch", "slot2Fine", "slot2BpmOffset", "slot2Speed",
		"slot3Volume", "slot3Pan", "slot3Pitch", "slot3Fine", "slot3BpmOffset", "slot3Speed",
		"slot4Volume", "slot4Pan", "slot4Pitch", "slot4Fine", "slot4BpmOffset", "slot4Speed",
		"slot5Volume", "slot5Pan", "slot5Pitch", "slot5Fine", "slot5BpmOffset", "slot5Speed",
		"slot6Volume", "slot6Pan", "slot6Pitch", "slot6Fine", "slot6BpmOffset", "slot6Speed",
		"slot7Volume", "slot7Pan", "slot7Pitch", "slot7Fine", "slot7BpmOffset", "slot7Speed",
		"slot8Volume", "slot8Pan", "slot8Pitch", "slot8Fine", "slot8BpmOffset", "slot8Speed" };

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
	std::atomic<float>* slotSpeedParams[8];

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
	void loadAudioFileAsync(const juce::String& trackId, const juce::File& audioData);
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

	TrackComponent* findTrackComponentByName(const juce::String& trackName, DjIaVstEditor* editor);

	juce::Button* findGenerateButtonInTrack(TrackComponent* trackComponent);

	juce::Slider* findBpmOffsetSliderInTrack(TrackComponent* trackComponent);

	juce::File getTrackAudioFile(const juce::String& trackId);

	void handleGenerationComplete(const juce::String& trackId,
		const DjIaClient::LoopRequest& originalRequest,
		const ObsidianEngine::LoopResponse& response);

	juce::File createTempAudioFile(const std::vector<float>& audioData, float duration);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DjIaVstProcessor);
};