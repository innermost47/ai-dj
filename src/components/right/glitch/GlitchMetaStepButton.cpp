#include "GlitchMetaStepButton.h"

GlitchMetaStepButton::GlitchMetaStepButton()
{
	setInterceptsMouseClicks(true, false);
}

void GlitchMetaStepButton::setValue(int newValue)
{
	newValue = juce::jlimit(0, 8, newValue);
	if (value == newValue)
		return;
	value = newValue;
	repaint();
}

void GlitchMetaStepButton::setPlaybackStep(bool isCurrent)
{
	if (isPlaybackStep == isCurrent)
		return;
	isPlaybackStep = isCurrent;
	repaint();
}

void GlitchMetaStepButton::setAccent(bool onBeat)
{
	if (accent == onBeat)
		return;
	accent = onBeat;
	repaint();
}

void GlitchMetaStepButton::mouseDown(const juce::MouseEvent &e)
{
	if (e.mods.isRightButtonDown())
		value = 0;
	else
		value = (value + 1) % 9;

	repaint();

	if (onValueChanged)
		onValueChanged(value);
}

void GlitchMetaStepButton::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat().reduced(1.0f);

	g.setColour(value > 0 ? ColourPalette::backgroundLight : ColourPalette::backgroundDark.withAlpha(0.4f));
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);

	if (isPlaybackStep)
	{
		g.setColour(juce::Colours::white.withAlpha(0.9f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 2.0f);
	}
	else
	{
		g.setColour(accent ? ColourPalette::textSecondary.withAlpha(0.55f)
		                   : ColourPalette::backgroundLight.withAlpha(0.5f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 1.0f);
	}

	g.setColour(value > 0 ? ColourPalette::textPrimary : ColourPalette::textSecondary.withAlpha(0.5f));
	g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
	g.drawText(value > 0 ? juce::String(value) : juce::String("-"), bounds, juce::Justification::centred, false);
}