#include "ConceptRowComponent.h"
#include "ColourPalette.h"
#include "Fonts.h"
#include "Sizes.h"

ConceptRowComponent::ConceptRowComponent(const ConceptRow &row) : data(row)
{
	icon = loadIconByName(data.iconName);

	addAndMakeVisible(titleLabel);
	titleLabel.setText(data.title, juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(ObsidianSizes::TEXT_REGULAR));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	titleLabel.setJustificationType(juce::Justification::centredLeft);
}

ConceptRowComponent::~ConceptRowComponent() = default;

std::unique_ptr<juce::Drawable> ConceptRowComponent::loadIconByName(const juce::String &name)
{

	auto load = [](const char *svgData, int svgSize) -> std::unique_ptr<juce::Drawable>
	{
		if (svgData == nullptr || svgSize == 0)
			return nullptr;
		auto drawable = juce::Drawable::createFromImageData(svgData, (size_t)svgSize);
		if (drawable != nullptr)
			drawable->replaceColour(juce::Colours::black, ColourPalette::textSecondary);
		return drawable;
	};

	if (name == "lightning")
		return load(BinaryData::zap_svg, BinaryData::zap_svgSize);
	if (name == "disk")
		return load(BinaryData::save_svg, BinaryData::save_svgSize);
	if (name == "play")
		return load(BinaryData::play_svg, BinaryData::play_svgSize);
	if (name == "headphones")
		return load(BinaryData::headphones_svg, BinaryData::headphones_svgSize);
	if (name == "waveform")
		return load(BinaryData::waveform_svg, BinaryData::waveform_svgSize);
	if (name == "grid")
		return load(BinaryData::grid_svg, BinaryData::grid_svgSize);
	if (name == "search")
		return load(BinaryData::search_svg, BinaryData::search_svgSize);
	if (name == "folder")
		return load(BinaryData::folderclosed_svg, BinaryData::folderclosed_svgSize);
	if (name == "plus")
		return load(BinaryData::plus_svg, BinaryData::plus_svgSize);
	if (name == "pencil")
		return load(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	if (name == "chat")
		return load(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
	if (name == "sliders")
		return load(BinaryData::sliders_svg, BinaryData::sliders_svgSize);
	if (name == "map")
		return load(BinaryData::map_svg, BinaryData::map_svgSize);
	if (name == "repeat")
		return load(BinaryData::repeat_svg, BinaryData::repeat_svgSize);
	if (name == "dice")
		return load(BinaryData::dice_svg, BinaryData::dice_svgSize);
	if (name == "dragndrop")
		return load(BinaryData::handgrabbing_svg, BinaryData::handgrabbing_svgSize);
	if (name == "export")
		return load(BinaryData::export_svg, BinaryData::export_svgSize);

	return nullptr;
}

int ConceptRowComponent::getPreferredHeight(int width) const
{
	const int paddingH = 12;
	const int paddingV = 10;

	const int innerWidth = width - 2 * paddingH;
	const int textX = ICON_BOX_SIZE + LEFT_GAP;
	const int textWidth = juce::jmax(0, innerWidth - textX);

	juce::Font bodyFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));

	juce::AttributedString attr;
	attr.append(data.body, bodyFont);
	attr.setWordWrap(juce::AttributedString::byWord);
	attr.setLineSpacing(2.0f);

	juce::TextLayout layout;
	layout.createLayout(attr, (float)textWidth);

	const int bodyHeight = (int)std::ceil(layout.getHeight());
	const int contentHeight = juce::jmax(ICON_BOX_SIZE, TITLE_HEIGHT + TITLE_BODY_GAP + bodyHeight);

	return contentHeight + 2 * paddingV;
}

void ConceptRowComponent::paint(juce::Graphics &g)
{
	auto fullBounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.4f));
	g.fillRoundedRectangle(fullBounds, ObsidianSizes::CORNER);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawRoundedRectangle(fullBounds, ObsidianSizes::CORNER, 1.0f);

	auto contentBounds = getLocalBounds().reduced(12, 10);
	auto iconBox = contentBounds.removeFromLeft(ICON_BOX_SIZE).withHeight(ICON_BOX_SIZE);

	g.setColour(ColourPalette::backgroundLight);
	g.fillRoundedRectangle(iconBox.toFloat(), 8.0f);

	if (icon != nullptr)
	{
		auto iconArea = iconBox.reduced(ICON_INSET);
		icon->drawWithin(g, iconArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
	}

	auto bounds = getLocalBounds();

	const int textX = 12 + ICON_BOX_SIZE + LEFT_GAP;
	auto bodyArea = juce::Rectangle<int>(textX, 10 + TITLE_HEIGHT + TITLE_BODY_GAP, getWidth() - textX - 12,
	                                     getHeight() - 10 - TITLE_HEIGHT - TITLE_BODY_GAP - 10);

	juce::Font bodyFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));

	juce::AttributedString attr;
	attr.append(data.body, bodyFont, ColourPalette::textSecondary);
	attr.setWordWrap(juce::AttributedString::byWord);
	attr.setLineSpacing(2.0f);
	attr.setJustification(juce::Justification::topLeft);

	attr.draw(g, bodyArea.toFloat());
}

void ConceptRowComponent::resized()
{
	auto bounds = getLocalBounds().reduced(12, 10);
	bounds.removeFromLeft(ICON_BOX_SIZE + LEFT_GAP);

	auto titleArea = bounds.removeFromTop(TITLE_HEIGHT);
	titleLabel.setBounds(titleArea);
}