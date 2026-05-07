#include "StandaloneTransport.h"

StandaloneTransport::StandaloneTransport() = default;

juce::Optional<juce::AudioPlayHead::PositionInfo> StandaloneTransport::getPosition() const
{
	juce::AudioPlayHead::PositionInfo info;
	info.setBpm(bpm.load());
	info.setIsPlaying(playing.load());
	info.setIsRecording(false);
	info.setIsLooping(false);
	info.setPpqPosition(currentPpq.load());
	info.setTimeInSeconds(currentTimeSeconds.load());

	juce::AudioPlayHead::TimeSignature timeSig;
	timeSig.numerator = timeSigNum.load();
	timeSig.denominator = timeSigDenom.load();
	info.setTimeSignature(timeSig);

	return info;
}

void StandaloneTransport::play()
{
	playing.store(true);
}

void StandaloneTransport::stop()
{
	playing.store(false);
}

void StandaloneTransport::togglePlayStop()
{
	playing.store(!playing.load());
}

void StandaloneTransport::setBpm(double newBpm)
{
	bpm.store(juce::jlimit(20.0, 300.0, newBpm));
}

void StandaloneTransport::setTimeSignature(int numerator, int denominator)
{
	timeSigNum.store(juce::jmax(1, numerator));
	timeSigDenom.store(juce::jmax(1, denominator));
}

void StandaloneTransport::advance(int numSamples, double sampleRate)
{
	if (!playing.load() || sampleRate <= 0.0)
		return;

	double secondsPerSample = 1.0 / sampleRate;
	double beatsPerSecond = bpm.load() / 60.0;
	double ppqPerSample = beatsPerSecond * secondsPerSample;

	currentPpq.store(currentPpq.load() + ppqPerSample * numSamples);
	currentTimeSeconds.store(currentTimeSeconds.load() + secondsPerSample * numSamples);
}

void StandaloneTransport::rewind()
{
	currentPpq.store(0.0);
	currentTimeSeconds.store(0.0);
}

void StandaloneTransport::pause()
{
	playing.store(false);
}