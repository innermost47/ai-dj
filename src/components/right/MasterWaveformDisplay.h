#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class MasterWaveformDisplay : public ObsidianComponent
{
  public:
	MasterWaveformDisplay();
	~MasterWaveformDisplay() override;

	void pushSamples(const float *left, const float *right, int numSamples);
	void setPositionInBeats(double ppqPosition);
	void paint(juce::Graphics &g) override;

  private:
	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	void rebuildPaths(juce::Rectangle<float> inner, int w, float cy, float hH, float ppx);
	void handleVBlank();

	std::vector<float> writeBuffer;
	std::vector<float> readBuffer;
	std::atomic<size_t> writePos{0};
	std::atomic<double> positionInBeats{0.0};
	std::atomic<bool> hasNewData{false};

	float animPhase{0.0f};
	float lastPeak{0.0f};
	int idleFrames{0};

	juce::Path cachedTop, cachedBot;
	bool pathsDirty{true};
	int lastW{0}, lastH{0};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterWaveformDisplay)
};