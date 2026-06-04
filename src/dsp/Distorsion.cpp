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
	updatePre();
}

void Distorsion::updatePre()
{
	auto &preGain = processorChain.template get<Obsidian::distorsionChain::preGain>();
	float multiplicator = getMultiplicator(distorsionType);
	preGain.setGainDecibels(pre * multiplicator);
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
	filter.setCutoffFrequency(cut);
}

float Distorsion::getMultiplicator(Obsidian::distorsionType type)
{
	float value = 1.f;
	switch (type)
	{
	case Obsidian::distorsionType::soft:
		value = 1.f;
		break;
	case Obsidian::distorsionType::hard:
		value = 2.f;
		break;
	case Obsidian::distorsionType::tube:
		value = 1.f;
		break;
	case Obsidian::distorsionType::fold:
		value = 1.f;
		break;
	case Obsidian::distorsionType::diode:
		value = 1.f;
		break;
	case Obsidian::distorsionType::cubic:
		value = 1.f;
		break;
	default:
		value = 1.f;
	}
	return value;
}

void Distorsion::setType(Obsidian::distorsionType type)
{
	distorsionType = type;
	auto &waveshaper = processorChain.template get<Obsidian::distorsionChain::waveshaper>();
	if (type == Obsidian::distorsionType::soft)
		waveshaper.functionToUse = [](float x) { return std::tanh(x); };
	else if (type == Obsidian::distorsionType::hard)
		waveshaper.functionToUse = [](float x) { return std::clamp(x, -1.f, 1.f); };
	else if (type == Obsidian::distorsionType::tube)
		waveshaper.functionToUse = [](float x) { return std::tanh(x + .3f) - std::tanh(.3f); };
	else if (type == Obsidian::distorsionType::fold)
		waveshaper.functionToUse = [](float x) { return std::sin(x * juce::MathConstants<float>::pi) * .5f; };
	else if (type == Obsidian::distorsionType::diode)
		waveshaper.functionToUse = [](float x)
		{
			if (x > 0.f)
				return std::tanh(x * 2.f);
			else
				return std::tanh(x * .5f);
		};
	else if (type == Obsidian::distorsionType::cubic)
		waveshaper.functionToUse = [](float x) { return 1.5f * x - .5f * x * x * x; };
	updatePre();
}

void Distorsion::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = spec.sampleRate;
	processorChain.prepare(spec);

	auto &filter = processorChain.template get<Obsidian::distorsionChain::filter>();
	filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
	filter.setCutoffFrequency(cut);
}

void Distorsion::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}