#include "Equalizer.h"

float Equalizer::getGain(Obsidian::eqBands band) const
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		return bandsChain.template get<Obsidian::eqBands::subBass>().getGain();
		break;
	case Obsidian::eqBands::bass:
		return bandsChain.template get<Obsidian::eqBands::bass>().getGain();
		break;
	case Obsidian::eqBands::lowMid:
		return bandsChain.template get<Obsidian::eqBands::lowMid>().getGain();
		break;
	case Obsidian::eqBands::mid:
		return bandsChain.template get<Obsidian::eqBands::mid>().getGain();
		break;
	case Obsidian::eqBands::highMid:
		return bandsChain.template get<Obsidian::eqBands::highMid>().getGain();
		break;
	case Obsidian::eqBands::presence:
		return bandsChain.template get<Obsidian::eqBands::presence>().getGain();
		break;
	case Obsidian::eqBands::high:
		return bandsChain.template get<Obsidian::eqBands::high>().getGain();
		break;
	case Obsidian::eqBands::air:
		return bandsChain.template get<Obsidian::eqBands::air>().getGain();
		break;
	default:
		return 1.0f;
		break;
	}
}

float Equalizer::getFrequency(Obsidian::eqBands band) const
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		return bandsChain.template get<Obsidian::eqBands::subBass>().getFrequency();
		break;
	case Obsidian::eqBands::bass:
		return bandsChain.template get<Obsidian::eqBands::bass>().getFrequency();
		break;
	case Obsidian::eqBands::lowMid:
		return bandsChain.template get<Obsidian::eqBands::lowMid>().getFrequency();
		break;
	case Obsidian::eqBands::mid:
		return bandsChain.template get<Obsidian::eqBands::mid>().getFrequency();
		break;
	case Obsidian::eqBands::highMid:
		return bandsChain.template get<Obsidian::eqBands::highMid>().getFrequency();
		break;
	case Obsidian::eqBands::presence:
		return bandsChain.template get<Obsidian::eqBands::presence>().getFrequency();
		break;
	case Obsidian::eqBands::high:
		return bandsChain.template get<Obsidian::eqBands::high>().getFrequency();
		break;
	case Obsidian::eqBands::air:
		return bandsChain.template get<Obsidian::eqBands::air>().getFrequency();
		break;
	default:
		return 20000.0f;
		break;
	}
}

float Equalizer::getQ(Obsidian::eqBands band) const
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		return bandsChain.template get<Obsidian::eqBands::subBass>().getQ();
		break;
	case Obsidian::eqBands::bass:
		return bandsChain.template get<Obsidian::eqBands::bass>().getQ();
		break;
	case Obsidian::eqBands::lowMid:
		return bandsChain.template get<Obsidian::eqBands::lowMid>().getQ();
		break;
	case Obsidian::eqBands::mid:
		return bandsChain.template get<Obsidian::eqBands::mid>().getQ();
		break;
	case Obsidian::eqBands::highMid:
		return bandsChain.template get<Obsidian::eqBands::highMid>().getQ();
		break;
	case Obsidian::eqBands::presence:
		return bandsChain.template get<Obsidian::eqBands::presence>().getQ();
		break;
	case Obsidian::eqBands::high:
		return bandsChain.template get<Obsidian::eqBands::high>().getQ();
		break;
	case Obsidian::eqBands::air:
		return bandsChain.template get<Obsidian::eqBands::air>().getQ();
		break;
	default:
		return 0.707f;
		break;
	}
}

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

void Equalizer::updateFrequency(Obsidian::eqBands band, float value)
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		bandsChain.template get<Obsidian::eqBands::subBass>().updateFrequency(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::bass:
		bandsChain.template get<Obsidian::eqBands::bass>().updateFrequency(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::lowMid:
		bandsChain.template get<Obsidian::eqBands::lowMid>().updateFrequency(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::mid:
		bandsChain.template get<Obsidian::eqBands::mid>().updateFrequency(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::highMid:
		bandsChain.template get<Obsidian::eqBands::highMid>().updateFrequency(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::presence:
		bandsChain.template get<Obsidian::eqBands::presence>().updateFrequency(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::high:
		bandsChain.template get<Obsidian::eqBands::high>().updateFrequency(Obsidian::filterType::highShelf, value);
		break;
	case Obsidian::eqBands::air:
		bandsChain.template get<Obsidian::eqBands::air>().updateFrequency(Obsidian::filterType::highShelf, value);
		break;
	default:
		break;
	}
}

void Equalizer::updateQ(Obsidian::eqBands band, float value)
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		bandsChain.template get<Obsidian::eqBands::subBass>().updateQ(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::bass:
		bandsChain.template get<Obsidian::eqBands::bass>().updateQ(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::lowMid:
		bandsChain.template get<Obsidian::eqBands::lowMid>().updateQ(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::mid:
		bandsChain.template get<Obsidian::eqBands::mid>().updateQ(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::highMid:
		bandsChain.template get<Obsidian::eqBands::highMid>().updateQ(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::presence:
		bandsChain.template get<Obsidian::eqBands::presence>().updateQ(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::high:
		bandsChain.template get<Obsidian::eqBands::high>().updateQ(Obsidian::filterType::highShelf, value);
		break;
	case Obsidian::eqBands::air:
		bandsChain.template get<Obsidian::eqBands::air>().updateQ(Obsidian::filterType::highShelf, value);
		break;
	default:
		break;
	}
}

void Equalizer::updateGain(Obsidian::eqBands band, float value)
{
	switch (band)
	{
	case Obsidian::eqBands::subBass:
		bandsChain.template get<Obsidian::eqBands::subBass>().updateGain(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::bass:
		bandsChain.template get<Obsidian::eqBands::bass>().updateGain(Obsidian::filterType::lowShelf, value);
		break;
	case Obsidian::eqBands::lowMid:
		bandsChain.template get<Obsidian::eqBands::lowMid>().updateGain(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::mid:
		bandsChain.template get<Obsidian::eqBands::mid>().updateGain(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::highMid:
		bandsChain.template get<Obsidian::eqBands::highMid>().updateGain(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::presence:
		bandsChain.template get<Obsidian::eqBands::presence>().updateGain(Obsidian::filterType::peakFilter, value);
		break;
	case Obsidian::eqBands::high:
		bandsChain.template get<Obsidian::eqBands::high>().updateGain(Obsidian::filterType::highShelf, value);
		break;
	case Obsidian::eqBands::air:
		bandsChain.template get<Obsidian::eqBands::air>().updateGain(Obsidian::filterType::highShelf, value);
		break;
	default:
		break;
	}
}

void Equalizer::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	bandsChain.process(context);
}