#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

class MasterWaveformDisplay : public juce::Component, public juce::Timer
{
  public:
	MasterWaveformDisplay();
	~MasterWaveformDisplay() override;

	void pushSamples(const float *left, const float *right, int numSamples);
	void setPositionInBeats(double ppqPosition);
	void timerCallback() override;
	void paint(juce::Graphics &g) override;

  private:
	void rebuildPaths(juce::Rectangle<float> inner, int w, float cy, float hH, float ppx);

	std::vector<float> writeBuffer;
	std::vector<float> readBuffer;
	std::atomic<size_t> writePos{0};
	std::atomic<double> positionInBeats{0.0};
	std::atomic<bool> hasNewData{false};

	float animPhase{0.0f};
	float lastPeak{0.0f};
	int idleFrames{0};

	juce::Path cachedTop, cachedBot, cachedEchoTop, cachedEchoBot;
	bool pathsDirty{true};
	float cachedMaxPeakVal{0.0f};
	float cachedMaxPeakX{0.0f};
	float cachedPeakAbs{0.0f};
	int lastW{0}, lastH{0};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterWaveformDisplay)
};