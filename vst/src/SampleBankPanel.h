#pragma once
#include "JuceHeader.h"
#include "SampleBank.h"
#include "ColourPalette.h"

class DjIaVstProcessor;

class SampleBankItem : public juce::Component, public juce::DragAndDropContainer, public juce::Timer
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
	void setIsPlaying(bool playing);
	void loadAudioDataIfNeeded();
	void showCategoryMenu();
	int getRequiredHeight();

	SampleBankEntry *getSampleEntry() const { return sampleEntry; }

	std::function<void(const juce::String &)> onDeleteRequested;
	std::function<void(SampleBankEntry *)> onPreviewRequested;
	std::function<void()> onStopRequested;
	std::function<void(SampleBankEntry *, const std::vector<juce::String> &)> onCategoriesChanged;

	std::function<std::vector<juce::String>()> getCategoriesList;

private:
	SampleBankEntry *sampleEntry;
	DjIaVstProcessor &audioProcessor;

	juce::Label nameLabel;
	juce::Label durationLabel;
	juce::Label bpmLabel;
	juce::Label usageLabel;
	juce::TextButton playButton;
	juce::TextButton deleteButton;

	juce::Rectangle<int> waveformBounds;
	std::vector<float> thumbnail;
	juce::AudioBuffer<float> audioBuffer;
	std::shared_ptr<std::atomic<bool>> validityFlag;
	std::atomic<bool> isDestroyed{false};

	int maxVisibleBadges = 0;
	double sampleRate = 48000.0;
	float playbackPosition = 0.0f;
	double lastTimerCall = 0.0;

	bool isPlaying = false;

	bool isSelected = false;
	bool isDragging = false;

	void updateLabels();
	juce::String formatDuration(float seconds);
	juce::String formatUsage();
	void updatePlayButton();
	void generateThumbnail();
	void loadAudioData();
	void drawMiniWaveform(juce::Graphics &g);
	void setPlaybackPosition(float positionInSeconds);
	void timerCallback() override;
	void drawCategoryBadges(juce::Graphics &g);
	juce::Colour getCategoryColor(const juce::String &category);
	void updateBadgeLayout();

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
};

class SampleBankPanel : public juce::Component,
						public juce::Timer
{
public:
	SampleBankPanel(DjIaVstProcessor &processor);
	~SampleBankPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void timerCallback() override;
	void refreshSampleList();
	void setVisible(bool shouldBeVisible) override;

	std::function<void(const juce::String &, const juce::String &)> onSampleDroppedToTrack;

private:
	DjIaVstProcessor &audioProcessor;

	juce::Label titleLabel;
	juce::TextButton cleanupButton;
	juce::Viewport samplesViewport;
	juce::Component samplesContainer;
	juce::Label infoLabel;
	juce::ComboBox sortMenu;

	juce::TextEditor categoryInput;
	juce::TextButton addCategoryButton;
	juce::TextButton editCategoryButton;
	juce::TextButton deleteCategoryButton;

	int currentCategoryId = 0;
	std::vector<CategoryInfo> categoryInfos = {
		{0, "All Samples"},
		{1, "Drums"},
		{2, "Bass"},
		{3, "Melody"},
		{4, "Ambient"},
		{5, "Percussion"},
		{6, "Vocal"},
		{7, "FX"},
		{8, "Loops"},
		{9, "One-shots"},
		{10, "House"},
		{11, "Techno"},
		{12, "Hip-Hop"},
		{13, "Jazz"},
		{14, "Rock"},
		{15, "Electronic"},
		{16, "Piano"},
		{17, "Guitar"},
		{18, "Synth"}};

	juce::ComboBox categoryFilter;
	SampleCategory currentCategory = SampleCategory::All;
	std::map<SampleCategory, juce::String> categoryNames;
	bool isCategoryEditable(int categoryId) const;
	void rebuildCategoryFilter();

	enum SortType
	{
		Time = 1,
		Prompt = 2,
		Usage = 3,
		BPM = 4,
		Duration = 5
	};
	SortType currentSortType = SortType::Prompt;

	std::vector<std::unique_ptr<SampleBankItem>> sampleItems;

	SampleBankEntry *currentPreviewEntry = nullptr;
	SampleBankItem *currentPreviewItem = nullptr;

	void setupUI();
	void createSampleItems(const std::vector<SampleBankEntry *> &samples);
	void playPreview(SampleBankEntry *entry);
	void stopPreview();
	void deleteSample(const juce::String &sampleId);
	void cleanupUnusedSamples();
	void showDeleteConfirmation(const juce::String &sampleId, const juce::String &sampleName);
	void addCategory();
	void editCategory();
	void deleteCategory();
	void saveCategoriesConfig();
	void loadCategoriesConfig();
	int getNextCategoryId();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankPanel)
};