#include "Flanger.h"

Flanger::Flanger()
{
}

void Flanger::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::flangerChain::flanger>(bypassed);
}

void Flanger::setRate(float r)
{
	rate = r;
	auto &flanger = processorChain.template get<Obsidian::flangerChain::flanger>();
	flanger.setRate(rate);
}

void Flanger::setDepth(float d)
{
	depth = d;
	auto &flanger = processorChain.template get<Obsidian::flangerChain::flanger>();
	flanger.setDepth(depth);
}

void Flanger::setCentre(float c)
{
	centre = c;
	auto &flanger = processorChain.template get<Obsidian::flangerChain::flanger>();
	flanger.setCentreDelay(centre);
}

void Flanger::setFeedback(float f)
{
	feedback = f;
	auto &flanger = processorChain.template get<Obsidian::flangerChain::flanger>();
	flanger.setFeedback(feedback);
}

void Flanger::setMix(float m)
{
	mix = m;
	auto &flanger = processorChain.template get<Obsidian::flangerChain::flanger>();
	flanger.setMix(mix);
}

void Flanger::reset() noexcept
{
	processorChain.reset();
}

void Flanger::prepare(const juce::dsp::ProcessSpec &spec)
{
	processorChain.prepare(spec);
}

void Flanger::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}