#include "GlitchStepButton.h"

GlitchStepButton::GlitchStepButton()
{
	setInterceptsMouseClicks(true, false);
}

void GlitchStepButton::setEffectType(GlitchEffectType type)
{
	if (effectType == type)
		return;
	effectType = type;
	setTooltip(effectType == GlitchEffectType::None ? juce::String() : getGlitchEffectName(effectType));
	repaint();
}

void GlitchStepButton::setAccentColour(juce::Colour colour)
{
	accentColour = colour;
	repaint();
}

void GlitchStepButton::setStepActive(bool isCurrentPlaybackStep)
{
	if (isPlaybackStep == isCurrentPlaybackStep)
		return;
	isPlaybackStep = isCurrentPlaybackStep;
	repaint();
}

void GlitchStepButton::mouseDown(const juce::MouseEvent &e)
{
	GlitchEffectType next;
	if (e.mods.isRightButtonDown())
		next = GlitchEffectType::None;
	else
	{
		const int current = static_cast<int>(effectType);
		const int count = static_cast<int>(GlitchEffectType::Count);
		next = static_cast<GlitchEffectType>((current + 1) % count);
	}

	if (onEffectChanged)
		onEffectChanged(next);

	setEffectType(next);
}

void GlitchStepButton::setAccent(Accent newAccent)
{
	if (accent == newAccent)
		return;
	accent = newAccent;
	repaint();
}

juce::Colour GlitchStepButton::getColourForType(GlitchEffectType type)
{
	if (type == GlitchEffectType::None)
		return ColourPalette::backgroundDark;

	int index = static_cast<int>(type) - 1;
	int count = static_cast<int>(GlitchEffectType::Count) - 1;
	float hue = static_cast<float>(index) / static_cast<float>(count);
	return juce::Colour::fromHSV(hue, 0.65f, 0.85f, 1.0f);
}

void GlitchStepButton::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat().reduced(1.0f);

	juce::Colour fillColour = getColourForType(effectType);
	if (effectType == GlitchEffectType::None)
		fillColour = fillColour.withAlpha(0.3f);

	g.setColour(fillColour);
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);

	if (isPlaybackStep)
	{
		g.setColour(juce::Colours::white.withAlpha(0.9f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 2.0f);
	}
	else if (accent == Accent::Downbeat)
	{
		g.setColour(ColourPalette::textPrimary.withAlpha(0.75f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 1.5f);
	}
	else if (accent == Accent::Beat)
	{
		g.setColour(ColourPalette::textSecondary.withAlpha(0.45f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 1.0f);
	}
	else
	{
		g.setColour(ColourPalette::textSecondary.withAlpha(0.15f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 1.0f);
	}

	if (effectType != GlitchEffectType::None && getWidth() > 20)
	{
		g.setColour(juce::Colours::white);
		g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
		g.drawText(getGlitchEffectShortName(effectType), bounds.reduced(1.0f), juce::Justification::centred, false);
	}
}