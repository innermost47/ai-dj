#include "BeatDetektor.h"
#include "BpmDetector.h"

class BeatDetektorBpmDetector : public BpmDetector
{
  public:
	double detectTempo(double hostBpm, const float *channelData, int numSamples, double sampleRate) override
	{
		const int fftOrder = 10;
		const int fftSize = 1 << fftOrder;
		const int hopSize = fftSize / 2;

		juce::dsp::FFT fft(fftOrder);
		juce::dsp::WindowingFunction<float> window(fftSize, juce::dsp::WindowingFunction<float>::hann);

		float bpmMin = juce::jmax(60.0f, (float)hostBpm * 0.8f);
		float bpmMax = juce::jmin(bpmMin * 1.99f, (float)hostBpm * 1.1f);
		BeatDetektor detektor(bpmMin, bpmMax);

		std::vector<float> fftBuffer(fftSize * 2, 0.0f);
		std::vector<float> magnitudes(fftSize / 2, 0.0f);

		for (int pos = 0; pos + fftSize <= numSamples; pos += hopSize)
		{
			std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
			for (int i = 0; i < fftSize; ++i)
				fftBuffer[i] = channelData[pos + i];

			window.multiplyWithWindowingTable(fftBuffer.data(), fftSize);
			fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

			for (int i = 0; i < fftSize / 2; ++i)
				magnitudes[i] = fftBuffer[i];

			float timestamp = (float)pos / (float)sampleRate;
			detektor.process(timestamp, magnitudes);
		}

		double rawValue = detektor.winning_bpm;
		if (rawValue <= 0.0)
			rawValue = detektor.current_bpm;

		if (rawValue > 0.0 && rawValue < 10.0)
			return 60.0 / rawValue;
		if (rawValue > 0.0)
			return rawValue;
		return 0.0;
	}
};

std::unique_ptr<BpmDetector> createBpmDetector()
{
	return std::make_unique<BeatDetektorBpmDetector>();
}