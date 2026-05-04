#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	auto makeTrigg = [](const juce::String &id, const juce::String &name)
	{
		auto attributes = juce::AudioParameterFloatAttributes()
							  .withAutomatable(false)
							  .withMeta(true);
		return std::make_unique<juce::AudioParameterFloat>(
			id, name,
			juce::NormalisableRange<float>(0.0f, 1.0f),
			0.0f,
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
	params.push_back(std::make_unique<juce::AudioParameterFloat>(
		"globalCrossfader", "Global Crossfader (Deck A/B)",
		0.0f, 1.0f, 0.5f));

	for (int i = 1; i <= 4; ++i)
	{
		juce::String pairId = "pairCrossfader" + juce::String(i);
		juce::String pairName = "Crossfader " + juce::String(i) + " <-> " + juce::String(i + 4);
		params.push_back(std::make_unique<juce::AudioParameterFloat>(
			pairId, pairName,
			0.0f, 1.0f, 0.5f));
	}

	params.push_back(std::make_unique<juce::AudioParameterChoice>(
		"crossfaderCurveMode", "Crossfader Curve",
		juce::StringArray{"Linear", "Equal Power", "DJ"}, 1));

	for (int i = 1; i <= 8; ++i)
	{
		juce::String slotId = "slot" + juce::String(i);
		juce::String slotName = "Slot " + juce::String(i);

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Volume", slotName + " Volume", 0.0f, 1.0f, 0.8f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Pan", slotName + " Pan", -1.0f, 1.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Mute", slotName + " Mute", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Solo", slotName + " Solo", false));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Pitch", slotName + " Pitch", -12.0f, 12.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Fine", slotName + " Fine", -50.0f, 50.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "BpmOffset", slotName + " BPM Offset", -20.0f, 20.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "RandomRetrigger", slotName + " Random Retrigger", false));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "RetriggerInterval", slotName + " Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f));

		params.push_back(makeTrigg(slotId + "Play", slotName + " Play"));
		params.push_back(makeTrigg(slotId + "Stop", slotName + " Stop"));
		params.push_back(makeTrigg(slotId + "Generate", slotName + " Generate"));
		params.push_back(makeTrigg(slotId + "PageA", slotName + " Page A"));
		params.push_back(makeTrigg(slotId + "PageB", slotName + " Page B"));
		params.push_back(makeTrigg(slotId + "PageC", slotName + " Page C"));
		params.push_back(makeTrigg(slotId + "PageD", slotName + " Page D"));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
			slotId + "AdsrAttack", slotName + " ADSR Attack",
			juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
			slotId + "AdsrDecay", slotName + " ADSR Decay",
			juce::NormalisableRange<float>(0.001f, 4.0f), 4.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
			slotId + "AdsrSustain", slotName + " ADSR Sustain",
			juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(
			slotId + "AdsrRelease", slotName + " ADSR Release",
			juce::NormalisableRange<float>(0.001f, 4.0f), 0.0f));

		for (int j = 1; j <= 8; ++j)
			params.push_back(makeTrigg(slotId + "Seq" + juce::String(j), slotName + " Sequence " + juce::String(j)));
	}

	return {params.begin(), params.end()};
}