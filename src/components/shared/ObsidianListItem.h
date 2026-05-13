#pragma once
#include "ObsidianBase.h"

class ObsidianListItem : public virtual ObsidianComponent
{
  public:
	ObsidianListItem();
	~ObsidianListItem() override;

	void setSelected(bool shouldBeSelected);
	bool isSelected() const
	{
		return selected;
	}

	void setEditable(bool editable);
	bool getEditable() const
	{
		return isEditable;
	}

	void setDraggable(bool draggable)
	{
		isDraggable = draggable;
	}
	bool getDraggable() const
	{
		return isDraggable;
	}

	std::function<void()> onItemClicked;

	std::function<void()> onItemDoubleClicked;

	std::function<void()> onEditRequested;

	std::function<void()> onDeleteRequested;

	std::function<void()> onRenameRequested;

	std::function<juce::String()> dragPayloadProvider;

	std::function<void()> onBuildContextMenu;

  protected:
	void mouseEnter(const juce::MouseEvent &) override;
	void mouseExit(const juce::MouseEvent &) override;
	void mouseDown(const juce::MouseEvent &e) override;
	void mouseDrag(const juce::MouseEvent &e) override;
	void mouseUp(const juce::MouseEvent &) override;

	virtual void selectionChanged()
	{
		repaint();
	}

  private:
	void showDefaultContextMenu();

	bool selected{false};
	bool isEditable{true};
	bool isDraggable{true};
	bool isDragging{false};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianListItem)
};