#include "PluginProcessor.h"
#include "AiModelDefinitions.h"
#include "MidiMapping.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "SequencerComponent.h"

DjIaVstProcessor::DjIaVstProcessor()
    : AudioProcessor(createBusLayout()), apiClient("", "http://localhost:8000"), parameterManager(*this),
      stateManager(*this), generationManager(*this), sequencerManager(*this, trackManager),
      audioManager(*this, trackManager, generationManager), midiManager(*this, midiLearnManager),
      obsidianEngine(std::make_unique<ObsidianEngine>()), sampleBank(std::make_unique<SampleBank>()),
      autoLoadEnabled(true)
{
	midiLearnManager.setProcessor(this);
	projectId = "legacy";
	loadGlobalConfig();
	promptBank = std::make_unique<PromptBank>();
	if (!promptBank->hasMigrated() && !customPrompts.isEmpty())
	{
		promptBank->migrateFromCustomPrompts(customPrompts);
		customPrompts.clear();
		saveGlobalConfig();
	}
	if (!promptBank->hasSeeded())
		promptBank->seedDefaultPromptsAndCategories();

	sharedFormatManager.registerBasicFormats();
	obsidianEngine->initialize();

	parameterManager.resolveParameters(this);
	midiLearnManager.loadDefaultMappings(this);
	audioManager.initDummySynth();
	audioManager.initBuffers(audioManager.MAX_SLOTS);

	trackManager.onPreviewEnded = [this](const juce::String &trackId)
	{ juce::MessageManager::callAsync([this, trackId]() { audioManager.stopTrackPreview(trackId); }); };

	static auto safeCallback = std::make_shared<std::function<void(int, TrackData *)>>(
	    [this](int slot, TrackData *track)
	    {
		    handleSampleParams(slot, track);
		    handleSendsParams();
	    });
	trackManager.parameterUpdateCallback.store(safeCallback.get());

	startTimerHz(30);
	autoLoadEnabled.store(true);

#if JucePlugin_Build_Standalone
	standaloneTransport = std::make_unique<StandaloneTransport>();
	setPlayHead(standaloneTransport.get());
#endif

	juce::Timer::callAfterDelay(500,
	                            [this]()
	                            {
		                            if (!stateLoaded)
			                            stateLoaded = true;
	                            });
}

DjIaVstProcessor::~DjIaVstProcessor()
{
	stopTimer();
	try
	{
		cleanProcessor();
	}
	catch (const std::exception &e)
	{
		std::cout << "Error: " << e.what() << std::endl;
	}
}

juce::AudioProcessorEditor *DjIaVstProcessor::createEditor()
{
	currentEditor = new DjIaVstEditor(*this);
	midiLearnManager.setEditor(currentEditor);
	return currentEditor;
}

void DjIaVstProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	audioManager.prepareToPlay(sampleRate, samplesPerBlock);
#if JucePlugin_Build_Standalone
	if (standaloneTransport)
		setPlayHead(standaloneTransport.get());
#endif
	masterConsoleBuss.prepare(sampleRate);
	delaySend.prepare(sampleRate, samplesPerBlock);
	delaySendBuffer.setSize(2, samplesPerBlock, false, false, true);
}

void DjIaVstProcessor::releaseResources()
{
	audioManager.releaseResources();
}

void DjIaVstProcessor::cleanProcessor()
{
	isShuttingDown.store(true);

	parameterManager.removeAllListeners(this);

	isNotePlaying = false;
	hasPendingAudioData = false;
	hasUnloadedSample = false;

	midiManager.setMidiIndicatorCallback(nullptr);
	trackManager.parameterUpdateCallback.store(nullptr);

	audioManager.releaseResources();
	obsidianEngine.reset();
}

juce::AudioProcessor::BusesProperties DjIaVstProcessor::createBusLayout()
{
	auto layout = juce::AudioProcessor::BusesProperties();
	layout = layout.withOutput("Main", juce::AudioChannelSet::stereo(), true);
	for (int i = 0; i < AudioManager::MAX_SLOTS + 1; ++i)
	{
		layout = layout.withOutput("Track " + juce::String(i + 1), juce::AudioChannelSet::stereo(), false);
	}
	return layout;
}

void DjIaVstProcessor::loadGlobalConfig()
{
	auto configFile = getGlobalConfigFile();

	if (configFile.existsAsFile())
	{
		auto configJson = juce::JSON::parse(configFile);
		if (auto *object = configJson.getDynamicObject())
		{
			apiKey = object->getProperty("apiKey").toString();
			serverUrl = object->getProperty("serverUrl").toString();
			requestTimeoutMS = object->getProperty("requestTimeoutMS").toString().getIntValue();
			onboardingDone = object->getProperty("onboardingDone").toString() == "true";
			useLocalModel = object->getProperty("useLocalModel").toString() == "true";
			localModelsPath = object->getProperty("localModelsPath").toString();
			panelStateJson = object->getProperty("panelStateJson").toString();

			if (!object->hasProperty("useLocalModel"))
			{
				useLocalModel = false;
			}

			auto promptsVar = object->getProperty("customPrompts");

			if (promptsVar.isArray())
			{
				customPrompts.clear();
				auto *promptsArray = promptsVar.getArray();

				for (int i = 0; i < promptsArray->size(); ++i)
				{
					juce::String prompt = promptsArray->getUnchecked(i).toString();
					customPrompts.add(prompt);
				}
			}
			auto keywordsVar = object->getProperty("customKeywords");
			if (keywordsVar.isArray())
			{
				customKeywords.clear();
				auto *keywordsArray = keywordsVar.getArray();
				for (int i = 0; i < keywordsArray->size(); ++i)
				{
					juce::String keyword = keywordsArray->getUnchecked(i).toString();
					customKeywords.add(keyword);
				}
			}
			setApiKey(apiKey);
			setServerUrl(serverUrl);
		}
	}
	if (trackManager.getAllTrackIds().empty())
	{
		initTracks();
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
				    editor->uiTrackManager->refreshTrackComponents();
		    });
	}
}

void DjIaVstProcessor::saveGlobalConfig()
{
	auto configFile = getGlobalConfigFile();
	configFile.getParentDirectory().createDirectory();

	juce::DynamicObject::Ptr config = new juce::DynamicObject();
	config->setProperty("apiKey", apiKey);
	config->setProperty("serverUrl", serverUrl);
	config->setProperty("requestTimeoutMS", requestTimeoutMS);
	config->setProperty("useLocalModel", useLocalModel ? "true" : "false");
	config->setProperty("localModelsPath", localModelsPath);
	config->setProperty("onboardingDone", onboardingDone ? "true" : "false");
	config->setProperty("panelStateJson", panelStateJson);

	juce::StringArray sortedPrompts = customPrompts;
	sortedPrompts.sort(true);
	juce::Array<juce::var> promptsArray;
	for (const auto &prompt : sortedPrompts)
	{
		promptsArray.add(juce::var(prompt));
	}
	config->setProperty("customPrompts", juce::var(promptsArray));

	juce::Array<juce::var> keywordsArray;
	for (const auto &keyword : customKeywords)
	{
		keywordsArray.add(juce::var(keyword));
	}
	config->setProperty("customKeywords", juce::var(keywordsArray));

	juce::String jsonString = juce::JSON::toString(juce::var(config.get()));
	configFile.replaceWithText(jsonString);
}

void DjIaVstProcessor::initTracks()
{

	trackManager.isInitializing.store(true);

	juce::String defaultPrompt;
	if (promptBank)
	{
		auto all = promptBank->getAllPrompts();
		if (!all.empty())
			defaultPrompt = all[0]->text;
	}

	for (int i = 0; i < 8; ++i)
	{
		juce::String newTrackId = trackManager.createTrack();
		if (auto *track = trackManager.getTrack(newTrackId))
		{
			track->slotIndex = i;
			attachPageChangeCallback(track);
			auto serverModels = AiModelDefinitions::getModelsForMode(false);
			juce::String modelName = serverModels[i % serverModels.size()];

			juce::String promptForThisModel = defaultPrompt;
			if (promptBank)
			{
				auto modelPrompts = promptBank->getPromptsByCategory("");
				for (auto *p : promptBank->getAllPrompts())
				{
					if (p->modelName == modelName)
					{
						promptForThisModel = p->text;
						break;
					}
				}
			}

			for (int p = 0; p < 4; ++p)
			{
				auto &page = track->pages[p];
				page.selectedModel = modelName;
				page.prompt = promptForThisModel;
				page.generationPrompt = promptForThisModel;
				page.selectedPrompt = promptForThisModel;
				page.selectedKeywords = customKeywords;
			}
		}
	}
	trackManager.isInitializing.store(false);
}

juce::StringArray DjIaVstProcessor::getAvailablePromptsForModel(const juce::String &modelName) const
{
	juce::StringArray result;
	if (!promptBank)
		return result;

	auto allPrompts = const_cast<PromptBank *>(promptBank.get())->getAllPrompts();

	for (auto *p : allPrompts)
	{
		if (modelName.isNotEmpty() && p->modelName != modelName)
			continue;
		result.add(p->text);
	}

	result.sort(true);
	return result;
}

void DjIaVstProcessor::attachPageChangeCallback(TrackData *track)
{
	if (!track)
		return;
	juce::WeakReference<TrackData> weakTrack(track);
	track->onPageChanged = [this, weakTrack]()
	{
		juce::MessageManager::callAsync(
		    [this, weakTrack]()
		    {
			    auto *t = weakTrack.get();
			    if (!t || t->slotIndex < 0 || t->slotIndex >= 8)
				    return;
			    const auto &page = t->getCurrentPage();
			    juce::String s = "slot" + juce::String(t->slotIndex + 1);
			    auto &apvts = parameterManager.getAPVTS();

			    float pitchValue =
			        juce::jlimit(-12.0f, 12.0f, (float(page.bpmOffset.load()) - page.fineOffset.load()) / 8.0f);
			    float fineValue = juce::jlimit(-50.0f, 50.0f, page.fineOffset.load() * 10.0f);

			    if (auto *p = apvts.getParameter(s + "Pitch"))
				    p->setValueNotifyingHost((pitchValue + 12.0f) / 24.0f);

			    if (auto *p = apvts.getParameter(s + "Fine"))
				    p->setValueNotifyingHost((fineValue + 50.0f) / 100.0f);
		    });
	};
}

juce::File DjIaVstProcessor::getGlobalConfigFile()
{
	return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	    .getChildFile("OBSIDIAN-Neural")
	    .getChildFile("global_config.json");
}

float DjIaVstProcessor::getGlobalBpm() const
{
	float hostBpm = static_cast<float>(getHostBpm());
	return hostBpm > 0 ? hostBpm : globalBpm;
}

DjIaClient::LoopRequest DjIaVstProcessor::createGlobalLoopRequest() const
{
	DjIaClient::LoopRequest request;
	request.prompt = globalPrompt;
	request.bpm = globalBpm;
	request.key = globalKey;
	request.generationDuration = static_cast<float>(globalDuration);
	return request;
}

void DjIaVstProcessor::timerCallback()
{
	if (!needsUIUpdate.load())
		return;
	if (onUIUpdateNeeded)
	{
		onUIUpdateNeeded();
	}
	needsUIUpdate.store(false);
}

bool DjIaVstProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
	{
		return false;
	}
	for (int i = 1; i < layouts.outputBuses.size(); ++i)
	{
		if (!layouts.outputBuses[i].isDisabled() && layouts.outputBuses[i] != juce::AudioChannelSet::stereo())
		{
			return false;
		}
	}
	return true;
}

void DjIaVstProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
#if JucePlugin_Build_Standalone
	if (standaloneTransport)
		standaloneTransport->advance(buffer.getNumSamples(), getSampleRate());
#endif
	internalSampleCounter += buffer.getNumSamples();
	audioManager.checkAndSwapStagingBuffers();
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());
	auto currentPlayHead = getPlayHead();
	double hostBpm = 126.0;
	double hostPpqPosition = 0.0;
	bool hostIsPlaying = false;

	if (currentPlayHead != nullptr)
		getDawInformations(currentPlayHead, hostIsPlaying, hostBpm, hostPpqPosition);

	lastHostBpmForQuantization.store(hostBpm);

	sequencerManager.handleSequencerPlayState(hostIsPlaying);
	sequencerManager.updateSequencers(hostIsPlaying);
	sequencerManager.checkBeatRepeatWithSampleCounter();
	sequencerManager.flushMidiBuffer(midiMessages, buffer.getNumSamples());

	midiManager.processMidiMessages(midiMessages, hostIsPlaying, hostBpm);
	midiManager.flushFeedbackBuffer(midiMessages, buffer.getNumSamples());
	if (hasPendingAudioData.load())
	{
		audioManager.processIncomingAudio(hostIsPlaying);
	}
	audioManager.resizeIndividualBuffers(buffer);
	audioManager.clearOutputBuffers(buffer);
	auto mainOutput = getBusBuffer(buffer, false, 0);
	mainOutput.clear();
	audioManager.updateTimeStretchRatios(hostBpm);
	auto previewBus = getBusBuffer(buffer, false, getBusCount(false) - 1);
	previewBus.clear();
	float pairCurrent[4];
	float pairPrev[4];
	for (int i = 0; i < 4; ++i)
	{
		pairCurrent[i] = parameterManager.getPairCrossfader(i);
		pairPrev[i] = pairCrossfaderPrevious[i];
		pairCrossfaderPrevious[i] = pairCurrent[i];
	}
	float globalCurrent = parameterManager.getGlobalCrossfader();
	float globalPrev = globalCrossfaderPrevious;
	globalCrossfaderPrevious = globalCurrent;

	int curveMode = parameterManager.getCrossfaderCurveMode();

	trackManager.renderAllTracks(mainOutput, audioManager.getIndividualOutputBuffers(), previewBus, hostBpm, pairPrev,
	                             pairCurrent, globalPrev, globalCurrent, curveMode);

	delaySendBuffer.setSize(2, buffer.getNumSamples(), false, false, true);
	delaySendBuffer.clear();

	const auto &individualBuffers = audioManager.getIndividualOutputBuffers();
	for (size_t i = 0; i < individualBuffers.size(); ++i)
	{
		auto &trackBuffer = individualBuffers[i];
		if (trackBuffer.getNumChannels() < 2)
			continue;

		TrackData *track = nullptr;
		for (const auto &id : trackManager.getAllTrackIds())
		{
			auto *t = trackManager.getTrack(id);
			if (t && t->slotIndex == (int)i)
			{
				track = t;
				break;
			}
		}
		if (!track)
			continue;

		float sendLevel = track->delaySend.load();
		if (sendLevel < 0.001f)
			continue;

		for (int ch = 0; ch < 2; ++ch)
			delaySendBuffer.addFrom(ch, 0, trackBuffer, ch, 0, buffer.getNumSamples(), sendLevel);
	}

	delaySend.setBpm(hostBpm);
	delaySend.setTimeDivision(static_cast<DelaySend::TimeDivision>(parameterManager.getDelayDivisionIndex()));
	delaySend.setFeedback(parameterManager.getFeedback());
	delaySend.setMode(static_cast<DelaySend::Mode>(parameterManager.getDelayModeIndex()));
	delaySend.process(delaySendBuffer);
	for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
		mainOutput.addFrom(ch, 0, delaySendBuffer, ch, 0, buffer.getNumSamples());

	audioManager.copyToIndividualOutputs(buffer);
	audioManager.applyMasterEffects(mainOutput);
	masterConsoleBuss.process(mainOutput, 0, mainOutput.getNumSamples());
	auto *lastBus = getBus(false, getBusCount(false) - 1);
	bool previewBusIsEffectivelyEnabled = (lastBus != nullptr && lastBus->isEnabled());
	audioManager.renderPreviewToOutput(previewBus, mainOutput, buffer.getNumSamples(), getSampleRate(),
	                                   previewBusIsEffectivelyEnabled);

	if (onMasterOutput)
	{
		double ppq = 0.0;
		if (auto *ph = getPlayHead())
		{
			if (auto info = ph->getPosition())
			{
				if (auto p = info->getPpqPosition())
					ppq = *p;
			}
		}
		onMasterOutput(buffer.getReadPointer(0), buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr,
		               buffer.getNumSamples(), ppq);
	}
	audioManager.computeAndSetPeakLevels(buffer);
	checkIfUIUpdateNeeded(midiMessages);
}

void DjIaVstProcessor::setPairCrossfaderValue(int pairIdx, float value)
{
	if (pairIdx < 0 || pairIdx >= 4)
		return;
	juce::String pairId = "pairCrossfader" + juce::String(pairIdx + 1);
	if (auto *p = parameterManager.getAPVTS().getParameter(pairId))
	{
		p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackPairCrossfader(pairIdx), MidiMapping::volumeToMidi(value),
		                             MidiMapping::feedbackChannelShaping);
	}
}

float DjIaVstProcessor::getPairCrossfaderValue(int pairIdx) const
{
	if (pairIdx < 0 || pairIdx >= 4)
		return 0.5f;
	return parameterManager.getPairCrossfader(pairIdx);
}

void DjIaVstProcessor::setGlobalCrossfaderValue(float value)
{
	if (auto *p = parameterManager.getAPVTS().getParameter("globalCrossfader"))
		p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
}

float DjIaVstProcessor::getGlobalCrossfaderValue() const
{
	return parameterManager.getGlobalCrossfader();
}

void DjIaVstProcessor::setCrossfaderCurveMode(int mode)
{
	if (auto *p = parameterManager.getAPVTS().getParameter("crossfaderCurveMode"))
	{
		p->setValueNotifyingHost(juce::jlimit(0, 2, mode) / 2.0f);
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackCrossfaderCurve, mode, MidiMapping::feedbackChannelShaping);
	}
}

int DjIaVstProcessor::getCrossfaderCurveMode() const
{
	return parameterManager.getCrossfaderCurveMode();
}

void DjIaVstProcessor::checkIfUIUpdateNeeded(juce::MidiBuffer &midiMessages)
{
	bool anyTrackPlaying = false;
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (track && track->isPlaying.load())
		{
			anyTrackPlaying = true;
			break;
		}
	}

	if (anyTrackPlaying || midiMessages.getNumEvents() > 0)
	{
		needsUIUpdate.store(true);
	}
}

void DjIaVstProcessor::getDawInformations(juce::AudioPlayHead *currentPlayHead, bool &hostIsPlaying, double &hostBpm,
                                          double &hostPpqPosition)
{
	if (currentPlayHead == nullptr)
		return;
	double localSampleRate = getSampleRate();
	if (localSampleRate > 0.0)
	{
		hostSampleRate = localSampleRate;
	}

	if (auto positionInfo = currentPlayHead->getPosition())
	{
		hostIsPlaying = positionInfo->getIsPlaying();

		if (auto bpm = positionInfo->getBpm())
		{
			double newBpm = *bpm;
			hostBpm = newBpm;

			double oldBpm = cachedHostBpm.load();
			cachedHostBpm.store(newBpm);

			if (std::abs(newBpm - oldBpm) > 0.1)
			{
				if (onHostBpmChanged)
				{
					onHostBpmChanged(newBpm);
				}
			}
		}

		if (auto ppq = positionInfo->getPpqPosition())
		{
			hostPpqPosition = *ppq;
		}
		if (auto timeSig = positionInfo->getTimeSignature())
		{
			timeSignatureNumerator.store(timeSig->numerator);
			timeSignatureDenominator.store(timeSig->denominator);
		}
	}
}

void DjIaVstProcessor::previewTrack(const juce::String &trackId)
{
	TrackData *track = trackManager.getTrack(trackId);
	auto &currentPage = track->getCurrentPage();
	if (!track || currentPage.numSamples <= 0)
		return;

	if (track->isPreviewMode.load())
		return;

	if (track->isPlaying.load())
	{
		return;
	}

	track->readPosition = 0.0;
	track->isPlaying.store(true);
	track->isPreviewMode.store(true);
	track->previewEndPending.store(false);
	needsUIUpdate.store(true);

	if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
	{
		auto *trackComp = editor->uiTrackManager->getTrackComponent(trackId);
		if (trackComp)
			trackComp->setPreviewPlaying(true);
	}
}

void DjIaVstProcessor::playTrack(const juce::MidiMessage &message, double hostBpm)
{
	int noteNumber = message.getNoteNumber();
	juce::String noteName = juce::MidiMessage::getMidiNoteName(noteNumber, true, true, 3);
	bool trackFound = false;
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		auto &currentPage = track->getCurrentPage();
		if (track && track->midiNote == noteNumber)
		{
			if (trackId == trackIdWaitingForLoad)
			{
				correctMidiNoteReceived = true;
			}
			if (currentPage.numSamples > 0)
			{
				startNotePlaybackForTrack(trackId, noteNumber, hostBpm);
				trackFound = true;
			}
			break;
		}
	}
}

void DjIaVstProcessor::handleSampleParams(int slot, TrackData *track)
{
	jassert(slot >= 0 && slot < ParameterManager::MAX_SLOTS);
	jassert(track != nullptr);

	auto &pm = parameterManager;
	auto &currentPage = track->getCurrentPage();
	float paramVolume = pm.getVolume(slot);
	float paramPan = pm.getPan(slot);
	float paramPitch = pm.getPitch(slot) * 8;
	float paramFine = pm.getFine(slot) * 2;
	bool isSolo = pm.getSolo(slot);
	bool isMuted = pm.getMute(slot);
	float paramRandomRetrigger = pm.getRandomRetrigger(slot);
	float paramRetriggerInterval = pm.getRetriggerInterval(slot);
	int slotNumber = slot + 1;
	bool isRetriggerEnabled = paramRandomRetrigger > 0.5f;
	int retriggerInterval = juce::jlimit(1, 10, (int)juce::roundToInt(paramRetriggerInterval));

	if (std::abs(track->lastFeedbackVolume.load() - paramVolume) > 0.01f)
	{
		track->lastFeedbackVolume = paramVolume;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackVolume(slotNumber), MidiMapping::volumeToMidi(paramVolume));
	}
	if (std::abs(track->volume.load() - paramVolume) > 0.01f)
		track->volume = paramVolume;

	if (std::abs(track->lastFeedbackPan.load() - paramPan) > 0.01f)
	{
		track->lastFeedbackPan = paramPan;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackPan(slotNumber), MidiMapping::panToMidi(paramPan));
	}
	if (std::abs(track->pan.load() - paramPan) > 0.01f)
		track->pan = paramPan;

	if (std::abs(track->lastFeedbackPitch.load() - paramPitch) > 0.01f)
	{
		track->lastFeedbackPitch = paramPitch;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackPitch(slotNumber), MidiMapping::pitchToMidi(paramPitch));
	}
	if (std::abs(track->lastFeedbackFine.load() - paramFine) > 0.01f)
	{
		track->lastFeedbackFine = paramFine;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackFine(slotNumber), MidiMapping::fineToMidi(paramFine));
	}

	if (track->isSolo.load() != isSolo)
	{
		track->isSolo = isSolo;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackSolo(slotNumber),
		                             isSolo ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
	}
	if (track->isMuted.load() != isMuted)
	{
		track->isMuted = isMuted;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackMute(slotNumber),
		                             isMuted ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
	}

	if (track->lastFeedbackBeatRepeat.load() != isRetriggerEnabled)
	{
		track->lastFeedbackBeatRepeat = isRetriggerEnabled;
		midiManager.sendMidiFeedback(MidiMapping::ccFeedbackBeatRepeat(slotNumber),
		                             isRetriggerEnabled ? MidiMapping::feedbackActive : MidiMapping::feedbackIdle);
	}
	if (track->randomRetriggerEnabled.load() != isRetriggerEnabled)
	{
		track->randomRetriggerEnabled = isRetriggerEnabled;
		if (!isRetriggerEnabled)
			track->beatRepeatStopPending.store(true);
		else
			track->beatRepeatPending.store(true);
	}
	if (track->randomRetriggerInterval.load() != retriggerInterval)
	{
		track->randomRetriggerInterval = retriggerInterval;
		if (track->beatRepeatActive.load())
		{
			double hostBpm = lastHostBpmForQuantization.load();
			if (hostBpm <= 0.0)
				hostBpm = 120.0;
			double startPosition = track->beatRepeatStartPosition.load();
			double repeatDuration = sequencerManager.calculateRetriggerInterval(retriggerInterval, hostBpm);
			double repeatDurationSamples = repeatDuration * currentPage.sampleRate;
			track->beatRepeatEndPosition.store(startPosition + repeatDurationSamples);
			double maxSamples = currentPage.numSamples;
			if (track->beatRepeatEndPosition.load() > maxSamples)
				track->beatRepeatEndPosition.store(maxSamples);
		}
	}
}

void DjIaVstProcessor::handleSendsParams()
{
	auto &pm = parameterManager;
	const int ch = MidiMapping::feedbackChannelSends;

	auto pushFloatIfChanged = [&](std::atomic<float> &last, float cur, int cc)
	{
		if (std::abs(last.load() - cur) > 0.001f)
		{
			last.store(cur);
			midiManager.sendMidiFeedback(cc, MidiMapping::normalizedToMidi(cur), ch);
		}
	};

	auto pushIntIfChanged = [&](std::atomic<int> &last, int cur, int cc, int total)
	{
		if (last.load() != cur)
		{
			last.store(cur);
			midiManager.sendMidiFeedback(cc, MidiMapping::indexToMidi(cur, total), ch);
		}
	};

	pushFloatIfChanged(lastFeedbackDelayFeedback, pm.getFeedback(), MidiMapping::ccFeedbackDelayFeedback);
	pushFloatIfChanged(lastFeedbackReverbSize, pm.getReverbSize(), MidiMapping::ccFeedbackReverbSize);
	pushFloatIfChanged(lastFeedbackReverbDamping, pm.getReverbDamping(), MidiMapping::ccFeedbackReverbDamping);
	pushFloatIfChanged(lastFeedbackReverbWidth, pm.getReverbWidth(), MidiMapping::ccFeedbackReverbWidth);
	pushFloatIfChanged(lastFeedbackReverbMix, pm.getReverbMix(), MidiMapping::ccFeedbackReverbMix);

	pushIntIfChanged(lastFeedbackDelayDivision, pm.getDelayDivisionIndex(), MidiMapping::ccFeedbackDelayDivision, 8);
	pushIntIfChanged(lastFeedbackDelayMode, pm.getDelayModeIndex(), MidiMapping::ccFeedbackDelayMode, 3);
}

void DjIaVstProcessor::startNotePlaybackForTrack(const juce::String &trackId, int noteNumber, double /*hostBpm*/)
{
	TrackData *track = trackManager.getTrack(trackId);
	auto &currentPage = track->getCurrentPage();
	if (!track || currentPage.numSamples == 0)
		return;
	if (getBypassSequencer())
	{
		if (!track->beatRepeatActive.load())
		{
			track->readPosition = 0.0;
		}
		track->setPlaying(true);
		track->isCurrentlyPlaying.store(true);
		playingTracks[noteNumber] = trackId;
		return;
	}
	if (track->isArmedToStop.load())
	{
		return;
	}
	if (!track->isArmed.load() && !track->isCurrentlyPlaying.load())
	{
		return;
	}
	if (track->isPlaying.load())
	{
		return;
	}

	if (!track->beatRepeatActive.load())
	{
		track->readPosition = 0.0;
	}
	track->setPlaying(true);
	track->isCurrentlyPlaying.store(true);
	track->isArmed = false;
	playingTracks[noteNumber] = trackId;
}

void DjIaVstProcessor::stopNotePlaybackForTrack(int noteNumber)
{
	auto it = playingTracks.find(noteNumber);
	if (it != playingTracks.end())
	{
		TrackData *track = trackManager.getTrack(it->second);
		if (track)
		{
			track->isPlaying = false;
			if (!track->isArmed.load() && !track->isCurrentlyPlaying.load())
				midiManager.sendMidiFeedback(MidiMapping::ccFeedbackPlay(track->slotIndex + 1),
				                             MidiMapping::feedbackIdle);
		}
		playingTracks.erase(it);
	}
}

void DjIaVstProcessor::handleAsyncUpdate()
{
	if (!hasPendingNotification)
		return;

	hasPendingNotification = false;

	juce::MessageManager::callAsync(
	    [this]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
		    {
			    if (generationListener)
			    {
				    generationListener->onGenerationComplete(lastGeneratedTrackId, pendingMessage);
			    }
		    }
	    });
}

void DjIaVstProcessor::reloadTrackWithVersion(const juce::String &trackId, bool useOriginal)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track)
		return;

	juce::File fileToLoad;

	if (!track->getCurrentPage().hasOriginalVersion.load())
		return;

	if (useOriginal)
	{
		char pageName = static_cast<char>('A' + track->currentPageIndex.load());
		auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		                    .getChildFile("OBSIDIAN-Neural")
		                    .getChildFile("AudioCache");
		if (projectId != "legacy" && !projectId.isEmpty())
		{
			audioDir = audioDir.getChildFile(projectId);
		}
		fileToLoad = audioDir.getChildFile(trackId + "_original_" + juce::String(pageName) + ".wav");
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
			    {
				    editor->statusLabel.setText("Original file loaded...", juce::dontSendNotification);
				    editor->uiStatusManager->updateLCD();
			    }
		    });
	}
	else
	{
		fileToLoad = audioManager.getTrackPageAudioFile(trackId, track->currentPageIndex.load());
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
			    {
				    editor->statusLabel.setText("Stretched file loaded...", juce::dontSendNotification);
				    editor->uiStatusManager->updateLCD();
			    }
		    });
	}

	if (!fileToLoad.existsAsFile())
	{
		int asciiCode = 'A' + track->currentPageIndex.load();
		auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		                    .getChildFile("OBSIDIAN-Neural")
		                    .getChildFile("AudioCache");
		if (projectId != "legacy" && !projectId.isEmpty())
		{
			audioDir = audioDir.getChildFile(projectId);
		}
		if (useOriginal)
		{
			fileToLoad = audioDir.getChildFile(trackId + "_" + juce::String(asciiCode) + "_original.wav");
		}
		else
		{
			fileToLoad = audioDir.getChildFile(trackId + "_" + juce::String(asciiCode) + ".wav");
		}

		if (!fileToLoad.existsAsFile())
			return;
	}

	int currentPageIndex = track->currentPageIndex.load();
	juce::Thread::launch([this, trackId, currentPageIndex, fileToLoad]()
	                     { audioManager.loadAudioFileForPageSwitch(trackId, currentPageIndex, fileToLoad); });
}

void DjIaVstProcessor::loadPendingSample()
{
	if (hasUnloadedSample.load() && !pendingTrackId.isEmpty())
	{
		waitingForMidiToLoad = true;
		canLoad = true;
		trackIdWaitingForLoad = pendingTrackId;
	}
}

void DjIaVstProcessor::setAutoLoadEnabled(bool enabled)
{
	autoLoadEnabled.store(enabled);
}

void DjIaVstProcessor::setBypassLLM(bool bypassed)
{
	bypassLLM.store(bypassed);
}

void DjIaVstProcessor::setApiKey(const juce::String &key)
{
	apiKey = key;
	apiClient.setApiKey(apiKey);
}

void DjIaVstProcessor::setServerUrl(const juce::String &url)
{
	serverUrl = url;
	apiClient.setBaseUrl(serverUrl);
}

double DjIaVstProcessor::getHostBpm() const
{
	if (auto currentPlayHead = getPlayHead())
	{
		if (auto positionInfo = currentPlayHead->getPosition())
		{
			if (positionInfo->getBpm().hasValue())
			{
				double bpm = *positionInfo->getBpm();
				if (bpm > 0.0)
				{
					return bpm;
				}
			}
		}
	}
	return 110.0;
}

void DjIaVstProcessor::addCustomPrompt(const juce::String &prompt)
{
	if (prompt.isEmpty() || !promptBank)
		return;

	for (auto *p : promptBank->getAllPrompts())
		if (p->text == prompt)
			return;

	promptBank->addPrompt(prompt, "stable-audio-open-1.0", "");
}

const juce::StringArray DjIaVstProcessor::getCustomPrompts() const
{
	juce::StringArray result;
	if (promptBank)
	{
		auto allPrompts = const_cast<PromptBank *>(promptBank.get())->getAllPrompts();
		for (auto *p : allPrompts)
			result.add(p->text);
	}
	return result;
}

void DjIaVstProcessor::getStateInformation(juce::MemoryBlock &destData)
{
	stateManager.getStateInformation(destData);
}

void DjIaVstProcessor::setStateInformation(const void *data, int sizeInBytes)
{
	stateManager.setStateInformation(data, sizeInBytes);
}

void DjIaVstProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
	if (parameterID == "generate" && newValue > 0.5f)
	{
		juce::MessageManager::callAsync(
		    [this]() { parameterManager.getAPVTS().getParameter("generate")->setValueNotifyingHost(0.0f); });
	}
	else if (parameterID.startsWith("slot") && parameterID.contains("Page") && newValue > 0.5f)
	{
		sequencerManager.handlePageChange(parameterID);
		juce::MessageManager::callAsync(
		    [this, parameterID]()
		    {
			    if (auto *param = parameterManager.getAPVTS().getParameter(parameterID))
				    param->setValueNotifyingHost(0.0f);
		    });
	}
	else if (parameterID.startsWith("slot") && parameterID.contains("Seq") && newValue > 0.5f)
	{
		sequencerManager.handleSequenceChange(parameterID);
		juce::MessageManager::callAsync(
		    [this, parameterID]()
		    {
			    if (auto *param = parameterManager.getAPVTS().getParameter(parameterID))
				    param->setValueNotifyingHost(0.0f);
		    });
	}
	else if (parameterID.startsWith("slot") && (parameterID.endsWith("Pitch") || parameterID.endsWith("Fine")))
	{
		int slotNum = parameterID.substring(4, 5).getIntValue();
		if (slotNum < 1 || slotNum > 8)
			return;
		auto trackIds = trackManager.getAllTrackIds();
		for (const auto &tid : trackIds)
		{
			TrackData *t = trackManager.getTrack(tid);
			if (!t || t->slotIndex != slotNum - 1)
				continue;
			auto &page = t->getCurrentPage();

			if (parameterID.endsWith("Pitch"))
			{
				float paramPitch = newValue * 8.0f;
				page.bpmOffset.store(paramPitch + page.fineOffset.load());
			}
			else
			{
				float paramFine = newValue * 2.0f;
				page.fineOffset.store(paramFine * 0.05f);
				float currentPitch = parameterManager.getPitch(slotNum - 1) * 8.0f;
				page.bpmOffset.store(currentPitch + page.fineOffset.load());
			}

			if (parameterID.endsWith("Pitch"))
			{
				float paramPitch = newValue * 8.0f;
				if (std::abs(t->lastFeedbackPitch.load() - paramPitch) > 0.01f)
				{
					t->lastFeedbackPitch.store(paramPitch);
					midiManager.sendMidiFeedback(MidiMapping::ccFeedbackPitch(slotNum),
					                             MidiMapping::pitchToMidi(paramPitch));
				}
			}
			else
			{
				float paramFine = newValue * 2.0f;
				if (std::abs(t->lastFeedbackFine.load() - paramFine) > 0.01f)
				{
					t->lastFeedbackFine.store(paramFine);
					midiManager.sendMidiFeedback(MidiMapping::ccFeedbackFine(slotNum),
					                             MidiMapping::fineToMidi(paramFine));
				}
			}
			break;
		}
	}
	else if (parameterID == "globalCrossfader")
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshFromProcessor();
			    }
		    });
	}
	else if (parameterID.startsWith("pairCrossfader"))
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshFromProcessor();
			    }
		    });
	}
	else if (parameterID == "crossfaderCurveMode")
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshCurveButtons();
			    }
		    });
	}
	else if (parameterID.startsWith("slot") &&
	         (parameterID.endsWith("AdsrAttack") || parameterID.endsWith("AdsrDecay") ||
	          parameterID.endsWith("AdsrSustain") || parameterID.endsWith("AdsrRelease")))
	{
		int slotNum = parameterID.substring(4, 5).getIntValue();
		if (slotNum < 1 || slotNum > 8)
			return;

		auto trackIds = trackManager.getAllTrackIds();
		for (const auto &tid : trackIds)
		{
			TrackData *t = trackManager.getTrack(tid);
			if (!t || t->slotIndex != slotNum - 1)
				continue;

			auto &page = t->getCurrentPage();

			if (parameterID.endsWith("AdsrAttack"))
			{
				page.adsrAttack.store(newValue);
			}
			else if (parameterID.endsWith("AdsrDecay"))
			{
				page.adsrDecay.store(newValue);
			}
			else if (parameterID.endsWith("AdsrSustain"))
			{
				page.adsrSustain.store(newValue);
			}
			else if (parameterID.endsWith("AdsrRelease"))
			{
				page.adsrRelease.store(newValue);
			}
			break;
		}
	}
}

void DjIaVstProcessor::removeCustomPrompt(const juce::String &prompt)
{
	customPrompts.removeString(prompt);
	saveGlobalConfig();
}

void DjIaVstProcessor::editCustomPrompt(const juce::String &oldPrompt, const juce::String &newPrompt)
{
	int index = customPrompts.indexOf(oldPrompt);
	if (index >= 0 && !newPrompt.isEmpty() && !customPrompts.contains(newPrompt))
	{
		customPrompts.set(index, newPrompt);
		saveGlobalConfig();
	}
}
