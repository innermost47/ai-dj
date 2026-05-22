#pragma once
#include "ObsidianAccordion.h"
#include "PromptBankItem.h"
#include <JuceHeader.h>

class PromptCategoryAccordion : public ObsidianAccordion
{
  public:
	PromptCategoryAccordion(const juce::String &name, juce::Colour colour) : ObsidianAccordion(name, colour)
	{
	}

	~PromptCategoryAccordion() override = default;

	void setItems(std::vector<std::unique_ptr<PromptBankItem>> &&newItems);

  private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptCategoryAccordion)
};