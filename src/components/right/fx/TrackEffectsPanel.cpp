#include "TrackEffectsPanel.h"
#include "CompressorComponent.h"
#include "DistortionComponent.h"
#include "EqualizerComponent.h"
#include "FilterComponent.h"
#include "LimiterComponent.h"
#include "PluginProcessor.h"

TrackEffectsPanel::TrackEffectsPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	setupUI();
}

TrackEffectsPanel::~TrackEffectsPanel() = default;

void TrackEffectsPanel::refresh()
{
	removeAllChildren();
	trackSelectors.clear();
	resetComponents();
	setupUI();
	resized();
}

void TrackEffectsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_04));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::LIST_PANEL_CORNER_SIZE, 1.0f);

	auto titleArea = getLocalBounds().reduced(8, 4).removeFromTop(18);
	g.setColour(ColourPalette::textAccent);
	g.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_INFO));
	g.drawText("EFFECTS", titleArea, juce::Justification::centredLeft, false);
}

void TrackEffectsPanel::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	area.removeFromTop(24);

	auto selectorsArea = area.removeFromTop(26);
	area.removeFromTop(Obsidian::GAP_4);
	auto distortionArea = area.removeFromTop(82);
	area.removeFromTop(Obsidian::GAP_4);
	auto eqArea = area.removeFromTop(130);
	area.removeFromTop(Obsidian::GAP_4);
	auto filterArea = area.removeFromTop(60);
	area.removeFromTop(Obsidian::GAP_4);
	auto compressorArea = area.removeFromTop(70);
	area.removeFromTop(Obsidian::GAP_4);
	auto limiterArea = area.removeFromTop(50);

	juce::FlexBox selectors;
	selectors.flexDirection = juce::FlexBox::Direction::row;
	selectors.justifyContent = juce::FlexBox::JustifyContent::center;
	selectors.alignContent = juce::FlexBox::AlignContent::center;

	for (int i = 0; i < (int)trackSelectors.size(); ++i)
	{
		selectors.items.add(juce::FlexItem(*trackSelectors[i]).withFlex(1.f).withMargin(juce::FlexItem::Margin(1.f)));
	}

	selectors.performLayout(selectorsArea);

	if (distortionComponent)
		distortionComponent->setBounds(distortionArea);
	if (equalizerComponent)
		equalizerComponent->setBounds(eqArea);
	if (filterComponent)
		filterComponent->setBounds(filterArea);
	if (compressorComponent)
		compressorComponent->setBounds(compressorArea);
	if (limiterComponent)
		limiterComponent->setBounds(limiterArea);
}

void TrackEffectsPanel::updateModelUI(const juce::String &trackId)
{
	if (distortionComponent)
		distortionComponent->updateModelUI();
	if (filterComponent)
		filterComponent->updateModelUI();
	if (compressorComponent)
		compressorComponent->updateModelUI();
	if (equalizerComponent)
		equalizerComponent->updateModelUI();
	if (limiterComponent)
		limiterComponent->updateModelUI();

	for (auto &selector : trackSelectors)
	{
		if (selector->getName() == "trackFXSelector" + trackId)
		{
			auto *track = audioProcessor.getTrack(trackId);
			if (!track)
				continue;
			auto &currentPage = track->getCurrentPage();
			auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
			bool darkText = modelColour.getBrightness() > 0.6f;
			auto textColour = darkText ? juce::Colours::black : juce::Colours::white;
			selector->setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(Obsidian::ALPHA_02));
			selector->setColour(juce::TextButton::buttonOnColourId, modelColour);
		}
	}
}

void TrackEffectsPanel::setupUI()
{
	int index = 1;
	auto trackIds = audioProcessor.getAllTrackIds();
	int selectedValue = 1;
	for (const auto &trackId : trackIds)
	{
		auto *track = audioProcessor.getTrack(trackId);
		if (!track)
			continue;
		auto &currentPage = track->getCurrentPage();
		auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
		bool darkText = modelColour.getBrightness() > 0.6f;
		auto textColour = darkText ? juce::Colours::black : juce::Colours::white;

		juce::String label = "T" + juce::String(index);
		auto btn = std::make_unique<IconButtonSimple>(label, "");
		btn->setRadioGroupId(Obsidian::RadioGroupIDs::TrackFXSelector);

		btn->setToggleState(index == selectedValue, juce::dontSendNotification);

		btn->setLabelText(label);
		btn->setName("trackFXSelector" + trackId);
		btn->setShowBackground(true);
		btn->setClickingTogglesState(true);
		btn->setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(Obsidian::ALPHA_02));
		btn->setColour(juce::TextButton::buttonOnColourId, modelColour);
		btn->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		btn->setColour(juce::TextButton::textColourOnId, textColour);

		btn->onClick = [this, trackId]() { addComponents(trackId); };

		addAndMakeVisible(*btn);
		trackSelectors.push_back(std::move(btn));

		if (index == selectedValue)
		{
			addComponents(trackId);
		}

		index++;
	}
	juce::String label = "M";
	auto btn = std::make_unique<IconButtonSimple>(label, "");
	bool darkText = ColourPalette::playArmed.getBrightness() > 0.6f;
	auto textColour = darkText ? juce::Colours::black : juce::Colours::white;
	btn->setRadioGroupId(Obsidian::RadioGroupIDs::TrackFXSelector);
	btn->setToggleState(false, juce::dontSendNotification);
	btn->setLabelText(label);
	btn->setShowBackground(true);
	btn->setClickingTogglesState(true);
	btn->setColour(juce::TextButton::buttonColourId, ColourPalette::playArmed.withAlpha(Obsidian::ALPHA_02));
	btn->setColour(juce::TextButton::buttonOnColourId, ColourPalette::playArmed);
	btn->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	btn->setColour(juce::TextButton::textColourOnId, textColour);
	btn->onClick = [this]()
	{
		resetComponents();
		resized();
	};

	addAndMakeVisible(*btn);
	trackSelectors.push_back(std::move(btn));
}

void TrackEffectsPanel::addComponents(const juce::String &trackId)
{
	if (auto *currentTrack = audioProcessor.getTrack(trackId))
	{
		distortionComponent = std::make_unique<DistortionComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*distortionComponent);
		filterComponent = std::make_unique<FilterComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*filterComponent);
		equalizerComponent = std::make_unique<EqualizerComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*equalizerComponent);
		compressorComponent = std::make_unique<CompressorComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*compressorComponent);
		limiterComponent = std::make_unique<LimiterComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*limiterComponent);
		resized();
	}
}

void TrackEffectsPanel::resetComponents()
{
	distortionComponent = nullptr;
	equalizerComponent = nullptr;
	filterComponent = nullptr;
	compressorComponent = nullptr;
	limiterComponent = nullptr;
}