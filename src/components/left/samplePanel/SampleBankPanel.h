#pragma once
#include "DetailPanel.h"
#include "ObsidianAccordion.h"
#include "ObsidianBankHeader.h"
#include "ObsidianBase.h"
#include "SampleBank.h"
#include <JuceHeader.h>
#include <set>
#include <vector>

class DjIaVstProcessor;

class SampleBankPanel : public ObsidianComponent, private juce::Timer
{
  public:
	enum SortType
	{
		Time = 1,
		Prompt = 2,
		Usage = 3,
		BPM = 4,
		Duration = 5
	};

	SampleBankPanel(DjIaVstProcessor &processor);
	~SampleBankPanel() override;

	std::function<void(const juce::String &sampleId, const juce::String &trackId)> onSampleDroppedToTrack;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void setVisible(bool v) override;

	void selectEntry(SampleBankEntry *entry);
	void playPreview(SampleBankEntry *entry);
	void stopPreview();

	void refreshSampleList();
	void refreshSampleListSilent();

	juce::var saveUIState() const;
	void restoreUIState(const juce::var &state);

  private:
	void timerCallback() override;

	void applyFiltersAndSort();
	void rebuildAccordions();
	void onAccordionExpanded(const juce::String &categoryName, bool expanded);

	void setupUI();
	void onSampleClicked(SampleBankEntry *entry);
	void onSampleDeleteRequested(SampleBankEntry *entry);
	void showEditPromptDialog(SampleBankEntry *entry);
	void showChangeCategoryDialog(SampleBankEntry *entry);
	void showDeleteConfirmation(const juce::String &id, const juce::String &name);
	void deleteSample(const juce::String &id);
	void cleanupUnusedSamples();

	void ensureAccordionItemsCreated(ObsidianAccordion *accordion, const juce::String &categoryName);

	void expandAll();
	void collapseAll();
	juce::Colour resolveCategoryColour(const juce::String &name) const;

	void drawEmptyState(juce::Graphics &g);

	DjIaVstProcessor &audioProcessor;

	ObsidianBankHeader header;
	juce::Viewport accordionViewport;
	juce::Component accordionContainer;
	std::vector<std::unique_ptr<ObsidianAccordion>> accordions;
	std::map<juce::String, std::vector<SampleBankEntry *>> samplesByCategory;

	DetailPanel detailPanel;

	SortType currentSortType{Time};
	juce::String currentSearch;
	std::vector<SampleBankEntry *> filteredSamples;

	SampleBankEntry *selectedEntry{nullptr};
	SampleBankEntry *currentPreviewEntry{nullptr};

	std::set<juce::String> openCategories;

	std::atomic<bool> hasEverLoaded{false};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankPanel)
};