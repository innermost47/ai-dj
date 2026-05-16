#pragma once
#include "ObsidianBase.h"

class ObsidianBankHeader : public ObsidianComponent
{
  public:
	ObsidianBankHeader();
	~ObsidianBankHeader() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void setTitle(const juce::String &title);
	void setHelpText(const juce::String &help);

	void setSearchEnabled(bool enabled);
	void setSearchPlaceholder(const juce::String &placeholder);
	void setExpanded(bool expanded);
	void setChildNum(int num)
	{
		childNum = num;
	}

	void setSortOptions(const std::vector<std::pair<int, juce::String>> &options, int initialSelectedId = -1);

	void addPrimaryButton(const juce::String &label, juce::Colour backgroundColour, std::function<void()> onClick);

	void clearPrimaryButtons();

	void setShowExpandCollapseButtons(bool show);

	int getPreferredHeight() const;

	std::function<void(const juce::String &)> onSearchChanged;
	std::function<void(int)> onSortChanged;
	std::function<void()> onExpandAllRequested;
	std::function<void()> onCollapseAllRequested;

	juce::String getSearchText() const;
	int getSelectedSortId() const;

	void setSearchText(const juce::String &text, bool notify = false);
	void setSelectedSortId(int id, bool notify = false);

  private:
	struct PrimaryButton
	{
		std::unique_ptr<juce::TextButton> button;
		std::function<void()> onClick;
	};

	juce::Label titleLabel;
	juce::Label helpLabel;
	EscapableTextEditor searchInput;
	juce::ComboBox sortMenu;
	std::vector<PrimaryButton> primaryButtons;
	IconButton expandCollapseButton{"expand-collapse", ""};

	bool searchEnabled{false};
	bool hasSortOptions{false};
	bool showExpandCollapse{false};
	bool isExpanded{false};

	int childNum = 0;

	void rebuildLayout();
	void updateExpandCollapseIconButton();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianBankHeader)
};