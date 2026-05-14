#pragma once
#include "BasePanel.h"
#include "DetailPanel.h"
#include "ObsidianAccordion.h"
#include "SampleBank.h"
#include <JuceHeader.h>
#include <set>
#include <vector>

class DjIaVstProcessor;

class SampleBankPanel : public BasePanel, private juce::Timer
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

	static constexpr SortType firstSort = Time;
	static constexpr SortType lastSort = Duration;

	std::function<void(const juce::String &sampleId, const juce::String &trackId)> onSampleDroppedToTrack;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void setVisible(bool v) override;

	void selectEntry(SampleBankEntry *entry);
	void playPreview(SampleBankEntry *entry);
	void stopPreview();

	void refreshSampleList();
	void refreshSampleListSilent();

	int getSortType()
	{
		return static_cast<int>(currentSortType);
	}

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

	void addCategoryDialog();

	void deleteCategoryDialog(const juce::String &categoryName);
	void editCategoryDialog(const juce::String &categoryName);

	void drawEmptyState(juce::Graphics &g);

	std::map<juce::String, std::vector<SampleBankEntry *>> samplesByCategory;

	DetailPanel detailPanel;

	SortType currentSortType{Time};

	std::vector<SampleBankEntry *> filteredSamples;

	SampleBankEntry *selectedEntry{nullptr};
	SampleBankEntry *currentPreviewEntry{nullptr};

	std::atomic<bool> hasEverLoaded{false};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankPanel)
};