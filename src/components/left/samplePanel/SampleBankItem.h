#pragma once
#include "AccordionItem.h"
#include "ObsidianListItem.h"
#include "SampleBank.h"

class DjIaVstProcessor;

#if JUCE_MSVC
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

class SampleBankItem : public AccordionItem, public ObsidianListItem
{
  public:
	SampleBankItem(SampleBankEntry *entry, DjIaVstProcessor &processor);
	~SampleBankItem() override;

	void paint(juce::Graphics &g) override;
	int getPreferredHeight(int width) const override;

	SampleBankEntry *getSampleEntry() const
	{
		return sampleEntry;
	}

	void setCategoryColourResolver(std::function<juce::Colour(const juce::String &)> resolver)
	{
		categoryColourResolver = std::move(resolver);
	}

	std::function<void(SampleBankEntry *)> onPromptEditRequested;
	std::function<void(SampleBankEntry *)> onSampleDeleteRequested;
	std::function<void(SampleBankEntry *)> onChangeCategoryRequested;

  protected:
	void mouseDrag(const juce::MouseEvent &event) override;

  private:
	void buildSampleContextMenu(const juce::MouseEvent &event);

	SampleBankEntry *sampleEntry{nullptr};
	DjIaVstProcessor &audioProcessor;
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankItem)
};

#if JUCE_MSVC
#pragma warning(pop)
#endif