#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class CategoryTag : public juce::Button
{
  public:
	CategoryTag(const juce::String &name);
	void paintButton(juce::Graphics &g, bool isMouseOverButton, bool /*isButtonDown*/) override;

  private:
	enum CategoryRadioButtonIds
	{
		categories = 666
	};
};

class CategoryPanel : public ObsidianComponent
{
  public:
	CategoryPanel(const juce::String &currentCategory, const std::vector<juce::String> &availableCategories);
	juce::String getSelectedCategory() const;
	void resized() override;

  private:
	juce::Viewport viewport;
	std::unique_ptr<juce::Component> toggleContainer;
	juce::OwnedArray<CategoryTag> tags;
};