#include "PromptCategoryAccordion.h"

PromptCategoryAccordion::PromptCategoryAccordion(const juce::String &name, juce::Colour colour)
    : categoryName(name), categoryColour(colour)
{
	setSize(100, HEADER_HEIGHT);

	editButton.loadIcon(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	editButton.setColour(juce::TextButton::buttonColourId, ColourPalette::amber);
	editButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	editButton.setCompactMode(true);
	editButton.setTooltip("Edit category");
	editButton.onClick = [this]()
	{
		if (onEditRequested)
			onEditRequested();
	};

	deleteButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	deleteButton.setCompactMode(true);
	deleteButton.setTooltip("Delete category");
	deleteButton.onClick = [this]()
	{
		if (onDeleteRequested)
			onDeleteRequested();
	};

	addChildComponent(editButton);
	addChildComponent(deleteButton);
}

PromptCategoryAccordion::~PromptCategoryAccordion() = default;

void PromptCategoryAccordion::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();
	auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);

	g.setColour(ColourPalette::backgroundDeep.brighter(0.05f));
	g.fillRoundedRectangle(headerBounds.toFloat(), 4.0f);

	g.setColour(categoryColour);
	g.fillRect(0, headerBounds.getY(), 4, headerBounds.getHeight());

	auto chevronArea = headerBounds.removeFromRight(24);

	if (isEditable)
	{
		const int btnSize = 20;
		headerBounds.removeFromRight(btnSize + 4);
		headerBounds.removeFromRight(btnSize + 4);
	}

	juce::Path chevron;
	auto cBounds = chevronArea.toFloat().reduced(8, 12);
	if (expanded)
	{
		chevron.startNewSubPath(cBounds.getX(), cBounds.getY());
		chevron.lineTo(cBounds.getCentreX(), cBounds.getBottom());
		chevron.lineTo(cBounds.getRight(), cBounds.getY());
	}
	else
	{
		chevron.startNewSubPath(cBounds.getX(), cBounds.getY());
		chevron.lineTo(cBounds.getRight(), cBounds.getCentreY());
		chevron.lineTo(cBounds.getX(), cBounds.getBottom());
	}
	g.setColour(ColourPalette::textSecondary);
	g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	auto textArea = headerBounds.withTrimmedLeft(12);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
	g.drawText(categoryName, textArea, juce::Justification::centredLeft, true);

	g.setColour(ColourPalette::textSecondary.withAlpha(0.7f));
	g.setFont(juce::FontOptions(11.0f));
	g.drawText("(" + juce::String((int)items.size()) + ")", textArea, juce::Justification::centredRight, true);
}

void PromptCategoryAccordion::setEditable(bool editable)
{
	isEditable = editable;
	editButton.setVisible(editable);
	deleteButton.setVisible(editable);
	resized();
	repaint();
}

void PromptCategoryAccordion::resized()
{
	auto headerBounds = getLocalBounds().withHeight(HEADER_HEIGHT);

	headerBounds.removeFromRight(24);

	if (isEditable)
	{
		const int btnSize = 20;
		deleteButton.setBounds(headerBounds.removeFromRight(btnSize).withSizeKeepingCentre(btnSize, btnSize));
		editButton.setBounds(headerBounds.removeFromRight(btnSize + 4).withSizeKeepingCentre(btnSize, btnSize));
	}

	if (!expanded)
		return;

	int y = HEADER_HEIGHT + 2;
	for (auto &item : items)
	{
		int h = item->getPreferredHeight(getWidth());
		item->setBounds(0, y, getWidth(), h);
		y += h + ITEM_SPACING;
	}
}

void PromptCategoryAccordion::mouseDown(const juce::MouseEvent &e)
{
	if (e.y > HEADER_HEIGHT)
		return;
	setExpanded(!expanded);
}

void PromptCategoryAccordion::setItems(std::vector<std::unique_ptr<PromptBankItem>> &&newItems)
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

void PromptCategoryAccordion::setExpanded(bool e, bool sendNotification)
{
	if (expanded == e)
		return;
	expanded = e;
	for (auto &item : items)
		item->setVisible(expanded);

	setSize(getWidth(), getPreferredHeight());
	resized();
	repaint();

	if (sendNotification && onExpansionChanged)
		onExpansionChanged(expanded);
}

int PromptCategoryAccordion::getPreferredHeight() const
{
	if (!expanded || items.empty())
		return HEADER_HEIGHT;
	int total = HEADER_HEIGHT + 2;
	for (auto &item : items)
		total += item->getPreferredHeight(getWidth()) + ITEM_SPACING;
	return total;
}
