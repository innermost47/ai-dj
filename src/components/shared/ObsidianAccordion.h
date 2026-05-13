#pragma once
#include "AccordionItem.h"
#include "ObsidianBase.h"

class ObsidianAccordion : public ObsidianComponent, private juce::TextEditor::Listener
{
  public:
	ObsidianAccordion(const juce::String &name, juce::Colour accentColour);
	~ObsidianAccordion() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent &e) override;
	void mouseDoubleClick(const juce::MouseEvent &e) override;

	void setItems(std::vector<std::unique_ptr<AccordionItem>> &&newItems);

	const std::vector<std::unique_ptr<AccordionItem>> &getItems() const
	{
		return items;
	}

	void setExpanded(bool shouldBeExpanded, bool sendNotification = true);
	bool isExpanded() const
	{
		return expanded;
	}

	void setEditable(bool editable);
	bool getEditable() const
	{
		return isEditable;
	}
	void setShowCount(bool v)
	{
		showCount = v;
	}

	void setAccentColour(juce::Colour newColour);
	juce::Colour getAccentColour() const
	{
		return accentColour;
	}

	void setName(const juce::String &newName);
	juce::String getName() const
	{
		return accordionName;
	}

	int getPreferredHeight() const;

	std::function<void(bool)> onExpansionChanged;

	std::function<void()> onEditRequested;

	std::function<void()> onDeleteRequested;

	std::function<void(const juce::String &)> onRenameRequested;

  private:
	void showContextMenu();
	void startInlineRename();
	void finishInlineRename(bool acceptChanges);

	void textEditorReturnKeyPressed(juce::TextEditor &) override;
	void textEditorEscapeKeyPressed(juce::TextEditor &) override;
	void textEditorFocusLost(juce::TextEditor &) override;

	juce::Rectangle<int> getHeaderBounds() const;

	juce::String accordionName;
	juce::Colour accentColour;
	std::vector<std::unique_ptr<AccordionItem>> items;
	std::unique_ptr<juce::TextEditor> renameEditor;

	bool expanded{false};
	bool isEditable{true};
	bool showCount{true};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianAccordion)
};