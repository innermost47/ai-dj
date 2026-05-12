#pragma once
#include "ColourPalette.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class CategoryTag : public juce::Button
{
  public:
	CategoryTag(const juce::String &name);
	void paintButton(juce::Graphics &g, bool isMouseOverButton, bool /*isButtonDown*/) override;
};

class CategoryPanel : public ObsidianComponent
{
  public:
	CategoryPanel(const std::vector<juce::String> &currentCategories,
	              const std::vector<juce::String> &availableCategories);
	void clearAll();
	std::vector<juce::String> getSelectedCategories() const;
	void resized() override;

  private:
	juce::Viewport viewport;
	std::unique_ptr<juce::Component> toggleContainer;
	juce::OwnedArray<CategoryTag> tags;
};