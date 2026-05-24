#pragma once
#include "BasePanel.h"
#include "PromptBank.h"
#include "PromptBankItem.h"
#include "PromptCategoryAccordion.h"
#include <JuceHeader.h>
#include <set>

class DjIaVstProcessor;
class DjIaVstEditor;

class PromptBankPanel : public BasePanel
{
  public:
	enum SortType
	{
		Recent = 1,
		Alphabetical = 2,
		MostUsed = 3,
		Model = 4
	};

	PromptBankPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~PromptBankPanel() override;

	static constexpr SortType firstSort = Recent;
	static constexpr SortType lastSort = Model;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void refreshList();

	int getSortType()
	{
		return static_cast<int>(currentSortType);
	}

  private:
	void setupUI();
	void rebuildAccordions(bool autoExpandOnSort = false);
	void applyFilterAndSort();

	void onAccordionExpanded(const juce::String &categoryName, bool expanded);
	void onPromptClicked(PromptBankEntry *entry);
	void onPromptEditRequested(PromptBankEntry *entry);
	void onPromptDeleteRequested(PromptBankEntry *entry);

	void addCategoryDialog();
	void editCategoryDialog(const juce::String &categoryName);
	void deleteCategoryDialog(const juce::String &categoryName);

	void addPromptDialog();
	void scrollToSelected();

	DjIaVstEditor &editor;

	std::vector<PromptBankEntry *> filteredPrompts;

	SortType currentSortType{Recent};

	PromptBankEntry *selectedEntry = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(PromptBankPanel);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBankPanel);
};