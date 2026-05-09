#include "Console6Bus.h"
#include <cmath>

Console6Buss::Console6Buss()
{
	reset();
}

void Console6Buss::prepare(double sampleRate)
{
	currentSampleRate = sampleRate;
	reset();
}

void Console6Buss::reset()
{
	fpdL = 1;
	while (fpdL < 16386)
		fpdL = static_cast<uint32_t>(rand()) * static_cast<uint32_t>(UINT32_MAX);
	fpdR = 1;
	while (fpdR < 16386)
		fpdR = static_cast<uint32_t>(rand()) * static_cast<uint32_t>(UINT32_MAX);
}

void Console6Buss::process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
	if (buffer.getNumChannels() < 2 || numSamples <= 0)
		return;

	float *in1 = buffer.getWritePointer(0, startSample);
	float *in2 = buffer.getWritePointer(1, startSample);

	const double localGain = static_cast<double>(gain);

	for (int i = 0; i < numSamples; ++i)
	{
		double inputSampleL = in1[i];
		double inputSampleR = in2[i];

		if (std::fabs(inputSampleL) < 1.18e-23)
			inputSampleL = fpdL * 1.18e-17;
		if (std::fabs(inputSampleR) < 1.18e-23)
			inputSampleR = fpdR * 1.18e-17;

		if (localGain != 1.0)
		{
			inputSampleL *= localGain;
			inputSampleR *= localGain;
		}

		if (inputSampleL > 1.0)
			inputSampleL = 1.0;
		else if (inputSampleL > 0.0)
			inputSampleL = inputSampleL / (1.0 + std::sqrt(1.0 - inputSampleL));
		if (inputSampleL < -1.0)
			inputSampleL = -1.0;
		else if (inputSampleL < 0.0)
			inputSampleL = inputSampleL / (std::sqrt(inputSampleL + 1.0) + 1.0);

		if (inputSampleR > 1.0)
			inputSampleR = 1.0;
		else if (inputSampleR > 0.0)
			inputSampleR = inputSampleR / (1.0 + std::sqrt(1.0 - inputSampleR));
		if (inputSampleR < -1.0)
			inputSampleR = -1.0;
		else if (inputSampleR < 0.0)
			inputSampleR = inputSampleR / (std::sqrt(inputSampleR + 1.0) + 1.0);

		int expon;
		std::frexp(static_cast<float>(inputSampleL), &expon);
		fpdL ^= fpdL << 13;
		fpdL ^= fpdL >> 17;
		fpdL ^= fpdL << 5;
		inputSampleL +=
		    ((static_cast<double>(fpdL) - static_cast<uint32_t>(0x7fffffff)) * 5.5e-36 * std::pow(2.0, expon + 62));

		std::frexp(static_cast<float>(inputSampleR), &expon);
		fpdR ^= fpdR << 13;
		fpdR ^= fpdR >> 17;
		fpdR ^= fpdR << 5;
		inputSampleR +=
		    ((static_cast<double>(fpdR) - static_cast<uint32_t>(0x7fffffff)) * 5.5e-36 * std::pow(2.0, expon + 62));

		in1[i] = static_cast<float>(inputSampleL);
		in2[i] = static_cast<float>(inputSampleR);
	}
}