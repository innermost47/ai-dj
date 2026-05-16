#include "ObsidianAccordion.h"

ObsidianAccordion::ObsidianAccordion(const juce::String &name, juce::Colour colour)
    : accordionName(name), accentColour(colour)
{
	setSize(100, ObsidianSizes::ACCORDION_HEADER_HEIGHT);
}

ObsidianAccordion::~ObsidianAccordion() = default;

juce::Rectangle<int> ObsidianAccordion::getHeaderBounds() const
{
	return getLocalBounds().withHeight(ObsidianSizes::ACCORDION_HEADER_HEIGHT);
}

void ObsidianAccordion::paint(juce::Graphics &g)
{
	auto headerBounds = getHeaderBounds();

	headerBounds.removeFromLeft(6);
	auto circleArea = headerBounds.removeFromLeft(10);
	auto circleRect = circleArea.withSizeKeepingCentre(6, 6).toFloat();

	g.setColour(accentColour);
	g.fillEllipse(circleRect);

	if (renameEditor != nullptr)
		return;

	auto textArea = headerBounds.withTrimmedLeft(ObsidianSizes::ACCORDION_TEXT_LEFT_PADDING);

	auto folderArea = textArea.removeFromLeft(ObsidianSizes::ACCORDION_FOLDER_AREA_WIDTH);

	auto folderBounds = folderArea.withSizeKeepingCentre(ObsidianSizes::ACCORDION_FOLDER_ICON_SIZE,
	                                                     ObsidianSizes::ACCORDION_FOLDER_ICON_SIZE);
	if (expanded)
	{
		auto folderOpenSvg =
		    juce::Drawable::createFromImageData(BinaryData::folderopen_svg, BinaryData::folderopen_svgSize);
		if (folderOpenSvg != nullptr)
		{
			folderOpenSvg->replaceColour(juce::Colours::black, ColourPalette::textPrimary);
			folderOpenSvg->drawWithin(g, folderBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
		}
	}
	else
	{
		auto folderClosedSvg =
		    juce::Drawable::createFromImageData(BinaryData::folderclosed_svg, BinaryData::folderclosed_svgSize);
		if (folderClosedSvg != nullptr)
		{
			folderClosedSvg->replaceColour(juce::Colours::black, ColourPalette::textPrimary);
			folderClosedSvg->drawWithin(g, folderBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
		}
	}
	textArea.removeFromLeft(ObsidianSizes::SPACER_SM);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
	g.drawText(accordionName, textArea, juce::Justification::centredLeft, true);
}

void ObsidianAccordion::resized()
{
	if (renameEditor != nullptr)
	{
		auto editorBounds = getHeaderBounds()
		                        .withTrimmedLeft(ObsidianSizes::ACCORDION_TEXT_LEFT_PADDING)
		                        .withTrimmedRight(ObsidianSizes::ACCORDION_CHEVRON_AREA_WIDTH);
		renameEditor->setBounds(editorBounds.reduced(0, 4));
	}

	if (!expanded)
		return;

	int y = ObsidianSizes::ACCORDION_HEADER_HEIGHT + ObsidianSizes::ACCORDION_ITEM_SPACING;
	for (auto &item : items)
	{
		int h = item->getPreferredHeight(getWidth());
		item->setBounds(0, y, getWidth(), h);
		y += h + ObsidianSizes::ACCORDION_ITEM_SPACING;
	}
}

void ObsidianAccordion::mouseEnter(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void ObsidianAccordion::mouseExit(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ObsidianAccordion::mouseDown(const juce::MouseEvent &e)
{
	if (e.y > ObsidianSizes::ACCORDION_HEADER_HEIGHT)
		return;

	if (renameEditor != nullptr)
		return;

	if (e.mods.isPopupMenu())
	{
		if (isEditable)
			showContextMenu(e);
		return;
	}

	setExpanded(!expanded);
}

void ObsidianAccordion::mouseDoubleClick(const juce::MouseEvent &e)
{
	if (e.y > ObsidianSizes::ACCORDION_HEADER_HEIGHT)
		return;
	if (!isEditable || onRenameRequested == nullptr)
		return;

	startInlineRename();
}

void ObsidianAccordion::showContextMenu(const juce::MouseEvent &e)
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
		                   auto *self = dynamic_cast<ObsidianAccordion *>(weakSelf.get());
		                   if (self == nullptr)
			                   return;

		                   switch (result)
		                   {
		                   case MenuRename:
			                   self->startInlineRename();
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

void ObsidianAccordion::startInlineRename()
{
	if (renameEditor != nullptr)
		return;

	renameEditor = std::make_unique<juce::TextEditor>();
	renameEditor->setText(accordionName, juce::dontSendNotification);
	renameEditor->setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
	renameEditor->setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDeep);
	renameEditor->setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
	renameEditor->setColour(juce::TextEditor::outlineColourId, accentColour.withAlpha(0.5f));
	renameEditor->setColour(juce::TextEditor::focusedOutlineColourId, accentColour);
	renameEditor->setColour(juce::TextEditor::highlightColourId, accentColour.withAlpha(0.3f));
	renameEditor->setColour(juce::CaretComponent::caretColourId, ColourPalette::textPrimary);
	renameEditor->setIndents(4, 2);
	renameEditor->setBorder(juce::BorderSize<int>(1));
	renameEditor->setSelectAllWhenFocused(true);
	renameEditor->addListener(this);

	addAndMakeVisible(*renameEditor);
	resized();
	renameEditor->grabKeyboardFocus();
	repaint();
}

void ObsidianAccordion::finishInlineRename(bool acceptChanges)
{
	if (renameEditor == nullptr)
		return;

	const auto newName = renameEditor->getText().trim();

	renameEditor->removeListener(this);
	removeChildComponent(renameEditor.get());
	renameEditor.reset();

	if (acceptChanges && newName.isNotEmpty() && newName != accordionName)
	{
		if (onRenameRequested)
			onRenameRequested(newName);
	}

	repaint();
}

void ObsidianAccordion::textEditorReturnKeyPressed(juce::TextEditor &)
{
	finishInlineRename(true);
}

void ObsidianAccordion::textEditorEscapeKeyPressed(juce::TextEditor &)
{
	finishInlineRename(false);
}

void ObsidianAccordion::textEditorFocusLost(juce::TextEditor &)
{
	finishInlineRename(true);
}

void ObsidianAccordion::setItems(std::vector<std::unique_ptr<AccordionItem>> &&newItems)
{
	for (auto &item : items)
		removeChildComponent(item.get());

	items = std::move(newItems);

	for (auto &item : items)
	{
		addChildComponent(*item);
		item->setVisible(expanded);
	}

	setSize(getWidth(), getPreferredHeight());
	resized();
	repaint();
}

void ObsidianAccordion::setExpanded(bool shouldBeExpanded, bool sendNotification)
{
	if (expanded == shouldBeExpanded)
		return;

	expanded = shouldBeExpanded;
	for (auto &item : items)
		item->setVisible(expanded);

	setSize(getWidth(), getPreferredHeight());
	resized();
	repaint();

	if (sendNotification && onExpansionChanged)
		onExpansionChanged(expanded);
}

void ObsidianAccordion::setAccentColour(juce::Colour newColour)
{
	accentColour = newColour;
	repaint();
}

void ObsidianAccordion::setName(const juce::String &newName)
{
	accordionName = newName;
	repaint();
}

int ObsidianAccordion::getPreferredHeight() const
{
	if (!expanded || items.empty())
		return ObsidianSizes::ACCORDION_HEADER_HEIGHT;

	int total = ObsidianSizes::ACCORDION_HEADER_HEIGHT + ObsidianSizes::ACCORDION_ITEM_SPACING;
	for (auto &item : items)
		total += item->getPreferredHeight(getWidth()) + ObsidianSizes::ACCORDION_ITEM_SPACING;
	return total;
}