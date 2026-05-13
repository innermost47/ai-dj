#include "TrackManager.h"
#include "TrackData.h"

juce::String TrackManager::createTrack(const juce::String &name)
{
	juce::ScopedLock lock(tracksLock);
	for (int i = 0; i < ObsidianDataConst::MAX_TRACKS; ++i)
	{
		usedSlots[i] = false;
	}
	for (const auto &pair : tracks)
	{
		if (pair.second->slotIndex >= 0 && pair.second->slotIndex < ObsidianDataConst::MAX_TRACKS)
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
		track->delaySendProcessor.prepare(currentSampleRate, currentMaxBlockSize);

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

void TrackManager::prepareDelays(double sampleRate, int maxBlockSize)
{
	const juce::ScopedLock sl(tracksLock);
	currentSampleRate = sampleRate;
	currentMaxBlockSize = maxBlockSize;
	audioPrepared = true;

	perTrackDelayBuffer.setSize(2, maxBlockSize, false, false, true);
	for (const auto &pair : tracks)
	{
		if (pair.second)
			pair.second->delaySendProcessor.prepare(sampleRate, maxBlockSize);
	}
}

void TrackManager::processPerTrackDelays(std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                         juce::AudioBuffer<float> &mainOutput, double hostBpm,
                                         DelaySend::TimeDivision division, float feedback, DelaySend::Mode mode,
                                         int numSamples)
{
	if (!audioPrepared)
		return;
	if (perTrackDelayBuffer.getNumSamples() < numSamples)
		perTrackDelayBuffer.setSize(2, numSamples, false, false, true);

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

		perTrackDelayBuffer.clear(0, numSamples);
		if (sendLevel > 0.0001f)
		{
			for (int ch = 0; ch < 2; ++ch)
				perTrackDelayBuffer.addFrom(ch, 0, trackBuffer, ch, 0, numSamples, sendLevel);
		}

		track->delaySendProcessor.process(perTrackDelayBuffer, 0, numSamples);

		for (int ch = 0; ch < 2; ++ch)
			trackBuffer.addFrom(ch, 0, perTrackDelayBuffer, ch, 0, numSamples);

		for (int ch = 0; ch < std::min(2, mainOutput.getNumChannels()); ++ch)
			mainOutput.addFrom(ch, 0, perTrackDelayBuffer, ch, 0, numSamples);
	}
}

void TrackManager::renderAllTracks(juce::AudioBuffer<float> &outputBuffer,
                                   std::vector<juce::AudioBuffer<float>> &individualOutputs,
                                   juce::AudioBuffer<float> &previewOutput, double hostBpm, const float pairPrev[4],
                                   const float pairCurrent[4], float globalPrev, float globalCurrent, int curveMode,
                                   int timeSignatureNumerator, int timeSignatureDenominator)
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
			                  hostBpm, timeSignatureNumerator, timeSignatureDenominator);

			track->consoleChannel.process(tempIndividualBuffer, 0, numSamples);

			for (int ch = 0; ch < std::min(tempMixBuffer.getNumChannels(), tempIndividualBuffer.getNumChannels()); ++ch)
				tempMixBuffer.copyFrom(ch, 0, tempIndividualBuffer, ch, 0, numSamples);

			float deckGainStart = 1.0f;
			float deckGainEnd = 1.0f;
			int pairIdx = track->getPairIndex();
			if (pairIdx >= 0 && pairIdx < ObsidianDataConst::MAX_CROSSFADER_PAIR)
			{
				bool isA = track->isDeckA();

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
		if (track->slotIndex >= 0 && track->slotIndex < ObsidianDataConst::MAX_TRACKS)
		{
			actualUsage[track->slotIndex] = true;
		}
	}

	for (int i = 0; i < ObsidianDataConst::MAX_TRACKS; ++i)
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
                                     int timeSignatureDenominator) const
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
	sampleRateToUse = currentPage.sampleRate;
	loopStartToUse = currentPage.loopStart;
	loopEndToUse = currentPage.loopEnd;
	originalBpmToUse = currentPage.originalBpm;
	adsrAttack = currentPage.adsrAttack.load();
	adsrDecay = currentPage.adsrDecay.load();
	adsrSustain = currentPage.adsrSustain.load();
	adsrRelease = currentPage.adsrRelease.load();

	if (numSamplesToUse == 0 || !track.isPlaying.load() || !bufferToUse)
		return;

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

	switch (track.timeStretchMode)
	{
	case 1:
		playbackRatio = 1.0;
		break;
	case 2:
		if (originalBpmToUse > 0.0f)
		{
			float totalBpmAdjust = static_cast<float>(currentPage.bpmOffset.load()) + currentPage.fineOffset.load();
			float adjustedBpm = originalBpmToUse + totalBpmAdjust;
			adjustedBpm = juce::jlimit(1.0f, 1000.0f, adjustedBpm);
			playbackRatio = adjustedBpm / originalBpmToUse;
		}
		break;
	case 3:
		if (originalBpmToUse > 0.0f && hostBpm > 0.0)
		{
			playbackRatio = hostBpm / originalBpmToUse;
		}
		break;
	case 4:
		if (originalBpmToUse > 0.0f && hostBpm > 0.0)
		{
			float totalManualAdjust = static_cast<float>(currentPage.bpmOffset.load()) + currentPage.fineOffset.load();
			float effectiveHostBpm = static_cast<float>(hostBpm) + totalManualAdjust;
			effectiveHostBpm = juce::jlimit(1.0f, 1000.0f, effectiveHostBpm);
			playbackRatio = effectiveHostBpm / originalBpmToUse;
		}
		break;
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

	const double totalSamplesBeforeFadeOut = juce::jmin((samplesPerMeasure * numMeasures) + startSample, endSample);

	const double fadeLength = 64.0;
	const float fadeRcp = 1.0f / static_cast<float>(fadeLength);

	for (int i = 0; i < numSamples; ++i)
	{
		if (beatRepeatActive)
		{
			double absolutePos = startSample + currentPosition;
			if (absolutePos >= beatRepeatEnd)
			{
				currentPosition = beatRepeatStart - startSample;
				track.readPosition.store(beatRepeatStart);
			}
		}

		double absolutePosition = startSample + currentPosition;

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

		float safetyFade = 1.0f;

		if (absolutePosition < startSample + numSamples)
		{
			safetyFade = static_cast<float>(absolutePosition) * fadeRcp;
		}
		else if (absolutePosition > (totalSamplesBeforeFadeOut - numSamples))
		{
			safetyFade = static_cast<float>(totalSamplesBeforeFadeOut - absolutePosition) * fadeRcp;
		}

		safetyFade = juce::jlimit(0.0f, 1.0f, safetyFade);

		float totalGain = adsrGain * safetyFade;

		float leftSample = interpolateLinear(leftChannel, absolutePosition, bufferSize);
		float rightSample = interpolateLinear(rightChannel, absolutePosition, bufferSize);

		leftSample *= volume * leftGain * totalGain;
		rightSample *= volume * rightGain * totalGain;

		mixOutput.addSample(0, i, leftSample);
		mixOutput.addSample(1, i, rightSample);
		individualOutput.setSample(0, i, leftSample);
		individualOutput.setSample(1, i, rightSample);

		currentPosition += playbackRatio;
	}

	track.readPosition = currentPosition;
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
		return std::sin(x * juce::MathConstants<float>::halfPi);

	case 2:
		if (x >= 0.5f)
			return 1.0f;
		return std::sin(x * juce::MathConstants<float>::pi);

	default:
		return x;
	}
}
