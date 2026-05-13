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
	std::atomic<float> *slotBpmOffsetParams[ObsidianDataConst::MAX_TRACKS] = {};
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

	const juce::StringArray floatParamIds = {"bpm",
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
	                                         "slot1Volume",
	                                         "slot1Pan",
	                                         "slot1Pitch",
	                                         "slot1Fine",
	                                         "slot1BpmOffset",
	                                         "slot2Volume",
	                                         "slot2Pan",
	                                         "slot2Pitch",
	                                         "slot2Fine",
	                                         "slot2BpmOffset",
	                                         "slot3Volume",
	                                         "slot3Pan",
	                                         "slot3Pitch",
	                                         "slot3Fine",
	                                         "slot3BpmOffset",
	                                         "slot4Volume",
	                                         "slot4Pan",
	                                         "slot4Pitch",
	                                         "slot4Fine",
	                                         "slot4BpmOffset",
	                                         "slot5Volume",
	                                         "slot5Pan",
	                                         "slot5Pitch",
	                                         "slot5Fine",
	                                         "slot5BpmOffset",
	                                         "slot6Volume",
	                                         "slot6Pan",
	                                         "slot6Pitch",
	                                         "slot6Fine",
	                                         "slot6BpmOffset",
	                                         "slot7Volume",
	                                         "slot7Pan",
	                                         "slot7Pitch",
	                                         "slot7Fine",
	                                         "slot7BpmOffset",
	                                         "slot8Volume",
	                                         "slot8Pan",
	                                         "slot8Pitch",
	                                         "slot8Fine",
	                                         "slot8BpmOffset",
	                                         "slot1DelaySend",
	                                         "slot2DelaySend",
	                                         "slot3DelaySend",
	                                         "slot4DelaySend",
	                                         "slot5DelaySend",
	                                         "slot6DelaySend",
	                                         "slot7DelaySend",
	                                         "slot8DelaySend",
	                                         "slot1ReverbSend",
	                                         "slot2ReverbSend",
	                                         "slot3ReverbSend",
	                                         "slot4ReverbSend",
	                                         "slot5ReverbSend",
	                                         "slot6ReverbSend",
	                                         "slot7ReverbSend",
	                                         "slot8ReverbSend",
	                                         "globalCrossfader",
	                                         "pairCrossfader1",
	                                         "pairCrossfader2",
	                                         "pairCrossfader3",
	                                         "pairCrossfader4",
	                                         "delayDivision",
	                                         "delayMode",
	                                         "crossfaderCurveMode"};

	const juce::StringArray booleanParamIds = {"generate",      "play",
	                                           "slot1Mute",     "slot1Solo",
	                                           "slot1Play",     "slot1Stop",
	                                           "slot1Generate", "slot1RandomRetrigger",
	                                           "slot2Mute",     "slot2Solo",
	                                           "slot2Play",     "slot2Stop",
	                                           "slot2Generate", "slot2RandomRetrigger",
	                                           "slot3Mute",     "slot3Solo",
	                                           "slot3Play",     "slot3Stop",
	                                           "slot3Generate", "slot3RandomRetrigger",
	                                           "slot4Mute",     "slot4Solo",
	                                           "slot4Play",     "slot4Stop",
	                                           "slot4Generate", "slot4RandomRetrigger",
	                                           "slot5Mute",     "slot5Solo",
	                                           "slot5Play",     "slot5Stop",
	                                           "slot5Generate", "slot5RandomRetrigger",
	                                           "slot6Mute",     "slot6Solo",
	                                           "slot6Play",     "slot6Stop",
	                                           "slot6Generate", "slot6RandomRetrigger",
	                                           "slot7Mute",     "slot7Solo",
	                                           "slot7Play",     "slot7Stop",
	                                           "slot7Generate", "slot7RandomRetrigger",
	                                           "slot8Mute",     "slot8Solo",
	                                           "slot8Play",     "slot8Stop",
	                                           "slot8Generate", "slot8RandomRetrigger",
	                                           "nextTrack",     "prevTrack",
	                                           "slot1PageA",    "slot1PageB",
	                                           "slot1PageC",    "slot1PageD",
	                                           "slot2PageA",    "slot2PageB",
	                                           "slot2PageC",    "slot2PageD",
	                                           "slot3PageA",    "slot3PageB",
	                                           "slot3PageC",    "slot3PageD",
	                                           "slot4PageA",    "slot4PageB",
	                                           "slot4PageC",    "slot4PageD",
	                                           "slot5PageA",    "slot5PageB",
	                                           "slot5PageC",    "slot5PageD",
	                                           "slot6PageA",    "slot6PageB",
	                                           "slot6PageC",    "slot6PageD",
	                                           "slot7PageA",    "slot7PageB",
	                                           "slot7PageC",    "slot7PageD",
	                                           "slot8PageA",    "slot8PageB",
	                                           "slot8PageC",    "slot8PageD"};

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