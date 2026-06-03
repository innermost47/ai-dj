#include "MonoEQ.h"

void MonoEQ::updateCoefficients(Obsidian::filterType type, float fq, float q, float g)
{
	frequency = fq;
	resonance = q;
	gain = g;

	switch (type)
	{
	case Obsidian::filterType::lowShelf:
	{
		auto newCoefs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, frequency, resonance, gain);
		if (newCoefs)
			*stereoFilter.state = *newCoefs;
		break;
	}
	case Obsidian::filterType::peakFilter:
	{
		auto newCoefs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, frequency, resonance, gain);
		if (newCoefs)
			*stereoFilter.state = *newCoefs;
		break;
	}
	case Obsidian::filterType::highShelf:
	{
		auto newCoefs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, frequency, resonance, gain);
		if (newCoefs)
			*stereoFilter.state = *newCoefs;
		break;
	}
	default:
		break;
	}
}

void MonoEQ::setSampleRate(double sr)
{
	sampleRate = sr;
}

void MonoEQ::reset() noexcept
{
	stereoFilter.reset();
}

void MonoEQ::init(float fr)
{
	frequency = fr;
	resonance = 0.707f;
	gain = 0.f;
}

void MonoEQ::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = spec.sampleRate;
	stereoFilter.prepare(spec);
	updateCoefficients(Obsidian::filterType::peakFilter, frequency, resonance, 1.0f);
}

void MonoEQ::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	stereoFilter.process(context);
}