#include "OnboardingStep.h"
#include "ColourPalette.h"
#include "Fonts.h"
#include "Sizes.h"

OnboardingStep::OnboardingStep(const OnboardingStepData &dataIn, OnboardingVariant variantIn)
    : data(dataIn), variant(variantIn)
{
	addAndMakeVisible(headlineLabel);
	headlineLabel.setText(data.headline, juce::dontSendNotification);
	headlineLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(ObsidianSizes::TEXT_XL));
	headlineLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	headlineLabel.setJustificationType(juce::Justification::topLeft);

	addAndMakeVisible(leadLabel);
	leadLabel.setText(data.getLeadForVariant(variant), juce::dontSendNotification);
	leadLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));
	leadLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	leadLabel.setJustificationType(juce::Justification::topLeft);

	for (const auto &row : data.rows)
	{
		auto rowComp = std::make_unique<ConceptRowComponent>(row);
		addAndMakeVisible(*rowComp);
		rowComponents.push_back(std::move(rowComp));
	}
}

OnboardingStep::~OnboardingStep() = default;

void OnboardingStep::resized()
{
	auto bounds = getLocalBounds().reduced(24, 20);
	const int width = bounds.getWidth();

	headlineLabel.setBounds(bounds.removeFromTop(28));
	bounds.removeFromTop(ObsidianSizes::GAP);

	juce::Font leadFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));
	juce::AttributedString attr;
	attr.append(leadLabel.getText(), leadFont);
	attr.setWordWrap(juce::AttributedString::byWord);
	juce::TextLayout layout;
	layout.createLayout(attr, (float)width);
	const int leadHeight = (int)std::ceil(layout.getHeight());

	leadLabel.setBounds(bounds.removeFromTop(leadHeight));
	bounds.removeFromTop(ObsidianSizes::GAP_XL);

	for (auto &rowComp : rowComponents)
	{
		const int rowHeight = rowComp->getPreferredHeight(width);
		rowComp->setBounds(bounds.removeFromTop(rowHeight));
		bounds.removeFromTop(ObsidianSizes::GAP_4);
	}
}

int OnboardingStep::getPreferredHeight(int width) const
{
	const int paddingH = 24;
	const int paddingV = 20;

	const int innerWidth = juce::jmax(0, width - 2 * paddingH);

	int total = paddingV;

	total += 28 + ObsidianSizes::GAP;

	if (data.lead.isNotEmpty() || data.getLeadForVariant(variant).isNotEmpty())
	{
		juce::Font leadFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));
		juce::AttributedString attr;
		attr.append(data.getLeadForVariant(variant), leadFont);
		attr.setWordWrap(juce::AttributedString::byWord);

		juce::TextLayout layout;
		layout.createLayout(attr, (float)innerWidth);

		total += (int)std::ceil(layout.getHeight()) + ObsidianSizes::GAP_XL;
	}

	for (const auto &rowComp : rowComponents)
	{
		total += rowComp->getPreferredHeight(innerWidth);
		total += ObsidianSizes::GAP_4;
	}

	if (!rowComponents.empty())
		total -= ObsidianSizes::GAP_4;

	total += paddingV;

	return total;
}