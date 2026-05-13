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

	g.setColour(ColourPalette::backgroundDeep.brighter(0.05f));
	g.fillRoundedRectangle(headerBounds.toFloat(), ObsidianSizes::LIST_PANEL_CORNER_SIZE);

	g.setColour(accentColour);
	g.fillRect(0, headerBounds.getY(), ObsidianSizes::ACCORDION_ACCENT_BAR_WIDTH, headerBounds.getHeight());

	auto chevronArea = headerBounds.removeFromRight(ObsidianSizes::ACCORDION_CHEVRON_AREA_WIDTH);

	auto cBounds = chevronArea.toFloat().reduced(8, 12);
	if (expanded)
	{
		auto chevronUpSvg = juce::Drawable::createFromImageData(BinaryData::caretup_svg, BinaryData::caretup_svgSize);
		if (chevronUpSvg != nullptr)
		{
			chevronUpSvg->replaceColour(juce::Colours::black, ColourPalette::textPrimary);
			chevronUpSvg->drawWithin(g, cBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
		}
	}
	else
	{
		auto chevronDownSvg =
		    juce::Drawable::createFromImageData(BinaryData::caretdown_svg, BinaryData::caretdown_svgSize);
		if (chevronDownSvg != nullptr)
		{
			chevronDownSvg->replaceColour(juce::Colours::black, ColourPalette::textPrimary);
			chevronDownSvg->drawWithin(g, cBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
		}
	}

	if (renameEditor != nullptr)
		return;

	auto textArea = headerBounds.withTrimmedLeft(ObsidianSizes::ACCORDION_TEXT_LEFT_PADDING);

	if (!isEditable)
	{
		auto lockArea = textArea.removeFromLeft(ObsidianSizes::ACCORDION_LOCK_AREA_WIDTH);
		auto lockBounds = lockArea.withSizeKeepingCentre(ObsidianSizes::ACCORDION_LOCK_ICON_SIZE,
		                                                 ObsidianSizes::ACCORDION_LOCK_ICON_SIZE);

		auto lockSvg = juce::Drawable::createFromImageData(BinaryData::lockfill_svg, BinaryData::lockfill_svgSize);
		if (lockSvg != nullptr)
		{
			lockSvg->replaceColour(juce::Colours::black, ColourPalette::textSecondary);
			lockSvg->drawWithin(g, lockBounds.toFloat(), juce::RectanglePlacement::xLeft, ObsidianShades::ALPHA_04);
		}
	}
	textArea.removeFromLeft(ObsidianSizes::SPACER_SM);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
	g.drawText(accordionName, textArea, juce::Justification::centredLeft, true);

	if (showCount)
	{
		g.setColour(ColourPalette::textSecondary.withAlpha(0.7f));
		g.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO));
		g.drawText("(" + juce::String((int)items.size()) + ")", textArea, juce::Justification::centredRight, true);
	}
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

void ObsidianAccordion::mouseDown(const juce::MouseEvent &e)
{
	if (e.y > ObsidianSizes::ACCORDION_HEADER_HEIGHT)
		return;

	if (renameEditor != nullptr)
		return;

	if (e.mods.isPopupMenu())
	{
		if (isEditable)
			showContextMenu();
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

void ObsidianAccordion::showContextMenu()
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

	juce::WeakReference<juce::Component> weakSelf(this);
	menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
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

void ObsidianAccordion::setEditable(bool editable)
{
	if (isEditable == editable)
		return;

	isEditable = editable;

	if (!editable && renameEditor != nullptr)
		finishInlineRename(false);

	repaint();
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