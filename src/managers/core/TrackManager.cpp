#include "TrackManager.h"
#include "TrackData.h"

void TrackManager::prepareTrack(TrackData &track)
{
	if (!track.isPrepared.load())
	{
		track.delaySendProcessor.prepare(currentSampleRate, currentMaxBlockSize);
		track.reverbSendProcessor.prepare(currentSampleRate, currentMaxBlockSize);

		track.filter.setMode(juce::dsp::LadderFilterMode::HPF12);
		track.filter.setSamplingRate(currentSampleRate);
		track.filter.setCutoffFrequency(Obsidian::FILTER_CUT);
		track.filter.setResonance(Obsidian::FILTER_RES);
		track.filter.setDrive(Obsidian::FILTER_DRIVE);
		track.filter.setBypassed(Obsidian::FILTER_BYPASSED);

		track.compressor.setThreshold(Obsidian::COMPRESSOR_THRESHOLD);
		track.compressor.setRatio(Obsidian::COMPRESSOR_RATIO);
		track.compressor.setAttack(Obsidian::COMPRESSOR_ATTACK);
		track.compressor.setRelease(Obsidian::COMPRESSOR_RELEASE);
		track.compressor.setMakeUpGain(Obsidian::COMPRESSOR_MAKEUP_GAIN);
		track.compressor.setBypassed(Obsidian::COMPRESSOR_BYPASSED);

		track.limiter.setThreshold(Obsidian::LIMITER_THRESHOLD);
		track.limiter.setRelease(Obsidian::LIMITER_RELEASE);
		track.limiter.setMakeUpGain(Obsidian::LIMITER_MAKEUP_GAIN);
		track.limiter.setBypassed(Obsidian::LIMITER_BYPASSED);

		track.chorus.setRate(Obsidian::CHORUS_RATE);
		track.chorus.setDepth(Obsidian::CHORUS_DEPTH);
		track.chorus.setCentre(Obsidian::CHORUS_CENTRE);
		track.chorus.setFeedback(Obsidian::CHORUS_FEEDBACK);
		track.chorus.setMix(Obsidian::CHORUS_MIX);
		track.chorus.setBypassed(Obsidian::CHORUS_BYPASSED);

		track.distortion.setPre(Obsidian::DISTORTION_PRE);
		track.distortion.setPost(Obsidian::DISTORTION_POST);
		track.distortion.setCut(Obsidian::DISTORTION_CUT);
		track.distortion.setType(Obsidian::distortionType::soft);
		track.distortion.setBypassed(Obsidian::DISTORTION_BYPASSED);

		track.equalizer.setBypassed(Obsidian::EQ_BYPASSED);

		track.isPrepared.store(true);

		juce::dsp::ProcessSpec spec = juce::dsp::ProcessSpec();
		spec.maximumBlockSize = static_cast<juce::uint32>(currentMaxBlockSize);
		spec.numChannels = 2;
		spec.sampleRate = currentSampleRate;

		track.distortion.prepare(spec);
		track.equalizer.prepare(spec);
		track.filter.prepare(spec);
		track.chorus.prepare(spec);
		track.compressor.prepare(spec);
		track.limiter.prepare(spec);
	}
}

juce::String TrackManager::createTrack(const juce::String &name)
{
	juce::ScopedLock lock(tracksLock);
	std::fill(std::begin(usedSlots), std::end(usedSlots), false);
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
	if (track->slotIndex == 0)
	{
		track->isSelected.store(true);
	}

	if (audioPrepared)
	{
		prepareTrack(*track);
	}

	tracks[stdId] = std::move(track);
	trackOrder.push_back(stdId);
	return trackId;
}

void TrackManager::addTrack(const std::string &trackId, std::unique_ptr<TrackData> track)
{
	const juce::ScopedLock sl(tracksLock);

	if (audioPrepared && track)
	{
		prepareTrack(*track);
	}

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
			prepareTrack(*pair.second);
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
		buffer.clear();

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
			constexpr float kUnityHeadroom = 0.707f;
			float trackVol = track->volume.load();

			float deckGainStart = useCrossfader ? 1.0f : trackVol * kUnityHeadroom;
			float deckGainEnd = useCrossfader ? 1.0f : trackVol * kUnityHeadroom;
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
					for (int ch = 0; ch < previewOutput.getNumChannels(); ++ch)
						previewOutput.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
				else
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
						outputBuffer.addFrom(ch, 0, tempMixBuffer, ch, 0, numSamples);
			}
			else
			{
				if (shouldHearTrack)
					for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
						outputBuffer.addFromWithRamp(ch, 0, tempMixBuffer.getReadPointer(ch), numSamples, deckGainStart,
						                             deckGainEnd);
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
		return;

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
		page.audioBuffer.copyFrom(1, 0, page.audioBuffer, 0, 0, numSamples);

	page.numSamples = numSamples;
	page.sampleRate = reader->sampleRate;
	page.isLoaded = true;
	page.isLoading = false;

	float maxSample = 0.0f;
	for (int ch = 0; ch < page.audioBuffer.getNumChannels(); ++ch)
	{
		auto *channelData = page.audioBuffer.getReadPointer(ch);
		for (int i = 0; i < page.audioBuffer.getNumSamples(); ++i)
			maxSample = std::max(maxSample, std::abs(channelData[i]));
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
			callback(id, it->second.get());
	}
}

void TrackManager::forEachTrack(std::function<void(const TrackData *)> callback) const
{
	const juce::ScopedLock sl(tracksLock);
	for (const auto &id : trackOrder)
	{
		auto it = tracks.find(id);
		if (it != tracks.end())
			callback(it->second.get());
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
			actualUsage[track->slotIndex] = true;
	}

	for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
	{
		if (!usedSlots[i])
			return i;
	}

	return -1;
}

void TrackManager::updateBeatRepeat(TrackData *track, int value, double hostBpm, double repeatDuration)
{
	if (track->randomRetriggerInterval.load() != value)
	{
		track->randomRetriggerInterval.store(value);

		if (track->beatRepeatActive.load())
		{
			if (hostBpm <= 0.0)
				hostBpm = 120.0;

			double startPosition = track->beatRepeatStartPosition.load();
			double repeatDurationSamples = repeatDuration * track->getCurrentPage().sampleRate;

			const double GATE_SAMPLES = 64.0;
			double newEnd = startPosition + repeatDurationSamples - GATE_SAMPLES;

			double maxSamples = track->getCurrentPage().numSamples;
			if (newEnd > maxSamples)
				newEnd = maxSamples;
			if (newEnd <= startPosition)
				newEnd = startPosition + 1.0;

			track->beatRepeatEndPosition.store(newEnd);

			double currentPos = track->readPosition.load();
			if (currentPos >= newEnd)
				track->brFadeInPending.store(64);
		}
	}
}

TrackManager::PageInfo TrackManager::getPageInfo(const TrackPage &page, double sampleRate) const
{
	return PageInfo{page,
	                &page.audioBuffer,
	                page.numSamples,
	                sampleRate,
	                page.loopStart,
	                page.loopEnd,
	                page.originalBpm,
	                page.adsrAttack.load(),
	                page.adsrDecay.load(),
	                page.adsrSustain.load(),
	                page.adsrRelease.load()};
}

TrackManager::TrackInfo TrackManager::getTrackInfo(const TrackData &track, const TrackPage &page,
                                                   const PageInfo &pageInfo) const
{
	const float volume = juce::jlimit(0.0f, 1.0f, track.volume.load());
	const float pan = juce::jlimit(-1.0f, 1.0f, track.pan.load());

	float leftGain = 1.0f;
	float rightGain = 1.0f;
	if (pan < 0.0f)
		rightGain = 1.0f + pan;
	else if (pan > 0.0f)
		leftGain = 1.0f - pan;

	double currentPosition = track.readPosition.load();
	double playbackRatio = 1.0;

	float pitchSemis = juce::jlimit(-12.0f, 12.0f, page.pitchSemitones.load());
	float fineCents = juce::jlimit(-50.0f, 50.0f, page.fineOffset.load());
	float totalSemis = pitchSemis + (fineCents / 100.0f);

	if (std::abs(totalSemis) > 0.001f)
		playbackRatio *= std::pow(2.0f, totalSemis / 12.0f);

	double startSample = pageInfo.loopStartToUse * pageInfo.sampleRateToUse;
	double endSample = pageInfo.loopEndToUse * pageInfo.sampleRateToUse;

	startSample = juce::jlimit(0.0, (double)pageInfo.numSamplesToUse - 1, startSample);
	endSample = juce::jlimit(startSample + 1, (double)pageInfo.numSamplesToUse, endSample);

	return TrackInfo{volume,     pan,       leftGain,   rightGain,   currentPosition, playbackRatio,
	                 pitchSemis, fineCents, totalSemis, startSample, endSample};
}

double TrackManager::getNextStepSampleOn(double stepsPerMeasure, double samplesPerStep, SequencerData &seqData,
                                         int numMeasures, double realPosition) const
{
	double samplesCounter = 0.0;
	for (int i = 0; i < numMeasures; i++)
	{
		for (int j = 0; j < stepsPerMeasure; j++)
		{
			if (seqData.steps[i][j] && samplesCounter > realPosition)
				return samplesCounter - realPosition;
			samplesCounter += samplesPerStep;
		}
	}
	return 0.0;
}

TrackManager::FadeInfo TrackManager::getFadeInfo(TrackData &track, const TrackInfo &trackInfo, const TrackPage &page,
                                                 const PageInfo &pageInfo, int timeSignatureNumerator,
                                                 int timeSignatureDenominator, double hostBpm) const
{
	const bool beatRepeatActive = track.beatRepeatActive.load();
	const double beatRepeatStart = beatRepeatActive ? track.beatRepeatStartPosition.load() : 0.0;
	const double beatRepeatEnd = beatRepeatActive ? track.beatRepeatEndPosition.load() : 0.0;
	const double fadeLength = 64.0;
	const float fadeRcp = 1.0f / static_cast<float>(fadeLength);
	double samplesSourceUntilEnd = 0.0;

	SequencerData seqData = page.getCurrentSequence();

	const int numMeasures = seqData.numMeasures;
	const double beatsPerMeasure = (double)timeSignatureNumerator * (4.0 / timeSignatureDenominator);
	double samplesPerBeat = (60.0 / hostBpm) * pageInfo.sampleRateToUse;
	double samplesPerMeasure = samplesPerBeat * beatsPerMeasure;
	double stepsPerMeasure = (double)timeSignatureNumerator * (4 / ((double)timeSignatureDenominator / 4));
	double samplesPerMeasureScaled = samplesPerMeasure * trackInfo.playbackRatio;
	double samplesPerStep = samplesPerMeasureScaled / stepsPerMeasure;
	double endSampleLoop = 0.0;
	const double FADE_DURATION = samplesPerStep / 4;

	double nextStepSampleOn = getNextStepSampleOn(stepsPerMeasure, samplesPerStep, seqData, numMeasures,
	                                              track.numSamplesAccPerSequence.load());

	bool isEndOfSequence =
	    track.numSamplesAccPerSequence.load() >= (samplesPerMeasureScaled * numMeasures) - (FADE_DURATION);
	bool isEndGreaterThanSequenceLength = true;

	if (trackInfo.endSample - trackInfo.startSample < samplesPerMeasureScaled)
	{
		endSampleLoop = trackInfo.endSample;
		isEndGreaterThanSequenceLength = false;
	}
	else
		endSampleLoop = samplesPerMeasureScaled * numMeasures;

	if (nextStepSampleOn > 0.0)
		samplesSourceUntilEnd = nextStepSampleOn;
	else if (isEndOfSequence && isEndGreaterThanSequenceLength)
		samplesSourceUntilEnd = endSampleLoop - (trackInfo.startSample + track.numSamplesAccPerSequence.load());
	else
		samplesSourceUntilEnd = endSampleLoop - (trackInfo.startSample + track.readPosition.load());

	double samplesUntilLoopEnd = samplesSourceUntilEnd / trackInfo.playbackRatio;
	bool fadeOutThisBuffer = (samplesUntilLoopEnd > 0 && samplesUntilLoopEnd <= FADE_DURATION) || isEndOfSequence;

	double samplesUntilBeatRepeatEnd = -1.0;
	if (beatRepeatActive)
	{
		double samplesSourceUntilBREnd = beatRepeatEnd - (trackInfo.startSample + trackInfo.currentPosition);
		samplesUntilBeatRepeatEnd = samplesSourceUntilBREnd / trackInfo.playbackRatio;
	}
	double brLengthSamples = beatRepeatActive ? (beatRepeatEnd - beatRepeatStart) / trackInfo.playbackRatio : 0.0;
	const int BR_FADE_DURATION = beatRepeatActive ? juce::jlimit(16, 64, static_cast<int>(brLengthSamples / 16.0)) : 64;
	const int BR_FADE_IN_LENGTH = BR_FADE_DURATION;
	int brFadeInCounter = 0;
	int pendingBrFadeIn = track.brFadeInPending.exchange(0);
	if (pendingBrFadeIn > 0)
		brFadeInCounter = pendingBrFadeIn;
	int fadeInCounter = 0;
	int pendingFadeIn = track.fadeInPending.exchange(0);
	if (pendingFadeIn > 0)
		fadeInCounter = pendingFadeIn;

	return FadeInfo{beatRepeatActive,    beatRepeatEnd,     beatRepeatStart,
	                BR_FADE_DURATION,    BR_FADE_IN_LENGTH, samplesUntilBeatRepeatEnd,
	                endSampleLoop,       brFadeInCounter,   fadeInCounter,
	                fadeLength,          fadeRcp,           fadeOutThisBuffer,
	                samplesUntilLoopEnd, FADE_DURATION};
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
			(*safeCallback)(slot, &track);
	}

	auto handleEndOfPreview = [this, &track]()
	{
		if (track.isPreviewMode.load() && !track.previewEndPending.exchange(true))
		{
			if (onPreviewEnded)
				onPreviewEnded(track.trackId);
		}
	};

	const auto &currentPage = track.getCurrentPage();
	PageInfo pageInfo = getPageInfo(currentPage, sampleRate);

	if (pageInfo.numSamplesToUse == 0 || !track.isPlaying.load() || !pageInfo.bufferToUse)
	{
		track.audioLevelLeft.store(0.0f);
		track.audioLevelRight.store(0.0f);
		track.meterAccumPeakLeft = 0.0f;
		track.meterAccumPeakRight = 0.0f;
		track.meterSampleCounter = 0;
		return;
	}

	TrackInfo trackInfo = getTrackInfo(track, currentPage, pageInfo);

	double sectionLength = trackInfo.endSample - trackInfo.startSample;

	if (sectionLength < 100)
	{
		trackInfo.startSample = 0.0;
		trackInfo.endSample = pageInfo.numSamplesToUse;
		sectionLength = pageInfo.numSamplesToUse;
	}

	const float *leftChannel = pageInfo.bufferToUse->getReadPointer(0);
	const float *rightChannel =
	    pageInfo.bufferToUse->getNumChannels() > 1 ? pageInfo.bufferToUse->getReadPointer(1) : leftChannel;

	const int bufferSize = pageInfo.bufferToUse->getNumSamples();

	FadeInfo fadeInfo =
	    getFadeInfo(track, trackInfo, currentPage, pageInfo, timeSignatureNumerator, timeSignatureDenominator, hostBpm);

	for (int i = 0; i < numSamples; ++i)
	{
		double absolutePosition = trackInfo.startSample + trackInfo.currentPosition;
		if (fadeInfo.beatRepeatActive)
		{
			double absolutePos = trackInfo.startSample + trackInfo.currentPosition;
			if (absolutePos >= fadeInfo.beatRepeatEnd)
			{
				trackInfo.currentPosition = fadeInfo.beatRepeatStart - trackInfo.startSample;
				track.readPosition.store(fadeInfo.beatRepeatStart);
				fadeInfo.brFadeInCounter = fadeInfo.BR_FADE_IN_LENGTH;
				double samplesSourceUntilBREnd =
				    fadeInfo.beatRepeatEnd - (trackInfo.startSample + trackInfo.currentPosition);
				fadeInfo.samplesUntilBeatRepeatEnd = (samplesSourceUntilBREnd / trackInfo.playbackRatio) + i;
			}
		}

		if (absolutePosition >= trackInfo.endSample)
		{
			track.readPosition.store(0.0);
			track.numSamplesAccPerSequence.store(0.0);
			track.isPlaying.store(false);
			handleEndOfPreview();
			return;
		}

		if (absolutePosition >= pageInfo.numSamplesToUse)
		{
			trackInfo.currentPosition = 0.0;
			absolutePosition = trackInfo.startSample;
		}

		int sampleIndex = static_cast<int>(absolutePosition);
		if (sampleIndex >= bufferSize)
		{
			track.isPlaying.store(false);
			handleEndOfPreview();
			break;
		}

		float adsrGain = getADSRGain(absolutePosition, trackInfo.startSample, sectionLength, pageInfo);

		double posInLoop = absolutePosition - trackInfo.startSample;
		double loopLength = fadeInfo.endSampleLoop - trackInfo.startSample;

		float safetyFade = prepareSafetyFade(i, posInLoop, loopLength, fadeInfo);

		prepareOutput(adsrGain, safetyFade, leftChannel, rightChannel, absolutePosition, bufferSize,
		              fadeInfo.beatRepeatActive, fadeInfo.endSampleLoop, individualOutput, track, i, trackInfo);
	}
	double accumulated = track.numSamplesAccPerSequence.load() + numSamples;
	track.numSamplesAccPerSequence.store(accumulated);
	handleOutput(individualOutput, mixOutput, track, trackInfo.currentPosition, trackInfo.volume);
}

float TrackManager::prepareSafetyFade(int i, double posInLoop, double loopLength, FadeInfo &fadeInfo) const
{
	float safetyFade = 1.0f;

	if (!fadeInfo.beatRepeatActive && fadeInfo.fadeInCounter > 0)
	{
		float fadeIn =
		    (Obsidian::SAFETY_FADE_IN_LENGTH - (double)fadeInfo.fadeInCounter) / Obsidian::SAFETY_FADE_IN_LENGTH;
		safetyFade = std::min(safetyFade, fadeIn);
		fadeInfo.fadeInCounter--;
	}

	if (fadeInfo.beatRepeatActive && fadeInfo.samplesUntilBeatRepeatEnd > 0)
	{
		double samplesUntilBR = fadeInfo.samplesUntilBeatRepeatEnd - i;
		if (samplesUntilBR > 0 && samplesUntilBR <= fadeInfo.BR_FADE_DURATION)
		{
			float brFade = static_cast<float>(samplesUntilBR / static_cast<double>(fadeInfo.BR_FADE_DURATION));
			safetyFade = std::min(safetyFade, brFade);
		}
	}

	if (fadeInfo.brFadeInCounter > 0)
	{
		float brFadeIn = static_cast<float>(fadeInfo.BR_FADE_IN_LENGTH - fadeInfo.brFadeInCounter) /
		                 static_cast<float>(fadeInfo.BR_FADE_IN_LENGTH);
		safetyFade = std::min(safetyFade, brFadeIn);
		fadeInfo.brFadeInCounter--;
	}

	if (fadeInfo.fadeOutThisBuffer && !fadeInfo.beatRepeatActive)
	{
		double samplesUntilTrigger = fadeInfo.samplesUntilLoopEnd - i;
		if (samplesUntilTrigger > 0 && samplesUntilTrigger <= fadeInfo.FADE_DURATION)
		{
			float triggerFade = static_cast<float>(samplesUntilTrigger / fadeInfo.FADE_DURATION);
			safetyFade = std::min(safetyFade, triggerFade);
		}
	}

	safetyFade = juce::jlimit(0.0f, 1.0f, safetyFade);
	return safetyFade;
}

void TrackManager::prepareOutput(float adsrGain, float safetyFade, const float *leftChannel, const float *rightChannel,
                                 double absolutePosition, int bufferSize, const bool beatRepeatActive,
                                 double endSampleLoop, juce::AudioSampleBuffer &individualOutput, TrackData &track,
                                 int i, TrackInfo &trackInfo) const
{
	float totalGain = adsrGain * safetyFade;

	float leftSample = interpolateLinear(leftChannel, absolutePosition, bufferSize);
	float rightSample = interpolateLinear(rightChannel, absolutePosition, bufferSize);

	float gainDb = juce::jlimit(-60.0f, 12.0f, track.getCurrentPage().gain.load());
	float gainLinear = std::pow(10.0f, gainDb / 20.0f);

	leftSample *= trackInfo.leftGain * totalGain * gainLinear;
	rightSample *= trackInfo.rightGain * totalGain * gainLinear;

	individualOutput.setSample(0, i, leftSample);
	individualOutput.setSample(1, i, rightSample);

	trackInfo.currentPosition += trackInfo.playbackRatio;

	if (beatRepeatActive)
	{
		double newTheoretical = track.theoreticalPosition.load() + trackInfo.playbackRatio;

		if (newTheoretical >= endSampleLoop)
		{
			double loopLen = endSampleLoop - trackInfo.startSample;
			double overshoot = newTheoretical - endSampleLoop;
			newTheoretical = trackInfo.startSample + std::fmod(overshoot, loopLen);
		}

		track.theoreticalPosition.store(newTheoretical);
	}
}

void TrackManager::handleOutput(juce::AudioSampleBuffer &individualOutput, juce::AudioSampleBuffer &mixOutput,
                                TrackData &track, double &currentPosition, float volume) const
{
	auto block = juce::dsp::AudioBlock<float>(individualOutput);
	auto blockToUse = block.getSubBlock(0, individualOutput.getNumSamples());
	auto contextToUse = juce::dsp::ProcessContextReplacing<float>(blockToUse);

	track.distortion.process(contextToUse);
	track.equalizer.process(contextToUse);
	track.filter.process(contextToUse);
	track.chorus.process(contextToUse);
	track.compressor.process(contextToUse);
	track.limiter.process(contextToUse);

	individualOutput.applyGain(volume);

	const int numSamplesProcessed = individualOutput.getNumSamples();
	const float *postLeft = individualOutput.getReadPointer(0);
	const float *postRight = individualOutput.getReadPointer(1);

	for (int i = 0; i < numSamplesProcessed; i++)
	{
		float absLeft = std::abs(postLeft[i]);
		float absRight = std::abs(postRight[i]);

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

		mixOutput.addSample(0, i, individualOutput.getSample(0, i));
		mixOutput.addSample(1, i, individualOutput.getSample(1, i));
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

float TrackManager::getADSRGain(double absolutePosition, double startSample, double sectionLength, PageInfo &info) const
{
	float adsrGain = 1.f;
	double posInSection = (absolutePosition - startSample) / info.sampleRateToUse;
	double sectionDuration = sectionLength / info.sampleRateToUse;

	float totalADR = info.adsrAttack + info.adsrDecay + info.adsrRelease;
	float scale = 1.0f;
	if (totalADR > (float)sectionDuration * 0.95f)
		scale = (float)sectionDuration * 0.95f / totalADR;

	float a = info.adsrAttack * scale;
	float d = info.adsrDecay * scale;
	float r = info.adsrRelease * scale;
	double releaseStart = sectionDuration - (double)r;

	if (posInSection < 0.0)
		adsrGain = 0.0f;
	else if (a > 0.0f && posInSection < (double)a)
		adsrGain = (float)(posInSection / a);
	else if (d > 0.0f && posInSection < (double)(a + d))
	{
		float t = (float)((posInSection - a) / d);
		adsrGain = 1.0f - t * (1.0f - info.adsrSustain);
	}
	else if (posInSection < releaseStart)
		adsrGain = info.adsrSustain;
	else if (r > 0.0f && posInSection < sectionDuration)
	{
		float t = (float)((posInSection - releaseStart) / r);
		adsrGain = info.adsrSustain * (1.0f - t);
	}
	else
		adsrGain = 0.0f;

	adsrGain = juce::jlimit(0.0f, 1.0f, adsrGain);
	return adsrGain;
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