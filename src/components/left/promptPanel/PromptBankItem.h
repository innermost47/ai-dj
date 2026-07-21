#pragma once
#include "ObsidianListItem.h"
#include "PromptBank.h"
#include <JuceHeader.h>
#if JUCE_MSVC
#pragma warning(push)
#pragma warning(disable : 4250)
#endif
class PromptBankItem : public ObsidianListItem
{
  public:
	explicit PromptBankItem(PromptBankEntry *entry);
	~PromptBankItem() override;
	void paint(juce::Graphics &g) override;
	void setCategoryColourResolver(std::function<juce::Colour(const juce::String &)> resolver)
	{
		categoryColourResolver = std::move(resolver);
	}
	PromptBankEntry *getEntry() const
	{
		return entry;
	}

  protected:
	int getBaseHeight() const override
	{
		return Obsidian::ACCORDION_ITEM_MIN_HEIGHT;
	}

  private:
	PromptBankEntry *entry{nullptr};
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBankItem)
};
#if JUCE_MSVC
#pragma warning(pop)
#endif