#include "Filter.h"

Filter::Filter()
{
}

void Filter::setMode(juce::dsp::LadderFilterMode newMode)
{
	mode = newMode;
	filter.setMode(mode);
}

void Filter::setSamplingRate(double sr)
{
	samplingRate = sr;
}

void Filter::setCutoffFrequency(float frequency)
{
	cutoffFrequency = frequency;
	filter.setCutoffFrequencyHz(frequency);
}

void Filter::setResonance(float q)
{
	resonance = juce::jlimit(0.f, 0.9f, q);
	filter.setResonance(resonance);
}

void Filter::setDrive(float newDrive)
{
	drive = newDrive;
	filter.setDrive(drive);
}

void Filter::reset()
{
	filter.reset();
}

void Filter::prepare(const juce::dsp::ProcessSpec &spec)
{
	filter.prepare(spec);
	filter.setEnabled(true);
	filter.setMode(juce::dsp::LadderFilterMode::LPF12);
	reset();
}

void Filter::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	filter.process(context);
}