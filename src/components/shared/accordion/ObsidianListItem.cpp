#include "ObsidianListItem.h"

ObsidianListItem::ObsidianListItem() = default;
ObsidianListItem::~ObsidianListItem() = default;

void ObsidianListItem::setSelected(bool shouldBeSelected)
{
	if (selected == shouldBeSelected)
		return;
	selected = shouldBeSelected;
	selectionChanged();
}

void ObsidianListItem::setEditable(bool editable)
{
	isEditable = editable;
}

void ObsidianListItem::mouseEnter(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ObsidianListItem::mouseExit(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ObsidianListItem::mouseDown(const juce::MouseEvent &e)
{
	if (e.mods.isPopupMenu())
	{
		if (onBuildContextMenu)
			onBuildContextMenu(e);
		else if (isEditable)
			showDefaultContextMenu(e);
		return;
	}

	if (e.getNumberOfClicks() == 2)
	{
		if (onItemClicked)
			onItemClicked();
		if (onItemDoubleClicked)
			onItemDoubleClicked();
		return;
	}

	if (onItemClicked)
		onItemClicked();
}

void ObsidianListItem::mouseDrag(const juce::MouseEvent &e)
{
	if (!isDraggable || isDragging || dragPayloadProvider == nullptr)
		return;
	if (e.getDistanceFromDragStart() < 6)
		return;

	auto payload = dragPayloadProvider();
	if (payload.isEmpty())
		return;

	isDragging = true;
	if (auto *dc = juce::DragAndDropContainer::findParentDragContainerFor(this))
		dc->startDragging(payload, this);
}

void ObsidianListItem::mouseUp(const juce::MouseEvent &)
{
	isDragging = false;
}

void ObsidianListItem::showDefaultContextMenu(const juce::MouseEvent &e)
{
	juce::PopupMenu menu;

	enum MenuIds
	{
		MenuRename = 1,
		MenuEdit = 2,
		MenuDelete = 3
	};

	const bool hasRename = (onRenameRequested != nullptr);
	const bool hasEdit = (onEditRequested != nullptr);
	const bool hasDelete = (onDeleteRequested != nullptr);

	if (hasRename)
		menu.addItem(MenuRename, "Rename");
	if (hasEdit)
		menu.addItem(MenuEdit, "Edit");
	if ((hasRename || hasEdit) && hasDelete)
		menu.addSeparator();
	if (hasDelete)
		menu.addItem(MenuDelete, "Delete");

	if (menu.getNumItems() == 0)
		return;

	auto screenPos = e.getScreenPosition();
	auto screenArea = juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1);

	juce::WeakReference<juce::Component> weakSelf(this);
	menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screenArea),
	                   [weakSelf](int result)
	                   {
		                   if (weakSelf == nullptr)
			                   return;
		                   auto *self = dynamic_cast<ObsidianListItem *>(weakSelf.get());
		                   if (self == nullptr)
			                   return;

		                   switch (result)
		                   {
		                   case MenuRename:
			                   if (self->onRenameRequested)
				                   self->onRenameRequested();
			                   break;
		                   case MenuEdit:
			                   if (self->onEditRequested)
				                   self->onEditRequested();
			                   break;
		                   case MenuDelete:
			                   if (self->onDeleteRequested)
				                   self->onDeleteRequested();
			                   break;
		                   default:
			                   break;
		                   }
	                   });
}

float ObsidianListItem::measureTextWidth(const juce::Font &font, const juce::String &text) const
{
	return juce::GlyphArrangement::getStringWidth(font, text);
}

juce::StringArray ObsidianListItem::truncateToLines(const juce::Font &font, const juce::String &text, float maxWidth,
                                                    int maxLines) const
{
	juce::StringArray lines;
	juce::String remaining = text;

	for (int i = 0; i < maxLines && remaining.isNotEmpty(); ++i)
	{
		if (measureTextWidth(font, remaining) <= maxWidth)
		{
			lines.add(remaining);
			remaining.clear();
			break;
		}

		bool isLastLine = (i == maxLines - 1);
		int cut = remaining.length();

		while (cut > 0 && measureTextWidth(font, remaining.substring(0, cut) + (isLastLine ? "..." : "")) > maxWidth)
			--cut;

		if (!isLastLine)
		{
			int lastSpace = remaining.substring(0, cut).lastIndexOfChar(' ');
			if (lastSpace > 0)
				cut = lastSpace;
		}

		lines.add(remaining.substring(0, cut) + (isLastLine ? "..." : ""));
		remaining = remaining.substring(cut).trimStart();
	}

	return lines;
}

int ObsidianListItem::getPreferredHeight(int /*width*/) const
{
	return getBaseHeight();
}