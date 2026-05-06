#include "AudioManager.h"
#include "AudioAnalyzer.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

AudioManager::AudioManager(DjIaVstProcessor &processor, TrackManager &trackManager,
                           GenerationManager &generationManager)
    : audioProcessor(processor), trackManager(trackManager), generationManager(generationManager)
{
}

void AudioManager::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	hostSampleRate = newSampleRate;
	blockSize = samplesPerBlock;
	synth.setCurrentPlaybackSampleRate(newSampleRate);
	for (auto &buffer : individualOutputBuffers)
	{
		buffer.setSize(2, samplesPerBlock);
		buffer.clear();
	}
	masterEQ.prepare(newSampleRate, samplesPerBlock);
}

void AudioManager::releaseResources()
{
	for (auto &buffer : individualOutputBuffers)
	{
		buffer.setSize(0, 0);
	}
}

void AudioManager::initBuffers(int numTracks)
{
	individualOutputBuffers.resize(numTracks);
	for (auto &buffer : individualOutputBuffers)
		buffer.setSize(2, 512);

	previewBuffer.setSize(2, 512);
}

void AudioManager::initDummySynth()
{
	for (int i = 0; i < 4; ++i)
		synth.addVoice(new DummyVoice());
	synth.addSound(new DummySound());
}

void AudioManager::processIncomingAudio(bool hostIsPlaying)
{
	if (!audioProcessor.getHasPendingAudioData())
	{
		return;
	}
	if (audioProcessor.getPendingTrackId().isEmpty())
	{
		return;
	}

	TrackData *track = trackManager.getTrack(audioProcessor.getPendingTrackId());
	if (!track)
	{
		return;
	}
	if (audioProcessor.getWaitingForMidiToLoad() && !audioProcessor.getCorrectMidiNoteReceived() && hostIsPlaying &&
	    track->isPlaying.load())
	{
		return;
	}
	if (!audioProcessor.getCanLoad() && !audioProcessor.getAutoLoadEnabled())
	{
		audioProcessor.setHasUnloadedSample(true);
		return;
	}

	juce::MessageManager::callAsync(
	    [this]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		    {
			    editor->statusLabel.setText("Loading sample...", juce::dontSendNotification);
			    editor->updateLCD();
		    }
	    });

	juce::Thread::launch(
	    [this, trackId = audioProcessor.getPendingTrackId(), audioFile = audioProcessor.getPendingAudioFile()]()
	    { loadAudioFileAsync(trackId, audioFile); });

	generationManager.clearPendingAudio();
	audioProcessor.setHasUnloadedSample(false);
	audioProcessor.setWaitingForMidiToLoad(false);
	audioProcessor.setCorrectMidiNoteReceived(false);
	audioProcessor.setCanLoad(false);
	audioProcessor.clearTrackIdWaitingForLoad();
}

void AudioManager::applyMasterEffects(juce::AudioSampleBuffer &mainOutput)
{
	updateMasterEQ();
	masterEQ.processBlock(mainOutput);
	auto &pm = audioProcessor.getParameterManager();
	float targetVol = pm.getMasterVolume();
	float targetPan = pm.getMasterPan();

	const float smoothingCoeff = 0.95f;
	smoothedMasterVol = smoothedMasterVol * smoothingCoeff + targetVol * (1.0f - smoothingCoeff);
	smoothedMasterPan = smoothedMasterPan * smoothingCoeff + targetPan * (1.0f - smoothingCoeff);

	mainOutput.applyGain(smoothedMasterVol);

	if (mainOutput.getNumChannels() >= 2 && std::abs(smoothedMasterPan) > 0.01f)
	{
		if (smoothedMasterPan < 0.0f)
		{
			mainOutput.applyGain(1, 0, mainOutput.getNumSamples(), 1.0f + smoothedMasterPan);
		}
		else
		{
			mainOutput.applyGain(0, 0, mainOutput.getNumSamples(), 1.0f - smoothedMasterPan);
		}
	}
}

void AudioManager::copyToIndividualOutputs(juce::AudioSampleBuffer &buffer)
{
	for (int busIndex = 1; busIndex < audioProcessor.getTotalNumOutputChannels() / 2; ++busIndex)
	{
		if (busIndex * 2 + 1 < audioProcessor.getTotalNumOutputChannels())
		{
			auto busBuffer = audioProcessor.getBusBuffer(buffer, false, busIndex);

			int trackIndex = busIndex - 1;
			if (trackIndex < individualOutputBuffers.size())
			{
				for (int ch = 0; ch < std::min(busBuffer.getNumChannels(), 2); ++ch)
				{
					busBuffer.copyFrom(ch, 0, individualOutputBuffers[trackIndex], ch, 0, buffer.getNumSamples());
				}
			}
		}
	}
}

void AudioManager::clearOutputBuffers(juce::AudioSampleBuffer &buffer)
{
	for (int busIndex = 0; busIndex < audioProcessor.getTotalNumOutputChannels() / 2; ++busIndex)
	{
		if (busIndex * 2 + 1 < audioProcessor.getTotalNumOutputChannels() && busIndex <= MAX_SLOTS)
		{
			auto busBuffer = audioProcessor.getBusBuffer(buffer, false, busIndex);
			busBuffer.clear();
		}
	}
}

void AudioManager::resizeIndividualBuffers(juce::AudioSampleBuffer &buffer)
{
	for (auto &indivBuffer : individualOutputBuffers)
	{
		if (indivBuffer.getNumSamples() != buffer.getNumSamples())
		{
			indivBuffer.setSize(2, buffer.getNumSamples(), false, false, true);
		}
		indivBuffer.clear();
	}
}

void AudioManager::loadAudioToStaging(std::unique_ptr<juce::AudioFormatReader> &reader, TrackData *track)
{
	int numChannels = reader->numChannels;
	int numSamples = static_cast<int>(reader->lengthInSamples);
	double sampleRate = reader->sampleRate;

	track->stagingBuffer.setSize(2, numSamples, false, false, true);
	track->stagingBuffer.clear();

	reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

	if (numChannels == 1)
	{
		track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
	}

	track->stagingNumSamples = numSamples;
	track->stagingSampleRate = sampleRate;
}

void AudioManager::checkAndSwapStagingBuffers()
{
	auto trackIds = trackManager.getAllTrackIds();

	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (!track)
			continue;
		if (track->swapRequested.exchange(false))
		{
			if (track->hasStagingData.load())
			{
				performAtomicSwap(track, trackId);
			}
		}
	}
}

void AudioManager::performAtomicSwap(TrackData *track, const juce::String &trackId)
{
	int targetPageIndex = track->stagingTargetPageIndex.load();
	if (targetPageIndex < 0 || targetPageIndex >= 4)
		targetPageIndex = track->currentPageIndex.load();

	auto &targetPage = track->pages[targetPageIndex];
	bool preservedHasOriginal = targetPage.hasOriginalVersion.load();

	std::swap(targetPage.audioBuffer, track->stagingBuffer);
	targetPage.numSamples = track->stagingNumSamples.load();
	targetPage.sampleRate = track->stagingSampleRate.load();
	targetPage.originalBpm = targetPage.stagingOriginalBpm;
	targetPage.isLoaded = true;

	if (track->isVersionSwitch.load())
	{
		targetPage.hasOriginalVersion.store(preservedHasOriginal);
		targetPage.loopStart = track->preservedLoopStart;
		targetPage.loopEnd = track->preservedLoopEnd;
		targetPage.loopPointsLocked = track->preservedLoopLocked.load();
		double maxDuration = targetPage.numSamples / targetPage.sampleRate;
		targetPage.loopEnd = std::min(targetPage.loopEnd, maxDuration);
		targetPage.loopStart = std::min(targetPage.loopStart, targetPage.loopEnd);
		track->isVersionSwitch.store(false);
	}
	else
	{
		targetPage.hasOriginalVersion.store(track->nextHasOriginalVersion.load());
		targetPage.useOriginalFile = false;
		double sampleDuration = targetPage.numSamples / targetPage.sampleRate;
		if (sampleDuration <= 8.0)
		{
			targetPage.loopStart = 0.0;
			targetPage.loopEnd = sampleDuration;
		}
		else
		{
			double beatDuration = 60.0 / targetPage.originalBpm;
			double fourBars = beatDuration * 16.0;
			targetPage.loopStart = 0.0;
			targetPage.loopEnd = std::min(fourBars, sampleDuration);
		}
	}

	track->stagingTargetPageIndex.store(-1);

	juce::MessageManager::callAsync([this, trackId]() { updateWaveformDisplay(trackId); });
	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		juce::MessageManager::callAsync([editor, trackId]() { editor->onSampleLoaded(trackId); });
	}
}

void AudioManager::updateWaveformDisplay(const juce::String &trackId)
{
	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		for (auto &trackComp : editor->getTrackComponents())
		{
			if (trackComp->getTrackId() == trackId)
			{
				if (trackComp->isWaveformVisible())
				{
					trackComp->refreshWaveformDisplay();
				}
				break;
			}
		}
	}
}

void AudioManager::updateTimeStretchRatios(double hostBpm)
{
	auto trackIds = trackManager.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (!track)
			continue;

		auto &currentPage = track->getCurrentPage();

		double ratio = 1.0;

		switch (track->timeStretchMode)
		{
		case 1:
		case 3:
			ratio = 1.0;
			break;

		case 2:
		case 4:
			if (currentPage.originalBpm > 0.0f && hostBpm > 0.0)
			{
				double hostRatio = hostBpm / currentPage.originalBpm;
				double manualAdjust = currentPage.bpmOffset / currentPage.originalBpm;
				ratio = hostRatio + manualAdjust;
			}
			break;
		}

		ratio = juce::jlimit(0.25, 4.0, ratio);
		track->cachedPlaybackRatio = ratio;
	}
}

void AudioManager::processAudioBPMAndSync(TrackData *track)
{
	track->nextHasOriginalVersion.store(false);
	auto &currentPage = track->getCurrentPage();
	float serverDetectedBpm = audioProcessor.getPendingDetectedBpm();
	float soundTouchDetectedBpm = AudioAnalyzer::detectBPM(track->stagingBuffer, track->stagingSampleRate);
	double hostBpm = audioProcessor.getCachedHostBpm();

	float correctedServerBpm = serverDetectedBpm;
	float correctedSoundTouchBpm = soundTouchDetectedBpm;

	if (hostBpm > 0)
	{
		double directTolerance = 20.0;
		double halfDoubleTolerance = hostBpm * 0.2;

		if (serverDetectedBpm > 0.0f)
		{
			float directDiff = std::abs(serverDetectedBpm - static_cast<float>(hostBpm));
			float halfDiff = std::abs(serverDetectedBpm * 2.0f - static_cast<float>(hostBpm));
			float doubleDiff = std::abs(serverDetectedBpm / 2.0f - static_cast<float>(hostBpm));

			if (directDiff <= directTolerance)
			{
				correctedServerBpm = serverDetectedBpm;
			}
			else if (halfDiff < directDiff && halfDiff <= halfDoubleTolerance)
			{
				correctedServerBpm = serverDetectedBpm * 2.0f;
			}
			else if (doubleDiff < directDiff && doubleDiff <= halfDoubleTolerance)
			{
				correctedServerBpm = serverDetectedBpm / 2.0f;
			}
			else
			{
				correctedServerBpm = serverDetectedBpm;
			}
		}

		if (soundTouchDetectedBpm > 0.0f)
		{
			float directDiff = std::abs(soundTouchDetectedBpm - static_cast<float>(hostBpm));
			float halfDiff = std::abs(soundTouchDetectedBpm * 2.0f - static_cast<float>(hostBpm));
			float doubleDiff = std::abs(soundTouchDetectedBpm / 2.0f - static_cast<float>(hostBpm));

			if (directDiff <= directTolerance)
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm;
			}
			else if (halfDiff < directDiff && halfDiff <= halfDoubleTolerance)
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm * 2.0f;
			}
			else if (doubleDiff < directDiff && doubleDiff <= halfDoubleTolerance)
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm / 2.0f;
			}
			else
			{
				correctedSoundTouchBpm = soundTouchDetectedBpm;
			}
		}
	}

	float detectedBPM;

	if (serverDetectedBpm > 0.0f)
	{
		detectedBPM = correctedServerBpm;
	}
	else
	{
		detectedBPM = correctedSoundTouchBpm;
	}

	audioProcessor.setPendingDetectedBpm(-1.0f);

	bool bpmValid = (detectedBPM > 60.0f && detectedBPM < 200.0f);
	currentPage.stagingOriginalBpm = bpmValid ? detectedBPM : static_cast<float>(hostBpm);

	double bpmDifference = std::abs(hostBpm - currentPage.stagingOriginalBpm);
	bool hostBpmValid = (hostBpm > 0.0);
	bool originalBpmValid = (currentPage.stagingOriginalBpm > 0.0f);
	bool bpmDifferenceSignificant = (bpmDifference > 0.01 && bpmDifference < 5.0);

	if ((hostBpmValid && originalBpmValid && bpmDifferenceSignificant) || audioProcessor.getUseLocalModel())
	{
		track->originalStagingBuffer.makeCopyOf(track->stagingBuffer);
		double stretchRatio = hostBpm / static_cast<double>(currentPage.stagingOriginalBpm);
		AudioAnalyzer::timeStretchBufferHQ(track->stagingBuffer, stretchRatio, track->stagingSampleRate);
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		currentPage.stagingOriginalBpm = static_cast<float>(hostBpm);
		track->nextHasOriginalVersion.store(true);
	}
	else
	{
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		currentPage.stagingOriginalBpm = static_cast<float>(hostBpm);
		track->nextHasOriginalVersion.store(false);
	}
}

void AudioManager::updateMasterEQ()
{
	auto &pm = audioProcessor.getParameterManager();
	masterEQ.setHighGain(pm.getMasterHigh());
	masterEQ.setMidGain(pm.getMasterMid());
	masterEQ.setLowGain(pm.getMasterLow());
}

void AudioManager::saveBufferToFile(const juce::AudioBuffer<float> &buffer, const juce::File &outputFile,
                                    double sampleRate)
{
	if (buffer.getNumSamples() == 0)
	{
		return;
	}

	juce::WavAudioFormat wavFormat;
	if (outputFile.exists())
	{
		outputFile.deleteFile();
	}

	juce::FileOutputStream *fileStream = new juce::FileOutputStream(outputFile);
	if (!fileStream->openedOk())
	{
		delete fileStream;
		return;
	}

	std::unique_ptr<juce::AudioFormatWriter> writer(
	    wavFormat.createWriterFor(fileStream, sampleRate, buffer.getNumChannels(), 16, {}, 0));
	if (writer == nullptr)
	{
		delete fileStream;
		return;
	}

	if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
	{
		writer.reset();
		return;
	}
	writer.reset();

	auto *bank = audioProcessor.getSampleBank();

	if (bank && outputFile.getFileName().endsWith(".wav") && !audioProcessor.getIsLoadingFromBank())
	{
		juce::String filename = outputFile.getFileNameWithoutExtension();

		if (filename.contains("_original"))
		{
			return;
		}

		juce::String trackId = filename;

		for (char page = 'A'; page <= 'D'; ++page)
		{
			juce::String pageSuffix = "_" + juce::String::charToString(page);
			if (trackId.endsWith(pageSuffix))
			{
				trackId = trackId.dropLastCharacters(2);
				break;
			}
		}

		for (int asciiCode = 65; asciiCode <= 68; ++asciiCode)
		{
			juce::String asciiSuffix = "_" + juce::String(asciiCode);
			if (trackId.endsWith(asciiSuffix))
			{
				trackId = trackId.dropLastCharacters(asciiSuffix.length());
				break;
			}
		}

		if (trackId == audioProcessor.getCurrentBankLoadTrackId())
		{
			return;
		}

		TrackData *track = trackManager.getTrack(trackId);
		if (!track)
		{
			return;
		}

		juce::String prompt;
		float bpm = 126.0f;
		juce::String key = "Unknown";
		juce::String modelName;

		auto &currentPage = track->getCurrentPage();
		prompt = currentPage.generationPrompt;
		if (prompt.isEmpty())
			prompt = currentPage.selectedPrompt;
		bpm = currentPage.generationBpm > 0 ? currentPage.generationBpm : currentPage.originalBpm;
		key = currentPage.generationKey.isEmpty() ? "Unknown" : currentPage.generationKey;
		modelName = currentPage.selectedModel;

		if (prompt.isEmpty())
		{
			return;
		}

		if (!track->currentSampleId.isEmpty())
		{
			bank->markSampleAsUnused(track->currentSampleId, audioProcessor.getProjectId());
		}

		juce::String sampleId = bank->addSample(prompt, outputFile, bpm, key, modelName);

		if (!sampleId.isEmpty())
		{
			bank->markSampleAsUsed(sampleId, audioProcessor.getProjectId());
			track->currentSampleId = sampleId;

			track->getCurrentPage().generationPrompt = "";
		}
	}
}

void AudioManager::saveOriginalAndStretchedBuffers(const juce::AudioBuffer<float> &originalBuffer,
                                                   const juce::AudioBuffer<float> &stretchedBuffer,
                                                   const juce::String &trackId, double sampleRate)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                    .getChildFile("OBSIDIAN-Neural")
	                    .getChildFile("AudioCache");

	if (audioProcessor.getProjectId() != "legacy" && !audioProcessor.getProjectId().isEmpty())
	{
		audioDir = audioDir.getChildFile(audioProcessor.getProjectId());
	}
	audioDir.createDirectory();

	TrackData *track = trackManager.getTrack(trackId);

	juce::File originalFile;
	juce::File stretchedFile;

	char pageName = static_cast<char>('A' + track->currentPageIndex.load());
	originalFile = audioDir.getChildFile(trackId + "_original_" + juce::String(pageName) + ".wav");
	stretchedFile = audioDir.getChildFile(trackId + "_" + juce::String(pageName) + ".wav");

	saveBufferToFile(originalBuffer, originalFile, sampleRate);
	saveBufferToFile(stretchedBuffer, stretchedFile, sampleRate);
}

void AudioManager::loadAudioFileForSwitch(const juce::String &trackId, const juce::File &audioFile)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track)
		return;

	auto &currentPage = track->getCurrentPage();
	double preservedLoopStart = currentPage.loopStart;
	double preservedLoopEnd = currentPage.loopEnd;
	bool preservedLocked = currentPage.loopPointsLocked.load();

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));

		if (!reader)
			return;
		loadAudioToStaging(reader, track);
		track->isVersionSwitch.store(true);
		track->preservedLoopStart = preservedLoopStart;
		track->preservedLoopEnd = preservedLoopEnd;
		track->preservedLoopLocked.store(preservedLocked);
		track->hasStagingData = true;
		track->swapRequested = true;

		juce::MessageManager::callAsync([this, trackId]() { updateWaveformDisplay(trackId); });
	}
	catch (const std::exception &)
	{
		currentPage.loopStart = preservedLoopStart;
		currentPage.loopEnd = preservedLoopEnd;
		currentPage.loopPointsLocked = preservedLocked;
	}
}

void AudioManager::loadAudioFileForPageSwitch(const juce::String &trackId, int pageIndex, const juce::File &audioFile)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto &page = track->pages[pageIndex];
	double preservedLoopStart = page.loopStart;
	double preservedLoopEnd = page.loopEnd;
	bool preservedLocked = page.loopPointsLocked.load();

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
		if (!reader)
			return;
		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		track->stagingBuffer.setSize(2, numSamples);
		reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

		if (numChannels == 1)
		{
			track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
		}

		track->stagingNumSamples = numSamples;
		track->stagingSampleRate = reader->sampleRate;

		track->isVersionSwitch.store(true);
		track->preservedLoopStart = preservedLoopStart;
		track->preservedLoopEnd = preservedLoopEnd;
		track->preservedLoopLocked.store(preservedLocked);

		if (pageIndex == track->currentPageIndex.load())
		{
			track->hasStagingData = true;
			track->swapRequested = true;
		}
		else
		{
			page.audioBuffer.makeCopyOf(track->stagingBuffer);
			page.numSamples = numSamples;
			page.sampleRate = reader->sampleRate;
			page.isLoaded = true;
		}

		juce::MessageManager::callAsync([this, trackId, pageIndex]() { updateWaveformDisplay(trackId); });
	}
	catch (const std::exception &)
	{
		page.loopStart = preservedLoopStart;
		page.loopEnd = preservedLoopEnd;
		page.loopPointsLocked = preservedLocked;
	}
}

void AudioManager::loadSampleToBankPage(const juce::String &trackId, int pageIndex, const juce::File &sampleFile,
                                        const juce::String &sampleId)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track || pageIndex < 0 || pageIndex >= 4)
		return;

	auto &page = track->pages[pageIndex];

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
		if (!reader)
			return;

		int numChannels = reader->numChannels;
		int numSamples = static_cast<int>(reader->lengthInSamples);

		track->stagingBuffer.setSize(2, numSamples);
		reader->read(&track->stagingBuffer, 0, numSamples, 0, true, true);

		if (numChannels == 1)
		{
			track->stagingBuffer.copyFrom(1, 0, track->stagingBuffer, 0, 0, numSamples);
		}
		auto &currentPage = track->getCurrentPage();

		track->stagingNumSamples = numSamples;
		track->stagingSampleRate = reader->sampleRate;
		currentPage.stagingOriginalBpm = 126.0f;

		processAudioBPMAndSync(track);

		auto permanentFile = getTrackPageAudioFile(trackId, pageIndex);
		permanentFile.getParentDirectory().createDirectory();

		if (track->nextHasOriginalVersion.load())
		{
			auto originalFile = getTrackPageAudioFile(trackId + "_original", pageIndex);
			auto stretchedFile = getTrackPageAudioFile(trackId, pageIndex);
			saveBufferToFile(track->originalStagingBuffer, originalFile, track->stagingSampleRate);
			saveBufferToFile(track->stagingBuffer, stretchedFile, track->stagingSampleRate);
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate);
		}

		page.audioFilePath = permanentFile.getFullPathName();
		page.numSamples = track->stagingNumSamples.load();
		page.sampleRate = track->stagingSampleRate.load();
		page.originalBpm = page.stagingOriginalBpm;
		page.isLoaded = true;
		page.isLoading = false;

		auto *bank = audioProcessor.getSampleBank();
		auto *sampleEntry = bank->getSample(sampleId);
		if (sampleEntry)
		{
			page.prompt = sampleEntry->originalPrompt;
			page.selectedPrompt = sampleEntry->originalPrompt;
			page.generationBpm = sampleEntry->bpm;
			page.generationKey = sampleEntry->key;
		}

		if (pageIndex == track->currentPageIndex.load())
		{
			track->hasStagingData = true;
			track->swapRequested = true;
		}

		juce::MessageManager::callAsync(
		    [this, trackId, pageIndex]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    editor->setStatusWithTimeout("Sample loaded to page " + juce::String((char)('A' + pageIndex)) +
				                                 "!");
				    TrackData *track = trackManager.getTrack(trackId);
				    if (track && pageIndex == track->currentPageIndex.load())
				    {
					    for (auto &trackComp : editor->getTrackComponents())
					    {
						    if (trackComp->getTrackId() == trackId)
						    {
							    trackComp->updateFromTrackData();
							    if (trackComp->isWaveformVisible())
							    {
								    trackComp->refreshWaveformDisplay();
							    }
							    break;
						    }
					    }
				    }
			    }
		    });
	}
	catch (const std::exception & /*e*/)
	{
	}
}

juce::File AudioManager::getExportDirectory()
{
	auto documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
	auto exportDir = documentsDir.getChildFile("OBSIDIAN_Exports");

	if (!exportDir.exists())
		exportDir.createDirectory();

	return exportDir;
}

juce::File AudioManager::exportSampleForDragDrop(const juce::File &originalFile)
{
	if (!originalFile.existsAsFile())
		return juce::File();

	auto exportDir = getExportDirectory();

	auto now = juce::Time::getCurrentTime();
	juce::String timestamp = now.formatted("%Y%m%d_%H%M%S");

	juce::String baseName = originalFile.getFileNameWithoutExtension();
	juce::String extension = originalFile.getFileExtension();
	juce::String newFileName = baseName + "_" + timestamp + extension;

	auto exportFile = exportDir.getChildFile(newFileName);

	if (originalFile.copyFileTo(exportFile))
	{
		return exportFile;
	}

	return juce::File();
}

juce::File AudioManager::getTrackPageAudioFile(const juce::String &trackId, int pageIndex)
{
	auto audioDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                    .getChildFile("OBSIDIAN-Neural")
	                    .getChildFile("AudioCache");
	if (audioProcessor.getProjectId() != "legacy" && !audioProcessor.getProjectId().isEmpty())
	{
		audioDir = audioDir.getChildFile(audioProcessor.getProjectId());
	}

	char pageName = static_cast<char>('A' + pageIndex);
	juce::String filename = trackId + "_" + juce::String(pageName) + ".wav";
	return audioDir.getChildFile(filename);
}

void AudioManager::loadAudioFileAsync(const juce::String &trackId, const juce::File &audioFile)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track)
		return;

	try
	{
		std::unique_ptr<juce::AudioFormatReader> reader(sharedFormatManager.createReaderFor(audioFile));
		if (!reader)
		{
			return;
		}

		int targetPageIndex = track->stagingTargetPageIndex.load();
		if (targetPageIndex < 0 || targetPageIndex >= 4)
			targetPageIndex = track->currentPageIndex.load();

		loadAudioToStaging(reader, track);
		processAudioBPMAndSync(track);

		juce::File permanentFile = getTrackPageAudioFile(trackId, targetPageIndex);
		permanentFile.getParentDirectory().createDirectory();

		if (track->nextHasOriginalVersion.load())
		{
			saveOriginalAndStretchedBuffers(track->originalStagingBuffer, track->stagingBuffer, trackId,
			                                track->stagingSampleRate);
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate);
		}

		track->pages[targetPageIndex].audioFilePath = permanentFile.getFullPathName();
		track->hasStagingData = true;
		track->swapRequested = true;

		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    editor->statusLabel.setText("Sample loaded! Ready to play.", juce::dontSendNotification);
				    juce::Timer::callAfterDelay(
				        2000,
				        [this]()
				        {
					        if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
					        {
						        editor->statusLabel.setText("Ready", juce::dontSendNotification);
						        editor->updateLCD();
					        }
				        });
			    }
		    });
	}
	catch (const std::exception & /*e*/)
	{
		track->hasStagingData = false;
		track->swapRequested = false;
		track->stagingTargetPageIndex.store(-1);
	}
}

void AudioManager::loadSampleFromBank(const juce::String &sampleId, const juce::String &trackId)
{
	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
		return;

	auto *sampleEntry = bank->getSample(sampleId);
	if (!sampleEntry)
		return;

	juce::File sampleFile(sampleEntry->filePath);
	if (!sampleFile.exists())
		return;

	TrackData *track = trackManager.getTrack(trackId);
	if (!track)
		return;

	if (!track->currentSampleId.isEmpty() && track->currentSampleId != sampleId)
	{
		bank->markSampleAsUnused(track->currentSampleId, audioProcessor.getProjectId());
	}

	audioProcessor.setIsLoadingFromBank(true);
	audioProcessor.setCurrentBankLoadTrackId(trackId);

	bank->markSampleAsUsed(sampleId, audioProcessor.getProjectId());

	if (track)
	{
		track->currentSampleId = sampleId;
	}

	juce::Thread::launch(
	    [this, trackId, sampleFile, sampleId]()
	    {
		    TrackData *track = trackManager.getTrack(trackId);
		    if (!track)
			    return;

		    loadSampleToBankPage(trackId, track->currentPageIndex.load(), sampleFile, sampleId);

		    juce::Timer::callAfterDelay(2000,
		                                [this]()
		                                {
			                                audioProcessor.setIsLoadingFromBank(false);
			                                audioProcessor.clearCurrentBankLoadTrackId();
		                                });
	    });
}

bool AudioManager::previewSampleFromBank(const juce::String &sampleId)
{
	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
		return false;
	auto *entry = bank->getSample(sampleId);
	if (!entry)
		return false;

	juce::File sampleFile(entry->filePath);
	if (!sampleFile.exists())
		return false;

	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();
	std::unique_ptr<juce::AudioFormatReader> testReader(formatManager.createReaderFor(sampleFile));
	if (!testReader)
	{
		return false;
	}

	stopSamplePreview();

	juce::Thread::launch(
	    [this, sampleFile]()
	    {
		    juce::AudioFormatManager formatManager;
		    formatManager.registerBasicFormats();
		    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
		    if (!reader)
		    {
			    juce::ScopedLock lock(previewLock);
			    isPreviewPlaying.store(false);
			    return;
		    }

		    {
			    juce::ScopedLock lock(previewLock);
			    previewBuffer.setSize(2, (int)reader->lengthInSamples);
			    reader->read(&previewBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
			    if (reader->numChannels == 1)
			    {
				    previewBuffer.copyFrom(1, 0, previewBuffer, 0, 0, previewBuffer.getNumSamples());
			    }
			    previewSampleRate = reader->sampleRate;
			    previewPosition.store(0.0);
			    isPreviewPlaying.store(true);
		    }
	    });

	return true;
}

void AudioManager::stopTrackPreview(const juce::String &trackId)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (track)
	{
		track->isPlaying.store(false);
		track->readPosition = 0.0;
		track->isPreviewMode.store(false);
		track->previewEndPending.store(false);
	}

	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		auto *trackComp = editor->getTrackComponent(trackId);
		if (trackComp)
			trackComp->setPreviewPlaying(false);
	}
}

void AudioManager::renderPreviewToOutput(juce::AudioBuffer<float> &previewBus, juce::AudioBuffer<float> &mainOutput,
                                         int numSamples, double currentSampleRate, bool previewBusIsEffectivelyEnabled)
{
	juce::ScopedLock lock(previewLock);

	if (!previewActive.load() || previewBuffer.getNumSamples() == 0)
		return;

	double ratio = previewSampleRate.load() / currentSampleRate;
	auto &target = previewBusIsEffectivelyEnabled ? previewBus : mainOutput;

	for (int s = 0; s < numSamples; ++s)
	{
		int pos = (int)(previewPosition.load() + s * ratio);
		if (pos < previewBuffer.getNumSamples())
		{
			for (int ch = 0; ch < target.getNumChannels(); ++ch)
			{
				target.addSample(ch, s, previewBuffer.getSample(ch % 2, pos) * 0.7f);
			}
		}
	}

	previewPosition.store(previewPosition.load() + numSamples * ratio);

	if ((int)previewPosition.load() >= previewBuffer.getNumSamples())
		previewActive.store(false);
}

void AudioManager::computeAndSetPeakLevels(const juce::AudioBuffer<float> &buffer)
{
	float currentPeakL = 0.0f;
	float currentPeakR = 0.0f;

	const float *leftData = buffer.getReadPointer(0);
	const float *rightData = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : leftData;

	for (int s = 0; s < buffer.getNumSamples(); ++s)
	{
		float absL = std::abs(leftData[s]);
		float absR = std::abs(rightData[s]);
		if (absL > currentPeakL)
			currentPeakL = absL;
		if (absR > currentPeakR)
			currentPeakR = absR;
	}

	setPeakLevels(currentPeakL, currentPeakR);
}

void AudioManager::stopSamplePreview()
{
	isPreviewPlaying = false;
	previewPosition = 0.0;
	if (!currentPreviewTrackId.isEmpty())
	{
		if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		{
			auto *trackComp = editor->getTrackComponent(currentPreviewTrackId);
			if (trackComp)
			{
				trackComp->setPreviewPlaying(false);
			}
		}
		currentPreviewTrackId = "";
	}
}