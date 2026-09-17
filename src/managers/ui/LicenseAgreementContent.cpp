#include "LicenseAgreementContent.h"

LicenseAgreementContent::LicenseAgreementContent(const std::vector<LicenseEntry> &licenses) : licenses_(licenses)
{
	introLbl_.setText("Please read and accept the following license agreements before proceeding.",
	                  juce::dontSendNotification);
	introLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	introLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
	introLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(introLbl_);

	for (const auto &entry : licenses_)
	{
		auto *titleLbl = titleLabels_.add(std::make_unique<juce::Label>());
		titleLbl->setText(entry.title, juce::dontSendNotification);
		titleLbl->setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		titleLbl->setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
		addAndMakeVisible(titleLbl);

		auto *summaryLbl = summaryLabels_.add(std::make_unique<juce::Label>());
		summaryLbl->setText(entry.summary, juce::dontSendNotification);
		summaryLbl->setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		summaryLbl->setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
		summaryLbl->setJustificationType(juce::Justification::topLeft);
		addAndMakeVisible(summaryLbl);

		auto *linkBtn = linkButtons_.add(std::make_unique<juce::TextButton>());
		linkBtn->setButtonText("Read full license");
		linkBtn->setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
		linkBtn->setColour(juce::TextButton::textColourOffId, ColourPalette::slate);
		juce::String url = entry.url;
		linkBtn->onClick = [url]() { juce::URL(url).launchInDefaultBrowser(); };
		addAndMakeVisible(linkBtn);

		auto *toggleBtn = checkboxes_.add(std::make_unique<juce::TextButton>());
		toggleBtn->setButtonText("I have read and agree to the " + entry.title);
		toggleBtn->setClickingTogglesState(true);
		toggleBtn->setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
		toggleBtn->setColour(juce::TextButton::buttonOnColourId, ColourPalette::slate.withAlpha(0.3f));
		toggleBtn->setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		toggleBtn->setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
		toggleBtn->onStateChange = [this]()
		{
			if (onAcceptanceChanged)
				onAcceptanceChanged(allAccepted());
		};
		addAndMakeVisible(toggleBtn);
	}
}

bool LicenseAgreementContent::allAccepted() const
{
	for (auto *cb : checkboxes_)
		if (!cb->getToggleState())
			return false;
	return true;
}

void LicenseAgreementContent::resized()
{
	auto area = getLocalBounds().reduced(Obsidian::PADDING);

	introLbl_.setBounds(area.removeFromTop(32));
	area.removeFromTop(Obsidian::GAP_4);

	for (int i = 0; i < (int)licenses_.size(); ++i)
	{
		if (i > 0)
		{
			auto sep = area.removeFromTop(1);
			separatorRects_.resize((size_t)i);
			separatorRects_[i - 1] = sep;
			area.removeFromTop(Obsidian::GAP_4);
		}

		titleLabels_[i]->setBounds(area.removeFromTop(20));
		area.removeFromTop(4);

		summaryLabels_[i]->setBounds(area.removeFromTop(52));
		area.removeFromTop(4);

		linkButtons_[i]->setBounds(area.removeFromTop(24).removeFromLeft(180));
		area.removeFromTop(8);

		checkboxes_[i]->setBounds(area.removeFromTop(24));
		area.removeFromTop(Obsidian::GAP_4 * 2);
	}
}

void LicenseAgreementContent::paint(juce::Graphics &g)
{
	for (const auto &r : separatorRects_)
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.4f));
		g.fillRect(r);
	}
}