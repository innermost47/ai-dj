#pragma once
#include "ObsidianBase.h"
#include "PromptBankItem.h"
#include <JuceHeader.h>

class PromptCategoryAccordion : public ObsidianComponent
{
  public:
	PromptCategoryAccordion(const juce::String &categoryName, juce::Colour categoryColour);
	~PromptCategoryAccordion() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent &) override;

	void setItems(std::vector<std::unique_ptr<PromptBankItem>> &&items);
	void setExpanded(bool expanded, bool sendNotification = true);
	bool isExpanded() const
	{
		return expanded;
	}
	const juce::String &getCategoryName() const
	{
		return categoryName;
	}
	int getCount() const
	{
		return (int)items.size();
	}

	int getPreferredHeight() const;

	void setEditable(bool editable);

	std::function<void()> onEditRequested;
	std::function<void()> onDeleteRequested;

	std::function<void(bool)> onExpansionChanged;

	static constexpr int HEADER_HEIGHT = 32;
	static constexpr int ITEM_SPACING = 2;

  private:
	juce::String categoryName;
	juce::Colour categoryColour;
	bool expanded = false;
	std::vector<std::unique_ptr<PromptBankItem>> items;
	bool isEditable = false;
	juce::Rectangle<int> editButtonBounds;
	juce::Rectangle<int> deleteButtonBounds;
	IconButton editButton{"cat-edit"};
	IconButton deleteButton{"cat-delete"};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptCategoryAccordion)
};