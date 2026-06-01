#include "TrackManager.h"
#include "TrackData.h"

juce::String TrackManager::createTrack(const juce::String &name)
{
	juce::ScopedLock lock(tracksLock);
	for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
	{
		usedSlots[i] = false;
	}
	for (const auto &pair : tracks)
	{
		if (pair.second->slotIndex >= 0 && pair.second->slotIndex < Obsidian::MAX_TRACKS)
		{
			usedSlots[pair.second->slotIndex] = true;
		}
	}

	auto track = std::make_unique<TrackData>();
	track->trackName = name + " " + juce::String(tracks.size() + 1);
	track->midiNote = 60 + static_cast<int>(tracks.size());
	juce::String trackId = track->trackId;
	std::string stdId = trackId.toStdString();
	track->slotIndex = findFreeSlot();

	if (track->slotIndex != -1)
	{
		usedSlots[track->slotIndex] = true;
	}

	if (audioPrepared)
	{
		track->delaySendProcessor.prepare(currentSampleRate, currentMaxBlockSize);
		track->reverbSendProcessor.prepare(currentSampleRate, currentMaxBlockSize);
	}

	tracks[stdId] = std::move(track);
	trackOrder.push_back(stdId);
	return trackId;
}

void TrackManager::addTrack(const std::string &trackId, std::unique_ptr<TrackData> track)
{
	const juce::ScopedLock sl(tracksLock);

	if (audioPrepared && track)
		track->delaySendProcessor.prepare(currentSampleRate, currentMaxBlockSize);

	tracks[trackId] = std::move(track);
	trackOrder.push_back(trackId);
}

TrackData *TrackManager::getTrack(const juce::String &trackId)
{
	juce::ScopedLock lock(tracksLock);
	auto it = tracks.find(trackId.toStdString());
	return (it != tracks.end()) ? it->second.get() : nullptr;
}

std::vector<juce::String> TrackManager::getAllTrackIds() const
{
	juce::ScopedLock lock(tracksLock);
	std::vector<juce::String> ids;
	for (const auto &stdId : trackOrder)
	{
		if (tracks.count(stdId))
		{
			ids.push_back(juce::String(stdId));
		}
	}
	return ids;
}

void TrackManager::prepareSends(double sampleRate, int maxBlockSize)
{
	const juce::ScopedLock sl(tracksLock);
	currentSampleRate = sampleRate;
	currentMaxBlockSize = maxBlockSize;
	audioPrepared = true;

	perTrackFxBuffer.setSize(2, maxBlockSize, false, false, true);

	int interval = static_cast<int>(sampleRate * 0.05);
	for (const auto &pair : tracks)
	{
		if (pair.second)
		{
			pair.second->meterUpdateInterval = interval;
			pair.second->delaySendProcessor.prepare(sampleRate, maxBlockSize);
			pair.second->reverbSendProcessor.prepare(sampleRate, maxBlockSize);
		}
	}
}

void TrackManager::processPerTrackDelays(std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                         juce::AudioBuffer<float> &mainOutput, double hostBpm,
                                         DelaySend::TimeDivision division, float feedback, DelaySend::Mode mode,
                                         int numSamples)
{
	if (!audioPrepared)
		return;
	if (perTrackFxBuffer.getNumSamples() < numSamples)
		perTrackFxBuffer.setSize(2, numSamples, false, false, true);

	const juce::ScopedLock sl(tracksLock);

	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it == tracks.end() || !it->second)
			continue;

		TrackData *track = it->second.get();
		const int slot = track->slotIndex;
		if (slot < 0 || slot >= (int)individualOutputs.size())
			continue;

		auto &trackBuffer = individualOutputs[slot];
		if (trackBuffer.getNumChannels() < 2)
			continue;

		track->delaySendProcessor.setBpm(hostBpm);
		track->delaySendProcessor.setTimeDivision(division);
		track->delaySendProcessor.setFeedback(feedback);
		track->delaySendProcessor.setMode(mode);

		const float sendLevel = track->delaySend.load();

		perTrackFxBuffer.clear(0, numSamples);
		if (sendLevel > 0.0001f)
		{
			for (int ch = 0; ch < 2; ++ch)
				perTrackFxBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, sendLevel);
		}

		track->delaySendProcessor.process(perTrackFxBuffer, 0, numSamples);

		for (int ch = 0; ch < 2; ++ch)
			trackBuffer.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);

		for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
			mainOutput.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);
	}
}

void TrackManager::processPerTrackReverbs(std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                          juce::AudioBuffer<float> &mainOutput, float size, float damping, float width,
                                          float mix, int numSamples)
{
	if (!audioPrepared)
		return;

	if (perTrackFxBuffer.getNumSamples() < numSamples)
		perTrackFxBuffer.setSize(2, numSamples, false, false, true);

	const juce::ScopedLock sl(tracksLock);

	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it == tracks.end() || !it->second)
			continue;

		TrackData *track = it->second.get();
		const int slot = track->slotIndex;
		if (slot < 0 || slot >= (int)individualOutputs.size())
			continue;

		auto &trackBuffer = individualOutputs[slot];
		if (trackBuffer.getNumChannels() < 2)
			continue;

		track->reverbSendProcessor.setSize(size);
		track->reverbSendProcessor.setDamping(damping);
		track->reverbSendProcessor.setWidth(width);
		track->reverbSendProcessor.setMix(mix);

		const float sendLevel = track->reverbSend.load();

		perTrackFxBuffer.clear(0, numSamples);
		if (sendLevel > 0.0001f)
		{
			for (int ch = 0; ch < 2; ++ch)
				perTrackFxBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, sendLevel);
		}

		track->reverbSendProcessor.process(perTrackFxBuffer, 0, numSamples);

		for (int ch = 0; ch < 2; ++ch)
			trackBuffer.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);

		for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
			mainOutput.addFrom(ch, 0, perTrackFxBuffer, ch, 0, numSamples);
	}
}

void TrackManager::renderAllTracks(juce::AudioBuffer<float> &outputBuffer,
                                   std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                   juce::AudioBuffer<float> &previewOutput, double hostBpm, const float pairPrev[4],
                                   const float pairCurrent[4], float globalPrev, float globalCurrent, int curveMode,
                                   int timeSignatureNumerator, int timeSignatureDenominator, double sampleRate,
                                   bool useCrossfader)
{
	const int numSamples = outputBuffer.getNumSamples();
	bool anyTrackSolo = false;
	{
		juce::ScopedLock lock(tracksLock);
		for (const auto &pair : tracks)
		{
			if (pair.second->isSolo.load())
			{
				anyTrackSolo = true;
				break;
			}
		}
	}
	outputBuffer.clear();
	for (auto &buffer : individualOutputs)
	{
		buffer.clear();
	}

	for (const auto &pair : tracks)
	{
		auto *track = pair.second.get();
		auto &currentPage = track->getCurrentPage();
		if (track->isEnabled.load() && currentPage.numSamples > 0 && track->slotIndex >= 0 &&
		    track->slotIndex < individualOutputs.size())
		{
			int bufferIndex = track->slotIndex;
			juce::AudioBuffer<float> tempMixBuffer(outputBuffer.getNumChannels(), numSamples);
			juce::AudioBuffer<float> tempIndividualBuffer(2, numSamples);
			tempMixBuffer.clear();
			tempIndividualBuffer.clear();
			renderSingleTrack(*track, tempMixBuffer, tempIndividualBuffer, previewOutput, numSamples, bufferIndex,
			                  hostBpm, timeSignatureNumerator, timeSignatureDenominator, sampleRate);

			track->consoleChannel.process(tempIndividualBuffer, 0, numSamples);

			for (int ch = 0; ch < std::min(tempMixBuffer.getNumChannels(), tempIndividualBuffer.getNumChannels()); ++ch)
				tempMixBuffer.copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);

			bool isA = track->isDeckA();
			float defaultCenterGain = applyCrossfadeCurve(0.5f, isA, curveMode);
			float trackVol = track->volume.load();

			float deckGainStart = useCrossfader ? 1.0f : trackVol * defaultCenterGain;
			float deckGainEnd = useCrossfader ? 1.0f : trackVol * defaultCenterGain;
			int pairIdx = track->getPairIndex();
			if (pairIdx >= 0 && pairIdx < Obsidian::MAX_CROSSFADER_PAIR && useCrossfader)
			{
				float pairXfStart = pairPrev[pairIdx];
				float globalXfStart = globalPrev;
				float pairGainStart = applyCrossfadeCurve(pairXfStart, isA, curveMode);
				float globalGainStart = applyCrossfadeCurve(globalXfStart, isA, curveMode);
				deckGainStart = pairGainStart * globalGainStart;

				float pairXfEnd = pairCurrent[pairIdx];
				float globalXfEnd = globalCurrent;
				float pairGainEnd = applyCrossfadeCurve(pairXfEnd, isA, curveMode);
				float globalGainEnd = applyCrossfadeCurve(globalXfEnd, isA, curveMode);
				deckGainEnd = pairGainEnd * globalGainEnd;
			}

			bool shouldHearTrack = !track->isMuted.load() && (!anyTrackSolo || track->isSolo.load());

			if (track->isPreviewMode.load())
			{
				if (previewOutput.getNumChannels() >= 2)
				{
					for (int ch = 0; ch < previewOutput.getNumChannels(); ++ch)
						previewOutput.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
				}
				else
				{
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
						outputBuffer.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
				}
			}
			else
			{
				if (shouldHearTrack)
				{
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
					{
						outputBuffer.addFromWithRamp(ch, 0, tempMixBuffer.getReadPointer(ch), numSamples, deckGainStart,
						                             deckGainEnd);
					}
				}
				for (int ch = 0; ch < std::min(2, individualOutputs[bufferIndex].getNumChannels()); ++ch)
				{
					if (!shouldHearTrack)
					{
						individualOutputs[bufferIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);
						individualOutputs[bufferIndex].applyGain(ch, 0, numSamples, 0.0f);
					}
					else
					{
						individualOutputs[bufferIndex].copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);
						individualOutputs[bufferIndex].applyGainRamp(ch, 0, numSamples, deckGainStart, deckGainEnd);
					}
				}
			}
		}
	}
}

void TrackManager::loadAudioFileForPage(TrackData *track, int pageIndex, const juce::File &audioFile)
{
	if (!track || pageIndex < 0 || pageIndex >= 4)
	{
		return;
	}

	auto &page = track->pages[pageIndex];

	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();

	std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
	if (!reader)
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	int numChannels = reader->numChannels;
	int numSamples = static_cast<int>(reader->lengthInSamples);

	if (numSamples <= 0)
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	page.audioBuffer.setSize(2, numSamples, false, true, true);
	page.audioBuffer.clear();

	if (!reader->read(&page.audioBuffer, 0, numSamples, 0, true, true))
	{
		page.numSamples = 0;
		page.isLoaded = false;
		page.audioBuffer.setSize(0, 0);
		return;
	}

	if (numChannels == 1)
	{
		page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);
	}

	page.numSamples = numSamples;
	page.sampleRate = reader->sampleRate;
	page.isLoaded = true;
	page.isLoading = false;

	float maxSample = 0.0f;
	for (int ch = 0; ch < page.audioBuffer.getNumChannels(); ++ch)
	{
		auto *channelData = page.audioBuffer.getReadPointer(ch);
		for (int i = 0; i < page.audioBuffer.getNumSamples(); ++i)
		{
			maxSample = std::max(maxSample, std::abs(channelData[i]));
		}
	}
}

size_t TrackManager::getNumTracks() const
{
	const juce::ScopedLock sl(tracksLock);
	return tracks.size();
}

void TrackManager::clearAllTracks()
{
	const juce::ScopedLock sl(tracksLock);
	tracks.clear();
	trackOrder.clear();
}

void TrackManager::forEachTrack(std::function<void(const std::string &, const TrackData *)> callback) const
{
	const juce::ScopedLock sl(tracksLock);
	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it != tracks.end())
		{
			callback(id, it->second.get());
		}
	}
}

void TrackManager::forEachTrack(std::function<void(const TrackData *)> callback) const
{
	const juce::ScopedLock sl(tracksLock);
	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it != tracks.end())
		{
			callback(it->second.get());
		}
	}
}

bool TrackManager::isSlotUsed(int index) const
{
	const juce::ScopedLock sl(tracksLock);
	if (index >= 0 && index < usedSlots.size())
		return usedSlots[index];
	return false;
}

void TrackManager::setSlotUsed(int index, bool used)
{
	const juce::ScopedLock sl(tracksLock);
	if (index >= 0 && index < usedSlots.size())
		usedSlots[index] = used;
}

void TrackManager::resetAllSlots()
{
	const juce::ScopedLock sl(tracksLock);
	usedSlots.fill(false);
}

int TrackManager::findFreeSlot()
{
	std::vector<bool> actualUsage(8, false);
	for (const auto &pair : tracks)
	{
		const auto &track = pair.second;
		if (track->slotIndex >= 0 && track->slotIndex < Obsidian::MAX_TRACKS)
		{
			actualUsage[track->slotIndex] = true;
		}
	}

	for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
	{
		if (!usedSlots[i])
		{
			return i;
		}
	}

	return -1;
}

void TrackManager::renderSingleTrack(TrackData &track, juce::AudioBuffer<float> &mixOutput,
                                     juce::AudioBuffer<float> &individualOutput,
                                     juce::AudioBuffer<float> & /* previewOutput */, int numSamples,
                                     int /* trackIndex */, double hostBpm, int timeSignatureNumerator,
                                     int timeSignatureDenominator, double sampleRate) const
{

	auto *safeCallback = parameterUpdateCallback.load();
	if (safeCallback != nullptr && *safeCallback)
	{
		int slot = track.slotIndex;
		if (slot != -1)
		{
			(*safeCallback)(slot, &track);
		}
	}

	auto handleEndOfPreview = [this, &track]()
	{
		if (track.isPreviewMode.load() && !track.previewEndPending.exchange(true))
		{
			if (onPreviewEnded)
				onPreviewEnded(track.trackId);
		}
	};

	const juce::AudioSampleBuffer *bufferToUse = nullptr;
	int numSamplesToUse = 0;
	double sampleRateToUse = 0;
	double loopStartToUse = 0;
	double loopEndToUse = 0;
	float originalBpmToUse = 126.0f;
	float adsrAttack = 0.0f;
	float adsrDecay = 4.0f;
	float adsrSustain = 1.0f;
	float adsrRelease = 0.0f;

	const auto &currentPage = track.getCurrentPage();
	bufferToUse = &currentPage.audioBuffer;
	numSamplesToUse = currentPage.numSamples;
	sampleRateToUse = sampleRate;
	loopStartToUse = currentPage.loopStart;
	loopEndToUse = currentPage.loopEnd;
	originalBpmToUse = currentPage.originalBpm;
	adsrAttack = currentPage.adsrAttack.load();
	adsrDecay = currentPage.adsrDecay.load();
	adsrSustain = currentPage.adsrSustain.load();
	adsrRelease = currentPage.adsrRelease.load();

	if (numSamplesToUse == 0 || !track.isPlaying.load() || !bufferToUse)
	{
		track.audioLevelLeft.store(0.0f);
		track.audioLevelRight.store(0.0f);
		track.meterAccumPeakLeft = 0.0f;
		track.meterAccumPeakRight = 0.0f;
		track.meterSampleCounter = 0;
		return;
	}

	const float volume = juce::jlimit(0.0f, 1.0f, track.volume.load());
	const float pan = juce::jlimit(-1.0f, 1.0f, track.pan.load());

	float leftGain = 1.0f;
	float rightGain = 1.0f;
	if (pan < 0.0f)
	{
		rightGain = 1.0f + pan;
	}
	else if (pan > 0.0f)
	{
		leftGain = 1.0f - pan;
	}

	double currentPosition = track.readPosition.load();
	double playbackRatio = 1.0;

	float pitchSemis = juce::jlimit(-12.0f, 12.0f, currentPage.pitchSemitones.load());
	float fineCents = juce::jlimit(-50.0f, 50.0f, currentPage.fineOffset.load());
	float totalSemis = pitchSemis + (fineCents / 100.0f);

	if (std::abs(totalSemis) > 0.001f)
	{
		playbackRatio *= std::pow(2.0f, totalSemis / 12.0f);
	}

	double startSample = loopStartToUse * sampleRateToUse;
	double endSample = loopEndToUse * sampleRateToUse;

	startSample = juce::jlimit(0.0, (double)numSamplesToUse - 1, startSample);
	endSample = juce::jlimit(startSample + 1, (double)numSamplesToUse, endSample);

	double sectionLength = endSample - startSample;

	if (sectionLength < 100)
	{
		startSample = 0.0;
		endSample = numSamplesToUse;
		sectionLength = numSamplesToUse;
	}

	const float *leftChannel = bufferToUse->getReadPointer(0);
	const float *rightChannel = bufferToUse->getNumChannels() > 1 ? bufferToUse->getReadPointer(1) : leftChannel;

	const int bufferSize = bufferToUse->getNumSamples();

	const bool beatRepeatActive = track.beatRepeatActive.load();
	const double beatRepeatStart = beatRepeatActive ? track.beatRepeatStartPosition.load() : 0.0;
	const double beatRepeatEnd = beatRepeatActive ? track.beatRepeatEndPosition.load() : 0.0;

	SequencerData seqData = currentPage.getCurrentSequence();

	const int numMeasures = seqData.numMeasures;
	const double beatsPerMeasure = (double)timeSignatureNumerator * (4.0 / timeSignatureDenominator);
	double samplesPerBeat = (60.0 / hostBpm) * sampleRateToUse;
	double samplesPerMeasure = samplesPerBeat * beatsPerMeasure;
	double endSampleLoop = 0.0;
	double samplesPerMeasureScaled = samplesPerMeasure * playbackRatio;

	if (startSample > 0.0 && endSample - startSample > samplesPerMeasureScaled)
		endSampleLoop = (samplesPerMeasureScaled * numMeasures) + startSample;
	else if (startSample > 0.0 && endSample - startSample < samplesPerMeasureScaled)
		endSampleLoop = endSample;
	else if (endSample - startSample < samplesPerMeasureScaled)
		endSampleLoop = endSample;
	else if (endSample - startSample > samplesPerMeasureScaled)
		endSampleLoop = samplesPerMeasureScaled * numMeasures;

	const double FADE_DURATION = 512.0;
	double samplesSourceUntilEnd = endSampleLoop - (startSample + currentPosition);
	double samplesUntilLoopEnd = samplesSourceUntilEnd / playbackRatio;
	bool fadeOutThisBuffer = (samplesUntilLoopEnd > 0 && samplesUntilLoopEnd <= FADE_DURATION + numSamples);

	const double fadeLength = 64.0;
	const float fadeRcp = 1.0f / static_cast<float>(fadeLength);

	double samplesUntilBeatRepeatEnd = -1.0;
	if (beatRepeatActive)
	{
		double samplesSourceUntilBREnd = beatRepeatEnd - (startSample + currentPosition);
		samplesUntilBeatRepeatEnd = samplesSourceUntilBREnd / playbackRatio;
	}
	double brLengthSamples = beatRepeatActive ? (beatRepeatEnd - beatRepeatStart) / playbackRatio : 0.0;
	const int BR_FADE_DURATION = beatRepeatActive ? juce::jlimit(16, 64, static_cast<int>(brLengthSamples / 16.0)) : 64;
	const int BR_FADE_IN_LENGTH = BR_FADE_DURATION;
	int brFadeInCounter = 0;
	int pendingFadeIn = track.brFadeInPending.exchange(0);
	if (pendingFadeIn > 0)
	{
		brFadeInCounter = pendingFadeIn;
	}

	for (int i = 0; i < numSamples; ++i)
	{
		double absolutePosition = startSample + currentPosition;
		if (beatRepeatActive)
		{
			double absolutePos = startSample + currentPosition;
			if (absolutePos >= beatRepeatEnd)
			{
				currentPosition = beatRepeatStart - startSample;
				track.readPosition.store(beatRepeatStart);
				brFadeInCounter = BR_FADE_IN_LENGTH;
				double samplesSourceUntilBREnd = beatRepeatEnd - (startSample + currentPosition);
				samplesUntilBeatRepeatEnd = (samplesSourceUntilBREnd / playbackRatio) + i;
			}
		}

		if (absolutePosition >= endSample)
		{
			track.readPosition = 0.0;
			track.isPlaying = false;
			handleEndOfPreview();
			return;
		}

		if (absolutePosition >= numSamplesToUse)
		{
			currentPosition = 0.0;
			absolutePosition = startSample;
		}

		int sampleIndex = static_cast<int>(absolutePosition);
		if (sampleIndex >= bufferSize)
		{
			track.isPlaying = false;
			handleEndOfPreview();
			break;
		}

		float adsrGain = 1.0f;
		{
			double posInSection = (absolutePosition - startSample) / sampleRateToUse;
			double sectionDuration = sectionLength / sampleRateToUse;

			float totalADR = adsrAttack + adsrDecay + adsrRelease;
			float scale = 1.0f;
			if (totalADR > (float)sectionDuration * 0.95f)
				scale = (float)sectionDuration * 0.95f / totalADR;

			float a = adsrAttack * scale;
			float d = adsrDecay * scale;
			float r = adsrRelease * scale;
			double releaseStart = sectionDuration - (double)r;

			if (posInSection < 0.0)
			{
				adsrGain = 0.0f;
			}
			else if (a > 0.0f && posInSection < (double)a)
			{
				adsrGain = (float)(posInSection / a);
			}
			else if (d > 0.0f && posInSection < (double)(a + d))
			{
				float t = (float)((posInSection - a) / d);
				adsrGain = 1.0f - t * (1.0f - adsrSustain);
			}
			else if (posInSection < releaseStart)
			{
				adsrGain = adsrSustain;
			}
			else if (r > 0.0f && posInSection < sectionDuration)
			{
				float t = (float)((posInSection - releaseStart) / r);
				adsrGain = adsrSustain * (1.0f - t);
			}
			else
			{
				adsrGain = 0.0f;
			}

			adsrGain = juce::jlimit(0.0f, 1.0f, adsrGain);
		}

		double posInLoop = absolutePosition - startSample;
		double loopLength = endSampleLoop - startSample;

		float safetyFade = 1.0f;

		if (!beatRepeatActive)
		{
			if (posInLoop < fadeLength)
				safetyFade = static_cast<float>(posInLoop) * fadeRcp;
			else if (posInLoop > loopLength - fadeLength)
				safetyFade = static_cast<float>(loopLength - posInLoop) * fadeRcp;
		}

		if (beatRepeatActive && samplesUntilBeatRepeatEnd > 0)
		{
			double samplesUntilBR = samplesUntilBeatRepeatEnd - i;
			if (samplesUntilBR > 0 && samplesUntilBR <= BR_FADE_DURATION)
			{
				float brFade = static_cast<float>(samplesUntilBR / static_cast<double>(BR_FADE_DURATION));
				safetyFade = std::min(safetyFade, brFade);
			}
		}

		if (brFadeInCounter > 0)
		{
			float brFadeIn =
			    static_cast<float>(BR_FADE_IN_LENGTH - brFadeInCounter) / static_cast<float>(BR_FADE_IN_LENGTH);
			safetyFade = std::min(safetyFade, brFadeIn);
			brFadeInCounter--;
		}

		if (fadeOutThisBuffer && !beatRepeatActive)
		{
			double samplesUntilTrigger = samplesUntilLoopEnd - i;
			if (samplesUntilTrigger > 0 && samplesUntilTrigger <= FADE_DURATION)
			{
				float triggerFade = static_cast<float>(samplesUntilTrigger / FADE_DURATION);
				safetyFade = std::min(safetyFade, triggerFade);
			}
		}

		safetyFade = juce::jlimit(0.0f, 1.0f, safetyFade);

		float totalGain = adsrGain * safetyFade;

		float leftSample = interpolateLinear(leftChannel, absolutePosition, bufferSize);
		float rightSample = interpolateLinear(rightChannel, absolutePosition, bufferSize);

		float gainDb = juce::jlimit(-60.0f, 12.0f, track.getCurrentPage().gain.load());
		float gainLinear = std::pow(10.0f, gainDb / 20.0f);

		leftSample *= volume * leftGain * totalGain * gainLinear;
		rightSample *= volume * rightGain * totalGain * gainLinear;

		float absLeft = std::abs(leftSample);
		float absRight = std::abs(rightSample);

		if (absLeft > track.meterAccumPeakLeft)
			track.meterAccumPeakLeft = absLeft;
		if (absRight > track.meterAccumPeakRight)
			track.meterAccumPeakRight = absRight;

		track.meterSampleCounter++;

		if (track.meterSampleCounter >= track.meterUpdateInterval)
		{
			track.audioLevelLeft.store(juce::jlimit(0.0f, 1.0f, track.meterAccumPeakLeft));
			track.audioLevelRight.store(juce::jlimit(0.0f, 1.0f, track.meterAccumPeakRight));

			track.meterAccumPeakLeft = 0.0f;
			track.meterAccumPeakRight = 0.0f;
			track.meterSampleCounter = 0;
		}

		mixOutput.addSample(0, i, leftSample);
		mixOutput.addSample(1, i, rightSample);
		individualOutput.setSample(0, i, leftSample);
		individualOutput.setSample(1, i, rightSample);

		currentPosition += playbackRatio;

		if (beatRepeatActive)
		{
			double newTheoretical = track.theoreticalPosition.load() + playbackRatio;

			if (newTheoretical >= endSampleLoop)
			{
				double loopLen = endSampleLoop - startSample;
				double overshoot = newTheoretical - endSampleLoop;
				newTheoretical = startSample + std::fmod(overshoot, loopLen);
			}

			track.theoreticalPosition.store(newTheoretical);
		}
	}
	track.readPosition.store(currentPosition);
}

float TrackManager::interpolateLinear(const float *buffer, double position, int bufferSize) const
{
	int index = static_cast<int>(position);
	if (index >= bufferSize - 1)
		return buffer[bufferSize - 1];

	float fraction = static_cast<float>(position - index);
	return buffer[index] + fraction * (buffer[index + 1] - buffer[index]);
}

float TrackManager::applyCrossfadeCurve(float xfaderValue, bool isDeckA, int curveMode)
{
	float x = isDeckA ? (1.0f - xfaderValue) : xfaderValue;
	x = juce::jlimit(0.0f, 1.0f, x);

	switch (curveMode)
	{
	case 0:
		return x;

	case 1:
	{
		const float s = std::sin(x * juce::MathConstants<float>::halfPi);
		return s * s;
	}
	case 2:
	{
		if (x < 0.1f)
			return 0.0f;
		if (x > 0.9f)
			return 1.0f;

		const float remapped = (x - 0.1f) / 0.8f;
		const float s = std::sin(remapped * juce::MathConstants<float>::halfPi);
		return s * s;
	}
	default:
		return x;
	}
}