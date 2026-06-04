#include "Compressor.h"

void Compressor::setThreshold(float t)
{
	threshold = t;
	comp.setThreshold(threshold);
}

void Compressor::setRatio(float r)
{
	ratio = juce::jlimit(1.0f, 10.0f, r);
	comp.setRatio(ratio);
}

void Compressor::setAttack(float a)
{
	attack = a;
	comp.setAttack(attack);
}

void Compressor::setRelease(float r)
{
	release = r;
	comp.setRelease(release);
}

void Compressor::setMakeUpGain(float mk)
{
	makeUpGain = mk;
}

void Compressor::reset() noexcept
{
	comp.reset();
}

void Compressor::prepare(const juce::dsp::ProcessSpec &spec)
{
	comp.prepare(spec);
}

void Compressor::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	comp.process(context);
	context.getOutputBlock().multiplyBy(makeUpGain);
}