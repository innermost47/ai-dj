#include "LowpassHighpassFilter.h"

void LowpassHighpassFilter::setHighpass(bool value)
{
	highpass = value;
}

void LowpassHighpassFilter::setSamplingRate(double sr)
{
	samplingRate = sr;
}

void LowpassHighpassFilter::setCutoffFrequency(float frequency)
{
	cutoffFrequency = frequency;
}

void LowpassHighpassFilter::setResonance(float q)
{
	resonance = juce::jmax(0.1f, q);
}

void LowpassHighpassFilter::reset()
{
	for (auto &state : channelStates)
	{
		state.x1 = 0.f;
		state.x2 = 0.f;
		state.y1 = 0.f;
		state.y2 = 0.f;
	}
}

void LowpassHighpassFilter::prepare(int numChannels)
{
	dnBuffer.assign(numChannels, 0.f);
	channelStates.resize(numChannels);
	reset();
}

float LowpassHighpassFilter::softClip(float x) noexcept
{
	const float threshold = 1.0f;
	if (x > threshold)
		return threshold;
	if (x < -threshold)
		return -threshold;
	return x - (x * x * x) / 3.f;
}

LowpassHighpassFilter::BiquadCoeffs LowpassHighpassFilter::computeCoeffs(float cutoff, float res, bool hp,
                                                                         float sampleRate) const
{
	const float PI = juce::MathConstants<float>::pi;
	const auto w0 = (PI * 2 * cutoff) / static_cast<float>(sampleRate);
	const auto alpha = std::sin(w0) / (2 * res);
	const auto cosW0 = std::cos(w0);
	const auto base = hp ? (1.f + cosW0) : (1.f - cosW0);
	return {base / 2.f, hp ? -base : base, base / 2.f, 1 + alpha, -2 * cosW0, 1 - alpha};
}

void LowpassHighpassFilter::processBlock(juce::AudioBuffer<float> &buffer)
{
	juce::ScopedNoDenormals noDenormals;

	BiquadCoeffs coefs = computeCoeffs(cutoffFrequency, resonance, highpass, static_cast<float>(samplingRate));

	if (dnBuffer.size() < size_t(buffer.getNumChannels()))
		dnBuffer.resize(buffer.getNumChannels(), 0.f);

	for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
	{
		auto channelSamples = buffer.getWritePointer(channel);
		for (auto i = 0; i < buffer.getNumSamples(); ++i)
		{
			const float input = channelSamples[i];
			auto &state = channelStates[channel];
			float output = (coefs.b0 / coefs.a0 * input) + (coefs.b1 / coefs.a0 * state.x1) +
			               (coefs.b2 / coefs.a0 * state.x2) - (coefs.a1 / coefs.a0 * state.y1) -
			               (coefs.a2 / coefs.a0 * state.y2);

			output = softClip(output);

			state.x2 = state.x1;
			state.x1 = input;
			state.y2 = state.y1;
			state.y1 = juce::jlimit(-2.f, 2.f, output);
			channelSamples[i] = output;
		}
	}
}