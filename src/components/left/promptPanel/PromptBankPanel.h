#pragma once
#include "ObsidianBankHeader.h"
#include "ObsidianBase.h"
#include "PromptBank.h"
#include "PromptBankItem.h"
#include "PromptCategoryAccordion.h"
#include <JuceHeader.h>
#include <set>

class DjIaVstProcessor;
class DjIaVstEditor;

class PromptBankPanel : public juce::Component
{
  public:
	enum SortType
	{
		Recent = 1,
		Alphabetical,
		MostUsed,
		Model
	};

	PromptBankPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~PromptBankPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void refreshList();

	juce::var saveUIState() const;
	void restoreUIState(const juce::var &state);

  private:
	void setupUI();
	void rebuildAccordions();
	void applyFilterAndSort();

	void onAccordionExpanded(const juce::String &categoryName, bool expanded);
	void onPromptClicked(PromptBankEntry *entry);
	void onPromptEditRequested(PromptBankEntry *entry);
	void onPromptDeleteRequested(PromptBankEntry *entry);

	void addCategoryDialog();
	void editCategoryDialog(const juce::String &categoryName);
	void deleteCategoryDialog(const juce::String &categoryName);
	juce::Colour resolveCategoryColour(const juce::String &name) const;

	void addPromptDialog();

	void expandAll();
	void collapseAll();

	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;
	ObsidianBankHeader header;

	juce::Viewport accordionViewport;
	juce::Component accordionContainer;

	std::vector<std::unique_ptr<PromptCategoryAccordion>> accordions;
	std::vector<PromptBankEntry *> filteredPrompts;
	std::set<juce::String> openCategories;

	SortType currentSort = Recent;
	juce::String currentSearch;
	PromptBankEntry *selectedEntry = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBankPanel)
};