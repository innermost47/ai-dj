#pragma once
#include "AccordionItem.h"
#include "ObsidianListItem.h"
#include "PromptBank.h"
#include <JuceHeader.h>

#if JUCE_MSVC
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

class PromptBankItem : public AccordionItem, public ObsidianListItem
{
  public:
	explicit PromptBankItem(PromptBankEntry *entry);
	~PromptBankItem() override;

	void paint(juce::Graphics &g) override;
	int getPreferredHeight(int width) const override;

	void setCategoryColourResolver(std::function<juce::Colour(const juce::String &)> resolver)
	{
		categoryColourResolver = std::move(resolver);
	}

	PromptBankEntry *getEntry() const
	{
		return entry;
	}

  private:
	PromptBankEntry *entry{nullptr};
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBankItem)
};