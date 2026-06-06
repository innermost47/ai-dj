#include "Limiter.h"

Limiter::Limiter()
{
}

void Limiter::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::limiterChain::limiter>(bypassed);
	processorChain.setBypassed<Obsidian::limiterChain::limiterGain>(bypassed);
}

void Limiter::setThreshold(float t)
{
	threshold = t;
	auto &limiter = processorChain.template get<Obsidian::limiterChain::limiter>();
	limiter.setThreshold(threshold);
}

void Limiter::setRelease(float r)
{
	release = r;
	auto &limiter = processorChain.template get<Obsidian::limiterChain::limiter>();
	limiter.setRelease(release);
}

void Limiter::setMakeUpGain(float mk)
{
	makeUpGain = mk;
	auto &gain = processorChain.template get<Obsidian::limiterChain::limiterGain>();
	gain.setGainLinear(makeUpGain);
}

void Limiter::resetReductionAmount()
{
	reductionAmount = 0.f;
}

void Limiter::reset() noexcept
{
	reductionAmount = 0.f;
	processorChain.reset();
}

void Limiter::prepare(const juce::dsp::ProcessSpec &spec)
{
	reductionAmount = 0.f;
	processorChain.prepare(spec);
}

void Limiter::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	auto &block = context.getOutputBlock();
	juce::Range<float> minAndMaxBefore = block.findMinAndMax();
	float peakBefore = std::max(std::abs(minAndMaxBefore.getStart()), std::abs(minAndMaxBefore.getEnd()));
	processorChain.process(context);
	context.getOutputBlock().multiplyBy(makeUpGain);
	juce::Range<float> minAndMaxAfter = block.findMinAndMax();
	float peakAfter = std::max(std::abs(minAndMaxAfter.getStart()), std::abs(minAndMaxAfter.getEnd()));
	reductionAmount = juce::jlimit(0.f, 1.f, 1.f - (peakAfter / peakBefore));
}