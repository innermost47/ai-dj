#include "Phaser.h"

Phaser::Phaser()
{
}

void Phaser::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::phaserChain::phaser>(bypassed);
}

void Phaser::setRate(float r)
{
	rate = r;
	auto &phaser = processorChain.template get<Obsidian::phaserChain::phaser>();
	phaser.setRate(rate);
}

void Phaser::setDepth(float d)
{
	depth = d;
	auto &phaser = processorChain.template get<Obsidian::phaserChain::phaser>();
	phaser.setDepth(depth);
}

void Phaser::setCentre(float c)
{
	centre = c;
	auto &phaser = processorChain.template get<Obsidian::phaserChain::phaser>();
	phaser.setCentreFrequency(centre);
}

void Phaser::setFeedback(float f)
{
	feedback = f;
	auto &phaser = processorChain.template get<Obsidian::phaserChain::phaser>();
	phaser.setFeedback(feedback);
}

void Phaser::setMix(float m)
{
	mix = m;
	auto &phaser = processorChain.template get<Obsidian::phaserChain::phaser>();
	phaser.setMix(mix);
}

void Phaser::reset() noexcept
{
	processorChain.reset();
}

void Phaser::prepare(const juce::dsp::ProcessSpec &spec)
{
	processorChain.prepare(spec);
}

void Phaser::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}