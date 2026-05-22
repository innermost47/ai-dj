#include "Console6Channel.h"
#include <cmath>

Console6Channel::Console6Channel()
{
	reset();
}

void Console6Channel::prepare(double sampleRate)
{
	currentSampleRate = sampleRate;
	reset();
}

void Console6Channel::reset()
{
	iirSampleLA = 0.0;
	iirSampleRA = 0.0;
	iirSampleLB = 0.0;
	iirSampleRB = 0.0;
	lastSampleL = 0.0;
	lastSampleR = 0.0;
	flip = false;

	fpdL = 1;
	while (fpdL < 16386)
		fpdL = static_cast<uint32_t>(rand()) * static_cast<uint32_t>(UINT32_MAX);
	fpdR = 1;
	while (fpdR < 16386)
		fpdR = static_cast<uint32_t>(rand()) * static_cast<uint32_t>(UINT32_MAX);
}

void Console6Channel::setConsoleType(ConsoleType type)
{
	switch (type)
	{
	case ConsoleType::Neve:
		iirAmount = 0.005832;
		threshold = 0.33362176;
		break;
	case ConsoleType::API:
		iirAmount = 0.004096;
		threshold = 0.59969536;
		break;
	case ConsoleType::SSL:
		iirAmount = 0.004913;
		threshold = 0.84934656;
		break;
	}
}

void Console6Channel::process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
	if (buffer.getNumChannels() < 2 || numSamples <= 0)
		return;

	float *in1 = buffer.getWritePointer(0, startSample);
	float *in2 = buffer.getWritePointer(1, startSample);

	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= currentSampleRate;
	const double localiirAmount = iirAmount / overallscale;
	const double localthreshold = threshold / overallscale;
	const double density = std::pow(drive, 2);

	for (int i = 0; i < numSamples; ++i)
	{
		double inputSampleL = in1[i];
		double inputSampleR = in2[i];

		if (std::fabs(inputSampleL) < 1.18e-23)
			inputSampleL = fpdL * 1.18e-17;
		if (std::fabs(inputSampleR) < 1.18e-23)
			inputSampleR = fpdR * 1.18e-17;

		if (flip)
		{
			iirSampleLA = (iirSampleLA * (1.0 - localiirAmount)) + (inputSampleL * localiirAmount);
			inputSampleL = inputSampleL - iirSampleLA;
			iirSampleRA = (iirSampleRA * (1.0 - localiirAmount)) + (inputSampleR * localiirAmount);
			inputSampleR = inputSampleR - iirSampleRA;
		}
		else
		{
			iirSampleLB = (iirSampleLB * (1.0 - localiirAmount)) + (inputSampleL * localiirAmount);
			inputSampleL = inputSampleL - iirSampleLB;
			iirSampleRB = (iirSampleRB * (1.0 - localiirAmount)) + (inputSampleR * localiirAmount);
			inputSampleR = inputSampleR - iirSampleRB;
		}

		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;

		if (inputSampleL > 1.0)
			inputSampleL = 1.0;
		if (inputSampleL < -1.0)
			inputSampleL = -1.0;
		inputSampleL *= 1.2533141373155;
		double distSampleL = std::sin(inputSampleL * std::fabs(inputSampleL)) /
		                     ((std::fabs(inputSampleL) == 0.0) ? 1.0 : std::fabs(inputSampleL));
		inputSampleL = (drySampleL * (1.0 - density)) + (distSampleL * density);

		if (inputSampleR > 1.0)
			inputSampleR = 1.0;
		if (inputSampleR < -1.0)
			inputSampleR = -1.0;
		inputSampleR *= 1.2533141373155;
		double distSampleR = std::sin(inputSampleR * std::fabs(inputSampleR)) /
		                     ((std::fabs(inputSampleR) == 0.0) ? 1.0 : std::fabs(inputSampleR));
		inputSampleR = (drySampleR * (1.0 - density)) + (distSampleR * density);

		double clamp = inputSampleL - lastSampleL;
		if (clamp > localthreshold)
			inputSampleL = lastSampleL + localthreshold;
		if (-clamp > localthreshold)
			inputSampleL = lastSampleL - localthreshold;
		lastSampleL = inputSampleL;

		clamp = inputSampleR - lastSampleR;
		if (clamp > localthreshold)
			inputSampleR = lastSampleR + localthreshold;
		if (-clamp > localthreshold)
			inputSampleR = lastSampleR - localthreshold;
		lastSampleR = inputSampleR;

		flip = !flip;

		if (output < 1.0f)
		{
			inputSampleL *= output;
			inputSampleR *= output;
		}

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