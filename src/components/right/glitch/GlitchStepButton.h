#pragma once
#include "GlitchTypes.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class GlitchStepButton : public ObsidianComponent, public juce::SettableTooltipClient
{
  public:
	GlitchStepButton();

	enum class Accent
	{
		None,
		Beat,
		Downbeat
	};

	void paint(juce::Graphics &g) override;
	void mouseDown(const juce::MouseEvent &e) override;
	void setAccent(Accent newAccent);

	void setEffectType(GlitchEffectType type);
	GlitchEffectType getEffectType() const
	{
		return effectType;
	}

	void setAccentColour(juce::Colour colour);
	void setStepActive(bool isCurrentPlaybackStep);

	static juce::Colour getColourForType(GlitchEffectType type);

	std::function<void(GlitchEffectType)> onEffectChanged;

  private:
	Accent accent = Accent::None;
	GlitchEffectType effectType = GlitchEffectType::None;
	juce::Colour accentColour = juce::Colours::orange;
	bool isPlaybackStep = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchStepButton)
};