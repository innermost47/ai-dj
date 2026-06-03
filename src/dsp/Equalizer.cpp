#include "Equalizer.h"

void Equalizer::prepare(const juce::dsp::ProcessSpec &spec)
{
	sampleRate = static_cast<float>(spec.sampleRate);
	bandsChain.get<Obsidian::eqBands::subBass>().init(40.0f);
	bandsChain.get<Obsidian::eqBands::bass>().init(120.0f);
	bandsChain.get<Obsidian::eqBands::lowMid>().init(350.0f);
	bandsChain.get<Obsidian::eqBands::mid>().init(1000.0f);
	bandsChain.get<Obsidian::eqBands::highMid>().init(3000.0f);
	bandsChain.get<Obsidian::eqBands::presence>().init(5000.0f);
	bandsChain.get<Obsidian::eqBands::high>().init(8000.0f);
	bandsChain.get<Obsidian::eqBands::air>().init(15000.0f);
	bandsChain.prepare(spec);
}

void Equalizer::reset() noexcept
{
	bandsChain.reset();
}

void Equalizer::update(Obsidian::eqBands band, float frequency, float q, float gain)
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		bandsChain.template get<Obsidian::eqBands::subBass>().updateCoefficients(Obsidian::filterType::lowShelf,
		                                                                         frequency, q, gain);
		break;
	case Obsidian::eqBands::bass:
		bandsChain.template get<Obsidian::eqBands::bass>().updateCoefficients(Obsidian::filterType::lowShelf, frequency,
		                                                                      q, gain);
		break;
	case Obsidian::eqBands::lowMid:
		bandsChain.template get<Obsidian::eqBands::lowMid>().updateCoefficients(Obsidian::filterType::peakFilter,
		                                                                        frequency, q, gain);
		break;
	case Obsidian::eqBands::mid:
		bandsChain.template get<Obsidian::eqBands::mid>().updateCoefficients(Obsidian::filterType::peakFilter,
		                                                                     frequency, q, gain);
		break;
	case Obsidian::eqBands::highMid:
		bandsChain.template get<Obsidian::eqBands::highMid>().updateCoefficients(Obsidian::filterType::peakFilter,
		                                                                         frequency, q, gain);
		break;
	case Obsidian::eqBands::presence:
		bandsChain.template get<Obsidian::eqBands::presence>().updateCoefficients(Obsidian::filterType::peakFilter,
		                                                                          frequency, q, gain);
		break;
	case Obsidian::eqBands::high:
		bandsChain.template get<Obsidian::eqBands::high>().updateCoefficients(Obsidian::filterType::highShelf,
		                                                                      frequency, q, gain);
		break;
	case Obsidian::eqBands::air:
		bandsChain.template get<Obsidian::eqBands::air>().updateCoefficients(Obsidian::filterType::highShelf, frequency,
		                                                                     q, gain);
		break;
	default:
		break;
	}
}

void Equalizer::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	bandsChain.process(context);
}