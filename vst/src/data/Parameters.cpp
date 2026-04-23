#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	params.push_back(std::make_unique<juce::AudioParameterBool>("generate", "Generate Loop", false));
	params.push_back(std::make_unique<juce::AudioParameterBool>("play", "Play Loop", false));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterPan", "Master Pan", -1.0f, 1.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterHigh", "Master High EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterMid", "Master Mid EQ", -12.0f, 12.0f, 0.0f));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLow", "Master Low EQ", -12.0f, 12.0f, 0.0f));

	params.push_back(std::make_unique<juce::AudioParameterBool>("nextTrack", "Next Track", false));
	params.push_back(std::make_unique<juce::AudioParameterBool>("prevTrack", "Previous Track", false));

	for (int i = 1; i <= 8; ++i)
	{
		juce::String slotId = "slot" + juce::String(i);
		juce::String slotName = "Slot " + juce::String(i);

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Volume", slotName + " Volume", 0.0f, 1.0f, 0.8f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Pan", slotName + " Pan", -1.0f, 1.0f, 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Mute", slotName + " Mute", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Solo", slotName + " Solo", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Play", slotName + " Play", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Stop", slotName + " Stop", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "Generate", slotName + " Generate", false));

		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Pitch", slotName + " Pitch", -12.0f, 12.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "Fine", slotName + " Fine", -50.0f, 50.0f, 0.0f));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "BpmOffset", slotName + " BPM Offset", -20.0f, 20.0f, 0.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "RandomRetrigger", slotName + " Random Retrigger", false));
		params.push_back(std::make_unique<juce::AudioParameterFloat>(slotId + "RetriggerInterval", slotName + " Retrigger Interval", juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 3.0f));

		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "PageA", slotName + " Page A", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "PageB", slotName + " Page B", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "PageC", slotName + " Page C", false));
		params.push_back(std::make_unique<juce::AudioParameterBool>(slotId + "PageD", slotName + " Page D", false));

		for (int j = 1; j <= 8; ++j)
		{
			params.push_back(std::make_unique<juce::AudioParameterBool>(
				slotId + "Seq" + juce::String(j),
				slotName + " Sequence " + juce::String(j),
				false
			));
		}
	}

	return { params.begin(), params.end() };
}