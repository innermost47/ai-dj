#include "StateManager.h"
#include "DataConst.h"
#include "JuceHeader.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"
#include "config/version.h"

StateManager::StateManager(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

juce::ValueTree StateManager::saveState() const
{
	juce::ValueTree state("TrackManager");

	audioProcessor.getTrackManager().forEachTrack(
	    [&](const TrackData *track)
	    {
		    juce::ValueTree trackState("Track");

		    bool isPlaying = track->isPlaying.load() && track->isCurrentlyPlaying.load();
		    bool isArmed = isPlaying ? true : track->isArmed.load();

		    trackState.setProperty("id", track->trackId, nullptr);
		    trackState.setProperty("name", track->trackName, nullptr);
		    trackState.setProperty("currentSampleId", track->currentSampleId, nullptr);
		    trackState.setProperty("slotIndex", track->slotIndex, nullptr);
		    trackState.setProperty("style", track->style, nullptr);
		    trackState.setProperty("midiNote", track->midiNote, nullptr);
		    trackState.setProperty("volume", track->volume.load(), nullptr);
		    trackState.setProperty("pan", track->pan.load(), nullptr);
		    trackState.setProperty("muted", track->isMuted.load(), nullptr);
		    trackState.setProperty("solo", track->isSolo.load(), nullptr);
		    trackState.setProperty("enabled", track->isEnabled.load(), nullptr);
		    trackState.setProperty("timeStretchRatio", track->timeStretchRatio, nullptr);
		    trackState.setProperty("showWaveform", track->showWaveform.load(), nullptr);
		    trackState.setProperty("showSequencer", track->showSequencer.load(), nullptr);
		    trackState.setProperty("isPlaying", false, nullptr);
		    trackState.setProperty("isArmed", isArmed, nullptr);
		    trackState.setProperty("isArmedToStop", false, nullptr);
		    trackState.setProperty("isCurrentlyPlaying", false, nullptr);
		    trackState.setProperty("nextHasOriginalVersion", track->nextHasOriginalVersion.load(), nullptr);
		    trackState.setProperty("randomRetriggerEnabled", track->randomRetriggerEnabled.load(), nullptr);
		    trackState.setProperty("randomBeatRepeatInterval", track->randomBeatRepeatInterval.load(), nullptr);
		    trackState.setProperty("reverseActive", track->reverseActive.load(), nullptr);
		    trackState.setProperty("beatRepeatPending", track->beatRepeatPending.load(), nullptr);
		    trackState.setProperty("beatRepeatStopPending", track->beatRepeatStopPending.load(), nullptr);
		    trackState.setProperty("originalReadPosition", track->originalReadPosition.load(), nullptr);
		    trackState.setProperty("beatRepeatStartPosition", track->beatRepeatStartPosition.load(), nullptr);
		    trackState.setProperty("beatRepeatEndPosition", track->beatRepeatEndPosition.load(), nullptr);
		    trackState.setProperty("beatRepeatActive", track->beatRepeatActive.load(), nullptr);
		    trackState.setProperty("randomRetriggerDurationEnabled", track->randomRetriggerDurationEnabled.load(),
		                           nullptr);
		    trackState.setProperty("currentPageIndex", track->currentPageIndex.load(), nullptr);
		    trackState.setProperty("filterMode", static_cast<int>(track->filter.getMode()), nullptr);
		    trackState.setProperty("cutoffFrequency", track->filter.getCutoff(), nullptr);
		    trackState.setProperty("resonance", track->filter.getResonance(), nullptr);
		    trackState.setProperty("filterDrive", track->filter.getDrive(), nullptr);
		    trackState.setProperty("filterBypassed", track->filter.isBypassed(), nullptr);

		    trackState.setProperty("subBassGain", track->equalizer.getGain(Obsidian::eqBands::subBass), nullptr);
		    trackState.setProperty("bassGain", track->equalizer.getGain(Obsidian::eqBands::bass), nullptr);
		    trackState.setProperty("lowMidGain", track->equalizer.getGain(Obsidian::eqBands::lowMid), nullptr);
		    trackState.setProperty("midGain", track->equalizer.getGain(Obsidian::eqBands::mid), nullptr);
		    trackState.setProperty("highMidGain", track->equalizer.getGain(Obsidian::eqBands::highMid), nullptr);
		    trackState.setProperty("presenceGain", track->equalizer.getGain(Obsidian::eqBands::presence), nullptr);
		    trackState.setProperty("highGain", track->equalizer.getGain(Obsidian::eqBands::high), nullptr);
		    trackState.setProperty("airGain", track->equalizer.getGain(Obsidian::eqBands::air), nullptr);
		    trackState.setProperty("eqBypassed", track->equalizer.isBypassed(), nullptr);

		    trackState.setProperty("compressorThreshold", track->compressor.getThreshold(), nullptr);
		    trackState.setProperty("compressorRatio", track->compressor.getRatio(), nullptr);
		    trackState.setProperty("compressorAttack", track->compressor.getAttack(), nullptr);
		    trackState.setProperty("compressorRelease", track->compressor.getRelease(), nullptr);
		    trackState.setProperty("compressorMakeUpGain", track->compressor.getMakeUpGain(), nullptr);
		    trackState.setProperty("compressorBypassed", track->compressor.isBypassed(), nullptr);

		    trackState.setProperty("limiterThreshold", track->limiter.getThreshold(), nullptr);
		    trackState.setProperty("limiterRelease", track->limiter.getRelease(), nullptr);
		    trackState.setProperty("limiterMakeUpGain", track->limiter.getMakeUpGain(), nullptr);

		    trackState.setProperty("distortionPreGain", track->distortion.getPre(), nullptr);
		    trackState.setProperty("distortionPostGain", track->distortion.getPost(), nullptr);
		    trackState.setProperty("distortionCut", track->distortion.getCut(), nullptr);
		    trackState.setProperty("distortionType", track->distortion.getType(), nullptr);
		    trackState.setProperty("distortionBypassed", track->distortion.isBypassed(), nullptr);

		    trackState.setProperty("chorusRate", track->chorus.getRate(), nullptr);
		    trackState.setProperty("chorusDepth", track->chorus.getDepth(), nullptr);
		    trackState.setProperty("chorusCentre", track->chorus.getCentre(), nullptr);
		    trackState.setProperty("chorusFeedback", track->chorus.getFeedback(), nullptr);
		    trackState.setProperty("chorusMix", track->chorus.getMix(), nullptr);
		    trackState.setProperty("chorusBypassed", track->chorus.isBypassed(), nullptr);

		    trackState.setProperty("phaserRate", track->phaser.getRate(), nullptr);
		    trackState.setProperty("phaserDepth", track->phaser.getDepth(), nullptr);
		    trackState.setProperty("phaserCentre", track->phaser.getCentre(), nullptr);
		    trackState.setProperty("phaserFeedback", track->phaser.getFeedback(), nullptr);
		    trackState.setProperty("phaserMix", track->phaser.getMix(), nullptr);
		    trackState.setProperty("phaserBypassed", track->phaser.isBypassed(), nullptr);

		    trackState.setProperty("flangerRate", track->flanger.getRate(), nullptr);
		    trackState.setProperty("flangerDepth", track->flanger.getDepth(), nullptr);
		    trackState.setProperty("flangerCentre", track->flanger.getCentre(), nullptr);
		    trackState.setProperty("flangerFeedback", track->flanger.getFeedback(), nullptr);
		    trackState.setProperty("flangerMix", track->flanger.getMix(), nullptr);
		    trackState.setProperty("flangerBypassed", track->flanger.isBypassed(), nullptr);

		    trackState.setProperty("bitcrusherBitDepth", track->bitCrusher.getBitDepth(), nullptr);
		    trackState.setProperty("bitcrusherRate", track->bitCrusher.getSampleRateReduction(), nullptr);
		    trackState.setProperty("bitcrusherMix", track->bitCrusher.getMix(), nullptr);
		    trackState.setProperty("bitcrusherBypassed", track->bitCrusher.isBypassed(), nullptr);

		    trackState.setProperty("isSelected", track->isSelected.load(), nullptr);
		    trackState.setProperty("transientScatterActive", track->transientScatterActive.load(), nullptr);

		    for (int pageIndex = 0; pageIndex < Obsidian::MAX_PAGES; ++pageIndex)
		    {
			    auto pageState = juce::ValueTree("Page");
			    const auto &page = track->pages[pageIndex];

			    pageState.setProperty("index", pageIndex, nullptr);
			    pageState.setProperty("audioFilePath", page.audioFilePath, nullptr);
			    pageState.setProperty("originalFilePath", page.originalFilePath, nullptr);
			    pageState.setProperty("numSamples", page.numSamples, nullptr);
			    pageState.setProperty("sampleRate", page.sampleRate, nullptr);
			    pageState.setProperty("originalBpm", page.originalBpm, nullptr);
			    pageState.setProperty("prompt", page.prompt, nullptr);
			    pageState.setProperty("selectedPrompt", page.selectedPrompt, nullptr);
			    pageState.setProperty("selectedModel", page.selectedModel, nullptr);
			    pageState.setProperty("generationPrompt", page.generationPrompt, nullptr);
			    pageState.setProperty("generationBpm", page.generationBpm, nullptr);
			    pageState.setProperty("generationKey", page.generationKey, nullptr);
			    pageState.setProperty("generationDuration", page.generationDuration, nullptr);
			    pageState.setProperty("loopStart", page.loopStart, nullptr);
			    pageState.setProperty("loopEnd", page.loopEnd, nullptr);
			    pageState.setProperty("useOriginalFile", page.useOriginalFile.load(), nullptr);
			    pageState.setProperty("hasOriginalVersion", page.hasOriginalVersion.load(), nullptr);
			    pageState.setProperty("isLoaded", page.isLoaded.load(), nullptr);
			    pageState.setProperty("canvasData", page.canvasData, nullptr);
			    pageState.setProperty("canvasState", page.canvasState, nullptr);
			    pageState.setProperty("selectedKeywords", page.selectedKeywords.joinIntoString("|"), nullptr);
			    pageState.setProperty("currentSequenceIndex", page.currentSequenceIndex, nullptr);
			    pageState.setProperty("adsrAttack", page.adsrAttack.load(), nullptr);
			    pageState.setProperty("adsrDecay", page.adsrDecay.load(), nullptr);
			    pageState.setProperty("adsrSustain", page.adsrSustain.load(), nullptr);
			    pageState.setProperty("adsrRelease", page.adsrRelease.load(), nullptr);
			    pageState.setProperty("pitchSemitones", page.pitchSemitones.load(), nullptr);
			    pageState.setProperty("fineOffset", page.fineOffset.load(), nullptr);
			    pageState.setProperty("gain", page.gain.load(), nullptr);
			    pageState.setProperty("loopPointsLocked", page.loopPointsLocked.load(), nullptr);
			    pageState.setProperty("savedModelBeforeLocal", page.savedModelBeforeLocal, nullptr);

			    for (int seqIdx = 0; seqIdx < Obsidian::MAX_SEQUENCES; ++seqIdx)
			    {
				    juce::ValueTree sequencerState("Sequence");
				    const auto &seq = page.sequences[seqIdx];

				    sequencerState.setProperty("index", seqIdx, nullptr);
				    sequencerState.setProperty("isPlaying", seq.isPlaying, nullptr);
				    sequencerState.setProperty("currentStep", seq.currentStep, nullptr);
				    sequencerState.setProperty("currentMeasure", seq.currentMeasure, nullptr);
				    sequencerState.setProperty("numMeasures", seq.numMeasures, nullptr);
				    sequencerState.setProperty("beatsPerMeasure", seq.beatsPerMeasure, nullptr);

				    for (int m = 0; m < Obsidian::MAX_MEASURES; ++m)
				    {
					    for (int s = 0; s < Obsidian::MAX_STEPS_PER_MEASURE; ++s)
					    {
						    juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
						    sequencerState.setProperty(stepKey, seq.steps[m][s], nullptr);

						    juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
						    sequencerState.setProperty(velocityKey, seq.velocities[m][s], nullptr);
					    }
				    }

				    pageState.appendChild(sequencerState, nullptr);
			    }

			    trackState.appendChild(pageState, nullptr);
		    }

		    state.appendChild(trackState, nullptr);
	    });

	return state;
}

void StateManager::loadState(const juce::ValueTree &state)
{
	audioProcessor.getTrackManager().clearAllTracks();
	audioProcessor.clearPlayingTracks();
	audioProcessor.getTrackManager().resetAllSlots();
	audioProcessor.getSequencerManager().setWasPlaying(false);

	for (auto trackState : state)
	{
		if (!trackState.hasType("Track"))
			continue;

		auto track = std::make_unique<TrackData>();
		audioProcessor.attachPageChangeCallback(track.get());
		track->currentSampleId = trackState.getProperty("currentSampleId", "").toString();
		track->trackId = trackState.getProperty("id", juce::Uuid().toString());
		track->trackName = trackState.getProperty("name", "Track");
		track->slotIndex = trackState.getProperty("slotIndex", -1);
		track->style = trackState.getProperty("style", "");
		track->midiNote = trackState.getProperty("midiNote", 60);
		track->volume = trackState.getProperty("volume", 0.8f);
		track->pan = trackState.getProperty("pan", 0.0f);
		track->isEnabled = trackState.getProperty("enabled", true);
		track->timeStretchRatio = trackState.getProperty("timeStretchRatio", 1.0);
		track->showWaveform = trackState.getProperty("showWaveform", false);
		track->showSequencer = trackState.getProperty("showSequencer", false);
		track->isMuted = trackState.getProperty("muted", false);
		track->isSolo = trackState.getProperty("solo", false);
		track->isPlaying = trackState.getProperty("isPlaying", false);
		track->isArmed = trackState.getProperty("isArmed", false);
		track->isArmedToStop = trackState.getProperty("isArmedToStop", false);
		track->isCurrentlyPlaying = trackState.getProperty("isCurrentlyPlaying", false);
		track->nextHasOriginalVersion = trackState.getProperty("nextHasOriginalVersion", false);
		track->randomRetriggerEnabled = trackState.getProperty("randomRetriggerEnabled", false);
		track->reverseActive.store(trackState.getProperty("reverseActive", false));
		track->randomBeatRepeatInterval = trackState.getProperty("randomBeatRepeatInterval", 3);
		track->beatRepeatPending = trackState.getProperty("beatRepeatPending", false);
		track->beatRepeatStopPending = trackState.getProperty("beatRepeatStopPending", false);
		track->originalReadPosition = trackState.getProperty("originalReadPosition", 0.0);
		track->beatRepeatStartPosition = trackState.getProperty("beatRepeatStartPosition", 0.0);
		track->beatRepeatEndPosition = trackState.getProperty("beatRepeatEndPosition", 0.0);
		track->beatRepeatActive = trackState.getProperty("beatRepeatActive", false);
		track->randomRetriggerDurationEnabled = trackState.getProperty("randomRetriggerDurationEnabled", false);
		track->currentPageIndex.store(trackState.getProperty("currentPageIndex", 0));
		track->isSelected.store(trackState.getProperty("isSelected", track->slotIndex == 0));
		track->transientScatterActive.store(trackState.getProperty("transientScatterActive", false));

		track->distortion.reset();
		track->bitCrusher.reset();
		track->filter.reset();
		track->equalizer.reset();
		track->compressor.reset();
		track->limiter.reset();
		track->chorus.reset();
		track->phaser.reset();
		track->flanger.reset();
		track->delaySendProcessor.reset();
		track->reverbSendProcessor.reset();

		juce::dsp::ProcessSpec spec = juce::dsp::ProcessSpec();
		spec.maximumBlockSize = static_cast<juce::uint32>(audioProcessor.getBlockSize());
		spec.numChannels = 2;
		spec.sampleRate = audioProcessor.getSampleRate();

		track->distortion.prepare(spec);
		track->bitCrusher.prepare(spec);
		track->filter.prepare(spec);
		track->equalizer.prepare(spec);
		track->compressor.prepare(spec);
		track->limiter.prepare(spec);
		track->chorus.prepare(spec);
		track->phaser.prepare(spec);
		track->flanger.prepare(spec);

		int modeAsInt = trackState.getProperty("filterMode");
		auto mode = static_cast<juce::dsp::LadderFilterMode>(modeAsInt);
		track->filter.setMode(mode);
		track->filter.setCutoffFrequency(trackState.getProperty("cutoffFrequency", Obsidian::FILTER_CUT));
		track->filter.setResonance(trackState.getProperty("resonance", Obsidian::FILTER_RES));
		track->filter.setDrive(trackState.getProperty("filterDrive", Obsidian::FILTER_DRIVE));
		track->filter.setBypassed(trackState.getProperty("filterBypassed", Obsidian::FILTER_BYPASSED));

		int typeAsInt = trackState.getProperty("distortionType");
		auto type = static_cast<Obsidian::distortionType>(typeAsInt);
		track->distortion.setType(type);

		track->distortion.setPre(trackState.getProperty("distortionPreGain", Obsidian::DISTORTION_PRE));
		track->distortion.setPost(trackState.getProperty("distortionPostGain", Obsidian::DISTORTION_POST));
		track->distortion.setCut(trackState.getProperty("distortionCut", Obsidian::DISTORTION_CUT));
		track->distortion.setBypassed(trackState.getProperty("distortionBypassed", Obsidian::DISTORTION_BYPASSED));

		track->equalizer.updateGain(Obsidian::eqBands::subBass,
		                            trackState.getProperty("subBassGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::bass,
		                            trackState.getProperty("bassGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::lowMid,
		                            trackState.getProperty("lowMidGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::mid, trackState.getProperty("midGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::highMid,
		                            trackState.getProperty("highMidGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::presence,
		                            trackState.getProperty("presenceGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::high,
		                            trackState.getProperty("highGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.updateGain(Obsidian::eqBands::air, trackState.getProperty("airGain", Obsidian::EQ_BANDS_GAIN));
		track->equalizer.setBypassed(trackState.getProperty("eqBypassed", Obsidian::EQ_BYPASSED));

		track->compressor.setThreshold(trackState.getProperty("compressorThreshold", Obsidian::COMPRESSOR_THRESHOLD));
		track->compressor.setRatio(trackState.getProperty("compressorRatio", Obsidian::COMPRESSOR_RATIO));
		track->compressor.setAttack(trackState.getProperty("compressorAttack", Obsidian::COMPRESSOR_ATTACK));
		track->compressor.setRelease(trackState.getProperty("compressorRelease", Obsidian::COMPRESSOR_RELEASE));
		track->compressor.setMakeUpGain(
		    trackState.getProperty("compressorMakeUpGain", Obsidian::COMPRESSOR_MAKEUP_GAIN));
		track->compressor.setBypassed(trackState.getProperty("compressorBypassed", Obsidian::COMPRESSOR_BYPASSED));

		track->limiter.setThreshold(trackState.getProperty("limiterThreshold", Obsidian::LIMITER_THRESHOLD));
		track->limiter.setRelease(trackState.getProperty("limiterRelease", Obsidian::LIMITER_RELEASE));
		track->limiter.setMakeUpGain(trackState.getProperty("limiterMakeUpGain", Obsidian::LIMITER_MAKEUP_GAIN));
		track->limiter.setBypassed(trackState.getProperty("limiterBypassed", Obsidian::LIMITER_BYPASSED));

		track->chorus.setRate(trackState.getProperty("chorusRate", Obsidian::CHORUS_RATE));
		track->chorus.setDepth(trackState.getProperty("chorusDepth", Obsidian::CHORUS_DEPTH));
		track->chorus.setCentre(trackState.getProperty("chorusCentre", Obsidian::CHORUS_CENTRE));
		track->chorus.setFeedback(trackState.getProperty("chorusFeedback", Obsidian::CHORUS_FEEDBACK));
		track->chorus.setMix(trackState.getProperty("chorusMix", Obsidian::CHORUS_MIX));
		track->chorus.setBypassed(trackState.getProperty("chorusBypassed", Obsidian::CHORUS_BYPASSED));

		track->phaser.setRate(trackState.getProperty("phaserRate", Obsidian::PHASER_RATE));
		track->phaser.setDepth(trackState.getProperty("phaserDepth", Obsidian::PHASER_DEPTH));
		track->phaser.setCentre(trackState.getProperty("phaserCentre", Obsidian::PHASER_CENTRE));
		track->phaser.setFeedback(trackState.getProperty("phaserFeedback", Obsidian::PHASER_FEEDBACK));
		track->phaser.setMix(trackState.getProperty("phaserMix", Obsidian::PHASER_MIX));
		track->phaser.setBypassed(trackState.getProperty("phaserBypassed", Obsidian::PHASER_BYPASSED));

		track->flanger.setRate(trackState.getProperty("flangerRate", Obsidian::FLANGER_RATE));
		track->flanger.setDepth(trackState.getProperty("flangerDepth", Obsidian::FLANGER_DEPTH));
		track->flanger.setCentre(trackState.getProperty("flangerCentre", Obsidian::FLANGER_CENTRE));
		track->flanger.setFeedback(trackState.getProperty("flangerFeedback", Obsidian::FLANGER_FEEDBACK));
		track->flanger.setMix(trackState.getProperty("flangerMix", Obsidian::FLANGER_MIX));
		track->flanger.setBypassed(trackState.getProperty("flangerBypassed", Obsidian::FLANGER_BYPASSED));

		track->bitCrusher.setBitDepth(trackState.getProperty("bitcrusherBitDepth", Obsidian::BITCRUSHER_BIT_DEPTH));
		track->bitCrusher.setSampleRateReduction(
		    trackState.getProperty("bitcrusherRate", Obsidian::BITCRUSHER_SAMPLE_RATE_REDUCTION));
		track->bitCrusher.setMix(trackState.getProperty("bitcrusherMix", Obsidian::BITCRUSHER_MIX));
		track->bitCrusher.setBypassed(trackState.getProperty("bitcrusherBypassed", Obsidian::BITCRUSHER_BYPASSED));

		track->delaySendProcessor.prepare(audioProcessor.getSampleRate(),
		                                  static_cast<juce::uint32>(audioProcessor.getBlockSize()));
		track->reverbSendProcessor.prepare(audioProcessor.getSampleRate(),
		                                   static_cast<juce::uint32>(audioProcessor.getBlockSize()));

		track->isPrepared.store(true);

		for (int pageIndex = 0; pageIndex < Obsidian::MAX_PAGES; ++pageIndex)
		{
			juce::ValueTree pageState;

			for (int childIndex = 0; childIndex < trackState.getNumChildren(); ++childIndex)
			{
				auto child = trackState.getChild(childIndex);
				if (child.hasType("Page"))
				{
					int storedPageIndex = child.getProperty("index", -1);
					if (storedPageIndex == pageIndex)
					{
						pageState = child;
						break;
					}
				}
			}

			if (pageState.isValid())
			{
				auto &page = track->pages[pageIndex];
				page.originalFilePath = pageState.getProperty("originalFilePath", "").toString();
				page.audioFilePath = pageState.getProperty("audioFilePath", "").toString();
				page.numSamples = pageState.getProperty("numSamples", 0);
				page.sampleRate = pageState.getProperty("sampleRate", Obsidian::SAMPLERATE);
				page.originalBpm = pageState.getProperty("originalBpm", 126.0f);
				page.prompt = pageState.getProperty("prompt", "").toString();
				page.setSelectedPrompt(pageState.getProperty("selectedPrompt", "").toString());
				page.selectedModel = pageState.getProperty("selectedModel", "stable-audio-open-1.0").toString();
				page.generationPrompt = pageState.getProperty("generationPrompt", "").toString();
				page.generationBpm = pageState.getProperty("generationBpm", 126.0f);
				page.generationKey = pageState.getProperty("generationKey", "").toString();
				page.generationDuration = pageState.getProperty("generationDuration", 6);
				page.loopStart = pageState.getProperty("loopStart", 0.0);
				page.loopEnd = pageState.getProperty("loopEnd", 4.0);
				page.useOriginalFile = pageState.getProperty("useOriginalFile", false);
				page.hasOriginalVersion = pageState.getProperty("hasOriginalVersion", false);
				page.canvasData = pageState.getProperty("canvasData", "").toString();
				page.canvasState = pageState.getProperty("canvasState", "").toString();
				page.pitchSemitones.store(pageState.getProperty("pitchSemitones", 0.0f));
				page.fineOffset.store(pageState.getProperty("fineOffset", 0.0f));
				page.loopPointsLocked = pageState.getProperty("loopPointsLocked", false);
				page.savedModelBeforeLocal =
				    pageState.getProperty("savedModelBeforeLocal", "stable-audio-open-1.0").toString();

				juce::String pageKeywordsStr = pageState.getProperty("selectedKeywords", "");
				if (pageKeywordsStr.isNotEmpty())
				{
					page.selectedKeywords.addTokens(pageKeywordsStr, "|", "");
				}

				page.isLoaded = false;
				page.currentSequenceIndex = pageState.getProperty("currentSequenceIndex", 0);
				page.adsrAttack.store(pageState.getProperty("adsrAttack", Obsidian::ADSRDefaultValues::ATTACK_DEFAULT));
				page.adsrDecay.store(pageState.getProperty("adsrDecay", Obsidian::ADSRDefaultValues::DECAY_DEFAULT));
				page.adsrSustain.store(
				    pageState.getProperty("adsrSustain", Obsidian::ADSRDefaultValues::SUSTAIN_DEFAULT));
				page.adsrRelease.store(
				    pageState.getProperty("adsrRelease", Obsidian::ADSRDefaultValues::RELEASE_DEFAULT));
				page.gain.store(pageState.getProperty("gain", 0.0f));

				for (int seqIdx = 0; seqIdx < Obsidian::MAX_SEQUENCES; ++seqIdx)
				{
					juce::ValueTree sequencerState;

					for (int childIndex = 0; childIndex < pageState.getNumChildren(); ++childIndex)
					{
						auto child = pageState.getChild(childIndex);
						if (child.hasType("Sequence"))
						{
							int storedSeqIndex = child.getProperty("index", -1);
							if (storedSeqIndex == seqIdx)
							{
								sequencerState = child;
								break;
							}
						}
					}

					if (sequencerState.isValid())
					{
						auto &seq = page.sequences[seqIdx];
						seq.isPlaying = sequencerState.getProperty("isPlaying", false);
						seq.currentStep = 0;
						seq.currentMeasure = 0;
						seq.numMeasures = sequencerState.getProperty("numMeasures", 1);
						seq.beatsPerMeasure = sequencerState.getProperty("beatsPerMeasure", 4);

						for (int m = 0; m < Obsidian::MAX_MEASURES; ++m)
						{
							for (int s = 0; s < Obsidian::MAX_STEPS_PER_MEASURE; ++s)
							{
								juce::String stepKey = "step_" + juce::String(m) + "_" + juce::String(s);
								seq.steps[m][s] = sequencerState.getProperty(stepKey, false);

								juce::String velocityKey = "velocity_" + juce::String(m) + "_" + juce::String(s);
								seq.velocities[m][s] = sequencerState.getProperty(velocityKey, 0.8f);
							}
						}
					}
					else
					{
						auto &seq = page.sequences[seqIdx];
						if (seqIdx == 0)
						{
							seq.steps[0][0] = true;
							seq.velocities[0][0] = 0.8f;
						}
					}
				}

				if (!page.audioFilePath.isEmpty())
				{
					juce::File audioFile(page.audioFilePath);
					if (audioFile.existsAsFile())
					{
						juce::File fileToLoad = audioFile;

						if (page.useOriginalFile.load() && page.hasOriginalVersion.load())
						{
							if (page.originalFilePath.isNotEmpty())
							{
								juce::File originalFile(page.originalFilePath);
								if (originalFile.existsAsFile())
									fileToLoad = originalFile;
							}
							else
							{
								char pageName = static_cast<char>('A' + pageIndex);
								juce::String fileName = audioFile.getFileNameWithoutExtension();
								juce::String legacySuffix = "_" + juce::String(65 + pageIndex);
								juce::String newSuffix = "_" + juce::String::charToString(pageName);

								juce::String baseTrackId;
								juce::String actualSuffix;

								if (fileName.endsWith(newSuffix))
								{
									baseTrackId = fileName.dropLastCharacters(newSuffix.length());
									actualSuffix = newSuffix;
								}
								else if (fileName.endsWith(legacySuffix))
								{
									baseTrackId = fileName.dropLastCharacters(legacySuffix.length());
									actualSuffix = legacySuffix;
								}
								if (baseTrackId.isNotEmpty())
								{
									juce::File originalFile = audioFile.getParentDirectory().getChildFile(
									    baseTrackId + "_original" + actualSuffix + ".wav");

									if (originalFile.existsAsFile())
									{
										fileToLoad = originalFile;
									}
								}
							}
						}
						audioProcessor.getTrackManager().loadAudioFileForPage(track.get(), pageIndex, fileToLoad);
					}
				}
			}
		}

		if (track->slotIndex < 0 || track->slotIndex >= 8 ||
		    audioProcessor.getTrackManager().isSlotUsed(track->slotIndex))
		{
			track->slotIndex = audioProcessor.getTrackManager().findFreeSlot();
		}
		if (track->slotIndex >= 0 && track->slotIndex < Obsidian::MAX_TRACKS)
		{
			audioProcessor.getTrackManager().setSlotUsed(track->slotIndex, true);
		}

		juce::String stdId = track->trackId.toStdString();
		audioProcessor.getTrackManager().addTrack(stdId, std::move(track));
	}
	int trackCount = static_cast<int>(audioProcessor.getTrackManager().getNumTracks());
	for (int i = trackCount; i < Obsidian::MAX_TRACKS; ++i)
	{
		auto track = std::make_unique<TrackData>();
		audioProcessor.attachPageChangeCallback(track.get());
		track->trackName = "Track " + juce::String(i + 1);
		track->midiNote = 60 + i;
		track->slotIndex = audioProcessor.getTrackManager().findFreeSlot();

		const bool isLocalMode = audioProcessor.getUseLocalModel();
		auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);

		if (!modelsForMode.isEmpty())
		{
			for (int p = 0; p < Obsidian::MAX_PAGES; ++p)
				track->pages[p].selectedModel = modelsForMode[i % modelsForMode.size()];
		}

		if (track->slotIndex >= 0 && track->slotIndex < Obsidian::MAX_TRACKS)
			audioProcessor.getTrackManager().setSlotUsed(track->slotIndex, true);

		juce::String stdId = track->trackId.toStdString();
		audioProcessor.getTrackManager().addTrack(stdId, std::move(track));
	}
}

void StateManager::getStateInformation(juce::MemoryBlock &destData)
{
	juce::ValueTree state("DjIaVstState");

	state.setProperty("formatVersion", 1, nullptr);
	state.setProperty("appVersion", juce::String(Version::FULL), nullptr);

	const juce::String lineageToken = juce::Uuid().toString();
	state.setProperty("lineageToken", lineageToken, nullptr);
	getLineageSidecarFile(audioProcessor.getProjectId()).replaceWithText(lineageToken);

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (auto *standaloneTransport = audioProcessor.getStandaloneTransport())
		{
			state.setProperty("standaloneBpm", standaloneTransport->getBpm(), nullptr);
			state.setProperty("standaloneTimeSigNum", standaloneTransport->getTimeSigNumerator(), nullptr);
			state.setProperty("standaloneTimeSigDenom", standaloneTransport->getTimeSigDenominator(), nullptr);
		}
	}

	state.setProperty("projectId", audioProcessor.getProjectId(), nullptr);
	state.setProperty("lastPrompt", audioProcessor.getLastPrompt(), nullptr);
	state.setProperty("lastKey", audioProcessor.getLastKey(), nullptr);
	state.setProperty("lastBpm", audioProcessor.getLastBpm(), nullptr);
	state.setProperty("lastPresetIndex", audioProcessor.getLastPresetIndex(), nullptr);
	state.setProperty("hostBpmEnabled", audioProcessor.isHostBpmEnabled(), nullptr);
	state.setProperty("lastDuration", audioProcessor.getLastDuration(), nullptr);
	state.setProperty("lastKeyIndex", audioProcessor.getLastKeyIndex(), nullptr);
	state.setProperty("isGenerating", false, nullptr);
	state.setProperty("autoLoadEnabled", audioProcessor.getAutoLoadEnabled(), nullptr);
	state.setProperty("generatingTrackId", audioProcessor.getGeneratingTrackId(), nullptr);
	state.setProperty("bypassSequencer", audioProcessor.getBypassSequencer(), nullptr);
	state.setProperty("crossfaderValue", audioProcessor.getCrossfaderValue(), nullptr);
	state.setProperty("crossfadeMode", audioProcessor.getCrossfadeMode(), nullptr);
	state.setProperty("windowWidth", audioProcessor.getSavedWindowWidth(), nullptr);
	state.setProperty("windowHeight", audioProcessor.getSavedWindowHeight(), nullptr);
	state.setProperty("bankVisible", audioProcessor.getSavedPanelVisible(), nullptr);
	state.setProperty("useLocalModel", audioProcessor.getUseLocalModel(), nullptr);

	state.setProperty("masterEQGainSubBass", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::subBass),
	                  nullptr);
	state.setProperty("masterEQGainBass", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::bass), nullptr);
	state.setProperty("masterEQGainLowMid", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::lowMid),
	                  nullptr);
	state.setProperty("masterEQGainMid", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::mid), nullptr);
	state.setProperty("masterEQGainHiMid", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::highMid),
	                  nullptr);
	state.setProperty("masterEQGainPresence", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::presence),
	                  nullptr);
	state.setProperty("masterEQGainHigh", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::high), nullptr);
	state.setProperty("masterEQGainAir", audioProcessor.getEqualizer().getFrequency(Obsidian::eqBands::air), nullptr);

	state.setProperty("masterCompressorThreshold", audioProcessor.getCompressor().getThreshold(), nullptr);
	state.setProperty("masterCompressorRatio", audioProcessor.getCompressor().getRatio(), nullptr);
	state.setProperty("masterCompressorAttack", audioProcessor.getCompressor().getAttack(), nullptr);
	state.setProperty("masterCompressorRelease", audioProcessor.getCompressor().getRelease(), nullptr);
	state.setProperty("masterCompressorMakeUpGain", audioProcessor.getCompressor().getMakeUpGain(), nullptr);

	state.setProperty("masterLimiterThreshold", audioProcessor.getLimiter().getThreshold(), nullptr);
	state.setProperty("masterLimiterRelease", audioProcessor.getLimiter().getRelease(), nullptr);
	state.setProperty("masterLimiterMakeUpGain", audioProcessor.getLimiter().getMakeUpGain(), nullptr);

	state.setProperty("masterCompressorBypassed", audioProcessor.getCompressor().isBypassed(), nullptr);
	state.setProperty("masterLimiterBypassed", audioProcessor.getLimiter().isBypassed(), nullptr);
	state.setProperty("masterEQBypassed", audioProcessor.getEqualizer().isBypassed(), nullptr);

	juce::ValueTree midiMappingsState("MidiMappings");
	auto mappings = audioProcessor.getMidiLearnManager().getAllMappings();
	for (int i = 0; i < mappings.size(); ++i)
	{
		const auto &mapping = mappings[i];
		juce::ValueTree mappingState("Mapping");
		mappingState.setProperty("midiType", mapping.midiType, nullptr);
		mappingState.setProperty("midiNumber", mapping.midiNumber, nullptr);
		mappingState.setProperty("midiChannel", mapping.midiChannel, nullptr);
		mappingState.setProperty("parameterName", mapping.parameterName, nullptr);
		mappingState.setProperty("description", mapping.description, nullptr);
		midiMappingsState.appendChild(mappingState, nullptr);
	}
	state.appendChild(midiMappingsState, nullptr);

	auto tracksState = saveState();
	state.appendChild(tracksState, nullptr);

	juce::ValueTree parametersState("Parameters");

	auto &params = audioProcessor.getParameterTreeState();
	for (const auto &paramId : audioProcessor.getBooleanParamIds())
	{
		auto *param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	for (const auto &paramId : audioProcessor.getFloatParamIds())
	{
		auto *param = params.getParameter(paramId);
		if (param)
		{
			parametersState.setProperty(paramId, param->getValue(), nullptr);
		}
	}
	state.appendChild(parametersState, nullptr);

	auto globalGenState = juce::ValueTree("GlobalGeneration");
	globalGenState.setProperty("prompt", audioProcessor.getGlobalPrompt(), nullptr);
	globalGenState.setProperty("bpm", audioProcessor.getGlobalBpm(), nullptr);
	globalGenState.setProperty("key", audioProcessor.getGlobalKey(), nullptr);
	globalGenState.setProperty("duration", audioProcessor.getGlobalDuration(), nullptr);

	state.appendChild(globalGenState, nullptr);

	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	audioProcessor.copyXmlToBinary(*xml, destData);
}

void StateManager::setStateInformation(const void *data, int sizeInBytes)
{
	audioProcessor.setIsLoadingState(true);
	std::unique_ptr<juce::XmlElement> xml(audioProcessor.getXmlFromBinary(data, sizeInBytes));
	if (!xml || !xml->hasTagName("DjIaVstState"))
	{
		audioProcessor.setIsLoadingState(false);
		return;
	}

	juce::ValueTree state = juce::ValueTree::fromXml(*xml);

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (auto *standaloneTransport = audioProcessor.getStandaloneTransport())
		{
			standaloneTransport->setBpm(state.getProperty("standaloneBpm", 120.0));
			standaloneTransport->setTimeSignature(state.getProperty("standaloneTimeSigNum", 4),
			                                      state.getProperty("standaloneTimeSigDenom", 4));
		}
	}

	juce::String projectId = state.getProperty("projectId", "legacy").toString();
	bool forkedFromClone = false;

	const juce::String stateToken = state.getProperty("lineageToken", "").toString();
	if (stateToken.isNotEmpty())
	{
		auto sidecar = getLineageSidecarFile(projectId);
		const juce::String diskToken = sidecar.existsAsFile() ? sidecar.loadFileAsString().trim() : juce::String();

		if (diskToken.isNotEmpty() && diskToken != stateToken)
		{
			auto redirectFile = sidecar.getSiblingFile(Obsidian::FORKS_FILE());
			juce::String previousForkId;
			if (redirectFile.existsAsFile())
			{
				juce::StringArray lines;
				lines.addLines(redirectFile.loadFileAsString());
				for (const auto &line : lines)
					if (line.startsWith(stateToken + " "))
					{
						previousForkId = line.fromFirstOccurrenceOf(" ", false, false).trim();
						break;
					}
			}

			if (previousForkId.isNotEmpty())
				projectId = previousForkId;
			else
			{
				projectId = juce::Uuid().toString();
				forkedFromClone = true;
				redirectFile.appendText(stateToken + " " + projectId + "\n");
				getLineageSidecarFile(projectId).replaceWithText(stateToken);
			}
		}
	}
	audioProcessor.setProjectId(projectId);
	audioProcessor.setLastPrompt(state.getProperty("lastPrompt", "").toString());
	audioProcessor.setLastKey(state.getProperty("lastKey", "C minor").toString());
	audioProcessor.setLastBpm(state.getProperty("lastBpm", 126.0));
	audioProcessor.setLastPresetIndex(state.getProperty("lastPresetIndex", -1));
	audioProcessor.setHostBpmEnabled(state.getProperty("hostBpmEnabled", false));
	audioProcessor.setLastDuration(state.getProperty("lastDuration", 6.0));
	audioProcessor.setLastKeyIndex(state.getProperty("lastKeyIndex", 1));
	audioProcessor.setGeneratingTrackId(state.getProperty("generatingTrackId", "").toString());
	audioProcessor.setAutoLoadEnabled(state.getProperty("autoLoadEnabled", true));
	audioProcessor.setWindowSize(state.getProperty("windowWidth", 1620), state.getProperty("windowHeight", 840));
	audioProcessor.setPanelVisible(state.getProperty("bankVisible", true));
	audioProcessor.setUseLocalModel(state.getProperty("useLocalModel", false));

	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::subBass,
	                                         state.getProperty("masterEQGainSubBass", Obsidian::EQ_SUB_BAS_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::bass,
	                                         state.getProperty("masterEQGainBass", Obsidian::EQ_BASS_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::lowMid,
	                                         state.getProperty("masterEQGainLowMid", Obsidian::EQ_LOW_MID_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::mid,
	                                         state.getProperty("masterEQGainMid", Obsidian::EQ_MID_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::highMid,
	                                         state.getProperty("masterEQGainHiMid", Obsidian::EQ_HI_MID_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::presence,
	                                         state.getProperty("masterEQGainPresence", Obsidian::EQ_PRESENCE_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::high,
	                                         state.getProperty("masterEQGainHigh", Obsidian::EQ_HI_FRQ));
	audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::air,
	                                         state.getProperty("masterEQGainAir", Obsidian::EQ_AIR_FRQ));

	audioProcessor.getCompressor().setThreshold(
	    state.getProperty("masterCompressorThreshold", Obsidian::COMPRESSOR_THRESHOLD));
	audioProcessor.getCompressor().setRatio(state.getProperty("masterCompressorRatio", Obsidian::COMPRESSOR_RATIO));
	audioProcessor.getCompressor().setAttack(state.getProperty("masterCompressorAttack", Obsidian::COMPRESSOR_ATTACK));
	audioProcessor.getCompressor().setRelease(
	    state.getProperty("masterCompressorRelease", Obsidian::COMPRESSOR_RELEASE));
	audioProcessor.getCompressor().setMakeUpGain(
	    state.getProperty("masterCompressorMakeUpGain", Obsidian::COMPRESSOR_MAKEUP_GAIN));

	audioProcessor.getLimiter().setThreshold(state.getProperty("masterLimiterThreshold", Obsidian::LIMITER_THRESHOLD));
	audioProcessor.getLimiter().setRelease(state.getProperty("masterLimiterRelease", Obsidian::LIMITER_RELEASE));
	audioProcessor.getLimiter().setMakeUpGain(
	    state.getProperty("masterLimiterMakeUpGain", Obsidian::LIMITER_MAKEUP_GAIN));

	audioProcessor.getCompressor().setBypassed(
	    state.getProperty("masterCompressorBypassed", Obsidian::COMPRESSOR_BYPASSED));
	audioProcessor.getLimiter().setBypassed(state.getProperty("masterLimiterBypassed", Obsidian::LIMITER_BYPASSED));
	audioProcessor.getEqualizer().setBypassed(state.getProperty("masterEQBypassed", Obsidian::EQ_BYPASSED));

	bool bypassValue = state.getProperty("bypassSequencer", false);
	audioProcessor.setBypassSequencer(bypassValue);
	auto tracksState = state.getChildWithName("TrackManager");
	if (tracksState.isValid())
	{
		loadState(tracksState);
		if (forkedFromClone)
			if (auto *bank = audioProcessor.getSampleBank())
				for (const auto &trackId : audioProcessor.getAllTrackIds())
				{
					auto *track = audioProcessor.getTrack(trackId);
					if (track && track->currentSampleId.isNotEmpty())
						bank->markSampleAsUsed(track->currentSampleId, projectId);
				}

		const double sampleRate = audioProcessor.getSampleRate();
		const int blockSize = audioProcessor.getBlockSize();
		if (sampleRate > 0 && blockSize > 0)
			audioProcessor.getTrackManager().prepareSends(sampleRate, blockSize);
	}

	audioProcessor.setCrossfaderValue((float)state.getProperty("crossfaderValue", juce::var(0.5f)));
	audioProcessor.setCrossfadeMode((int)state.getProperty("crossfadeMode", juce::var(0)));

	auto paramsCheck = state.getChildWithName("Parameters");
	const bool needsCrossfaderMigration = !paramsCheck.isValid() || !paramsCheck.hasProperty("globalCrossfader");

	if (needsCrossfaderMigration)
	{
		if (state.hasProperty("globalCrossfader"))
		{
			if (auto *p = audioProcessor.getParameters().getParameter("globalCrossfader"))
				p->setValueNotifyingHost((float)state.getProperty("globalCrossfader", 0.5f));
		}
		for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
		{
			juce::String oldKey = "pairCrossfader" + juce::String(i);
			juce::String newId = "pairCrossfader" + juce::String(i + 1);
			if (state.hasProperty(oldKey))
			{
				if (auto *p = audioProcessor.getParameters().getParameter(newId))
					p->setValueNotifyingHost((float)state.getProperty(oldKey, 0.5f));
			}
		}
		if (state.hasProperty("crossfaderCurveMode"))
		{
			int mode = juce::jlimit(0, 2, (int)state.getProperty("crossfaderCurveMode", 1));
			if (auto *p = audioProcessor.getParameters().getParameter("crossfaderCurveMode"))
				p->setValueNotifyingHost(mode / 2.0f);
		}
	}

	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		audioProcessor.setPairCrossfaderPrevious(i, audioProcessor.getParameterManager().getPairCrossfader(i));
	}
	audioProcessor.setGlobalCrossfaderPrevious(audioProcessor.getParameterManager().getGlobalCrossfader());

	juce::ValueTree midiMappingsState = state.getChildWithName("MidiMappings");
	if (midiMappingsState.isValid())
	{
		audioProcessor.getMidiLearnManager().clearAllMappings();
		for (int i = 0; i < midiMappingsState.getNumChildren(); ++i)
		{
			MidiMapping mapping;
			juce::ValueTree mappingState = midiMappingsState.getChild(i);
			mapping.midiType = mappingState.getProperty("midiType");
			mapping.midiNumber = mappingState.getProperty("midiNumber");
			mapping.midiChannel = mappingState.getProperty("midiChannel");
			mapping.parameterName = mappingState.getProperty("parameterName");
			mapping.description = mappingState.getProperty("description");
			mapping.processor = &audioProcessor;
			mapping.uiCallback = nullptr;
			audioProcessor.getMidiLearnManager().addMapping(mapping);
		}
	}
	auto globalGenState = state.getChildWithName("GlobalGeneration");
	if (globalGenState.isValid())
	{
		audioProcessor.setGlobalPrompt(globalGenState.getProperty("prompt", "Generate a techno drum loop"));
		audioProcessor.setGlobalBpm(globalGenState.getProperty("bpm", 127.0f));
		audioProcessor.setGlobalKey(globalGenState.getProperty("key", "C Minor"));
		audioProcessor.setGlobalDuration(globalGenState.getProperty("duration", 6));
	}
	auto parametersState = state.getChildWithName("Parameters");
	if (parametersState.isValid())
	{
		auto &params = audioProcessor.getParameterTreeState();
		for (int slot = 1; slot <= 8; ++slot)
		{
			for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			{
				juce::String paramId = "slot" + juce::String(slot) + page;
				if (auto *param = params.getParameter(paramId))
					param->setValueNotifyingHost(0.0f);
			}
			for (int seq = 1; seq <= 8; ++seq)
			{
				juce::String paramId = "slot" + juce::String(slot) + "Seq" + juce::String(seq);
				if (auto *param = params.getParameter(paramId))
					param->setValueNotifyingHost(0.0f);
			}
		}
		for (const auto &paramId : audioProcessor.getBooleanParamIds())
		{
			if (parametersState.hasProperty(paramId))
			{
				auto *param = params.getParameter(paramId);
				if (param)
				{
					float value = parametersState.getProperty(paramId, 0.0f);
					if (paramId.contains("Bypassed"))
						continue;
					param->setValueNotifyingHost(value);
				}
			}
		}
		for (const auto &paramId : audioProcessor.getFloatParamIds())
		{
			if (parametersState.hasProperty(paramId))
			{
				auto *param = params.getParameter(paramId);
				if (param)
				{
					param->setValueNotifyingHost(parametersState.getProperty(paramId, 0.0f));
				}
			}
		}
	}

	juce::Timer::callAfterDelay(2000, [this]() { audioProcessor.getMidiManager().sendFullStateFeedback(); });

	audioProcessor.setStateReady(true);
	audioProcessor.setIsLoadingState(false);
	audioProcessor.setNeedsUIRefreshAfterLoad(true);
}

juce::File StateManager::getDefaultSessionsFolder()
{
	auto folder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                  .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
	                  .getChildFile(Obsidian::SESSIONS_DIR());

	if (!folder.exists())
		folder.createDirectory();

	return folder;
}

bool StateManager::saveToFile(const juce::File &file)
{
	juce::ValueTree state("DjIaVstState");

	juce::MemoryBlock data;
	getStateInformation(data);

	std::unique_ptr<juce::XmlElement> xml(audioProcessor.getXmlFromBinary(data.getData(), (int)data.getSize()));

	if (!xml)
		return false;

	auto xmlString = xml->toString();

	juce::MemoryOutputStream stream;
	std::string magicStr = Obsidian::MAGIC();
	stream.write(magicStr.data(), magicStr.length());
	std::fill(magicStr.begin(), magicStr.end(), 0);
	stream.writeInt(1);
	stream.writeInt((int)xmlString.getNumBytesAsUTF8());
	stream.write(xmlString.toRawUTF8(), xmlString.getNumBytesAsUTF8());

	return file.withFileExtension(".obsidian").replaceWithData(stream.getData(), stream.getDataSize());
}

bool StateManager::loadFromFile(const juce::File &file)
{
	if (!file.existsAsFile())
		return false;

	juce::MemoryBlock raw;
	if (!file.loadFileAsData(raw))
		return false;

	juce::MemoryInputStream stream(raw, false);

	char magic[9] = {};
	stream.read(magic, 8);
	if (juce::String(magic).toStdString() != Obsidian::MAGIC())
		return false;

	[[maybe_unused]] int version = stream.readInt();
	int dataSize = stream.readInt();

	juce::MemoryBlock xmlData;
	xmlData.setSize(dataSize);
	stream.read(xmlData.getData(), dataSize);

	juce::String xmlString = juce::String::fromUTF8(static_cast<const char *>(xmlData.getData()), dataSize);

	auto xml = juce::parseXML(xmlString);
	if (!xml)
		return false;

	juce::MemoryBlock stateData;
	audioProcessor.copyXmlToBinary(*xml, stateData);
	setStateInformation(stateData.getData(), (int)stateData.getSize());
	return true;
}

juce::File StateManager::getLineageSidecarFile(const juce::String &projectId) const
{
	auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	               .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
	               .getChildFile(Obsidian::AUDIO_CACHE_DIR());
	if (projectId != "legacy" && projectId.isNotEmpty())
		dir = dir.getChildFile(projectId);
	dir.createDirectory();
	return dir.getChildFile(Obsidian::LINEAGE_FILE());
}