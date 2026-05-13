#pragma once
#include "DetailPanel.h"
#include "ObsidianBase.h"
#include "SampleBank.h"
#include "SampleBankItem.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

enum class SampleCategory
{
	All = 0,
	Drums,
	Bass,
	Melody,
	Ambient,
	Percussion,
	Vocal,
	FX,
	Loop,
	OneShot,
	House,
	Techno,
	HipHop,
	Jazz,
	Rock,
	Electronic,
	Piano,
	Guitar,
	Synth,
	Custom
};

struct CategoryInfo
{
	int id;
	juce::String name;
	juce::Colour colour{juce::Colour(0)};
};

class SampleBankPanel : public ObsidianComponent, public juce::Timer, public juce::ListBoxModel
{
  public:
	SampleBankPanel(DjIaVstProcessor &processor);
	~SampleBankPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void timerCallback() override;
	void refreshSampleList();
	void refreshSampleListSilent();
	void setVisible(bool shouldBeVisible) override;

	int getNumRows() override;
	void paintListBoxItem(int, juce::Graphics &, int, int, bool) override
	{
	}
	juce::Component *refreshComponentForRow(int rowNumber, bool, juce::Component *existing) override;
	void listBoxItemClicked(int, const juce::MouseEvent &) override
	{
	}

	std::function<void(const juce::String &, const juce::String &)> onSampleDroppedToTrack;

  private:
	DjIaVstProcessor &audioProcessor;

	juce::Label titleLabel;
	IconButtonSimple cleanupButton{"Cleanup", "Clean unused samples"};
	juce::Label infoLabel;
	juce::ComboBox sortMenu;

	juce::ComboBox categoryFilter;

	EscapableTextEditor categoryInput;
	IconButtonSimple addCategoryButton{"AddCategory", ""};
	IconButtonSimple editCategoryButton{"EditCategory", ""};
	IconButtonSimple deleteCategoryButton{"DeleteCategory", ""};

	juce::ListBox sampleListBox;

	DetailPanel detailPanel;

	SampleBankEntry *selectedEntry = nullptr;
	SampleBankEntry *currentPreviewEntry = nullptr;

	std::atomic<bool> isLoading{false};
	std::atomic<bool> hasEverLoaded{false};
	float loadingAngle = 0.0f;

	int currentCategoryId = 0;
	std::vector<CategoryInfo> categoryInfos = {{0, "All Samples"}, {1, "Drums"},      {2, "Bass"},   {3, "Melody"},
	                                           {4, "Ambient"},     {5, "Percussion"}, {6, "Vocal"},  {7, "FX"},
	                                           {8, "Loops"},       {9, "One-shots"},  {10, "House"}, {11, "Techno"},
	                                           {12, "Hip-Hop"},    {13, "Jazz"},      {14, "Rock"},  {15, "Electronic"},
	                                           {16, "Piano"},      {17, "Guitar"},    {18, "Synth"}};

	SampleCategory currentCategory = SampleCategory::All;
	std::map<SampleCategory, juce::String> categoryNames;

	enum SortType
	{
		Time = 1,
		Prompt = 2,
		Usage = 3,
		BPM = 4,
		Duration = 5
	};
	SortType currentSortType = SortType::Prompt;

	std::vector<SampleBankEntry *> filteredSamples;

	void setupUI();
	void applyFiltersAndSort();

	void selectEntry(SampleBankEntry *entry);
	void playPreview(SampleBankEntry *entry);
	void stopPreview();

	void deleteSample(const juce::String &sampleId);
	void cleanupUnusedSamples();
	void showDeleteConfirmation(const juce::String &sampleId, const juce::String &name);

	void addCategory();
	void editCategory();
	void deleteCategory();
	bool isCategoryEditable(int id) const;
	void rebuildCategoryFilter();
	void saveCategoriesConfig();
	void loadCategoriesConfig();
	int getNextCategoryId();
	void showAddCategoryDialog();
	void showEditCategoryDialog();

	juce::Colour resolveCategoryColour(const juce::String &name) const;

	void showEditPromptDialog(SampleBankEntry *entry);

	void drawLoader(juce::Graphics &g);
	void drawEmptyState(juce::Graphics &g);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankPanel)
};

class SampleBankItemWrapper : public juce::Component
{
  public:
	SampleBankItemWrapper(SampleBankItem *itemToOwn) : item(itemToOwn)
	{
		addAndMakeVisible(item.get());
	}

	void resized() override
	{
		auto b = getLocalBounds();
		item->setBounds(b);
	}
	SampleBankItem *getItem() const
	{
		return item.get();
	}

  private:
	std::unique_ptr<SampleBankItem> item;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankItemWrapper)
};