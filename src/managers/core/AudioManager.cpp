#include "AudioManager.h"
#include "AudioAnalyzer.h"
#include "MiniBpm.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"
#include "TrackStretchImpl.h"
#include "signalsmith-stretch.h"

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
	int interval = static_cast<int>(newSampleRate * 0.05);
	meterUpdateInterval = interval;
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
		buffer.setSize(2, ObsidianDataConst::MAX_BLOCK_SIZE);

	previewBuffer.setSize(2, ObsidianDataConst::MAX_BLOCK_SIZE);
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
			    editor->uiStatusManager->updateLCD();
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
		if (busIndex * 2 + 1 < audioProcessor.getTotalNumOutputChannels() && busIndex <= ObsidianDataConst::MAX_TRACKS)
		{
			auto busBuffer = audioProcessor.getBusBuffer(buffer, false, busIndex);
			busBuffer.clear();
		}
	}
}

AudioManager::PreprocessResult AudioManager::preprocessAudioFile(const juce::File &rawFile, float serverSnappedBpm,
                                                                 const juce::String &trackId)
{
	PreprocessResult result;

	juce::AudioFormatManager fm;
	fm.registerBasicFormats();
	std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(rawFile));
	if (!reader)
		return result;

	const int numSamples = static_cast<int>(reader->lengthInSamples);
	const double sampleRate = reader->sampleRate;

	juce::AudioBuffer<float> rawBuffer(2, numSamples);
	rawBuffer.clear();
	reader->read(&rawBuffer, 0, numSamples, 0, true, true);
	if (reader->numChannels == 1)
		rawBuffer.copyFrom(1, 0, rawBuffer, 0, 0, numSamples);

	double hostBpm = audioProcessor.getCachedHostBpm();
	double tempo = 0.0;

	if (serverSnappedBpm <= 0.0f)
	{
		breakfastquay::MiniBPM bpm(static_cast<float>(sampleRate));
		bpm.setBPMRange(hostBpm - 20.0, hostBpm + 20.0);
		bpm.setBeatsPerBar(4);
		bpm.process(rawBuffer.getReadPointer(0), numSamples);
		tempo = bpm.estimateTempo();
		bpm.reset();
	}
	else
	{
		tempo = static_cast<double>(serverSnappedBpm);
	}

	int targetPageIndex = 0;
	if (TrackData *track = trackManager.getTrack(trackId))
		targetPageIndex = track->currentPageIndex.load();

	auto stretchedFile = getTrackPageAudioFile(trackId, targetPageIndex);
	stretchedFile.getParentDirectory().createDirectory();

	if (tempo <= 0.0 || hostBpm <= 0.0 || std::abs(hostBpm - tempo) < 0.1)
	{
		saveBufferToFile(rawBuffer, stretchedFile, sampleRate);
		result.stretchedFile = stretchedFile;
		result.hasOriginalVersion = false;
		result.originalBpm = static_cast<float>(hostBpm > 0 ? hostBpm : 126.0);
		result.success = true;
		return result;
	}

	double ratio = hostBpm / tempo;
	ratio = juce::jlimit(0.25, 4.0, ratio);

	const int outputSamples = static_cast<int>(numSamples / ratio);
	juce::AudioBuffer<float> stretchedBuffer(2, outputSamples);

	signalsmith::stretch::SignalsmithStretch<float> stretch;
	stretch.presetDefault(2, static_cast<float>(sampleRate));

	const float *const *inPtrs = rawBuffer.getArrayOfReadPointers();
	float *const *outPtrs = stretchedBuffer.getArrayOfWritePointers();
	stretch.process(inPtrs, numSamples, outPtrs, outputSamples);

	const float silenceThresholdRMS = 0.01f;
	const int windowSize = 256;
	int firstValidSample = 0;

	for (int i = 0; i < outputSamples - windowSize; i += windowSize / 4)
	{
		double sumSquares = 0.0;
		int countSamples = 0;

		for (int j = 0; j < windowSize && (i + j) < outputSamples; ++j)
		{
			for (int c = 0; c < 2; ++c)
			{
				const float s = stretchedBuffer.getSample(c, i + j);
				sumSquares += s * s;
				countSamples++;
			}
		}

		const float rms = std::sqrt(static_cast<float>(sumSquares / countSamples));

		if (rms > silenceThresholdRMS)
		{
			firstValidSample = i;
			break;
		}
	}
	juce::AudioBuffer<float> finalBuffer;
	int cleanedSize = outputSamples - firstValidSample;
	if (cleanedSize > 0)
	{
		finalBuffer.setSize(2, cleanedSize);
		for (int c = 0; c < 2; ++c)
			finalBuffer.copyFrom(c, 0, stretchedBuffer, c, firstValidSample, cleanedSize);
	}
	else
	{
		finalBuffer.makeCopyOf(stretchedBuffer);
	}

	auto audioDir = stretchedFile.getParentDirectory();
	char pageName = static_cast<char>('A' + targetPageIndex);
	auto originalFile = audioDir.getChildFile(trackId + "_original_" + juce::String(pageName) + ".wav");

	saveBufferToFile(rawBuffer, originalFile, sampleRate);
	saveBufferToFile(finalBuffer, stretchedFile, sampleRate);

	result.stretchedFile = stretchedFile;
	result.originalFile = originalFile;
	result.hasOriginalVersion = true;
	result.originalBpm = static_cast<float>(hostBpm);
	result.success = true;
	return result;
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

	track->stagingNumSamples.store(numSamples);
	track->stagingSampleRate.store(sampleRate);
}

void AudioManager::checkAndSwapStagingBuffers()
{
	auto trackIds = trackManager.getAllTrackIds();

	for (const auto &trackId : trackIds)
	{
		TrackData *track = trackManager.getTrack(trackId);
		if (!track)
			continue;
		if (track->swapRequested.exchange(false, std::memory_order_acquire))
		{
			if (track->hasStagingData.load(std::memory_order_acquire))
			{
				performAtomicSwap(track, trackId);
			}
		}
	}
}

void AudioManager::performAtomicSwap(TrackData *track, const juce::String &trackId)
{
	int targetPageIndex = track->stagingTargetPageIndex.load();
	if (targetPageIndex < 0 || targetPageIndex >= ObsidianDataConst::MAX_PAGES)
		targetPageIndex = track->currentPageIndex.load();

	auto &targetPage = track->pages[targetPageIndex];
	bool preservedHasOriginal = targetPage.hasOriginalVersion.load();

	std::swap(targetPage.audioBuffer, track->stagingBuffer);
	track->stretchNeedsReset.store(true);
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
		juce::MessageManager::callAsync([editor, trackId]() { editor->uiTrackManager->onSampleLoaded(trackId); });
	}
}

void AudioManager::updateWaveformDisplay(const juce::String &trackId)
{
	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		for (auto &trackComp : editor->uiTrackManager->getTrackComponents())
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

void AudioManager::processAudioBPMAndSync(TrackData *track)
{
	track->nextHasOriginalVersion.store(false);
	auto &currentPage = track->getCurrentPage();
	float serverSnappedBpm = audioProcessor.getPendingSnappedBpm();
	double hostBpm = audioProcessor.getCachedHostBpm();
	double tempo = 0.0;

	if (serverSnappedBpm <= 0.0f)
	{
		breakfastquay::MiniBPM bpm(static_cast<float>(track->stagingSampleRate.load()));
		bpm.setBPMRange(hostBpm - 20.0, hostBpm + 20.0);
		bpm.setBeatsPerBar(4);

		int nsamples = track->stagingBuffer.getNumSamples();
		const float *channelData = track->stagingBuffer.getReadPointer(0);

		bpm.process(channelData, nsamples);
		tempo = bpm.estimateTempo();

		bpm.reset();

		double bpmDifference = std::abs(hostBpm - tempo);

		if (bpmDifference < 0.1)
		{
			track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
			currentPage.stagingOriginalBpm = static_cast<float>(hostBpm);
			track->nextHasOriginalVersion.store(false);
			audioProcessor.setPendingSnappedBpm(-1.0f);
			return;
		}
	}
	else
	{
		tempo = static_cast<double>(serverSnappedBpm);
		if (tempo == hostBpm)
		{
			track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
			currentPage.stagingOriginalBpm = static_cast<float>(hostBpm);
			track->nextHasOriginalVersion.store(false);
			audioProcessor.setPendingSnappedBpm(-1.0f);
			return;
		}
	}
	if (tempo <= 0.0 || hostBpm <= 0.0)
	{
		track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
		currentPage.stagingOriginalBpm = static_cast<float>(hostBpm > 0 ? hostBpm : 126.0);
		track->nextHasOriginalVersion.store(false);
		return;
	}

	audioProcessor.setPendingSnappedBpm(-1.0f);

	double ratio = hostBpm / tempo;

	ratio = juce::jlimit(0.25, 4.0, ratio);

	track->originalStagingBuffer.makeCopyOf(track->stagingBuffer);

	int inputSamples = track->stagingBuffer.getNumSamples();
	int outputSamples = static_cast<int>(inputSamples / ratio);
	int numChannels = track->stagingBuffer.getNumChannels();

	juce::AudioBuffer<float> finalStretchedAudio(numChannels, outputSamples);

	signalsmith::stretch::SignalsmithStretch<float> stretch;
	stretch.presetDefault(numChannels, static_cast<float>(track->stagingSampleRate.load()));

	const float *const *inputPointers = track->stagingBuffer.getArrayOfReadPointers();
	float *const *outputPointers = finalStretchedAudio.getArrayOfWritePointers();

	stretch.process(inputPointers, inputSamples, outputPointers, outputSamples);

	const float silenceThreshold = 0.001f;
	int firstValidSample = 0;

	for (int i = 0; i < outputSamples; ++i)
	{
		float maxVal = 0.0f;
		for (int channel = 0; channel < numChannels; ++channel)
		{
			float sampleVal = std::abs(finalStretchedAudio.getSample(channel, i));
			if (sampleVal > maxVal)
				maxVal = sampleVal;
		}
		if (maxVal > silenceThreshold)
		{
			firstValidSample = i;
			break;
		}
	}

	int cleanedSize = outputSamples - firstValidSample;

	if (cleanedSize > 0)
	{
		juce::AudioBuffer<float> totalAudio(numChannels, cleanedSize);

		for (int channel = 0; channel < numChannels; ++channel)
		{
			totalAudio.copyFrom(channel, 0, finalStretchedAudio, channel, firstValidSample, cleanedSize);
		}
		track->stagingBuffer.makeCopyOf(totalAudio);
	}
	else
		track->stagingBuffer.makeCopyOf(finalStretchedAudio);

	track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
	currentPage.stagingOriginalBpm = static_cast<float>(hostBpm);
	track->nextHasOriginalVersion.store(true);
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

void AudioManager::loadAudioFileForPageSwitch(const juce::String &trackId, int pageIndex, const juce::File &audioFile)
{
	TrackData *track = trackManager.getTrack(trackId);
	if (!track || pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
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

		track->stagingNumSamples.store(numSamples);
		track->stagingSampleRate.store(reader->sampleRate);

		track->isVersionSwitch.store(true);
		track->preservedLoopStart = preservedLoopStart;
		track->preservedLoopEnd = preservedLoopEnd;
		track->preservedLoopLocked.store(preservedLocked);

		if (pageIndex == track->currentPageIndex.load())
		{
			track->hasStagingData.store(true);
			track->swapRequested.store(true);
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
	if (!track || pageIndex < 0 || pageIndex >= ObsidianDataConst::MAX_PAGES)
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

		track->stagingNumSamples.store(numSamples);
		track->stagingSampleRate.store(reader->sampleRate);
		currentPage.stagingOriginalBpm = 126.0f;

		processAudioBPMAndSync(track);

		auto permanentFile = getTrackPageAudioFile(trackId, pageIndex);
		permanentFile.getParentDirectory().createDirectory();

		if (track->nextHasOriginalVersion.load())
		{
			auto originalFile = getTrackPageAudioFile(trackId + "_original", pageIndex);
			auto stretchedFile = getTrackPageAudioFile(trackId, pageIndex);
			saveBufferToFile(track->originalStagingBuffer, originalFile, track->stagingSampleRate.load());
			saveBufferToFile(track->stagingBuffer, stretchedFile, track->stagingSampleRate.load());
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate.load());
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
			page.setSelectedPrompt(sampleEntry->originalPrompt);
			page.generationBpm = sampleEntry->bpm;
			page.generationKey = sampleEntry->key;
		}

		if (pageIndex == track->currentPageIndex.load())
		{
			track->hasStagingData.store(true);
			track->swapRequested.store(true);
		}

		juce::MessageManager::callAsync(
		    [this, trackId, pageIndex]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    editor->uiStatusManager->setStatusWithTimeout("Sample loaded to page " +
				                                                  juce::String((char)('A' + pageIndex)) + "!");
				    TrackData *track = trackManager.getTrack(trackId);
				    if (track && pageIndex == track->currentPageIndex.load())
				    {
					    for (auto &trackComp : editor->uiTrackManager->getTrackComponents())
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
		std::unique_ptr<juce::AudioFormatReader> reader(audioProcessor.sharedFormatManager.createReaderFor(audioFile));
		if (!reader)
		{
			return;
		}

		int targetPageIndex = track->stagingTargetPageIndex.load();
		if (targetPageIndex < 0 || targetPageIndex >= ObsidianDataConst::MAX_PAGES)
			targetPageIndex = track->currentPageIndex.load();

		loadAudioToStaging(reader, track);
		if (track->skipBpmSync.exchange(false))
		{
			auto &currentPage = track->getCurrentPage();
			track->stagingNumSamples.store(track->stagingBuffer.getNumSamples());
			currentPage.stagingOriginalBpm = track->preprocessOriginalBpm.load();
			track->nextHasOriginalVersion.store(track->preprocessHasOriginal.load());
		}
		else
		{
			processAudioBPMAndSync(track);
		}

		juce::File permanentFile = getTrackPageAudioFile(trackId, targetPageIndex);
		permanentFile.getParentDirectory().createDirectory();

		if (track->nextHasOriginalVersion.load())
		{
			saveOriginalAndStretchedBuffers(track->originalStagingBuffer, track->stagingBuffer, trackId,
			                                track->stagingSampleRate.load());
		}
		else
		{
			saveBufferToFile(track->stagingBuffer, permanentFile, track->stagingSampleRate.load());
		}

		track->pages[targetPageIndex].audioFilePath = permanentFile.getFullPathName();
		track->hasStagingData.store(true, std::memory_order_release);
		track->swapRequested.store(true, std::memory_order_release);

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
						        editor->uiStatusManager->updateLCD();
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
			    previewActive.store(false);
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
			    previewActive.store(true);
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
		track->readPosition.store(0.0);
		track->isPreviewMode.store(false);
		track->previewEndPending.store(false);
	}

	if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
	{
		auto *trackComp = editor->uiTrackManager->getTrackComponent(trackId);
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
	const float *leftData = buffer.getReadPointer(0);
	const float *rightData = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : leftData;

	for (int s = 0; s < buffer.getNumSamples(); ++s)
	{
		float absL = std::abs(leftData[s]);
		float absR = std::abs(rightData[s]);

		if (absL > meterAccumPeakLeft)
			meterAccumPeakLeft = absL;
		if (absR > meterAccumPeakRight)
			meterAccumPeakRight = absR;

		meterSampleCounter++;

		if (meterSampleCounter >= meterUpdateInterval)
		{
			setPeakLevels(meterAccumPeakLeft, meterAccumPeakRight);

			meterAccumPeakLeft = 0.0f;
			meterAccumPeakRight = 0.0f;
			meterSampleCounter = 0;
		}
	}
}

void AudioManager::stopSamplePreview()
{
	previewActive.store(false);
	previewPosition = 0.0;
	if (!currentPreviewTrackId.isEmpty())
	{
		if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		{
			auto *trackComp = editor->uiTrackManager->getTrackComponent(currentPreviewTrackId);
			if (trackComp)
			{
				trackComp->setPreviewPlaying(false);
			}
		}
		currentPreviewTrackId = "";
	}
}