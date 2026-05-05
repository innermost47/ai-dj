#pragma once
#include "JuceHeader.h"
#include "SoundTouch.h"
#include "BPMDetect.h"

class AudioAnalyzer
{
public:
	static float detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate);

	static void chunkAnalysis(std::vector<float>& monoData, soundtouch::BPMDetect& bpmDetect);

	static float normalizeAudio(const juce::AudioSampleBuffer& buffer,
		std::vector<float>& monoData,
		bool& retFlag,
		int maxSamples = -1);

	static void timeStretchBufferHQ(juce::AudioBuffer<float>& buffer,
		double ratio,
		double sampleRate);

	static void timeStretchBuffer(juce::AudioBuffer<float>& buffer,
		double ratio,
		double sampleRate,
		bool highQuality = false);
};