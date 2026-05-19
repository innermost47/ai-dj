#pragma once
#include "DataConst.h"
#include <JuceHeader.h>

class ParameterManager
{
  public:
	explicit ParameterManager(juce::AudioProcessor &processor);
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

	void removeAllListeners(juce::AudioProcessorValueTreeState::Listener *listener);

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

  private:
	juce::AudioProcessorValueTreeState apvts;

	std::atomic<float> *masterVolumeParam = nullptr;
	std::atomic<float> *masterPanParam = nullptr;
	std::atomic<float> *masterHighParam = nullptr;
	std::atomic<float> *masterMidParam = nullptr;
	std::atomic<float> *masterLowParam = nullptr;
	std::atomic<float> *generateParam = nullptr;
	std::atomic<float> *playParam = nullptr;
	std::atomic<float> *delayDivisionParam = nullptr;
	std::atomic<float> *delayFeedbackParam = nullptr;
	std::atomic<float> *delayModeParam = nullptr;
	std::atomic<float> *reverbSizeParam = nullptr;
	std::atomic<float> *reverbDampingParam = nullptr;
	std::atomic<float> *reverbWidthParam = nullptr;
	std::atomic<float> *reverbMixParam = nullptr;

	std::atomic<float> *slotVolumeParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotPanParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotMuteParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotSoloParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotPlayParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotStopParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotGenerateParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotPitchParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotFineParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotRandomRetriggerParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotRetriggerIntervalParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrAttackParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrDecayParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrSustainParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotAdsrReleaseParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotDelaySendParams[ObsidianDataConst::MAX_TRACKS] = {};
	std::atomic<float> *slotReverbSendParams[ObsidianDataConst::MAX_TRACKS] = {};

	std::atomic<float> *globalCrossfaderParam = nullptr;
	std::atomic<float> *pairCrossfaderParams[ObsidianDataConst::MAX_CROSSFADER_PAIR] = {};
	std::atomic<float> *crossfaderCurveModeParam = nullptr;

	std::atomic<float> *nextTrackParam = nullptr;
	std::atomic<float> *prevTrackParam = nullptr;

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

		static const juce::StringArray perSlotParams = {
		    "Volume",     "Pan",       "Pitch",       "Fine",        "DelaySend",        "ReverbSend",
		    "ADSRAttack", "ADSRDecay", "ADSRSustain", "ADSRRelease", "RetriggerInterval"};

		for (int slot = 1; slot <= 8; ++slot)
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
		juce::StringArray ids = {"generate", "play", "nextTrack", "prevTrack"};

		static const juce::StringArray perSlotParams = {
		    "Mute", "Solo", "Play", "Stop", "Generate", "RandomRetrigger", "PageA", "PageB", "PageC", "PageD"};

		for (int slot = 1; slot <= 8; ++slot)
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