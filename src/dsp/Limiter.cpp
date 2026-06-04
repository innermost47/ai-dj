#include "Limiter.h"

void Limiter::setThreshold(float t)
{
	threshold = t;
	lim.setThreshold(threshold);
}

void Limiter::setRelease(float r)
{
	release = r;
	lim.setRelease(release);
}

void Limiter::setMakeUpGain(float mk)
{
	makeUpGain = mk;
}

void Limiter::resetReductionAmount()
{
	reductionAmount = 0.f;
}

void Limiter::reset() noexcept
{
	reductionAmount = 0.f;
	lim.reset();
}

void Limiter::prepare(const juce::dsp::ProcessSpec &spec)
{
	reductionAmount = 0.f;
	lim.prepare(spec);
}

void Limiter::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	auto &block = context.getOutputBlock();
	juce::Range<float> minAndMaxBefore = block.findMinAndMax();
	float peakBefore = std::max(std::abs(minAndMaxBefore.getStart()), std::abs(minAndMaxBefore.getEnd()));
	lim.process(context);
	context.getOutputBlock().multiplyBy(makeUpGain);
	juce::Range<float> minAndMaxAfter = block.findMinAndMax();
	float peakAfter = std::max(std::abs(minAndMaxAfter.getStart()), std::abs(minAndMaxAfter.getEnd()));
	reductionAmount = juce::jlimit(0.f, 1.f, 1.f - (peakAfter / peakBefore));
}