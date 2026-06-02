#include "LowpassHighpassFilter.h"

void LowpassHighpassFilter::setHighpass(bool value)
{
	highpass = value;
	highpass ? filter.setMode(juce::dsp::LadderFilterMode::HPF12) : filter.setMode(juce::dsp::LadderFilterMode::LPF12);
}

void LowpassHighpassFilter::setSamplingRate(double sr)
{
	samplingRate = sr;
}

void LowpassHighpassFilter::setCutoffFrequency(float frequency)
{
	cutoffFrequency = frequency;
	filter.setCutoffFrequencyHz(frequency);
}

void LowpassHighpassFilter::setResonance(float q)
{
	resonance = juce::jlimit(0.f, 0.9f, q);
	filter.setResonance(resonance);
}

void LowpassHighpassFilter::reset()
{
	filter.reset();
}

void LowpassHighpassFilter::prepare(int numChannels, juce::uint32 maxBlockSize)
{
	auto spec = juce::dsp::ProcessSpec();
	spec.maximumBlockSize = maxBlockSize;
	spec.sampleRate = samplingRate;
	spec.numChannels = static_cast<juce::uint32>(numChannels);
	filter.prepare(spec);
	filter.setEnabled(true);
	filter.setMode(juce::dsp::LadderFilterMode::LPF12);
	reset();
}

void LowpassHighpassFilter::processBlock(juce::AudioBuffer<float> &buffer)
{
	juce::ScopedNoDenormals noDenormals;

	auto block = juce::dsp::AudioBlock<float>(buffer);
	auto blockToUse = block.getSubBlock(0, buffer.getNumSamples());
	auto contextToUse = juce::dsp::ProcessContextReplacing<float>(blockToUse);

	filter.process(contextToUse);
}