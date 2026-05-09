#pragma once
#include <JuceHeader.h>
class DjIaVstProcessor;
struct MidiMapping
{
	int midiType;
	int midiNumber;
	int midiChannel;
	juce::String parameterName;
	juce::String description;
	DjIaVstProcessor *processor;
	std::function<void(float)> uiCallback;

	static const int feedbackChannelMixer = 4;
	static const int feedbackChannelShaping = 5;
	static const int feedbackChannelSends = 6;

	static const int feedbackIdle = 0;
	static const int feedbackPending = 64;
	static const int feedbackActive = 127;

	static const int ccRequestState = 118;

	static int ccFeedbackPlay(int slot)
	{
		return 20 + slot;
	}
	static int ccFeedbackGenerate(int slot)
	{
		return 30 + slot;
	}
	static int ccFeedbackPage(int slot)
	{
		return 40 + slot;
	}
	static int ccFeedbackVolume(int slot)
	{
		return 50 + slot;
	}
	static int ccFeedbackPan(int slot)
	{
		return 60 + slot;
	}
	static int ccFeedbackPitch(int slot)
	{
		return 70 + slot;
	}
	static int ccFeedbackFine(int slot)
	{
		return 80 + slot;
	}
	static int ccFeedbackSeq(int slot)
	{
		return 90 + slot;
	}
	static int ccFeedbackMute(int slot)
	{
		return 100 + slot;
	}
	static int ccFeedbackSolo(int slot)
	{
		return 110 + slot;
	}
	static int ccFeedbackBeatRepeat(int slot)
	{
		return 118 + slot;
	}

	static int ccFeedbackAdsrAttack(int slot)
	{
		return 20 + slot;
	}
	static int ccFeedbackAdsrDecay(int slot)
	{
		return 30 + slot;
	}
	static int ccFeedbackAdsrSustain(int slot)
	{
		return 40 + slot;
	}
	static int ccFeedbackAdsrRelease(int slot)
	{
		return 50 + slot;
	}

	static int ccFeedbackPairCrossfader(int pair)
	{
		return 59 + pair;
	}
	static const int ccFeedbackGlobalCrossfader = 64;
	static const int ccFeedbackCrossfaderCurve = 65;
	static const int ccFeedbackMasterHigh = 66;
	static const int ccFeedbackMasterMid = 67;
	static const int ccFeedbackMasterLow = 68;

	static const int ccFeedbackDelayFeedback = 20;
	static const int ccFeedbackReverbSize = 21;
	static const int ccFeedbackReverbDamping = 22;
	static const int ccFeedbackReverbWidth = 23;
	static const int ccFeedbackReverbMix = 24;
	static const int ccFeedbackDelayDivision = 30;
	static const int ccFeedbackDelayMode = 31;

	static int adsrToMidi(float value, float rangeMin, float rangeMax)
	{
		float normalized = (value - rangeMin) / (rangeMax - rangeMin);
		return juce::jlimit(0, 127, juce::roundToInt(normalized * 127.0f));
	}
	static int volumeToMidi(float v)
	{
		return juce::roundToInt(v * 127.0f);
	}
	static int panToMidi(float p)
	{
		return juce::roundToInt((p + 1.0f) / 2.0f * 127.0f);
	}
	static int pitchToMidi(float p)
	{
		return juce::jlimit(0, 127, juce::roundToInt((p + 96.0f) / 192.0f * 127.0f));
	}
	static int fineToMidi(float f)
	{
		return juce::jlimit(0, 127, juce::roundToInt((f + 100.0f) / 200.0f * 127.0f));
	}

	static int normalizedToMidi(float v)
	{
		return juce::jlimit(0, 127, juce::roundToInt(v * 127.0f));
	}

	static int indexToMidi(int idx, int total)
	{
		if (total <= 1)
			return 0;
		return juce::jlimit(0, 127, (idx * 127) / (total - 1));
	}
};