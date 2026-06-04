#include "LowpassHighpassFilter.h"

void LowpassHighpassFilter::setMode(juce::dsp::LadderFilterMode newMode)
{
	mode = newMode;
	filter.setMode(mode);
}

void LowpassHighpassFilter::setSamplingRate(double sr)
{
	samplingRate = sr;
}

void LowpassHighpassFilter::setCutoffFrequency(float frequency)
{
	cutoffFrequency = frequency;
	filter.setCutoffFrequencyHz(frequency);
}

void LowpassHighpassFilter::setResonance(float q)
{
	resonance = juce::jlimit(0.f, 0.9f, q);
	filter.setResonance(resonance);
}

void LowpassHighpassFilter::setDrive(float newDrive)
{
	drive = newDrive;
	filter.setDrive(drive);
}

void LowpassHighpassFilter::reset()
{
	filter.reset();
}

void LowpassHighpassFilter::prepare(const juce::dsp::ProcessSpec &spec)
{
	filter.prepare(spec);
	filter.setEnabled(true);
	filter.setMode(juce::dsp::LadderFilterMode::LPF12);
	reset();
}

void LowpassHighpassFilter::process(juce::dsp::ProcessContextReplacing<float> &context)
{
	filter.process(context);
}