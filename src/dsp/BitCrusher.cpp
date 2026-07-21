#include "BitCrusher.h"

BitCrusher::BitCrusher()
{
}

void BitCrusher::setBypassed(bool b)
{
	bypassed = b;
}

void BitCrusher::setBitDepth(float b)
{
	bitDepth = juce::jlimit(1.0f, 32.0f, b);
}

void BitCrusher::setSampleRateReduction(float r)
{
	sampleRateReduction = juce::jmax(1.0f, r);
}

void BitCrusher::setMix(float m)
{
	mix = juce::jlimit(0.0f, 1.0f, m);
}

void BitCrusher::reset() noexcept
{
	phase = 0.0f;
	std::fill(holdBuffer.begin(), holdBuffer.end(), 0.0f);
}

void BitCrusher::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = spec.sampleRate;
	holdBuffer.assign(spec.numChannels, 0.0f);
	reset();
}

void BitCrusher::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	if (bypassed)
		return;

	auto &&outBlock = context.getOutputBlock();
	auto &&inBlock = context.getInputBlock();

	const auto numChannels = outBlock.getNumChannels();
	const auto numSamples = outBlock.getNumSamples();

	if (holdBuffer.size() < numChannels)
		holdBuffer.resize(numChannels, 0.0f);

	const float levels = std::pow(2.0f, bitDepth);
	const float step = 2.0f / levels;

	const float drive = 1.0f + (16.0f - bitDepth) * 0.4f;

	for (size_t sample = 0; sample < numSamples; ++sample)
	{
		bool updateHold = false;
		phase += 1.0f;
		if (phase >= sampleRateReduction)
		{
			phase -= sampleRateReduction;
			updateHold = true;
		}

		for (size_t channel = 0; channel < numChannels; ++channel)
		{
			const float dry = inBlock.getSample((int)channel, (int)sample);

			if (updateHold)
			{
				const float boosted = juce::jlimit(-1.0f, 1.0f, dry * drive);
				holdBuffer[channel] = boosted;
			}

			float wet = holdBuffer[channel];
			wet = step * std::floor(wet / step + 0.5f);
			wet = juce::jlimit(-1.0f, 1.0f, wet);
			wet /= drive;

			const float outSample = dry * (1.0f - mix) + wet * mix;
			outBlock.setSample((int)channel, (int)sample, outSample);
		}
	}
}