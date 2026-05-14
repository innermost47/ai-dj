#pragma once
#include "ObsidianBase.h"
#include "SampleBank.h"
#include <JuceHeader.h>

class DetailPanel : public ObsidianComponent, public juce::Timer
{
  public:
	DetailPanel();
	~DetailPanel() override;

	void setEntry(SampleBankEntry *entry);
	void paint(juce::Graphics &g) override;
	void resized() override;
	void setIsPlaying(bool playing);
	void updatePlaybackPosition(float pos);

	std::function<void(SampleBankEntry *)> onPlayRequested;
	std::function<void()> onStopRequested;
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

	void loadAudio();

  private:
	SampleBankEntry *entry = nullptr;

	juce::Label nameLabel;
	juce::Label metaLabel;
	IconButtonSimple playButton{"Play", ""};

	juce::Rectangle<int> waveformBounds;
	std::vector<float> thumbL, thumbR;
	juce::AudioBuffer<float> audioBuf;
	std::shared_ptr<std::atomic<bool>> validity{std::make_shared<std::atomic<bool>>(true)};
	std::atomic<bool> destroyed{false};

	bool isPlaying = false;
	float playbackPos = 0.0f;
	double lastTimerCall = 0.0;

	void timerCallback() override;
	void generateThumbnail();
	void drawWaveform(juce::Graphics &g);
	void updatePlayButton();

	juce::String formatDuration(float s);
	juce::Colour getCategoryColor(const juce::String &category);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetailPanel)
};