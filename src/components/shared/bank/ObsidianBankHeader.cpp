#include "ObsidianBankHeader.h"

ObsidianBankHeader::ObsidianBankHeader()
{
	addAndMakeVisible(titleLabel);
	titleLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_TITLE));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);

	addAndMakeVisible(helpLabel);
	helpLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));
	helpLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	helpLabel.setJustificationType(juce::Justification::topLeft);

	addChildComponent(searchInput);
	searchInput.onTextChange = [this]()
	{
		if (onSearchChanged)
			onSearchChanged(searchInput.getText());
	};

	addChildComponent(sortMenu);
	sortMenu.onChange = [this]()
	{
		if (onSortChanged)
			onSortChanged(sortMenu.getSelectedId());
	};

	addChildComponent(expandCollapseButton);
	updateExpandCollapseIconButton();
	expandCollapseButton.setCompactMode(true);
	expandCollapseButton.onClick = [this]()
	{
		if (isExpanded)
		{
			if (onCollapseAllRequested)
				onCollapseAllRequested();
		}
		else
		{
			if (onExpandAllRequested)
				onExpandAllRequested();
		}
		updateExpandCollapseIconButton();
	};
}

ObsidianBankHeader::~ObsidianBankHeader() = default;

void ObsidianBankHeader::setExpanded(bool expanded)
{
	isExpanded = expanded;
	updateExpandCollapseIconButton();
}

void ObsidianBankHeader::updateExpandCollapseIconButton()
{
	if (isExpanded)
	{
		expandCollapseButton.loadIcon(BinaryData::expand_svg, BinaryData::expand_svgSize);
	}
	else
	{
		expandCollapseButton.loadIcon(BinaryData::collapse_svg, BinaryData::collapse_svgSize);
	}
}

void ObsidianBankHeader::setTitle(const juce::String &title)
{
	titleLabel.setText(title, juce::dontSendNotification);
}

void ObsidianBankHeader::setHelpText(const juce::String &help)
{
	helpLabel.setText(help, juce::dontSendNotification);
	resized();
}

void ObsidianBankHeader::setSearchEnabled(bool enabled)
{
	if (searchEnabled == enabled)
		return;
	searchEnabled = enabled;
	searchInput.setVisible(enabled);
	resized();
}

void ObsidianBankHeader::setSearchPlaceholder(const juce::String &placeholder)
{
	searchInput.setTextToShowWhenEmpty(placeholder, ColourPalette::textSecondary);
}

void ObsidianBankHeader::setSortOptions(const std::vector<std::pair<int, juce::String>> &options, int initialSelectedId)
{
	sortMenu.clear(juce::dontSendNotification);
	for (const auto &opt : options)
		sortMenu.addItem(opt.second, opt.first);

	hasSortOptions = !options.empty();
	sortMenu.setVisible(hasSortOptions);

	if (hasSortOptions)
	{
		const int idToSelect = (initialSelectedId > 0) ? initialSelectedId : options.front().first;
		sortMenu.setSelectedId(idToSelect, juce::dontSendNotification);
	}

	resized();
}

void ObsidianBankHeader::addPrimaryButton(const juce::String &label, juce::Colour backgroundColour,
                                          std::function<void()> onClick)
{
	PrimaryButton pb;
	pb.button = std::make_unique<juce::TextButton>(label);
	pb.onClick = std::move(onClick);

	pb.button->setColour(juce::TextButton::buttonColourId, backgroundColour);
	pb.button->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);

	auto cb = pb.onClick;
	pb.button->onClick = [cb]()
	{
		if (cb)
			cb();
	};

	addAndMakeVisible(*pb.button);
	primaryButtons.push_back(std::move(pb));
	resized();
}

void ObsidianBankHeader::clearPrimaryButtons()
{
	for (auto &pb : primaryButtons)
		removeChildComponent(pb.button.get());
	primaryButtons.clear();
	resized();
}

void ObsidianBankHeader::setShowExpandCollapseButtons(bool show)
{
	if (showExpandCollapse == show)
		return;
	showExpandCollapse = show;
	expandCollapseButton.setVisible(show);
	resized();
}

juce::String ObsidianBankHeader::getSearchText() const
{
	return searchInput.getText();
}

int ObsidianBankHeader::getSelectedSortId() const
{
	return sortMenu.getSelectedId();
}

void ObsidianBankHeader::setSearchText(const juce::String &text, bool notify)
{
	searchInput.setText(text, notify ? juce::sendNotification : juce::dontSendNotification);
}

void ObsidianBankHeader::setSelectedSortId(int id, bool notify)
{
	sortMenu.setSelectedId(id, notify ? juce::sendNotification : juce::dontSendNotification);
}

int ObsidianBankHeader::getPreferredHeight() const
{
	int total = 0;

	total += ObsidianSizes::TITLE_PANEL_HEIGHT;
	total += ObsidianSizes::GAP_4;

	if (helpLabel.getText().isNotEmpty())
	{
		total += ObsidianSizes::INFO_PANEL_HEIGHT * 2;
	}

	if (searchEnabled)
	{
		total += ObsidianSizes::COMBO_BOX_BASE_HEIGHT;
		total += ObsidianSizes::GAP_4;
	}

	if (hasSortOptions)
	{
		total += ObsidianSizes::COMBO_BOX_BASE_HEIGHT;
		total += ObsidianSizes::GAP_4;
	}

	if (!primaryButtons.empty())
	{
		total += ObsidianSizes::COMBO_BOX_BASE_HEIGHT;
		total += ObsidianSizes::GAP_4;
	}

	return total;
}

void ObsidianBankHeader::paint(juce::Graphics &)
{
}

void ObsidianBankHeader::resized()
{
	auto area = getLocalBounds();

	titleLabel.setBounds(area.removeFromTop(ObsidianSizes::TITLE_PANEL_HEIGHT));
	area.removeFromTop(ObsidianSizes::GAP_4);

	if (helpLabel.getText().isNotEmpty())
	{
		helpLabel.setBounds(area.removeFromTop(ObsidianSizes::INFO_PANEL_HEIGHT * 2));
	}

	if (searchEnabled)
	{
		searchInput.setBounds(area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT));
		area.removeFromTop(ObsidianSizes::GAP_4);
	}

	if (hasSortOptions)
	{
		sortMenu.setBounds(area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT));
		area.removeFromTop(ObsidianSizes::GAP_4);
	}

	if (!primaryButtons.empty())
	{
		auto btnRow = area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT);
		const int n = (int)primaryButtons.size();
		const int totalSpacing = ObsidianSizes::GAP_4 * (n - 1);
		int btnW = (btnRow.getWidth() - totalSpacing) / n;
		const int expandW = 32;
		if (showExpandCollapse)
		{
			btnW = btnW - expandW / n;
		}

		for (int i = 0; i < n; ++i)
		{
			auto thisBtn = btnRow.removeFromLeft(btnW);
			primaryButtons[i].button->setBounds(thisBtn);
			if (i < n - 1)
				btnRow.removeFromLeft(ObsidianSizes::GAP_4);
		}
		if (showExpandCollapse)
		{
			btnRow.removeFromLeft(ObsidianSizes::GAP_4);
			expandCollapseButton.setBounds(btnRow.removeFromLeft(expandW));
		}
		area.removeFromTop(ObsidianSizes::GAP_4);
	}
}