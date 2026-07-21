#pragma once
#include "DataConst.h"
#include "TrackData.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class ParameterManager
{
  public:
	explicit ParameterManager(DjIaVstProcessor &processor);
	~ParameterManager() = default;

	juce::AudioProcessorValueTreeState &getAPVTS()
	{
		return apvts;
	}

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	void resolveParameters(juce::AudioProcessorValueTreeState::Listener *listener);

	float getVolume(int slot) const
	{
		return safeLoadIndexed(slotVolumeParams, slot);
	}
	float getPan(int slot) const
	{
		return safeLoadIndexed(slotPanParams, slot);
	}
	float getGain(int slot) const
	{
		return safeLoadIndexed(slotGainParams, slot);
	}
	float getPitch(int slot) const
	{
		return safeLoadIndexed(slotPitchParams, slot);
	}
	float getFine(int slot) const
	{
		return safeLoadIndexed(slotFineParams, slot);
	}
	float getAttack(int slot) const
	{
		return safeLoadIndexed(slotAdsrAttackParams, slot);
	}
	float getDecay(int slot) const
	{
		return safeLoadIndexed(slotAdsrDecayParams, slot);
	}
	float getSustain(int slot) const
	{
		return safeLoadIndexed(slotAdsrSustainParams, slot);
	}
	float getRelease(int slot) const
	{
		return safeLoadIndexed(slotAdsrReleaseParams, slot);
	}
	float getGenerate(int slot) const
	{
		return safeLoadIndexed(slotGenerateParams, slot);
	}
	float getCutoff(int slot) const
	{
		return safeLoadIndexed(slotCutoffParams, slot);
	}
	float getResonance(int slot) const
	{
		return safeLoadIndexed(slotResonanceParams, slot);
	}
	float getFilterMode(int slot) const
	{
		return safeLoadIndexed(slotFilterModeParams, slot);
	}
	float getFilterDrive(int slot) const
	{
		return safeLoadIndexed(slotFilterDriveParams, slot);
	}

	float getCompressorThreshold(int slot) const
	{
		return safeLoadIndexed(slotCompressorThresholdParams, slot);
	}
	float getCompressorRatio(int slot) const
	{
		return safeLoadIndexed(slotCompressorRatioParams, slot);
	}
	float getCompressorAttack(int slot) const
	{
		return safeLoadIndexed(slotCompressorAttackParams, slot);
	}
	float getCompressorRelease(int slot) const
	{
		return safeLoadIndexed(slotCompressorReleaseParams, slot);
	}
	float getCompressorMakeUpGain(int slot) const
	{
		return safeLoadIndexed(slotCompressorMakeUpGainParams, slot);
	}

	float getLimiterRelease(int slot) const
	{
		return safeLoadIndexed(slotLimiterReleaseParams, slot);
	}
	float getLimiterThreshold(int slot) const
	{
		return safeLoadIndexed(slotLimiterThresholdParams, slot);
	}
	float getLimiterMakeUpGain(int slot) const
	{
		return safeLoadIndexed(slotLimiterMakeUpGainParams, slot);
	}

	float getDistortionPreGain(int slot) const
	{
		return safeLoadIndexed(slotDistortionPreGainParams, slot);
	}
	float getDistortionPostGain(int slot) const
	{
		return safeLoadIndexed(slotDistortionPostGainParams, slot);
	}
	float getDistortionCut(int slot) const
	{
		return safeLoadIndexed(slotDistortionCutParams, slot);
	}
	float getDistortionType(int slot) const
	{
		return safeLoadIndexed(slotDistortionTypeParams, slot);
	}

	float getChorusRate(int slot) const
	{
		return safeLoadIndexed(slotChorusRateParams, slot);
	}
	float getChorusDepth(int slot) const
	{
		return safeLoadIndexed(slotChorusDepthParams, slot);
	}
	float getChorusCentre(int slot) const
	{
		return safeLoadIndexed(slotChorusCentreParams, slot);
	}
	float getChorusFeedback(int slot) const
	{
		return safeLoadIndexed(slotChorusFeedbackParams, slot);
	}
	float getChorusMix(int slot) const
	{
		return safeLoadIndexed(slotChorusMixParams, slot);
	}

	float getPhaserRate(int slot) const
	{
		return safeLoadIndexed(slotPhaserRateParams, slot);
	}
	float getPhaserDepth(int slot) const
	{
		return safeLoadIndexed(slotPhaserDepthParams, slot);
	}
	float getPhaserCentre(int slot) const
	{
		return safeLoadIndexed(slotPhaserCentreParams, slot);
	}
	float getPhaserFeedback(int slot) const
	{
		return safeLoadIndexed(slotPhaserFeedbackParams, slot);
	}
	float getPhaserMix(int slot) const
	{
		return safeLoadIndexed(slotPhaserMixParams, slot);
	}

	float getFlangerRate(int slot) const
	{
		return safeLoadIndexed(slotFlangerRateParams, slot);
	}
	float getFlangerDepth(int slot) const
	{
		return safeLoadIndexed(slotFlangerDepthParams, slot);
	}
	float getFlangerCentre(int slot) const
	{
		return safeLoadIndexed(slotFlangerCentreParams, slot);
	}
	float getFlangerFeedback(int slot) const
	{
		return safeLoadIndexed(slotFlangerFeedbackParams, slot);
	}
	float getFlangerMix(int slot) const
	{
		return safeLoadIndexed(slotFlangerMixParams, slot);
	}

	float getBitCrusherBitDepth(int slot) const
	{
		return safeLoadIndexed(slotBitCrusherBitDepthParams, slot);
	}
	float getBitCrusherSampleRateReduction(int slot) const
	{
		return safeLoadIndexed(slotBitCrusherSampleRateReductionParams, slot);
	}
	float getBitCrusherMix(int slot) const
	{
		return safeLoadIndexed(slotBitCrusherMixParams, slot);
	}

	bool getDistortionBypassed(int slot) const
	{
		return safeLoad(slotDistortionBypassedParams[slot]) > 0.5f;
	}
	bool getCompressorBypassed(int slot) const
	{
		return safeLoad(slotCompressorBypassedParams[slot]) > 0.5f;
	}
	bool getEQBypassed(int slot) const
	{
		return safeLoad(slotEQBypassedParams[slot]) > 0.5f;
	}
	bool getFilterBypassed(int slot) const
	{
		return safeLoad(slotFilterBypassedParams[slot]) > 0.5f;
	}
	bool getLimiterBypassed(int slot) const
	{
		return safeLoad(slotLimiterBypassedParams[slot]) > 0.5f;
	}
	bool getChorusBypassed(int slot) const
	{
		return safeLoad(slotChorusBypassedParams[slot]) > 0.5f;
	}
	bool getPhaserBypassed(int slot) const
	{
		return safeLoad(slotPhaserBypassedParams[slot]) > 0.5f;
	}
	bool getFlangerBypassed(int slot) const
	{
		return safeLoad(slotFlangerBypassedParams[slot]) > 0.5f;
	}
	bool getBitCrusherBypassed(int slot) const
	{
		return safeLoad(slotBitCrusherBypassedParams[slot]) > 0.5f;
	}
	bool getReverseActive(int slot) const
	{
		return safeLoad(slotReverseActiveParams[slot]) > 0.5f;
	}
	bool getTransientScatterActive(int slot) const
	{
		return safeLoad(slotTransientScatterActiveParams[slot]) > 0.5f;
	}

	bool getMute(int slot) const
	{
		return safeLoad(slotMuteParams[slot]) > 0.5f;
	}
	bool getSolo(int slot) const
	{
		return safeLoad(slotSoloParams[slot]) > 0.5f;
	}
	bool getPlay(int slot) const
	{
		return safeLoad(slotPlayParams[slot]) > 0.5f;
	}

	float getMasterVolume() const
	{
		return safeLoad(masterVolumeParam);
	}
	float getMasterPan() const
	{
		return safeLoad(masterPanParam);
	}
	float getMasterHigh() const
	{
		return safeLoad(masterHighParam);
	}
	float getMasterMid() const
	{
		return safeLoad(masterMidParam);
	}
	float getMasterLow() const
	{
		return safeLoad(masterLowParam);
	}
	bool getGenerate() const
	{
		return safeLoad(generateParam) > 0.5f;
	}
	bool getPlay() const
	{
		return safeLoad(playParam) > 0.5f;
	}
	bool useCrossfader() const
	{
		return safeLoad(useCrossfaderParam) > 0.5f;
	}

	float getGlobalCrossfader() const
	{
		return safeLoad(globalCrossfaderParam);
	}
	float getPairCrossfader(int pair) const
	{
		return safeLoad(pairCrossfaderParams[pair]);
	}

	const juce::StringArray &getFloatParamIds() const
	{
		return floatParamIds;
	}
	const juce::StringArray &getBooleanParamIds() const
	{
		return booleanParamIds;
	}

	float getReverbSize() const
	{
		return safeLoad(reverbSizeParam);
	}
	float getReverbDamping() const
	{
		return safeLoad(reverbDampingParam);
	}
	float getReverbWidth() const
	{
		return safeLoad(reverbWidthParam);
	}
	float getReverbMix() const
	{
		return safeLoad(reverbMixParam);
	}

	float getBeatRepeatActive(int slot) const
	{
		return safeLoad(slotBeatRepeatActiveParams[slot]);
	}
	float getBeatRepeatInterval(int slot) const
	{
		return safeLoad(slotBeatRepeatIntervalParams[slot]);
	}

	int getCrossfaderCurveMode() const
	{
		return juce::jlimit(0, 2, (int)safeLoad(crossfaderCurveModeParam));
	}

	int getDelayDivisionIndex() const
	{
		return delayDivisionParam ? static_cast<int>(delayDivisionParam->load()) : 0;
	}

	int getDelayModeIndex() const
	{
		return delayModeParam ? static_cast<int>(delayModeParam->load()) : 0;
	}

	float getFeedback() const noexcept
	{
		return delayFeedbackParam ? delayFeedbackParam->load() : 0.0f;
	}

	float getDelaySend(int slot) const
	{
		return safeLoadIndexed(slotDelaySendParams, slot);
	}

	float getReverbSend(int slot) const
	{
		return safeLoadIndexed(slotReverbSendParams, slot);
	}

	void parameterChanged(const juce::String &parameterID, float newValue);
	void handleSampleParams(int slot, TrackData *track);
	void applyPlayState(bool shouldArm, TrackData *track);
	void handleSendsParams();
	void removeAllListeners(juce::AudioProcessorValueTreeState::Listener *listener);

  private:
	juce::AudioProcessorValueTreeState apvts;
	DjIaVstProcessor &audioProcessor;

	bool isApplyingPlayState = false;

	std::atomic<float> *masterVolumeParam = nullptr;
	std::atomic<float> *masterPanParam = nullptr;
	std::atomic<float> *masterHighParam = nullptr;
	std::atomic<float> *masterMidParam = nullptr;
	std::atomic<float> *masterLowParam = nullptr;
	std::atomic<float> *generateParam = nullptr;
	std::atomic<float> *playParam = nullptr;
	std::atomic<float> *useCrossfaderParam = nullptr;
	std::atomic<float> *delayDivisionParam = nullptr;
	std::atomic<float> *delayFeedbackParam = nullptr;
	std::atomic<float> *delayModeParam = nullptr;
	std::atomic<float> *reverbSizeParam = nullptr;
	std::atomic<float> *reverbDampingParam = nullptr;
	std::atomic<float> *reverbWidthParam = nullptr;
	std::atomic<float> *reverbMixParam = nullptr;

	std::atomic<float> *masterEQGainSubBassParams = nullptr;
	std::atomic<float> *masterEQGainBassParams = nullptr;
	std::atomic<float> *masterEQGainLowMidParams = nullptr;
	std::atomic<float> *masterEQGainMidParams = nullptr;
	std::atomic<float> *masterEQGainHighMidParams = nullptr;
	std::atomic<float> *masterEQGainPresenceParams = nullptr;
	std::atomic<float> *masterEQGainHighParams = nullptr;
	std::atomic<float> *masterEQGainAirParams = nullptr;
	std::atomic<float> *masterEQBypassedParams = nullptr;

	std::atomic<float> *masterCompressorThresholdParams = nullptr;
	std::atomic<float> *masterCompressorRatioParams = nullptr;
	std::atomic<float> *masterCompressorAttackParams = nullptr;
	std::atomic<float> *masterCompressorReleaseParams = nullptr;
	std::atomic<float> *masterCompressorMakeUpGainParams = nullptr;
	std::atomic<float> *masterCompressorBypassedParams = nullptr;

	std::atomic<float> *masterLimiterReleaseParams = nullptr;
	std::atomic<float> *masterLimiterThresholdParams = nullptr;
	std::atomic<float> *masterLimiterMakeUpGainParams = nullptr;
	std::atomic<float> *masterLimiterBypassedParams = nullptr;

	std::atomic<float> *slotVolumeParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPanParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotMuteParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotSoloParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPlayParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotStopParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotGenerateParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPitchParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFineParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotBeatRepeatActiveParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotBeatRepeatIntervalParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrAttackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrDecayParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrSustainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDelaySendParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotReverbSendParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotReverseActiveParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotTransientScatterActiveParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotCutoffParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotResonanceParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFilterModeParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFilterDriveParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFilterBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotEQGainSubBassParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainBassParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainLowMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainHighMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainPresenceParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainHighParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainAirParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotCompressorThresholdParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorRatioParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorAttackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorMakeUpGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotLimiterReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotLimiterThresholdParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotLimiterMakeUpGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotLimiterBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotDistortionPreGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistortionPostGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistortionCutParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistortionBypassedParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistortionTypeParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotChorusRateParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotChorusDepthParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotChorusCentreParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotChorusFeedbackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotChorusMixParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotChorusBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotPhaserRateParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPhaserDepthParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPhaserCentreParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPhaserFeedbackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPhaserMixParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPhaserBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotFlangerRateParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFlangerDepthParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFlangerCentreParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFlangerFeedbackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFlangerMixParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFlangerBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotBitCrusherBitDepthParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotBitCrusherSampleRateReductionParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotBitCrusherMixParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotBitCrusherBypassedParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *globalCrossfaderParam = nullptr;
	std::atomic<float> *pairCrossfaderParams[Obsidian::MAX_CROSSFADER_PAIR] = {};
	std::atomic<float> *crossfaderCurveModeParam = nullptr;

	std::atomic<float> *nextTrackParam = nullptr;
	std::atomic<float> *prevTrackParam = nullptr;

	std::atomic<float> lastFeedbackDelayFeedback{0.0f};
	std::atomic<float> lastFeedbackReverbSize{0.0f};
	std::atomic<float> lastFeedbackReverbDamping{0.0f};
	std::atomic<float> lastFeedbackReverbWidth{0.0f};
	std::atomic<float> lastFeedbackReverbMix{0.0f};
	std::atomic<int> lastFeedbackDelayDivision{-1};
	std::atomic<int> lastFeedbackDelayMode{-1};

	static juce::StringArray buildFloatParamIds()
	{
		juce::StringArray ids = {
		    "bpm",
		    "delayFeedback",
		    "reverbSize",
		    "reverbDamping",
		    "reverbWidth",
		    "reverbMix",
		    "masterVolume",
		    "masterPan",
		    "masterHigh",
		    "masterMid",
		    "masterLow",
		    "globalCrossfader",
		    "pairCrossfader1",
		    "pairCrossfader2",
		    "pairCrossfader3",
		    "pairCrossfader4",
		    "delayDivision",
		    "delayMode",
		    "crossfaderCurveMode",
		    "masterEQGainSubBass",
		    "masterEQGainBass",
		    "masterEQGainLowMid",
		    "masterEQGainMid",
		    "masterEQGainHiMid",
		    "masterEQGainPresence",
		    "masterEQGainHigh",
		    "masterEQGainAir",
		    "masterCompressorThreshold",
		    "masterCompressorRatio",
		    "masterCompressorAttack",
		    "masterCompressorRelease",
		    "masterCompressorMakeUpGain",
		    "masterLimiterThreshold",
		    "masterLimiterRelease",
		    "masterLimiterMakeUpGain",
		};

		static const juce::StringArray perSlotParams = {"Volume",
		                                                "Pan",
		                                                "Pitch",
		                                                "Fine",
		                                                "DelaySend",
		                                                "ReverbSend",
		                                                "ADSRAttack",
		                                                "ADSRDecay",
		                                                "ADSRSustain",
		                                                "ADSRRelease",
		                                                "BeatRepeatInterval",
		                                                "Gain",
		                                                "Cutoff",
		                                                "Resonance",
		                                                "EQGainSubBass",
		                                                "EQGainBass",
		                                                "EQGainLowMid",
		                                                "EQGainMid",
		                                                "EQGainHiMid",
		                                                "EQGainPresence",
		                                                "EQGainHigh",
		                                                "EQGainAir",
		                                                "CompressorThreshold",
		                                                "CompressorRatio",
		                                                "CompressorAttack",
		                                                "CompressorRelease",
		                                                "CompressorMakeUpGain",
		                                                "LimiterThreshold",
		                                                "LimiterRelease",
		                                                "LimiterMakeUpGain",
		                                                "DistortionPreGain",
		                                                "DistortionPostGain",
		                                                "DistortionCut",
		                                                "DistortionType",
		                                                "ChorusRate",
		                                                "ChorusDepth",
		                                                "ChorusCentre",
		                                                "ChorusFeedback",
		                                                "ChorusMix",
		                                                "PhaserRate",
		                                                "PhaserDepth",
		                                                "PhaserCentre",
		                                                "PhaserFeedback",
		                                                "PhaserMix",
		                                                "FlangerRate",
		                                                "FlangerDepth",
		                                                "FlangerCentre",
		                                                "FlangerFeedback",
		                                                "FlangerMix",
		                                                "BitCrusherBitDepth",
		                                                "BitCrusherRate",
		                                                "BitCrusherMix"};

		for (int slot = 1; slot <= Obsidian::MAX_TRACKS; ++slot)
		{
			const juce::String prefix = "slot" + juce::String(slot);
			for (const auto &param : perSlotParams)
				ids.add(prefix + param);
		}

		return ids;
	}

	const juce::StringArray floatParamIds = buildFloatParamIds();

	static juce::StringArray buildBooleanParamIds()
	{
		juce::StringArray ids = {
		    "generate",
		    "play",
		    "nextTrack",
		    "prevTrack",
		    "useCrossfader",
		    "masterCompressorBypassed",
		    "masterLimiterBypassed",
		    "masterEQBypassed",
		};

		static const juce::StringArray perSlotParams = {"Mute",
		                                                "Solo",
		                                                "Play",
		                                                "Stop",
		                                                "Generate",
		                                                "BeatRepeatActive",
		                                                "PageA",
		                                                "PageB",
		                                                "PageC",
		                                                "PageD",
		                                                "FilterMode",
		                                                "FilterDrive",
		                                                "DistortionBypassed",
		                                                "FilterBypassed",
		                                                "CompressorBypassed",
		                                                "LimiterBypassed",
		                                                "EQBypassed",
		                                                "ChorusBypassed",
		                                                "PhaserBypassed",
		                                                "FlangerBypassed",
		                                                "BitCrusherBypassed",
		                                                "ReverseActive",
		                                                "TransientScatterActive"};

		for (int slot = 1; slot <= Obsidian::MAX_TRACKS; ++slot)
		{
			const juce::String prefix = "slot" + juce::String(slot);
			for (const auto &param : perSlotParams)
				ids.add(prefix + param);
		}

		return ids;
	}

	const juce::StringArray booleanParamIds = buildBooleanParamIds();

	static float safeLoad(const std::atomic<float> *p)
	{
		return p ? p->load() : 0.0f;
	}

  private:
	template <size_t N> static float safeLoadIndexed(std::atomic<float> *const (&arr)[N], int index)
	{
		if (index < 0 || index >= (int)N)
			return 0.0f;
		return safeLoad(arr[index]);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterManager)
};