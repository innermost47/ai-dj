#pragma once
#include "ObsidianBase.h"
#include "PromptBank.h"
#include <JuceHeader.h>

class PromptBankItem : public ObsidianComponent
{
  public:
	PromptBankItem(PromptBankEntry *entry);
	~PromptBankItem() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void mouseEnter(const juce::MouseEvent &) override;
	void mouseExit(const juce::MouseEvent &) override;
	void mouseDown(const juce::MouseEvent &event) override;
	void mouseDrag(const juce::MouseEvent &event) override;
	void mouseUp(const juce::MouseEvent &) override;

	void setSelected(bool s)
	{
		selected = s;
		repaint();
	}
	bool isSelected() const
	{
		return selected;
	}
	PromptBankEntry *getPromptEntry() const
	{
		return entry;
	}

	std::function<void(PromptBankEntry *)> onItemClicked;
	std::function<void(PromptBankEntry *)> onEditRequested;
	std::function<void(PromptBankEntry *)> onDeleteRequested;
	std::function<juce::Colour(const juce::String &)> categoryColourResolver;

	static constexpr int MIN_HEIGHT = 50;
	static constexpr int MAX_LINES = 4;

	int getPreferredHeight(int width) const;

  private:
	PromptBankEntry *entry = nullptr;
	bool selected = false;
	bool isDragging = false;

	IconButton editButton{"prompt-edit"};
	IconButton deleteButton{"prompt-delete"};

	juce::String getElidedText(const juce::String &text, juce::Font &font, float maxWidth, int maxLines);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBankItem)
};