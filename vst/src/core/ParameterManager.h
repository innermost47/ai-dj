#pragma once
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
		return safeLoad(slotVolumeParams[slot]);
	}
	float getPan(int slot) const
	{
		return safeLoad(slotPanParams[slot]);
	}
	float getPitch(int slot) const
	{
		return safeLoad(slotPitchParams[slot]);
	}
	float getFine(int slot) const
	{
		return safeLoad(slotFineParams[slot]);
	}
	float getAttack(int slot) const
	{
		return safeLoad(slotAdsrAttackParams[slot]);
	}
	float getDecay(int slot) const
	{
		return safeLoad(slotAdsrDecayParams[slot]);
	}
	float getSustain(int slot) const
	{
		return safeLoad(slotAdsrSustainParams[slot]);
	}
	float getRelease(int slot) const
	{
		return safeLoad(slotAdsrReleaseParams[slot]);
	}
	float getBpmOffset(int slot) const
	{
		return safeLoad(slotBpmOffsetParams[slot]);
	}
	float getGenerate(int slot) const
	{
		return safeLoad(slotGenerateParams[slot]);
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

	float getNextTrack() const
	{
		return safeLoad(nextTrackParam);
	}
	float getPrevTrack() const
	{
		return safeLoad(prevTrackParam);
	}

	std::atomic<float> *getRawSlotParam(const juce::String &paramId);
	std::atomic<float> *getRawGlobalCrossfaderParam()
	{
		return globalCrossfaderParam;
	}
	std::atomic<float> *getRawPairCrossfaderParam(int pair)
	{
		return pairCrossfaderParams[pair];
	}

	const juce::StringArray &getFloatParamIds() const
	{
		return floatParamIds;
	}
	const juce::StringArray &getBooleanParamIds() const
	{
		return booleanParamIds;
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

	static constexpr int MAX_SLOTS = 8;
	static constexpr int MAX_PAIRS = 4;

  private:
	juce::AudioProcessorValueTreeState apvts;

	std::atomic<float> *masterVolumeParam = nullptr;
	std::atomic<float> *masterPanParam = nullptr;
	std::atomic<float> *masterHighParam = nullptr;
	std::atomic<float> *masterMidParam = nullptr;
	std::atomic<float> *masterLowParam = nullptr;
	std::atomic<float> *generateParam = nullptr;
	std::atomic<float> *playParam = nullptr;

	std::atomic<float> *slotVolumeParams[MAX_SLOTS] = {};
	std::atomic<float> *slotPanParams[MAX_SLOTS] = {};
	std::atomic<float> *slotMuteParams[MAX_SLOTS] = {};
	std::atomic<float> *slotSoloParams[MAX_SLOTS] = {};
	std::atomic<float> *slotPlayParams[MAX_SLOTS] = {};
	std::atomic<float> *slotStopParams[MAX_SLOTS] = {};
	std::atomic<float> *slotGenerateParams[MAX_SLOTS] = {};
	std::atomic<float> *slotPitchParams[MAX_SLOTS] = {};
	std::atomic<float> *slotFineParams[MAX_SLOTS] = {};
	std::atomic<float> *slotBpmOffsetParams[MAX_SLOTS] = {};
	std::atomic<float> *slotRandomRetriggerParams[MAX_SLOTS] = {};
	std::atomic<float> *slotRetriggerIntervalParams[MAX_SLOTS] = {};
	std::atomic<float> *slotAdsrAttackParams[MAX_SLOTS] = {};
	std::atomic<float> *slotAdsrDecayParams[MAX_SLOTS] = {};
	std::atomic<float> *slotAdsrSustainParams[MAX_SLOTS] = {};
	std::atomic<float> *slotAdsrReleaseParams[MAX_SLOTS] = {};

	std::atomic<float> *globalCrossfaderParam = nullptr;
	std::atomic<float> *pairCrossfaderParams[MAX_PAIRS] = {};
	std::atomic<float> *crossfaderCurveModeParam = nullptr;

	std::atomic<float> *nextTrackParam = nullptr;
	std::atomic<float> *prevTrackParam = nullptr;

	const juce::StringArray floatParamIds = {"bpm",
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
	                                         "globalCrossfader",
	                                         "pairCrossfader1",
	                                         "pairCrossfader2",
	                                         "pairCrossfader3",
	                                         "pairCrossfader4",
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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterManager)
};