#include "Distortion.h"

Distortion::Distortion()
{
}

void Distortion::reset() noexcept
{
	processorChain.reset();
}

void Distortion::setBypassed(bool b)
{
	bypassed = b;
	processorChain.setBypassed<Obsidian::distortionChain::filter>(bypassed);
	processorChain.setBypassed<Obsidian::distortionChain::preGain>(bypassed);
	processorChain.setBypassed<Obsidian::distortionChain::waveshaper>(bypassed);
	processorChain.setBypassed<Obsidian::distortionChain::postGain>(bypassed);
}

void Distortion::setPre(float p)
{
	pre = p;
	updatePre();
}

void Distortion::updatePre()
{
	auto &preGain = processorChain.template get<Obsidian::distortionChain::preGain>();
	float multiplicator = getMultiplicator(distortionType);
	preGain.setGainDecibels(pre * multiplicator);
}

void Distortion::setPost(float p)
{
	post = p;
	auto &postGain = processorChain.template get<Obsidian::distortionChain::postGain>();
	postGain.setGainDecibels(post);
}

void Distortion::setCut(float c)
{
	cut = c;
	auto &filter = processorChain.template get<Obsidian::distortionChain::filter>();
	filter.setCutoffFrequency(cut);
}

float Distortion::getMultiplicator(Obsidian::distortionType type)
{
	float value = 1.f;
	switch (type)
	{
	case Obsidian::distortionType::soft:
		value = 1.f;
		break;
	case Obsidian::distortionType::hard:
		value = 2.f;
		break;
	case Obsidian::distortionType::tube:
		value = 1.f;
		break;
	case Obsidian::distortionType::fold:
		value = 1.f;
		break;
	case Obsidian::distortionType::diode:
		value = 1.f;
		break;
	case Obsidian::distortionType::cubic:
		value = 1.f;
		break;
	default:
		value = 1.f;
	}
	return value;
}

void Distortion::setType(Obsidian::distortionType type)
{
	distortionType = type;
	auto &waveshaper = processorChain.template get<Obsidian::distortionChain::waveshaper>();
	if (type == Obsidian::distortionType::soft)
		waveshaper.functionToUse = [](float x) { return std::tanh(x); };
	else if (type == Obsidian::distortionType::hard)
		waveshaper.functionToUse = [](float x) { return std::clamp(x, -1.f, 1.f); };
	else if (type == Obsidian::distortionType::tube)
		waveshaper.functionToUse = [](float x) { return std::tanh(x + .3f) - std::tanh(.3f); };
	else if (type == Obsidian::distortionType::fold)
		waveshaper.functionToUse = [](float x) { return std::sin(x * juce::MathConstants<float>::pi) * .5f; };
	else if (type == Obsidian::distortionType::diode)
		waveshaper.functionToUse = [](float x)
		{
			if (x > 0.f)
				return std::tanh(x * 2.f);
			else
				return std::tanh(x * .5f);
		};
	else if (type == Obsidian::distortionType::cubic)
		waveshaper.functionToUse = [](float x) { return 1.5f * x - .5f * x * x * x; };
	updatePre();
}

void Distortion::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = spec.sampleRate;
	processorChain.prepare(spec);

	auto &filter = processorChain.template get<Obsidian::distortionChain::filter>();
	filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
	filter.setCutoffFrequency(cut);

	auto &waveshaper = processorChain.template get<Obsidian::distortionChain::waveshaper>();
	if (waveshaper.functionToUse == nullptr)
		waveshaper.functionToUse = [](float x) { return std::tanh(x); };
}

void Distortion::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	processorChain.process(context);
}