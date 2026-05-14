#pragma once
#include "ObsidianAccordion.h"
#include "ObsidianBankHeader.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class BasePanel : public ObsidianComponent
{
  public:
	BasePanel(DjIaVstProcessor &processor);
	~BasePanel() override = default;

	void transferOpenCategoryState(const juce::String &oldName, const juce::String &newName);
	void resized() override;
	void expandAll(
	    std::function<void(ObsidianAccordion *accordion, const juce::String &categoryName)> callback = nullptr);
	void collapseAll();

	juce::var saveUIState(int sortType) const;
	void restoreUIState(const juce::var &state, std::function<void()> refreshCallback, int min, int max);

	juce::Colour resolveCategoryColour(const juce::String &name) const;

	void drawEmptyState(juce::Graphics &g, juce::Drawable &iconSvg, juce::String &noItemYet, juce::String &tip,
	                    juce::String noMatch);
	void drawEmptyBank(juce::Graphics &g, juce::Drawable &iconSvg, juce::String &noItemYet, juce::String &tip);
	void drawNoSearchResults(juce::Graphics &g, juce::String &noMatch);

	ObsidianBankHeader header;
	juce::Viewport accordionViewport;
	juce::Component accordionContainer;

	std::set<juce::String> openCategories;
	std::vector<std::unique_ptr<ObsidianAccordion>> accordions;

	juce::String currentSearch;

	bool isExpanded{false};

	DjIaVstProcessor &audioProcessor;
};