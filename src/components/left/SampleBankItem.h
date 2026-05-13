#pragma once
#include "ObsidianBase.h"
#include "SampleBank.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class SampleBankItem : public ObsidianComponent, public juce::DragAndDropContainer
{
  public:
	SampleBankItem(SampleBankEntry *entry, DjIaVstProcessor &processor);
	~SampleBankItem() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent &event) override;
	void mouseDrag(const juce::MouseEvent &event) override;
	void mouseUp(const juce::MouseEvent &event) override;
	void mouseEnter(const juce::MouseEvent &event) override;
	void mouseExit(const juce::MouseEvent &event) override;

	SampleBankEntry *getSampleEntry() const
	{
		return sampleEntry;
	}
	void setSelected(bool s)
	{
		selected = s;
		repaint();
	}

	std::function<void(SampleBankEntry *)> onItemClicked;
	std::function<void(SampleBankEntry *)> onDeleteRequested;
	std::function<void(SampleBankEntry *, const std::vector<juce::String> &)> onCategoriesChanged;
	std::function<std::vector<juce::String>()> getCategoriesList;
	std::function<void(SampleBankEntry *)> onPromptEditRequested;
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

  private:
	SampleBankEntry *sampleEntry;
	DjIaVstProcessor &audioProcessor;

	bool selected = false;
	bool isDragging = false;

	void showCategoryMenu();
	juce::Colour getCategoryColor(const juce::String &category);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankItem)
};