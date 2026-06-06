#include "Chorus.h"

Chorus::Chorus()
{
}

void Chorus::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::chorusChain::chorus>(bypassed);
}

void Chorus::setRate(float r)
{
	rate = r;
	auto &chorus = processorChain.template get<Obsidian::chorusChain::chorus>();
	chorus.setRate(rate);
}

void Chorus::setDepth(float d)
{
	depth = d;
	auto &chorus = processorChain.template get<Obsidian::chorusChain::chorus>();
	chorus.setDepth(depth);
}

void Chorus::setCentre(float c)
{
	centre = c;
	auto &chorus = processorChain.template get<Obsidian::chorusChain::chorus>();
	chorus.setCentreDelay(centre);
}

void Chorus::setFeedback(float f)
{
	feedback = f;
	auto &chorus = processorChain.template get<Obsidian::chorusChain::chorus>();
	chorus.setFeedback(feedback);
}

void Chorus::setMix(float m)
{
	mix = m;
	auto &chorus = processorChain.template get<Obsidian::chorusChain::chorus>();
	chorus.setMix(mix);
}

void Chorus::reset() noexcept
{
	processorChain.reset();
}

void Chorus::prepare(const juce::dsp::ProcessSpec &spec)
{
	processorChain.prepare(spec);
}

void Chorus::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}