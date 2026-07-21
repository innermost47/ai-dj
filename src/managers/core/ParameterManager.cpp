#include "ParameterManager.h"
#include "MidiMapping.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

ParameterManager::ParameterManager(DjIaVstProcessor &processor)
    : audioProcessor(processor), apvts(processor, nullptr, "Parameters", createParameterLayout())
{
}

void ParameterManager::resolveParameters(juce::AudioProcessorValueTreeState::Listener *listener)
{
	generateParam = apvts.getRawParameterValue("generate");
	playParam = apvts.getRawParameterValue("play");
	masterVolumeParam = apvts.getRawParameterValue("masterVolume");
	masterPanParam = apvts.getRawParameterValue("masterPan");
	masterHighParam = apvts.getRawParameterValue("masterHigh");
	masterMidParam = apvts.getRawParameterValue("masterMid");
	masterLowParam = apvts.getRawParameterValue("masterLow");
	useCrossfaderParam = apvts.getRawParameterValue("useCrossfader");

	masterEQGainSubBassParams = apvts.getRawParameterValue("masterEQGainSubBass");
	masterEQGainBassParams = apvts.getRawParameterValue("masterEQGainBass");
	masterEQGainLowMidParams = apvts.getRawParameterValue("masterEQGainLowMid");
	masterEQGainMidParams = apvts.getRawParameterValue("masterEQGainMid");
	masterEQGainHighMidParams = apvts.getRawParameterValue("masterEQGainHiMid");
	masterEQGainPresenceParams = apvts.getRawParameterValue("masterEQGainPresence");
	masterEQGainHighParams = apvts.getRawParameterValue("masterEQGainHigh");
	masterEQGainAirParams = apvts.getRawParameterValue("masterEQGainAir");
	masterEQBypassedParams = apvts.getRawParameterValue("masterEQBypassed");

	masterCompressorThresholdParams = apvts.getRawParameterValue("masterCompressorThreshold");
	masterCompressorRatioParams = apvts.getRawParameterValue("masterCompressorRatio");
	masterCompressorAttackParams = apvts.getRawParameterValue("masterCompressorAttack");
	masterCompressorReleaseParams = apvts.getRawParameterValue("masterCompressorRelease");
	masterCompressorMakeUpGainParams = apvts.getRawParameterValue("masterCompressorMakeUpGain");
	masterCompressorBypassedParams = apvts.getRawParameterValue("masterCompressorBypassed");

	masterLimiterThresholdParams = apvts.getRawParameterValue("masterLimiterThreshold");
	masterLimiterReleaseParams = apvts.getRawParameterValue("masterLimiterRelease");
	masterLimiterMakeUpGainParams = apvts.getRawParameterValue("masterLimiterMakeUpGain");
	masterLimiterBypassedParams = apvts.getRawParameterValue("masterLimiterBypassed");

	apvts.addParameterListener("masterEQGainSubBass", listener);
	apvts.addParameterListener("masterEQGainBass", listener);
	apvts.addParameterListener("masterEQGainLowMid", listener);
	apvts.addParameterListener("masterEQGainMid", listener);
	apvts.addParameterListener("masterEQGainHiMid", listener);
	apvts.addParameterListener("masterEQGainPresence", listener);
	apvts.addParameterListener("masterEQGainHigh", listener);
	apvts.addParameterListener("masterEQGainAir", listener);
	apvts.addParameterListener("masterCompressorThreshold", listener);
	apvts.addParameterListener("masterCompressorRatio", listener);
	apvts.addParameterListener("masterCompressorAttack", listener);
	apvts.addParameterListener("masterCompressorRelease", listener);
	apvts.addParameterListener("masterCompressorMakeUpGain", listener);
	apvts.addParameterListener("masterLimiterThreshold", listener);
	apvts.addParameterListener("masterLimiterRelease", listener);
	apvts.addParameterListener("masterLimiterMakeUpGain", listener);
	apvts.addParameterListener("masterCompressorBypassed", listener);
	apvts.addParameterListener("masterLimiterBypassed", listener);
	apvts.addParameterListener("masterEQBypassed", listener);

	apvts.addParameterListener("generate", listener);
	apvts.addParameterListener("play", listener);
	apvts.addParameterListener("useCrossfader", listener);

	delayDivisionParam = apvts.getRawParameterValue("delayDivision");
	delayFeedbackParam = apvts.getRawParameterValue("delayFeedback");
	delayModeParam = apvts.getRawParameterValue("delayMode");

	apvts.addParameterListener("delayDivision", listener);
	apvts.addParameterListener("delayFeedback", listener);
	apvts.addParameterListener("delayMode", listener);

	reverbSizeParam = apvts.getRawParameterValue("reverbSize");
	reverbDampingParam = apvts.getRawParameterValue("reverbDamping");
	reverbWidthParam = apvts.getRawParameterValue("reverbWidth");
	reverbMixParam = apvts.getRawParameterValue("reverbMix");

	apvts.addParameterListener("reverbSize", listener);
	apvts.addParameterListener("reverbDamping", listener);
	apvts.addParameterListener("reverbWidth", listener);
	apvts.addParameterListener("reverbMix", listener);

	for (int i = 0; i < Obsidian::MAX_TRACKS; ++i)
	{
		juce::String s = "slot" + juce::String(i + 1);

		slotVolumeParams[i] = apvts.getRawParameterValue(s + "Volume");
		slotPanParams[i] = apvts.getRawParameterValue(s + "Pan");
		slotGainParams[i] = apvts.getRawParameterValue(s + "Gain");
		slotMuteParams[i] = apvts.getRawParameterValue(s + "Mute");
		slotSoloParams[i] = apvts.getRawParameterValue(s + "Solo");
		slotPlayParams[i] = apvts.getRawParameterValue(s + "Play");
		slotStopParams[i] = apvts.getRawParameterValue(s + "Stop");
		slotGenerateParams[i] = apvts.getRawParameterValue(s + "Generate");
		slotPitchParams[i] = apvts.getRawParameterValue(s + "Pitch");
		slotFineParams[i] = apvts.getRawParameterValue(s + "Fine");
		slotBeatRepeatActiveParams[i] = apvts.getRawParameterValue(s + "BeatRepeatActive");
		slotBeatRepeatIntervalParams[i] = apvts.getRawParameterValue(s + "BeatRepeatInterval");
		slotReverseActiveParams[i] = apvts.getRawParameterValue(s + "ReverseActive");
		slotAdsrAttackParams[i] = apvts.getRawParameterValue(s + "AdsrAttack");
		slotAdsrDecayParams[i] = apvts.getRawParameterValue(s + "AdsrDecay");
		slotAdsrSustainParams[i] = apvts.getRawParameterValue(s + "AdsrSustain");
		slotAdsrReleaseParams[i] = apvts.getRawParameterValue(s + "AdsrRelease");
		slotDelaySendParams[i] = apvts.getRawParameterValue(s + "DelaySend");
		slotReverbSendParams[i] = apvts.getRawParameterValue(s + "ReverbSend");
		slotTransientScatterActiveParams[i] = apvts.getRawParameterValue(s + "TransientScatterActive");

		slotCutoffParams[i] = apvts.getRawParameterValue(s + "Cutoff");
		slotResonanceParams[i] = apvts.getRawParameterValue(s + "Resonance");
		slotFilterModeParams[i] = apvts.getRawParameterValue(s + "FilterMode");
		slotFilterDriveParams[i] = apvts.getRawParameterValue(s + "FilterDrive");
		slotFilterBypassedParams[i] = apvts.getRawParameterValue(s + "FilterBypassed");

		slotEQGainSubBassParams[i] = apvts.getRawParameterValue(s + "EQGainSubBass");
		slotEQGainBassParams[i] = apvts.getRawParameterValue(s + "EQGainBass");
		slotEQGainLowMidParams[i] = apvts.getRawParameterValue(s + "EQGainLowMid");
		slotEQGainMidParams[i] = apvts.getRawParameterValue(s + "EQGainMid");
		slotEQGainHighMidParams[i] = apvts.getRawParameterValue(s + "EQGainHiMid");
		slotEQGainPresenceParams[i] = apvts.getRawParameterValue(s + "EQGainPresence");
		slotEQGainHighParams[i] = apvts.getRawParameterValue(s + "EQGainHigh");
		slotEQGainAirParams[i] = apvts.getRawParameterValue(s + "EQGainAir");
		slotEQBypassedParams[i] = apvts.getRawParameterValue(s + "EQBypassed");

		slotCompressorThresholdParams[i] = apvts.getRawParameterValue(s + "CompressorThreshold");
		slotCompressorRatioParams[i] = apvts.getRawParameterValue(s + "CompressorRatio");
		slotCompressorAttackParams[i] = apvts.getRawParameterValue(s + "CompressorAttack");
		slotCompressorReleaseParams[i] = apvts.getRawParameterValue(s + "CompressorRelease");
		slotCompressorMakeUpGainParams[i] = apvts.getRawParameterValue(s + "CompressorMakeUpGain");
		slotCompressorBypassedParams[i] = apvts.getRawParameterValue(s + "CompressorBypassed");

		slotLimiterThresholdParams[i] = apvts.getRawParameterValue(s + "LimiterThreshold");
		slotLimiterReleaseParams[i] = apvts.getRawParameterValue(s + "LimiterRelease");
		slotLimiterMakeUpGainParams[i] = apvts.getRawParameterValue(s + "LimiterMakeUpGain");
		slotLimiterBypassedParams[i] = apvts.getRawParameterValue(s + "LimiterBypassed");

		slotDistortionPreGainParams[i] = apvts.getRawParameterValue(s + "DistortionPreGain");
		slotDistortionPostGainParams[i] = apvts.getRawParameterValue(s + "DistortionPostGain");
		slotDistortionCutParams[i] = apvts.getRawParameterValue(s + "DistortionCut");
		slotDistortionBypassedParams[i] = apvts.getRawParameterValue(s + "DistortionBypassed");
		slotDistortionTypeParams[i] = apvts.getRawParameterValue(s + "DistortionType");

		slotChorusRateParams[i] = apvts.getRawParameterValue(s + "ChorusRate");
		slotChorusDepthParams[i] = apvts.getRawParameterValue(s + "ChorusDepth");
		slotChorusCentreParams[i] = apvts.getRawParameterValue(s + "ChorusCentre");
		slotChorusFeedbackParams[i] = apvts.getRawParameterValue(s + "ChorusFeedback");
		slotChorusMixParams[i] = apvts.getRawParameterValue(s + "ChorusMix");
		slotChorusBypassedParams[i] = apvts.getRawParameterValue(s + "ChorusBypassed");

		slotPhaserRateParams[i] = apvts.getRawParameterValue(s + "PhaserRate");
		slotPhaserDepthParams[i] = apvts.getRawParameterValue(s + "PhaserDepth");
		slotPhaserCentreParams[i] = apvts.getRawParameterValue(s + "PhaserCentre");
		slotPhaserFeedbackParams[i] = apvts.getRawParameterValue(s + "PhaserFeedback");
		slotPhaserMixParams[i] = apvts.getRawParameterValue(s + "PhaserMix");
		slotPhaserBypassedParams[i] = apvts.getRawParameterValue(s + "PhaserBypassed");

		slotFlangerRateParams[i] = apvts.getRawParameterValue(s + "FlangerRate");
		slotFlangerDepthParams[i] = apvts.getRawParameterValue(s + "FlangerDepth");
		slotFlangerCentreParams[i] = apvts.getRawParameterValue(s + "FlangerCentre");
		slotFlangerFeedbackParams[i] = apvts.getRawParameterValue(s + "FlangerFeedback");
		slotFlangerMixParams[i] = apvts.getRawParameterValue(s + "FlangerMix");
		slotFlangerBypassedParams[i] = apvts.getRawParameterValue(s + "FlangerBypassed");

		slotBitCrusherBitDepthParams[i] = apvts.getRawParameterValue(s + "BitCrusherBitDepth");
		slotBitCrusherSampleRateReductionParams[i] = apvts.getRawParameterValue(s + "BitCrusherRate");
		slotBitCrusherMixParams[i] = apvts.getRawParameterValue(s + "BitCrusherMix");
		slotBitCrusherBypassedParams[i] = apvts.getRawParameterValue(s + "BitCrusherBypassed");

		apvts.addParameterListener(s + "Generate", listener);
		apvts.addParameterListener(s + "Pitch", listener);
		apvts.addParameterListener(s + "Gain", listener);
		apvts.addParameterListener(s + "Fine", listener);
		apvts.addParameterListener(s + "AdsrAttack", listener);
		apvts.addParameterListener(s + "AdsrDecay", listener);
		apvts.addParameterListener(s + "AdsrSustain", listener);
		apvts.addParameterListener(s + "AdsrRelease", listener);
		apvts.addParameterListener(s + "DelaySend", listener);
		apvts.addParameterListener(s + "ReverbSend", listener);
		apvts.addParameterListener(s + "Seq", listener);
		apvts.addParameterListener(s + "Play", listener);
		apvts.addParameterListener(s + "Mute", listener);
		apvts.addParameterListener(s + "Solo", listener);
		apvts.addParameterListener(s + "Volume", listener);
		apvts.addParameterListener(s + "Pan", listener);
		apvts.addParameterListener(s + "BeatRepeatActive", listener);
		apvts.addParameterListener(s + "BeatRepeatInterval", listener);

		apvts.addParameterListener(s + "Cutoff", listener);
		apvts.addParameterListener(s + "Resonance", listener);
		apvts.addParameterListener(s + "FilterMode", listener);
		apvts.addParameterListener(s + "FilterDrive", listener);
		apvts.addParameterListener(s + "FilterBypassed", listener);

		apvts.addParameterListener(s + "EQGainSubBass", listener);
		apvts.addParameterListener(s + "EQGainBass", listener);
		apvts.addParameterListener(s + "EQGainLowMid", listener);
		apvts.addParameterListener(s + "EQGainMid", listener);
		apvts.addParameterListener(s + "EQGainHiMid", listener);
		apvts.addParameterListener(s + "EQGainPresence", listener);
		apvts.addParameterListener(s + "EQGainHigh", listener);
		apvts.addParameterListener(s + "EQGainAir", listener);
		apvts.addParameterListener(s + "EQBypassed", listener);

		apvts.addParameterListener(s + "CompressorThreshold", listener);
		apvts.addParameterListener(s + "CompressorRatio", listener);
		apvts.addParameterListener(s + "CompressorAttack", listener);
		apvts.addParameterListener(s + "CompressorRelease", listener);
		apvts.addParameterListener(s + "CompressorMakeUpGain", listener);
		apvts.addParameterListener(s + "CompressorBypassed", listener);

		apvts.addParameterListener(s + "LimiterThreshold", listener);
		apvts.addParameterListener(s + "LimiterRelease", listener);
		apvts.addParameterListener(s + "LimiterMakeUpGain", listener);
		apvts.addParameterListener(s + "LimiterBypassed", listener);

		apvts.addParameterListener(s + "DistortionPreGain", listener);
		apvts.addParameterListener(s + "DistortionPostGain", listener);
		apvts.addParameterListener(s + "DistortionCut", listener);
		apvts.addParameterListener(s + "DistortionBypassed", listener);
		apvts.addParameterListener(s + "DistortionType", listener);

		apvts.addParameterListener(s + "ChorusRate", listener);
		apvts.addParameterListener(s + "ChorusDepth", listener);
		apvts.addParameterListener(s + "ChorusCentre", listener);
		apvts.addParameterListener(s + "ChorusFeedback", listener);
		apvts.addParameterListener(s + "ChorusMix", listener);
		apvts.addParameterListener(s + "ChorusBypassed", listener);

		apvts.addParameterListener(s + "PhaserRate", listener);
		apvts.addParameterListener(s + "PhaserDepth", listener);
		apvts.addParameterListener(s + "PhaserCentre", listener);
		apvts.addParameterListener(s + "PhaserFeedback", listener);
		apvts.addParameterListener(s + "PhaserMix", listener);
		apvts.addParameterListener(s + "PhaserBypassed", listener);

		apvts.addParameterListener(s + "FlangerRate", listener);
		apvts.addParameterListener(s + "FlangerDepth", listener);
		apvts.addParameterListener(s + "FlangerCentre", listener);
		apvts.addParameterListener(s + "FlangerFeedback", listener);
		apvts.addParameterListener(s + "FlangerMix", listener);
		apvts.addParameterListener(s + "FlangerBypassed", listener);

		apvts.addParameterListener(s + "BitCrusherBitDepth", listener);
		apvts.addParameterListener(s + "BitCrusherRate", listener);
		apvts.addParameterListener(s + "BitCrusherMix", listener);
		apvts.addParameterListener(s + "BitCrusherBypassed", listener);

		apvts.addParameterListener(s + "ReverseActive", listener);
		apvts.addParameterListener(s + "TransientScatterActive", listener);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.addParameterListener(s + page, listener);

		for (int seq = 1; seq <= Obsidian::MAX_TRACKS; ++seq)
			apvts.addParameterListener(s + "Seq" + juce::String(seq), listener);
	}

	globalCrossfaderParam = apvts.getRawParameterValue("globalCrossfader");
	crossfaderCurveModeParam = apvts.getRawParameterValue("crossfaderCurveMode");

	apvts.addParameterListener("globalCrossfader", listener);
	apvts.addParameterListener("crossfaderCurveMode", listener);

	for (int i = 0; i < Obsidian::MAX_CROSSFADER_PAIR; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i + 1);
		pairCrossfaderParams[i] = apvts.getRawParameterValue(pairId);
		apvts.addParameterListener(pairId, listener);
	}

	nextTrackParam = apvts.getRawParameterValue("nextTrack");
	prevTrackParam = apvts.getRawParameterValue("prevTrack");
	apvts.addParameterListener("nextTrack", listener);
	apvts.addParameterListener("prevTrack", listener);
}

void ParameterManager::removeAllListeners(juce::AudioProcessorValueTreeState::Listener *listener)
{
	apvts.removeParameterListener("generate", listener);
	apvts.removeParameterListener("play", listener);
	apvts.removeParameterListener("nextTrack", listener);
	apvts.removeParameterListener("prevTrack", listener);
	apvts.removeParameterListener("delayDivision", listener);
	apvts.removeParameterListener("delayFeedback", listener);
	apvts.removeParameterListener("delayMode", listener);
	apvts.removeParameterListener("reverbSize", listener);
	apvts.removeParameterListener("reverbDamping", listener);
	apvts.removeParameterListener("reverbWidth", listener);
	apvts.removeParameterListener("reverbMix", listener);
	apvts.removeParameterListener("useCrossfader", listener);

	apvts.removeParameterListener("masterEQGainSubBass", listener);
	apvts.removeParameterListener("masterEQGainBass", listener);
	apvts.removeParameterListener("masterEQGainLowMid", listener);
	apvts.removeParameterListener("masterEQGainMid", listener);
	apvts.removeParameterListener("masterEQGainHiMid", listener);
	apvts.removeParameterListener("masterEQGainPresence", listener);
	apvts.removeParameterListener("masterEQGainHigh", listener);
	apvts.removeParameterListener("masterEQGainAir", listener);
	apvts.removeParameterListener("masterCompressorThreshold", listener);
	apvts.removeParameterListener("masterCompressorRatio", listener);
	apvts.removeParameterListener("masterCompressorAttack", listener);
	apvts.removeParameterListener("masterCompressorRelease", listener);
	apvts.removeParameterListener("masterCompressorMakeUpGain", listener);
	apvts.removeParameterListener("masterLimiterThreshold", listener);
	apvts.removeParameterListener("masterLimiterRelease", listener);
	apvts.removeParameterListener("masterLimiterMakeUpGain", listener);
	apvts.removeParameterListener("masterCompressorBypassed", listener);
	apvts.removeParameterListener("masterLimiterBypassed", listener);
	apvts.removeParameterListener("masterEQBypassed", listener);

	for (int slot = 1; slot <= Obsidian::MAX_TRACKS; ++slot)
	{
		juce::String s = "slot" + juce::String(slot);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.removeParameterListener(s + page, listener);

		apvts.removeParameterListener(s + "Generate", listener);
		apvts.removeParameterListener(s + "Pitch", listener);
		apvts.removeParameterListener(s + "Fine", listener);
		apvts.removeParameterListener(s + "AdsrAttack", listener);
		apvts.removeParameterListener(s + "AdsrDecay", listener);
		apvts.removeParameterListener(s + "AdsrSustain", listener);
		apvts.removeParameterListener(s + "AdsrRelease", listener);
		apvts.removeParameterListener(s + "DelaySend", listener);
		apvts.removeParameterListener(s + "ReverbSend", listener);
		apvts.removeParameterListener(s + "Seq", listener);
		apvts.removeParameterListener(s + "Play", listener);
		apvts.removeParameterListener(s + "Mute", listener);
		apvts.removeParameterListener(s + "Solo", listener);
		apvts.removeParameterListener(s + "Volume", listener);
		apvts.removeParameterListener(s + "Pan", listener);
		apvts.removeParameterListener(s + "Gain", listener);
		apvts.removeParameterListener(s + "BeatRepeatActive", listener);
		apvts.removeParameterListener(s + "BeatRepeatInterval", listener);

		apvts.removeParameterListener(s + "Cutoff", listener);
		apvts.removeParameterListener(s + "Resonance", listener);
		apvts.removeParameterListener(s + "FilterMode", listener);
		apvts.removeParameterListener(s + "FilterDrive", listener);
		apvts.removeParameterListener(s + "FilterBypassed", listener);

		apvts.removeParameterListener(s + "EQGainSubBass", listener);
		apvts.removeParameterListener(s + "EQGainBass", listener);
		apvts.removeParameterListener(s + "EQGainLowMid", listener);
		apvts.removeParameterListener(s + "EQGainMid", listener);
		apvts.removeParameterListener(s + "EQGainHiMid", listener);
		apvts.removeParameterListener(s + "EQGainPresence", listener);
		apvts.removeParameterListener(s + "EQGainHigh", listener);
		apvts.removeParameterListener(s + "EQGainAir", listener);
		apvts.removeParameterListener(s + "EQBypassed", listener);

		apvts.removeParameterListener(s + "CompressorThreshold", listener);
		apvts.removeParameterListener(s + "CompressorRatio", listener);
		apvts.removeParameterListener(s + "CompressorAttack", listener);
		apvts.removeParameterListener(s + "CompressorRelease", listener);
		apvts.removeParameterListener(s + "CompressorMakeUpGain", listener);
		apvts.removeParameterListener(s + "CompressorBypassed", listener);

		apvts.removeParameterListener(s + "LimiterThreshold", listener);
		apvts.removeParameterListener(s + "LimiterRelease", listener);
		apvts.removeParameterListener(s + "LimiterMakeUpGain", listener);
		apvts.removeParameterListener(s + "LimiterBypassed", listener);

		apvts.removeParameterListener(s + "DistortionPreGain", listener);
		apvts.removeParameterListener(s + "DistortionPostGain", listener);
		apvts.removeParameterListener(s + "DistortionCut", listener);
		apvts.removeParameterListener(s + "DistortionType", listener);
		apvts.removeParameterListener(s + "DistortionBypassed", listener);

		apvts.removeParameterListener(s + "ChorusRate", listener);
		apvts.removeParameterListener(s + "ChorusDepth", listener);
		apvts.removeParameterListener(s + "ChorusCentre", listener);
		apvts.removeParameterListener(s + "ChorusFeedback", listener);
		apvts.removeParameterListener(s + "ChorusMix", listener);
		apvts.removeParameterListener(s + "ChorusBypassed", listener);

		apvts.removeParameterListener(s + "PhaserRate", listener);
		apvts.removeParameterListener(s + "PhaserDepth", listener);
		apvts.removeParameterListener(s + "PhaserCentre", listener);
		apvts.removeParameterListener(s + "PhaserFeedback", listener);
		apvts.removeParameterListener(s + "PhaserMix", listener);
		apvts.removeParameterListener(s + "PhaserBypassed", listener);

		apvts.removeParameterListener(s + "FlangerRate", listener);
		apvts.removeParameterListener(s + "FlangerDepth", listener);
		apvts.removeParameterListener(s + "FlangerCentre", listener);
		apvts.removeParameterListener(s + "FlangerFeedback", listener);
		apvts.removeParameterListener(s + "FlangerMix", listener);
		apvts.removeParameterListener(s + "FlangerBypassed", listener);

		apvts.removeParameterListener(s + "BitCrusherBitDepth", listener);
		apvts.removeParameterListener(s + "BitCrusherRate", listener);
		apvts.removeParameterListener(s + "BitCrusherMix", listener);
		apvts.removeParameterListener(s + "BitCrusherBypassed", listener);

		apvts.removeParameterListener(s + "ReverseActive", listener);
		apvts.removeParameterListener(s + "TransientScatterActive", listener);
	}

	for (int i = 1; i <= 4; ++i)
		apvts.removeParameterListener("pairCrossfader" + juce::String(i), listener);

	apvts.removeParameterListener("globalCrossfader", listener);
	apvts.removeParameterListener("crossfaderCurveMode", listener);
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	auto makeTrigg = [](const juce::String &id, const juce::String &name, const float defaulValue = 0.0f)
	{
		auto attributes = juce::AudioParameterFloatAttributes().withAutomatable(false).withMeta(true);
		return std::make_unique<juce::AudioParameterFloat>(id, name, juce::NormalisableRange<float>(0.0f, 1.0f),
		                                                   defaulValue, attributes);
	};

	params.push_back(makeTrigg("generate", "Generate Loop"));
	params.push_back(makeTrigg("play", "Play Loop"));
	params.push_back(makeTrigg("useCrossfader", "Use Crossfader", 1.0f));
	params.push_back(makeTrigg("nextTrack", "Next Track"));
	params.push_back(makeTrigg("prevTrack", "Previous Track"));
	params.push_back(makeTrigg("masterCompressorBypassed", "Master Compressor Bypassed"));
	params.push_back(makeTrigg("masterLimiterBypassed", "Master Limiter Bypassed"));
	params.push_back(makeTrigg("masterEQBypassed", "Master EQ Bypassed"));

	juce::NormalisableRange<float> masterEQGainRange(0.0f, 4.0f);
	masterEQGainRange.skew = std::log(0.5f) / std::log((1.0f - 0.0f) / (4.0f - 0.0f));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainSubBass", "Master EQ Gain Sub Bass",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainBass", "Master EQ Gain Bass",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainLowMid", "Master EQ Gain Low Mid",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainMid", "Master EQ Gain Mid",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainHiMid", "Master EQ Gain Hi Mid",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainPresence", "Master EQ Gain Presence",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainHigh", "Master EQ Gain High",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterEQGainAir", "Master EQ Gain Air",
	                                                             masterEQGainRange, Obsidian::EQ_BANDS_GAIN));

	params.push_back(std::make_unique<juce::AudioParameterFloat>(
	    "masterCompressorThreshold", "Master Compressor Threshold", juce::NormalisableRange<float>(-60.f, 0.f, .1f),
	    Obsidian::COMPRESSOR_THRESHOLD));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterCompressorRatio", "Master Compressor Ratio",
	                                                             juce::NormalisableRange<float>(1.f, 20.f, .01f),
	                                                             Obsidian::COMPRESSOR_RATIO));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterCompressorAttack", "Master Compressor Attack",
	                                                             juce::NormalisableRange<float>(.1f, 100.f, .01f, .3f),
	                                                             Obsidian::COMPRESSOR_ATTACK));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterCompressorRelease", "Master Compressor Release",
	                                                             juce::NormalisableRange<float>(10.f, 1000.f, .1f, .3f),
	                                                             Obsidian::COMPRESSOR_RELEASE));

	juce::NormalisableRange<float> masterMakeUpGainRange(0.0f, 10.0f);
	masterMakeUpGainRange.skew = std::log(0.5f) / std::log(1.f / 10.f);
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterCompressorMakeUpGain",
	                                                             "Master Compressor MakeUp Gain", masterMakeUpGainRange,
	                                                             Obsidian::COMPRESSOR_MAKEUP_GAIN));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLimiterThreshold", "Master Limiter Threshold",
	                                                             juce::NormalisableRange<float>(-20.f, 0.f, .1f),
	                                                             Obsidian::LIMITER_THRESHOLD));

	juce::NormalisableRange<float> masterLimiterReleaseRange(1.f, 500.0f);
	masterLimiterReleaseRange.skew = 0.3f;
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLimiterRelease", "Master Limiter Release",
	                                                             masterLimiterReleaseRange, Obsidian::LIMITER_RELEASE));

	params.push_back(std::make_unique<juce::AudioParameterFloat>(
	    "masterLimiterMakeUpGain", "Master Limiter MakeUp Gain", masterMakeUpGainRange, Obsidian::LIMITER_MAKEUP_GAIN));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("globalCrossfader", "Global Crossfader (Deck A/B)",
	                                                             0.0f, 1.0f, 0.5f));

	params.push_back(std::make_unique<juce::AudioParameterChoice>(
	    "delayDivision", "Delay Time Division",
	    juce::StringArray{"1/16", "1/8.", "1/8", "1/4.", "1/4", "1/2", "1 bar", "2 bars"}, 4));

	params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.4f));
	params.push_back(std::make_unique<juce::AudioParameterChoice>("delayMode", "Delay Mode",
	                                                              juce::StringArray{"Stereo", "Ping-Pong", "Mono"}, 0));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Reverb Damping", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Reverb Width", 0.0f, 1.0f, 1.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.3f));

	for (int i = 1; i <= Obsidian::MAX_TRACKS / 2; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i);
		juce::String pairName = "Crossfader " + juce::String(i) + " <-> " + juce::String(i + 4);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(pairId, pairName, 0.0f, 1.0f, 0.5f));
	}

	params.push_back(std::make_unique<juce::AudioParameterChoice>("crossfaderCurveMode", "Crossfader Curve",
	                                                              juce::StringArray{"Linear", "Equal Power", "DJ"}, 1));

	for (int i = 1; i <= Obsidian::MAX_TRACKS; ++i)
	{
		juce::String slotId = "slot" + juce::String(i);
		juce::String slotName = "Slot " + juce::String(i);

		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Volume", slotName + " Volume", 0.0f, 1.0f, 0.8f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pan", slotName + " Pan", -1.0f, 1.0f, 0.0f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Gain", slotName + " Gain", -12.0f, 12.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Mute", slotName + " Mute", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Solo", slotName + " Solo", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pitch", slotName + " Pitch", -12.0f, 12.0f, 0.0f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Fine", slotName + " Fine", -50.0f, 50.0f, 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "BeatRepeatActive",
		                                                            slotName + " Beat Repeat Active", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "BeatRepeatInterval", slotName + " Retrigger Interval",
		                                                juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "TransientScatterActive",
		                                                            slotName + " Transient Scatter Active", false));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "DelaySend", slotName + " Delay Send",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "ReverbSend", slotName + " Reverb Send",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "Cutoff", slotName + " Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.3f),
		    Obsidian::FILTER_CUT));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "Resonance", slotName + " Resonance", juce::NormalisableRange<float>(0.f, 1.0f, 0.f, 0.4f),
		    Obsidian::FILTER_RES));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "FilterDrive", slotName + " Filter Drive",
		                                                             juce::NormalisableRange<float>(1.0f, 10.0f),
		                                                             Obsidian::FILTER_DRIVE));
		params.push_back(std::make_unique<juce::AudioParameterChoice>(
		    slotId + "FilterMode", slotName + " Filter Mode",
		    juce::StringArray{"LPF12", "HPF12", "BPF12", "LPF24", "HPF24", "BPF24"}, Obsidian::FILTER_MODE));

		juce::NormalisableRange<float> gainRange(0.0f, 4.0f);
		gainRange.skew = std::log(0.5f) / std::log((1.0f - 0.0f) / (4.0f - 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "EQGainSubBass", slotName + " EQ Gain Sub Bass", gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "EQGainBass", slotName + " EQ Gain Bass",
		                                                             gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "EQGainLowMid", slotName + " EQ Gain Low Mid", gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "EQGainMid", slotName + " EQ Gain Mid",
		                                                             gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "EQGainHiMid", slotName + " EQ Gain Hi Mid", gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "EQGainPresence", slotName + " EQ Gain Presence", gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "EQGainHigh", slotName + " EQ Gain High",
		                                                             gainRange, Obsidian::EQ_BANDS_GAIN));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "EQGainAir", slotName + " EQ Gain Air",
		                                                             gainRange, Obsidian::EQ_BANDS_GAIN));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "CompressorThreshold", slotName + " Compressor Threshold",
		    juce::NormalisableRange<float>(-60.f, 0.f, .1f), Obsidian::COMPRESSOR_THRESHOLD));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "CompressorRatio", slotName + " Compressor Ratio", juce::NormalisableRange<float>(1.f, 20.f, .01f),
		    Obsidian::COMPRESSOR_RATIO));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "CompressorAttack", slotName + " Compressor Attack",
		    juce::NormalisableRange<float>(.1f, 100.f, .01f, .3f), Obsidian::COMPRESSOR_ATTACK));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "CompressorRelease", slotName + " Compressor Release",
		    juce::NormalisableRange<float>(10.f, 1000.f, .1f, .3f), Obsidian::COMPRESSOR_RELEASE));

		juce::NormalisableRange<float> makeUpGainRange(0.0f, 10.0f);
		makeUpGainRange.skew = std::log(0.5f) / std::log(1.f / 10.f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "CompressorMakeUpGain", slotName + " Compressor MakeUp Gain", makeUpGainRange,
		    Obsidian::COMPRESSOR_MAKEUP_GAIN));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "LimiterThreshold", slotName + " Limiter Threshold",
		    juce::NormalisableRange<float>(-20.f, 0.f, .1f), Obsidian::LIMITER_THRESHOLD));

		juce::NormalisableRange<float> limiterReleaseRange(1.f, 500.0f);
		limiterReleaseRange.skew = 0.3f;
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "LimiterRelease", slotName + " Limiter Release", limiterReleaseRange, Obsidian::LIMITER_RELEASE));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "LimiterMakeUpGain",
		                                                             slotName + " Limiter MakeUp Gain", makeUpGainRange,
		                                                             Obsidian::LIMITER_MAKEUP_GAIN));

		params.push_back(std::make_unique<juce::AudioParameterChoice>(
		    slotId + "DistortionType", slotName + " Distortion Type",
		    juce::StringArray{"SOFT", "HARD", "TUBE", "FOLD", "DIODE", "CUBIC"}, 0));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "DistortionPreGain", slotName + " Distortion PreGain",
		                                                juce::NormalisableRange<float>(0.f, 24.f, 0.f), 0.f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "DistortionPostGain", slotName + " Distortion PostGain",
		    juce::NormalisableRange<float>(-24.f, 0.f, 0.f), 0.f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "DistortionCut", slotName + " Distortion Cut",
		    juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.3f), 1000.f));

		juce::NormalisableRange<float> chorusRateRange(0.0f, 10.0f);
		chorusRateRange.skew = std::log(0.5f) / std::log(1.f / 10.f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "ChorusRate", slotName + " Chorus Rate",
		                                                             chorusRateRange, Obsidian::CHORUS_RATE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "ChorusDepth", slotName + " Chorus Depth",
		                                                             juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		                                                             Obsidian::CHORUS_DEPTH));

		juce::NormalisableRange<float> chorusCentreRange(1.0f, 30.0f);
		chorusCentreRange.skew = std::log(0.5f) / std::log(1.f / 30.f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "ChorusCentre", slotName + " Chorus Centre", chorusCentreRange, Obsidian::CHORUS_CENTRE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "ChorusFeedback", slotName + " Chorus Feedback",
		    juce::NormalisableRange<float>(-0.95f, 0.95f, 0.f), Obsidian::CHORUS_FEEDBACK));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "ChorusMix", slotName + " Chorus Mix",
		                                                             juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		                                                             Obsidian::CHORUS_MIX));

		juce::NormalisableRange<float> phaserRateRange(0.0f, 10.0f);
		phaserRateRange.skew = std::log(0.5f) / std::log(1.f / 10.f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "PhaserRate", slotName + " Phaser Rate",
		                                                             phaserRateRange, Obsidian::PHASER_RATE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "PhaserDepth", slotName + " Phaser Depth",
		                                                             juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		                                                             Obsidian::PHASER_DEPTH));

		juce::NormalisableRange<float> phaserCentreRange(20.0f, 5000.0f, 0.f, 0.25f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "PhaserCentre", slotName + " Phaser Centre", phaserCentreRange, Obsidian::PHASER_CENTRE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "PhaserFeedback", slotName + " Phaser Feedback",
		    juce::NormalisableRange<float>(-0.95f, 0.95f, 0.f), Obsidian::PHASER_FEEDBACK));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "PhaserMix", slotName + " Phaser Mix",
		                                                             juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		                                                             Obsidian::PHASER_MIX));

		juce::NormalisableRange<float> flangerRateRange(0.0f, 10.0f);
		flangerRateRange.skew = std::log(0.5f) / std::log(1.f / 10.f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "FlangerRate", slotName + " Flanger Rate",
		                                                             flangerRateRange, Obsidian::FLANGER_RATE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "FlangerDepth", slotName + " Flanger Depth", juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		    Obsidian::FLANGER_DEPTH));
		juce::NormalisableRange<float> flangerCentreDelayRange(1.0f, 15.0f, 0.f, 0.5f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "FlangerCentre", slotName + " Flanger Centre", flangerCentreDelayRange, Obsidian::FLANGER_CENTRE));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "FlangerFeedback", slotName + " Flanger Feedback",
		    juce::NormalisableRange<float>(-0.95f, 0.95f, 0.f), Obsidian::FLANGER_FEEDBACK));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "FlangerMix", slotName + " Flanger Mix",
		                                                             juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		                                                             Obsidian::FLANGER_MIX));

		juce::NormalisableRange<float> bitcrusherDepthRange(1.0f, 16.0f, 0.f, 0.4f);
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "BitCrusherBitDepth", slotName + " Bitcrusher Depth",
		                                                bitcrusherDepthRange, Obsidian::BITCRUSHER_BIT_DEPTH));

		juce::NormalisableRange<float> bitcrusherRateRange(1.0f, 50.0f, 0.f, 0.3f);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "BitCrusherRate", slotName + " Bitcrusher Rate Reduction", bitcrusherRateRange,
		    Obsidian::BITCRUSHER_SAMPLE_RATE_REDUCTION));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "BitCrusherMix", slotName + " Bitcrusher Mix", juce::NormalisableRange<float>(0.f, 1.f, 0.f),
		    Obsidian::BITCRUSHER_MIX));

		params.push_back(makeTrigg(slotId + "DistortionBypassed", slotName + " Distortion Bypassed"));
		params.push_back(makeTrigg(slotId + "CompressorBypassed", slotName + " Compressor Bypassed"));
		params.push_back(makeTrigg(slotId + "LimiterBypassed", slotName + " Limiter Bypassed"));
		params.push_back(makeTrigg(slotId + "EQBypassed", slotName + " EQ Bypassed"));
		params.push_back(makeTrigg(slotId + "FilterBypassed", slotName + " Filter Bypassed"));
		params.push_back(makeTrigg(slotId + "ChorusBypassed", slotName + " Chorus Bypassed"));
		params.push_back(makeTrigg(slotId + "PhaserBypassed", slotName + " Phaser Bypassed"));
		params.push_back(makeTrigg(slotId + "FlangerBypassed", slotName + " Flanger Bypassed"));
		params.push_back(makeTrigg(slotId + "BitCrusherBypassed", slotName + " Bitcrusher Bypassed"));

		params.push_back(makeTrigg(slotId + "Play", slotName + " Play"));
		params.push_back(makeTrigg(slotId + "Stop", slotName + " Stop"));
		params.push_back(makeTrigg(slotId + "Generate", slotName + " Generate"));
		params.push_back(makeTrigg(slotId + "ReverseActive", slotName + " Reverse Active"));
		params.push_back(makeTrigg(slotId + "PageA", slotName + " Page A"));
		params.push_back(makeTrigg(slotId + "PageB", slotName + " Page B"));
		params.push_back(makeTrigg(slotId + "PageC", slotName + " Page C"));
		params.push_back(makeTrigg(slotId + "PageD", slotName + " Page D"));

		params.push_back(std::make_unique<juce::AudioParameterInt>(slotId + "Seq", slotName + " Sequence", 1,
		                                                           Obsidian::MAX_TRACKS, 1));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrAttack", slotName + " ADSR Attack",
		    juce::NormalisableRange<float>(Obsidian::ADSRDefaultValues::ATTACK_MIN,
		                                   Obsidian::ADSRDefaultValues::ATTACK_MAX),
		    Obsidian::ADSRDefaultValues::ATTACK_DEFAULT));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrDecay", slotName + " ADSR Decay",
		    juce::NormalisableRange<float>(Obsidian::ADSRDefaultValues::DECAY_MIN,
		                                   Obsidian::ADSRDefaultValues::DECAY_MAX),
		    Obsidian::ADSRDefaultValues::DECAY_DEFAULT));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrSustain", slotName + " ADSR Sustain",
		    juce::NormalisableRange<float>(Obsidian::ADSRDefaultValues::SUSTAIN_MIN,
		                                   Obsidian::ADSRDefaultValues::SUSTAIN_MAX),
		    Obsidian::ADSRDefaultValues::SUSTAIN_DEFAULT));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrRelease", slotName + " ADSR Release",
		    juce::NormalisableRange<float>(Obsidian::ADSRDefaultValues::RELEASE_MIN,
		                                   Obsidian::ADSRDefaultValues::RELEASE_MAX),
		    Obsidian::ADSRDefaultValues::RELEASE_DEFAULT));
	}

	return {params.begin(), params.end()};
}

void ParameterManager::applyPlayState(bool shouldArm, TrackData *track)
{
	if (!track)
		return;
	juce::ScopedValueSetter<bool> guard(isApplyingPlayState, true);

	auto &currentPage = track->getCurrentPage();
	if (currentPage.numSamples <= 0)
		return;

	const bool isPlaying = track->isCurrentlyPlaying.load();
	const bool emptySeq = track->allSequencerStepsAreFalse();
	int slot = track->slotIndex + 1;

	if (shouldArm && !isPlaying)
	{
		if (emptySeq)
		{
			track->setArmedToStop(false);
			track->pendingAction = TrackData::PendingAction::None;
		}
		track->setArmed(true);
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
		                                                 MidiMapping::feedbackPending);
	}
	else if (!shouldArm && !isPlaying)
	{
		track->pendingAction = TrackData::PendingAction::None;
		track->setArmed(false);
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot), MidiMapping::feedbackIdle);
	}
	else if (!shouldArm && isPlaying && !emptySeq)
	{
		if (track->isArmedToStop.load())
			return;
		track->pendingAction = TrackData::PendingAction::StopOnNextMeasure;
		track->setArmed(false);
		track->setArmedToStop(true);
	}
	else if (emptySeq)
	{
		track->pendingAction = TrackData::PendingAction::None;
		track->isArmed.store(false);
		track->isArmedToStop.store(false);
		track->isPlaying.store(false);
		track->isCurrentlyPlaying.store(false);
		return;
	}
}

void ParameterManager::parameterChanged(const juce::String &parameterID, float newValue)
{

	if (parameterID == "generate" && newValue > 0.5f)
		juce::MessageManager::callAsync([this]() { getAPVTS().getParameter("generate")->setValueNotifyingHost(0.0f); });
	else if (parameterID == "globalCrossfader" || parameterID.startsWith("pairCrossfader") ||
	         parameterID == "useCrossfader")
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
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
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    if (auto *mixer = editor->getMixerPanel())
					    if (auto *cf = mixer->getCrossfader())
						    cf->refreshCurveButtons();
			    }
		    });
	}
	else if (parameterID == "masterEQGainSubBass")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::subBass, newValue);
	else if (parameterID == "masterEQGainBass")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::bass, newValue);
	else if (parameterID == "masterEQGainLowMid")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::lowMid, newValue);
	else if (parameterID == "masterEQGainMid")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::mid, newValue);
	else if (parameterID == "masterEQGainHiMid")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::highMid, newValue);
	else if (parameterID == "masterEQGainPresence")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::presence, newValue);
	else if (parameterID == "masterEQGainHigh")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::high, newValue);
	else if (parameterID == "masterEQGainAir")
		audioProcessor.getEqualizer().updateGain(Obsidian::eqBands::air, newValue);
	else if (parameterID == "masterCompressorThreshold")
		audioProcessor.getCompressor().setThreshold(newValue);
	else if (parameterID == "masterCompressorRatio")
		audioProcessor.getCompressor().setRatio(newValue);
	else if (parameterID == "masterCompressorAttack")
		audioProcessor.getCompressor().setAttack(newValue);
	else if (parameterID == "masterCompressorRelease")
		audioProcessor.getCompressor().setRelease(newValue);
	else if (parameterID == "masterCompressorMakeUpGain")
		audioProcessor.getCompressor().setMakeUpGain(newValue);
	else if (parameterID == "masterLimiterThreshold")
		audioProcessor.getLimiter().setThreshold(newValue);
	else if (parameterID == "masterLimiterRelease")
		audioProcessor.getLimiter().setRelease(newValue);
	else if (parameterID == "masterLimiterMakeUpGain")
		audioProcessor.getLimiter().setMakeUpGain(newValue);
	else if (parameterID == "masterEQBypassed")
		audioProcessor.getEqualizer().setBypassed(newValue < 0.5f);
	else if (parameterID == "masterLimiterBypassed")
		audioProcessor.getLimiter().setBypassed(newValue < 0.5f);
	else if (parameterID == "masterCompressorBypassed")
		audioProcessor.getCompressor().setBypassed(newValue < 0.5f);
	else if (parameterID.startsWith("slot"))
	{
		TrackData *track = audioProcessor.getTrackFromParamId(parameterID);
		if (!track)
			return;

		auto range = audioProcessor.getParameterTreeState().getParameterRange(parameterID);
		int slot = track->slotIndex + 1;
		int slotIdx = track->slotIndex;

		if (parameterID.contains("Page") && newValue > 0.5f)
		{
			audioProcessor.getSequencerManager().handlePageChange(parameterID);
			juce::MessageManager::callAsync(
			    [this, parameterID]()
			    {
				    if (auto *param = getAPVTS().getParameter(parameterID))
					    param->setValueNotifyingHost(0.0f);
			    });
		}
		else if (parameterID.contains("Seq"))
		{
			if (auto *param = dynamic_cast<juce::AudioParameterInt *>(getAPVTS().getParameter(parameterID)))
			{
				int targetSequence = param->get();
				int slotNum = parameterID.substring(4, 5).getIntValue();
				audioProcessor.getSequencerManager().handleSequenceChange(slotNum, targetSequence);
			}
		}
		else if ((parameterID.endsWith("AdsrAttack") || parameterID.endsWith("AdsrDecay") ||
		          parameterID.endsWith("AdsrSustain") || parameterID.endsWith("AdsrRelease")))
		{
			auto &page = track->getCurrentPage();

			if (parameterID.endsWith("AdsrAttack"))
				page.adsrAttack.store(newValue);
			else if (parameterID.endsWith("AdsrDecay"))
				page.adsrDecay.store(newValue);
			else if (parameterID.endsWith("AdsrSustain"))
				page.adsrSustain.store(newValue);
			else if (parameterID.endsWith("AdsrRelease"))
				page.adsrRelease.store(newValue);
		}
		else if (parameterID.endsWith("Mute"))
		{
			track->isMuted.store(newValue > 0.5f);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackMute(slot),
			                                                 track->isMuted.load() ? MidiMapping::feedbackActive
			                                                                       : MidiMapping::feedbackIdle);
		}
		else if (parameterID.endsWith("Volume"))
		{
			track->volume.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackVolume(slot),
			                                                 MidiMapping::volumeToMidi(getVolume(slotIdx)));
		}
		else if (parameterID.endsWith("Pan"))
		{
			track->pan.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPan(slot),
			                                                 MidiMapping::panToMidi(getPan(slotIdx)));
		}
		else if (parameterID.endsWith("CompressorMakeUpGain"))
			track->compressor.setMakeUpGain(newValue);
		else if (parameterID.endsWith("DistortionPreGain"))
			track->distortion.setPre(newValue);
		else if (parameterID.endsWith("DistortionPostGain"))
			track->distortion.setPost(newValue);
		else if (parameterID.endsWith("LimiterMakeUpGain"))
			track->limiter.setMakeUpGain(newValue);
		else if (parameterID.endsWith("Gain"))
			track->getCurrentPage().gain.store(newValue);
		else if (parameterID.endsWith("Solo"))
		{
			track->isSolo.store(newValue > 0.5f);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackSolo(slot),
			                                                 track->isSolo.load() ? MidiMapping::feedbackActive
			                                                                      : MidiMapping::feedbackIdle);
		}
		else if (parameterID.endsWith("Play"))
		{
			applyPlayState(newValue > 0.5f, track);
			if (track->isCurrentlyPlaying.load())
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
				                                                 MidiMapping::feedbackPending);
			else if (track->isArmed.load())
				audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPlay(slot),
				                                                 MidiMapping::feedbackPending);
		}
		else if (parameterID.endsWith("DelaySend"))
			track->delaySend.store(newValue);
		else if (parameterID.endsWith("ReverbSend"))
			track->reverbSend.store(newValue);
		else if (parameterID.endsWith("FilterMode"))
		{
			auto mode = static_cast<juce::dsp::LadderFilterMode>((int)newValue);
			track->filter.setMode(mode);
		}
		else if (parameterID.endsWith("FilterDrive"))
			track->filter.setDrive(newValue);
		else if (parameterID.endsWith("Cutoff"))
			track->filter.setCutoffFrequency(newValue);
		else if (parameterID.endsWith("Resonance"))
			track->filter.setResonance(newValue);
		else if (parameterID.endsWith("EQGainSubBass"))
			track->equalizer.updateGain(Obsidian::eqBands::subBass, newValue);
		else if (parameterID.endsWith("EQGainBass"))
			track->equalizer.updateGain(Obsidian::eqBands::bass, newValue);
		else if (parameterID.endsWith("EQGainLowMid"))
			track->equalizer.updateGain(Obsidian::eqBands::lowMid, newValue);
		else if (parameterID.endsWith("EQGainMid"))
			track->equalizer.updateGain(Obsidian::eqBands::mid, newValue);
		else if (parameterID.endsWith("EQGainHiMid"))
			track->equalizer.updateGain(Obsidian::eqBands::highMid, newValue);
		else if (parameterID.endsWith("EQGainPresence"))
			track->equalizer.updateGain(Obsidian::eqBands::presence, newValue);
		else if (parameterID.endsWith("EQGainHigh"))
			track->equalizer.updateGain(Obsidian::eqBands::high, newValue);
		else if (parameterID.endsWith("EQGainAir"))
			track->equalizer.updateGain(Obsidian::eqBands::air, newValue);
		else if (parameterID.endsWith("CompressorThreshold"))
			track->compressor.setThreshold(newValue);
		else if (parameterID.endsWith("CompressorRatio"))
			track->compressor.setRatio(newValue);
		else if (parameterID.endsWith("CompressorAttack"))
			track->compressor.setAttack(newValue);
		else if (parameterID.endsWith("CompressorRelease"))
			track->compressor.setRelease(newValue);
		else if (parameterID.endsWith("LimiterThreshold"))
			track->limiter.setThreshold(newValue);
		else if (parameterID.endsWith("LimiterRelease"))
			track->limiter.setRelease(newValue);
		else if (parameterID.endsWith("DistortionCut"))
			track->distortion.setCut(newValue);
		else if (parameterID.endsWith("DistortionType"))
		{
			auto type = static_cast<Obsidian::distortionType>((int)newValue);
			track->distortion.setType(type);
		}
		else if (parameterID.endsWith("ChorusRate"))
			track->chorus.setRate(newValue);
		else if (parameterID.endsWith("ChorusDepth"))
			track->chorus.setDepth(newValue);
		else if (parameterID.endsWith("ChorusCentre"))
			track->chorus.setCentre(newValue);
		else if (parameterID.endsWith("ChorusFeedback"))
			track->chorus.setFeedback(newValue);
		else if (parameterID.endsWith("ChorusMix"))
			track->chorus.setMix(newValue);
		else if (parameterID.endsWith("PhaserRate"))
			track->phaser.setRate(newValue);
		else if (parameterID.endsWith("PhaserDepth"))
			track->phaser.setDepth(newValue);
		else if (parameterID.endsWith("PhaserCentre"))
			track->phaser.setCentre(newValue);
		else if (parameterID.endsWith("PhaserFeedback"))
			track->phaser.setFeedback(newValue);
		else if (parameterID.endsWith("PhaserMix"))
			track->phaser.setMix(newValue);
		else if (parameterID.endsWith("FlangerRate"))
			track->flanger.setRate(newValue);
		else if (parameterID.endsWith("FlangerDepth"))
			track->flanger.setDepth(newValue);
		else if (parameterID.endsWith("FlangerCentre"))
			track->flanger.setCentre(newValue);
		else if (parameterID.endsWith("FlangerFeedback"))
			track->flanger.setFeedback(newValue);
		else if (parameterID.endsWith("FlangerMix"))
			track->flanger.setMix(newValue);
		else if (parameterID.endsWith("BitCrusherBitDepth"))
			track->bitCrusher.setBitDepth(newValue);
		else if (parameterID.endsWith("BitCrusherRate"))
			track->bitCrusher.setSampleRateReduction(newValue);
		else if (parameterID.endsWith("BitCrusherMix"))
			track->bitCrusher.setMix(newValue);
		else if (parameterID.endsWith("DistortionBypassed"))
			track->distortion.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("EQBypassed"))
			track->equalizer.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("FilterBypassed"))
			track->filter.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("LimiterBypassed"))
			track->limiter.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("CompressorBypassed"))
			track->compressor.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("ChorusBypassed"))
			track->chorus.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("PhaserBypassed"))
			track->phaser.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("FlangerBypassed"))
			track->flanger.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("BitCrusherBypassed"))
			track->bitCrusher.setBypassed(newValue < 0.5f);
		else if (parameterID.endsWith("Pitch"))
		{
			track->getCurrentPage().pitchSemitones.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackPitch(slot),
			                                                 MidiMapping::pitchToMidi(getPitch(slotIdx)));
		}
		else if (parameterID.endsWith("Fine"))
		{
			track->getCurrentPage().fineOffset.store(newValue);
			audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackFine(slot),
			                                                 MidiMapping::fineToMidi(getFine(slotIdx)));
		}
		else if (parameterID.endsWith("BeatRepeatActive"))
		{
			bool isEnabled = newValue > 0.5f;
			track->randomRetriggerEnabled.store(isEnabled);
			if (isEnabled)
				track->beatRepeatPending.store(true);
			else
				track->beatRepeatStopPending.store(true);
		}
		else if (parameterID.endsWith("ReverseActive"))
		{
			bool isEnabled = newValue > 0.5f;
			if (isEnabled)
				track->reversePending.store(true);
			else
				track->reverseStopPending.store(true);
		}
		else if (parameterID.endsWith("TransientScatterActive"))
		{
			bool isEnabled = newValue > 0.5f;
			if (isEnabled)
				track->transientScatterPending.store(true);
			else
				track->transientScatterStopPending.store(true);
		}
		else if (parameterID.endsWith("BeatRepeatInterval"))
		{
			int value = (int)juce::roundToInt(newValue);
			double hostBpm = audioProcessor.getHostBpm();
			double repeatDuration = audioProcessor.getSequencerManager().calculateBeatRepeatInterval(value, hostBpm);
			audioProcessor.getTrackManager().updateBeatRepeat(track, value, hostBpm, repeatDuration);
		}
	}
}

void ParameterManager::handleSampleParams(int slot, TrackData *track)
{
	if (audioProcessor.isShuttingDown.load())
		return;
	float paramBeatRepeatActive = getBeatRepeatActive(slot);
	int slotNumber = slot + 1;
	bool isRetriggerEnabled = paramBeatRepeatActive > 0.5f;

	if (track->lastFeedbackBeatRepeat.load() != isRetriggerEnabled)
	{
		track->lastFeedbackBeatRepeat = isRetriggerEnabled;
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackBeatRepeat(slotNumber),
		                                                 isRetriggerEnabled ? MidiMapping::feedbackActive
		                                                                    : MidiMapping::feedbackIdle);
	}
}

void ParameterManager::handleSendsParams()
{
	const int ch = MidiMapping::feedbackChannelSends;

	auto pushFloatIfChanged = [&](std::atomic<float> &last, float cur, int cc)
	{
		if (std::abs(last.load() - cur) > 0.001f)
		{
			last.store(cur);
			audioProcessor.getMidiManager().sendMidiFeedback(cc, MidiMapping::normalizedToMidi(cur), ch);
		}
	};

	auto pushIntIfChanged = [&](std::atomic<int> &last, int cur, int cc, int total)
	{
		if (last.load() != cur)
		{
			last.store(cur);
			audioProcessor.getMidiManager().sendMidiFeedback(cc, MidiMapping::indexToMidi(cur, total), ch);
		}
	};

	pushFloatIfChanged(lastFeedbackDelayFeedback, getFeedback(), MidiMapping::ccFeedbackDelayFeedback);
	pushFloatIfChanged(lastFeedbackReverbSize, getReverbSize(), MidiMapping::ccFeedbackReverbSize);
	pushFloatIfChanged(lastFeedbackReverbDamping, getReverbDamping(), MidiMapping::ccFeedbackReverbDamping);
	pushFloatIfChanged(lastFeedbackReverbWidth, getReverbWidth(), MidiMapping::ccFeedbackReverbWidth);
	pushFloatIfChanged(lastFeedbackReverbMix, getReverbMix(), MidiMapping::ccFeedbackReverbMix);
	pushIntIfChanged(lastFeedbackDelayDivision, getDelayDivisionIndex(), MidiMapping::ccFeedbackDelayDivision, 8);
	pushIntIfChanged(lastFeedbackDelayMode, getDelayModeIndex(), MidiMapping::ccFeedbackDelayMode, 3);
}
