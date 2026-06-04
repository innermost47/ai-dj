#include "Distorsion.h"

Distorsion::Distorsion()
{
}

void Distorsion::reset() noexcept
{
	processorChain.reset();
}

void Distorsion::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::distorsionChain::filter>(bypassed);
	processorChain.setBypassed<Obsidian::distorsionChain::preGain>(bypassed);
	processorChain.setBypassed<Obsidian::distorsionChain::waveshaper>(bypassed);
	processorChain.setBypassed<Obsidian::distorsionChain::postGain>(bypassed);
}

void Distorsion::setPre(float p)
{
	pre = p;
	auto &preGain = processorChain.template get<Obsidian::distorsionChain::preGain>();
	preGain.setGainDecibels(pre);
}

void Distorsion::setPost(float p)
{
	post = p;
	auto &postGain = processorChain.template get<Obsidian::distorsionChain::postGain>();
	postGain.setGainDecibels(post);
}

void Distorsion::setCut(float c)
{
	cut = c;
	auto &filter = processorChain.template get<Obsidian::distorsionChain::filter>();
	filter.state = FilterCoefs::makeFirstOrderHighPass(sampleRate, cut);
}

void Distorsion::setType(Obsidian::distorsionType type)
{
	distorsionType = type;
	auto &waveshaper = processorChain.template get<Obsidian::distorsionChain::waveshaper>();
	if (type == Obsidian::distorsionType::soft)
		waveshaper.functionToUse = [](float x) { return std::tanh(x); };
	else if (type == Obsidian::distorsionType::hard)
		waveshaper.functionToUse = [](float x) { return std::clamp(x, -1.f, 1.f); };
	else if (type == Obsidian::distorsionType::sigm)
		waveshaper.functionToUse = [](float x) { return (2.f / (1 + std::exp(-x)) - 1.f); };
	else if (type == Obsidian::distorsionType::arc)
		waveshaper.functionToUse = [](float x) { return (2.f / juce::MathConstants<float>::pi) * std::atan(x); };
	else if (type == Obsidian::distorsionType::fold)
		waveshaper.functionToUse = [](float x) { return std::sin(x); };
	else if (type == Obsidian::distorsionType::crush)
		waveshaper.functionToUse = [](float x) { return std::round(x * 16.f) / 16.f; };
}

void Distorsion::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = spec.sampleRate;
	auto &filter = processorChain.template get<Obsidian::distorsionChain::filter>();
	filter.state = FilterCoefs::makeFirstOrderHighPass(sampleRate, cut);
	processorChain.prepare(spec);
}

void Distorsion::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}