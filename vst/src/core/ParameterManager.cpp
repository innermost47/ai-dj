#include "ParameterManager.h"

ParameterManager::ParameterManager(juce::AudioProcessor &processor)
    : apvts(processor, nullptr, "Parameters", createParameterLayout())
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

	apvts.addParameterListener("generate", listener);
	apvts.addParameterListener("play", listener);

	for (int i = 0; i < MAX_SLOTS; ++i)
	{
		juce::String s = "slot" + juce::String(i + 1);

		slotVolumeParams[i] = apvts.getRawParameterValue(s + "Volume");
		slotPanParams[i] = apvts.getRawParameterValue(s + "Pan");
		slotMuteParams[i] = apvts.getRawParameterValue(s + "Mute");
		slotSoloParams[i] = apvts.getRawParameterValue(s + "Solo");
		slotPlayParams[i] = apvts.getRawParameterValue(s + "Play");
		slotStopParams[i] = apvts.getRawParameterValue(s + "Stop");
		slotGenerateParams[i] = apvts.getRawParameterValue(s + "Generate");
		slotPitchParams[i] = apvts.getRawParameterValue(s + "Pitch");
		slotFineParams[i] = apvts.getRawParameterValue(s + "Fine");
		slotBpmOffsetParams[i] = apvts.getRawParameterValue(s + "BpmOffset");
		slotRandomRetriggerParams[i] = apvts.getRawParameterValue(s + "RandomRetrigger");
		slotRetriggerIntervalParams[i] = apvts.getRawParameterValue(s + "RetriggerInterval");
		slotAdsrAttackParams[i] = apvts.getRawParameterValue(s + "AdsrAttack");
		slotAdsrDecayParams[i] = apvts.getRawParameterValue(s + "AdsrDecay");
		slotAdsrSustainParams[i] = apvts.getRawParameterValue(s + "AdsrSustain");
		slotAdsrReleaseParams[i] = apvts.getRawParameterValue(s + "AdsrRelease");

		apvts.addParameterListener(s + "Generate", listener);
		apvts.addParameterListener(s + "AdsrAttack", listener);
		apvts.addParameterListener(s + "AdsrDecay", listener);
		apvts.addParameterListener(s + "AdsrSustain", listener);
		apvts.addParameterListener(s + "AdsrRelease", listener);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.addParameterListener(s + page, listener);

		for (int seq = 1; seq <= 8; ++seq)
			apvts.addParameterListener(s + "Seq" + juce::String(seq), listener);
	}

	globalCrossfaderParam = apvts.getRawParameterValue("globalCrossfader");
	crossfaderCurveModeParam = apvts.getRawParameterValue("crossfaderCurveMode");

	apvts.addParameterListener("globalCrossfader", listener);
	apvts.addParameterListener("crossfaderCurveMode", listener);

	for (int i = 0; i < MAX_PAIRS; ++i)
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

	for (int slot = 1; slot <= 8; ++slot)
	{
		juce::String s = "slot" + juce::String(slot);

		for (const char *page : {"PageA", "PageB", "PageC", "PageD"})
			apvts.removeParameterListener(s + page, listener);

		apvts.removeParameterListener(s + "Generate", listener);
		apvts.removeParameterListener(s + "AdsrAttack", listener);
		apvts.removeParameterListener(s + "AdsrDecay", listener);
		apvts.removeParameterListener(s + "AdsrSustain", listener);
		apvts.removeParameterListener(s + "AdsrRelease", listener);

		for (int seq = 1; seq <= 8; ++seq)
			apvts.removeParameterListener(s + "Seq" + juce::String(seq), listener);
	}

	for (int i = 1; i <= 4; ++i)
		apvts.removeParameterListener("pairCrossfader" + juce::String(i), listener);

	apvts.removeParameterListener("globalCrossfader", listener);
	apvts.removeParameterListener("crossfaderCurveMode", listener);
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	auto makeTrigg = [](const juce::String &id, const juce::String &name)
	{
		auto attributes = juce::AudioParameterFloatAttributes().withAutomatable(false).withMeta(true);
		return std::make_unique<juce::AudioParameterFloat>(id, name, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f,
		                                                   attributes);
	};

	params.push_back(makeTrigg("generate", "Generate Loop"));
	params.push_back(makeTrigg("play", "Play Loop"));
	params.push_back(makeTrigg("nextTrack", "Next Track"));
	params.push_back(makeTrigg("prevTrack", "Previous Track"));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("globalCrossfader", "Global Crossfader (Deck A/B)",
	                                                             0.0f, 1.0f, 0.5f));

	for (int i = 1; i <= 4; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i);
		juce::String pairName = "Crossfader " + juce::String(i) + " <-> " + juce::String(i + 4);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(pairId, pairName, 0.0f, 1.0f, 0.5f));
	}

	params.push_back(std::make_unique<juce::AudioParameterChoice>("crossfaderCurveMode", "Crossfader Curve",
	                                                              juce::StringArray{"Linear", "Equal Power", "DJ"}, 1));

	for (int i = 1; i <= 8; ++i)
	{
		juce::String slotId = "slot" + juce::String(i);
		juce::String slotName = "Slot " + juce::String(i);

		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Volume", slotName + " Volume", 0.0f, 1.0f, 0.8f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pan", slotName + " Pan", -1.0f, 1.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Mute", slotName + " Mute", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Solo", slotName + " Solo", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Pitch", slotName + " Pitch", -12.0f, 12.0f, 0.0f));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "Fine", slotName + " Fine", -50.0f, 50.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "BpmOffset", slotName + " BPM Offset",
		                                                             -20.0f, 20.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "RandomRetrigger",
		                                                            slotName + " Random Retrigger", false));
		params.push_back(
		    std::make_unique<juce::AudioParameterFloat>(slotId + "RetriggerInterval", slotName + " Retrigger Interval",
		                                                juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f));

		params.push_back(makeTrigg(slotId + "Play", slotName + " Play"));
		params.push_back(makeTrigg(slotId + "Stop", slotName + " Stop"));
		params.push_back(makeTrigg(slotId + "Generate", slotName + " Generate"));
		params.push_back(makeTrigg(slotId + "PageA", slotName + " Page A"));
		params.push_back(makeTrigg(slotId + "PageB", slotName + " Page B"));
		params.push_back(makeTrigg(slotId + "PageC", slotName + " Page C"));
		params.push_back(makeTrigg(slotId + "PageD", slotName + " Page D"));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrAttack", slotName + " ADSR Attack", juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrDecay", slotName + " ADSR Decay", juce::NormalisableRange<float>(0.001f, 4.0f), 4.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "AdsrSustain", slotName + " ADSR Sustain",
		                                                             juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
		    slotId + "AdsrRelease", slotName + " ADSR Release", juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));

		for (int j = 1; j <= 8; ++j)
			params.push_back(makeTrigg(slotId + "Seq" + juce::String(j), slotName + " Sequence " + juce::String(j)));
	}

	return {params.begin(), params.end()};
}