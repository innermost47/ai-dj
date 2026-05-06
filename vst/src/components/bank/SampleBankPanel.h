#pragma once
#include "ColourPalette.h"
#include "IconButton.h"
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

class DetailPanel : public juce::Component, public juce::Timer
{
  public:
	DetailPanel();
	~DetailPanel() override;

	void setEntry(SampleBankEntry *entry);
	void paint(juce::Graphics &g) override;
	void resized() override;
	void setIsPlaying(bool playing);
	void updatePlaybackPosition(float pos);

	std::function<void(SampleBankEntry *)> onPlayRequested;
	std::function<void()> onStopRequested;
	std::function<void(const juce::String &)> onDeleteRequested;
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

	void loadAudio();

  private:
	SampleBankEntry *entry = nullptr;

	juce::Label nameLabel;
	juce::Label metaLabel;
	IconButtonSimple playButton{"Play", ""};
	IconButtonSimple deleteButton{"Delete", ""};

	juce::Rectangle<int> waveformBounds;
	std::vector<float> thumbL, thumbR;
	juce::AudioBuffer<float> audioBuf;
	std::shared_ptr<std::atomic<bool>> validity{std::make_shared<std::atomic<bool>>(true)};
	std::atomic<bool> destroyed{false};

	bool isPlaying = false;
	float playbackPos = 0.0f;
	double lastTimerCall = 0.0;

	void timerCallback() override;
	void generateThumbnail();
	void drawWaveform(juce::Graphics &g);
	void updatePlayButton();

	juce::String formatDuration(float s);
	juce::Colour getCategoryColor(const juce::String &category);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetailPanel)
};

class SampleBankPanel : public juce::Component, public juce::Timer, public juce::ListBoxModel
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
	IconButtonSimple cleanupButton{"Cleanup", ""};
	juce::Label infoLabel;
	juce::ComboBox sortMenu;

	juce::ComboBox categoryFilter;
	juce::TextEditor categoryInput;
	IconButtonSimple addCategoryButton{"AddCategory", ""};
	IconButtonSimple editCategoryButton{"EditCategory", ""};
	IconButtonSimple deleteCategoryButton{"DeleteCategory", ""};

	juce::ListBox sampleListBox;
	static constexpr int ROW_HEIGHT = 50;

	DetailPanel detailPanel;
	static constexpr int DETAIL_HEIGHT = 86;

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