#include "ReverbSend.h"

ReverbSend::ReverbSend()
{
	updateParameters();
}

void ReverbSend::prepare(double sampleRate, int /*maxBlockSize*/)
{
	currentSampleRate = sampleRate;
	reverbL.setSampleRate(sampleRate);
	reverbR.setSampleRate(sampleRate);
	reset();
	updateParameters();
}

void ReverbSend::reset()
{
	reverbL.reset();
	reverbR.reset();
}

void ReverbSend::updateParameters()
{
	juce::Reverb::Parameters params;
	params.roomSize = size;
	params.damping = damping;
	params.width = width;
	params.wetLevel = 1.0f;
	params.dryLevel = 0.0f;
	params.freezeMode = 0.0f;

	reverbL.setParameters(params);
	reverbR.setParameters(params);
}

void ReverbSend::process(juce::AudioBuffer<float> &buffer, int startSample, int numSamples)
{
	if (buffer.getNumChannels() < 2)
		return;

	if (numSamples < 0)
		numSamples = buffer.getNumSamples() - startSample;

	if (numSamples <= 0)
		return;

	float *chL = buffer.getWritePointer(0, startSample);
	float *chR = buffer.getWritePointer(1, startSample);

	reverbL.processMono(chL, numSamples);
	reverbR.processMono(chR, numSamples);

	if (mix < 0.999f)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			chL[i] *= mix;
			chR[i] *= mix;
		}
	}
}