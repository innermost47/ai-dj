#pragma once
#include "ObsidianBase.h"
#include "OnboardingStepData.h"
#include <JuceHeader.h>
#include <memory>

class ConceptRowComponent : public ObsidianComponent
{
  public:
	explicit ConceptRowComponent(const ConceptRow &row);
	~ConceptRowComponent() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	int getPreferredHeight(int width) const;

  private:
	static constexpr int ICON_BOX_SIZE = 36;
	static constexpr int ICON_INSET = 8;
	static constexpr int LEFT_GAP = 16;
	static constexpr int TITLE_HEIGHT = 20;
	static constexpr int TITLE_BODY_GAP = 4;

	std::unique_ptr<juce::Drawable> loadIconByName(const juce::String &name);

	ConceptRow data;
	std::unique_ptr<juce::Drawable> icon;
	juce::Label titleLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConceptRowComponent)
};