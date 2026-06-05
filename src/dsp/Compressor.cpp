#include "Compressor.h"

Compressor::Compressor()
{
}

void Compressor::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::compressorChain::compressor>(bypassed);
	processorChain.setBypassed<Obsidian::compressorChain::makeUpGain>(bypassed);
}

void Compressor::setThreshold(float t)
{
	threshold = t;
	auto &compressor = processorChain.template get<Obsidian::compressorChain::compressor>();
	compressor.setThreshold(threshold);
}

void Compressor::setRatio(float r)
{
	ratio = juce::jlimit(1.0f, 10.0f, r);
	auto &compressor = processorChain.template get<Obsidian::compressorChain::compressor>();
	compressor.setRatio(ratio);
}

void Compressor::setAttack(float a)
{
	attack = a;
	auto &compressor = processorChain.template get<Obsidian::compressorChain::compressor>();
	compressor.setAttack(attack);
}

void Compressor::setRelease(float r)
{
	release = r;
	auto &compressor = processorChain.template get<Obsidian::compressorChain::compressor>();
	compressor.setRelease(release);
}

void Compressor::setMakeUpGain(float mk)
{
	makeUpGain = mk;
	auto &gain = processorChain.template get<Obsidian::compressorChain::makeUpGain>();
	gain.setGainLinear(makeUpGain);
}

void Compressor::reset() noexcept
{
	processorChain.reset();
}

void Compressor::prepare(const juce::dsp::ProcessSpec &spec)
{
	processorChain.prepare(spec);
}

void Compressor::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}