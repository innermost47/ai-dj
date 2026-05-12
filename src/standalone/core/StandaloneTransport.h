#pragma once
#include <JuceHeader.h>

class StandaloneTransport : public juce::AudioPlayHead
{
  public:
	StandaloneTransport();
	~StandaloneTransport() override = default;

	juce::Optional<PositionInfo> getPosition() const override;

	void play();
	void stop();
	void togglePlayStop();
	bool isPlaying() const
	{
		return playing.load();
	}

	void setBpm(double newBpm);
	void setPlaying(bool v)
	{
		playing.store(v);
	}
	void setPpqPosition(double pos)
	{
		currentPpq.store(pos);
	}
	double getBpm() const
	{
		return bpm.load();
	}
	void setTimeSignature(int numerator, int denominator);
	int getTimeSigNumerator() const
	{
		return timeSigNum.load();
	}
	int getTimeSigDenominator() const
	{
		return timeSigDenom.load();
	}

	void advance(int numSamples, double sampleRate);

	void rewind();

	double getCurrentPpq() const
	{
		return currentPpq.load();
	}
	double getCurrentTimeInSeconds() const
	{
		return currentTimeSeconds.load();
	}
	void pause();

  private:
	std::atomic<bool> playing{false};
	std::atomic<double> bpm{120.0};
	std::atomic<int> timeSigNum{4};
	std::atomic<int> timeSigDenom{4};
	std::atomic<double> currentPpq{0.0};
	std::atomic<double> currentTimeSeconds{0.0};

	juce::SpinLock positionLock;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneTransport)
};