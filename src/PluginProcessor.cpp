#include "PluginProcessor.h"
#include "AiModelDefinitions.h"
#include "DataConst.h"
#include "MidiMapping.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "SequencerComponent.h"
#include "signalsmith-stretch.h"
#if JucePlugin_Build_Standalone
#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#endif

DjIaVstProcessor::DjIaVstProcessor()
    : AudioProcessor(createBusLayout()), apiClient("", "http://localhost:8000"), parameterManager(*this),
      stateManager(*this), generationManager(*this), sequencerManager(*this, trackManager),
      audioManager(*this, trackManager, generationManager), midiManager(*this, midiLearnManager),
      obsidianEngine(std::make_unique<ObsidianEngine>()), sampleBank(std::make_unique<SampleBank>()),
      autoLoadEnabled(true)
{
	midiLearnManager.setProcessor(this);
	parameterManager.resolveParameters(this);
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

	audioManager.initDummySynth();
	audioManager.initBuffers(ObsidianDataConst::MAX_TRACKS);

	trackManager.onPreviewEnded = [this](const juce::String &trackId)
	{ juce::MessageManager::callAsync([this, trackId]() { audioManager.stopTrackPreview(trackId); }); };

	static auto safeCallback = std::make_shared<std::function<void(int, TrackData *)>>(
	    [this](int slot, TrackData *track)
	    {
		    parameterManager.handleSampleParams(slot, track);
		    parameterManager.handleSendsParams();
	    });
	trackManager.parameterUpdateCallback.store(safeCallback.get());

	startTimerHz(30);
	autoLoadEnabled.store(true);
#if JucePlugin_Build_Standalone
	link.reset(new ableton::Link{120});
	link->setTempoCallback([this](const double p) { currentBpm.store(p); });
	link->enable(false);
	link->enableStartStopSync(false);
#endif

	sampleBank->onCheckCategoryExists = [this](const juce::String &name) -> bool
	{
		if (promptBank == nullptr)
			return false;
		for (const auto &c : promptBank->getCategories())
			if (c.name.compareIgnoreCase(name) == 0)
				return true;
		return false;
	};

	sampleBank->onMigrateLegacyCategory = [this](const juce::String &name, juce::Colour colour)
	{
		if (promptBank == nullptr)
			return;
		juce::MessageManager::callAsync(
		    [this, name, colour]()
		    {
			    if (promptBank != nullptr)
				    promptBank->addCategory(name, colour);
		    });
	};

	juce::Thread::launch(
	    [this]()
	    {
		    sampleBank->runLegacyCategoriesMigration();
		    heavyInitDone.store(true);
	    });

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		standaloneTransport = std::make_unique<StandaloneTransport>();
		setPlayHead(standaloneTransport.get());
	}

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

void DjIaVstProcessor::cleanProcessor()
{
	isShuttingDown.store(true);
	apiClient.cancelPendingRequests();
#if JucePlugin_Build_Standalone
	if (link->isEnabled())
		link->enable(false);
#endif
	threadPool.removeAllJobs(true, 5000);

	parameterManager.removeAllListeners(this);

	isNotePlaying = false;
	hasPendingAudioData = false;
	hasUnloadedSample = false;

	midiManager.setMidiIndicatorCallback(nullptr);
	trackManager.parameterUpdateCallback.store(nullptr);

	audioManager.releaseResources();
	obsidianEngine.reset();
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

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (standaloneTransport)
			setPlayHead(standaloneTransport.get());
	}

	masterConsoleBuss.prepare(sampleRate);
	trackManager.prepareSends(sampleRate, samplesPerBlock);
}

void DjIaVstProcessor::releaseResources()
{
	audioManager.releaseResources();
}

#if JucePlugin_Build_Standalone
void DjIaVstProcessor::calculateOutputTime(const double sample_rate, const int buffer_size)
{
	const auto host_time = host_time_filter.sampleTimeToHostTime(static_cast<double>(sample_time));
	const auto output_latency = std::chrono::microseconds{std::llround(1.0e6 * buffer_size / sample_rate)};
	output_time = output_latency + host_time;
}
#endif

juce::AudioProcessor::BusesProperties DjIaVstProcessor::createBusLayout()
{
	auto layout = juce::AudioProcessor::BusesProperties();
	layout = layout.withOutput("Main", juce::AudioChannelSet::stereo(), true);
	for (int i = 0; i < ObsidianDataConst::MAX_TRACKS + 1; ++i)
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

	for (int i = 0; i < ObsidianDataConst::MAX_TRACKS; ++i)
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

			for (int p = 0; p < ObsidianDataConst::MAX_PAGES; ++p)
			{
				auto &page = track->pages[p];
				page.selectedModel = modelName;
				page.prompt = promptForThisModel;
				page.generationPrompt = promptForThisModel;
				page.setSelectedPrompt(promptForThisModel);
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
		if (isLoadingState.load())
			return;
		juce::MessageManager::callAsync(
		    [this, weakTrack]()
		    {
			    if (isLoadingState.load())
				    return;
			    auto *t = weakTrack.get();
			    if (!t || t->slotIndex < 0 || t->slotIndex >= 8)
				    return;
			    const auto &page = t->getCurrentPage();
			    juce::String s = "slot" + juce::String(t->slotIndex + 1);
			    auto &apvts = parameterManager.getAPVTS();

			    auto pitchRange = apvts.getParameterRange(s + "Pitch");
			    auto fineRange = apvts.getParameterRange(s + "Fine");
			    auto gainRange = apvts.getParameterRange(s + "Gain");

			    if (auto *p = apvts.getParameter(s + "Pitch"))
				    p->setValueNotifyingHost(pitchRange.convertTo0to1(page.pitchSemitones.load()));

			    if (auto *p = apvts.getParameter(s + "Fine"))
				    p->setValueNotifyingHost(fineRange.convertTo0to1(page.fineOffset.load()));

			    if (auto *p = apvts.getParameter(s + "Gain"))
				    p->setValueNotifyingHost(gainRange.convertTo0to1(page.gain.load()));
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

#if JucePlugin_Build_Standalone
ableton::Link::SessionState DjIaVstProcessor::processSessionState(const EngineData &engine_data)
{
	auto sessionState = link->captureAudioSessionState();

	if (engine_data.request_start)
	{
		sessionState.setIsPlaying(true, output_time);
	}
	if (engine_data.request_stop)
	{
		sessionState.setIsPlaying(false, output_time);
	}

	if (!isLinkPlaying.load() && sessionState.isPlaying())
	{
		sessionState.requestBeatAtTime(0., output_time, engine_data.quantum);
		isLinkPlaying.store(true);
	}
	else if (isLinkPlaying.load() && !sessionState.isPlaying())
	{
		isLinkPlaying.store(false);
	}

	if (engine_data.requested_bpm > 0)
	{
		sessionState.setTempo(engine_data.requested_bpm, output_time);
	}

	link->commitAudioSessionState(sessionState);
	return sessionState;
}

DjIaVstProcessor::EngineData DjIaVstProcessor::pull_engine_data()
{
	auto engine_data = EngineData{};
	if (engine_data_guard.try_lock())
	{
		engine_data.requested_bpm = shared_engine_data.requested_bpm;
		shared_engine_data.requested_bpm = 0;

		engine_data.request_start = shared_engine_data.request_start;
		shared_engine_data.request_start = false;

		engine_data.request_stop = shared_engine_data.request_stop;
		shared_engine_data.request_stop = false;

		lock_free_engine_data.quantum = shared_engine_data.quantum;
		lock_free_engine_data.startstop_sync = shared_engine_data.startstop_sync;

		engine_data_guard.unlock();
	}

	engine_data.quantum = lock_free_engine_data.quantum;
	return engine_data;
}

void DjIaVstProcessor::requestLinkStart()
{
	std::lock_guard<std::mutex> lock(engine_data_guard);
	shared_engine_data.request_start = true;
}

void DjIaVstProcessor::requestLinkStop()
{
	std::lock_guard<std::mutex> lock(engine_data_guard);
	shared_engine_data.request_stop = true;
}

void DjIaVstProcessor::setLinkQuantum(double q)
{
	std::lock_guard<std::mutex> lock(engine_data_guard);
	shared_engine_data.quantum = q;
}

void DjIaVstProcessor::setLinkTempo(double bpm)
{
	std::lock_guard<std::mutex> lock(engine_data_guard);
	shared_engine_data.requested_bpm = bpm;
}
#endif

void DjIaVstProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
#if JucePlugin_Build_Standalone
	if (juce::JUCEApplicationBase::isStandaloneApp())
		if (link->isEnabled())
		{
			calculateOutputTime(getSampleRate(), buffer.getNumSamples());
			const auto engine_data = pull_engine_data();
			const auto localSession = processSessionState(engine_data);

			const bool linkIsPlaying = localSession.isPlaying();
			const double linkBpm = localSession.tempo();
			const double linkBeat = std::max(0.0, localSession.beatAtTime(output_time, engine_data.quantum));

			if (standaloneTransport)
			{
				standaloneTransport->setPlaying(linkIsPlaying);
				standaloneTransport->setBpm(linkBpm);
				standaloneTransport->setPpqPosition(linkBeat);
			}
		}
		else
		{
			standaloneTransport->advance(buffer.getNumSamples(), getSampleRate());
		}
#endif

	sequencerManager.internalSampleCounter += buffer.getNumSamples();
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
		buffer.clear(i, 0, buffer.getNumSamples());
	auto currentPlayHead = getPlayHead();
	double hostBpm = 126.0;
	double hostPpqPosition = 0.0;
	bool hostIsPlaying = false;

	if (currentPlayHead != nullptr)
		getDawInformations(currentPlayHead, hostIsPlaying, hostBpm, hostPpqPosition);

	lastHostBpmForQuantization.store(hostBpm);
	if (hasPendingAudioData.load())
	{
		audioManager.processIncomingAudio(hostIsPlaying);
	}
	audioManager.checkAndSwapStagingBuffers();
	sequencerManager.handleSequencerPlayState(hostIsPlaying);
	sequencerManager.updateSequencers(hostIsPlaying, buffer.getNumSamples());
	sequencerManager.checkBeatRepeatWithSampleCounter();
	sequencerManager.flushMidiBuffer(midiMessages, buffer.getNumSamples());

	midiManager.processMidiMessages(midiMessages, hostIsPlaying, hostBpm);
	midiManager.flushFeedbackBuffer(midiMessages, buffer.getNumSamples());
	audioManager.resizeIndividualBuffers(buffer);
	audioManager.clearOutputBuffers(buffer);
	auto mainOutput = getBusBuffer(buffer, false, 0);
	mainOutput.clear();
	auto previewBus = getBusBuffer(buffer, false, getBusCount(false) - 1);
	previewBus.clear();
	float pairCurrent[4];
	float pairPrev[4];
	for (int i = 0; i < ObsidianDataConst::MAX_CROSSFADER_PAIR; ++i)
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
	                             pairCurrent, globalPrev, globalCurrent, curveMode, timeSignatureNumerator.load(),
	                             timeSignatureDenominator.load(), getSampleRate());

	trackManager.processPerTrackDelays(
	    audioManager.getIndividualOutputBuffers(), mainOutput, hostBpm,
	    static_cast<DelaySend::TimeDivision>(parameterManager.getDelayDivisionIndex()), parameterManager.getFeedback(),
	    static_cast<DelaySend::Mode>(parameterManager.getDelayModeIndex()), buffer.getNumSamples());

	trackManager.processPerTrackReverbs(audioManager.getIndividualOutputBuffers(), mainOutput,
	                                    parameterManager.getReverbSize(), parameterManager.getReverbDamping(),
	                                    parameterManager.getReverbWidth(), parameterManager.getReverbMix(),
	                                    buffer.getNumSamples());

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
	sample_time += buffer.getNumSamples();
}

float DjIaVstProcessor::getPairCrossfaderValue(int pairIdx) const
{
	if (pairIdx < 0 || pairIdx >= 4)
		return 0.5f;
	return parameterManager.getPairCrossfader(pairIdx);
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

	track->readPosition.store(0.0);
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
			track->readPosition.store(0.0);
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
		track->readPosition.store(0.0);
	}
	track->setPlaying(true);
	track->isCurrentlyPlaying.store(true);
	track->isArmed.store(false);
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
			track->isPlaying.store(false);
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

TrackData *DjIaVstProcessor::getTrackFromParamId(const juce::String &parameterID)
{
	if (!parameterID.startsWith("slot"))
		return nullptr;

	int slotNum = parameterID.substring(4, 5).getIntValue();
	if (slotNum < 1 || slotNum > ObsidianDataConst::MAX_TRACKS)
		return nullptr;

	for (const auto &tid : getAllTrackIds())
	{
		TrackData *t = getTrack(tid);
		if (t && t->slotIndex == slotNum - 1)
			return t;
	}
	return nullptr;
}

void DjIaVstProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
	parameterManager.parameterChanged(parameterID, newValue);
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
