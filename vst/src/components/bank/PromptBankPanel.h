#pragma once
#include "ColourPalette.h"
#include "IconButton.h"
#include "PromptBank.h"
#include "PromptBankItem.h"
#include "PromptCategoryAccordion.h"
#include <JuceHeader.h>
#include <set>

class DjIaVstProcessor;
class DjIaVstEditor;

class ScaleAndDurationPanel : public juce::Component
{
  public:
	ScaleAndDurationPanel(DjIaVstProcessor &processor);
	~ScaleAndDurationPanel() = default;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void update();

  private:
	DjIaVstProcessor &audioProcessor;
	juce::ComboBox keySelector;
	juce::ComboBox durationSelector;
	juce::Label titleLabel;
	juce::Label helpLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleAndDurationPanel)
};

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
	void updateFromProcessor();
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
	std::unique_ptr<ScaleAndDurationPanel> scaleAndDurationPanel;

	static constexpr int SCALE_AND_DURATION_HEIGHT = 86;

	juce::Label titleLabel;
	juce::Label helpLabel;

	juce::TextEditor searchInput;
	juce::ComboBox sortMenu;

	IconButton addCategoryButton{"add-cat", "New Category"};
	IconButton addPromptButton{"add-prompt", "New Prompt"};
	IconButton expandAllButton{"expand-all"};
	IconButton collapseAllButton{"collapse-all"};

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