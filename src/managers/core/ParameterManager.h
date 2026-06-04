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

	float getDistorsionPreGain(int slot) const
	{
		return safeLoadIndexed(slotDistorsionPreGainParams, slot);
	}
	float getDistorsionPostGain(int slot) const
	{
		return safeLoadIndexed(slotDistorsionPostGainParams, slot);
	}
	float getDistorsionCut(int slot) const
	{
		return safeLoadIndexed(slotDistorsionCutParams, slot);
	}
	float getDistorsionType(int slot) const
	{
		return safeLoadIndexed(slotDistorsionTypeParams, slot);
	}

	bool getDistorsionBypassed(int slot) const
	{
		return safeLoad(slotDistorsionBypassedParams[slot]) > 0.5f;
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

	float getRandomRetrigger(int slot) const
	{
		return safeLoad(slotRandomRetriggerParams[slot]);
	}
	float getRetriggerInterval(int slot) const
	{
		return safeLoad(slotRetriggerIntervalParams[slot]);
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

	std::atomic<float> *slotVolumeParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPanParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotMuteParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotSoloParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPlayParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotStopParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotGenerateParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotPitchParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCutoffParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotResonanceParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFilterModeParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFilterDriveParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotFineParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotRandomRetriggerParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotRetriggerIntervalParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrAttackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrDecayParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrSustainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDelaySendParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotReverbSendParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotEQGainSubBassParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainBassParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainLowMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainHighMidParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainPresenceParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainHighParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotEQGainAirParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotCompressorThresholdParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorRatioParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorAttackParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotCompressorMakeUpGainParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotLimiterReleaseParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotLimiterThresholdParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotLimiterMakeUpGainParams[Obsidian::MAX_TRACKS] = {};

	std::atomic<float> *slotDistorsionPreGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistorsionPostGainParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistorsionCutParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistorsionBypassedParams[Obsidian::MAX_TRACKS] = {};
	std::atomic<float> *slotDistorsionTypeParams[Obsidian::MAX_TRACKS] = {};

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
		juce::StringArray ids = {"bpm",
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
		                         "crossfaderCurveMode"};

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
		                                                "RetriggerInterval",
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
		                                                "DistorsionPreGain",
		                                                "DistorsionPostGain",
		                                                "DistorsionCut",
		                                                "DistorsionType"};

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
		juce::StringArray ids = {"generate", "play", "nextTrack", "prevTrack", "useCrossfader"};

		static const juce::StringArray perSlotParams = {
		    "Mute",  "Solo",  "Play",  "Stop",       "Generate",    "RandomRetrigger",   "PageA",
		    "PageB", "PageC", "PageD", "FilterMode", "FilterDrive", "DistorsionBypassed"};

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