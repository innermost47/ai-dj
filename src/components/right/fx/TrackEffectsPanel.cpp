#include "TrackEffectsPanel.h"
#include "FilterComponent.h"
#include "PluginProcessor.h"

TrackEffectsPanel::TrackEffectsPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	setupUI();
}

void TrackEffectsPanel::refresh()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;
		auto it = std::find(trackIds.begin(), trackIds.end(), trackId);
		int idx = static_cast<int>(it - trackIds.begin());
		filterComponents[idx]->setTrackData(trackData);
		filterComponents[idx]->wireParameters();
	}
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
	auto filterArea = area.removeFromTop(60);

	juce::FlexBox selectors;
	selectors.flexDirection = juce::FlexBox::Direction::row;
	selectors.justifyContent = juce::FlexBox::JustifyContent::center;
	selectors.alignContent = juce::FlexBox::AlignContent::center;

	for (int i = 0; i < (int)trackSelectors.size(); ++i)
	{
		selectors.items.add(juce::FlexItem(*trackSelectors[i]).withFlex(1.f).withMargin(juce::FlexItem::Margin(1.f)));
	}

	selectors.performLayout(selectorsArea);
	if (filterComponent)
		filterComponent->setBounds(filterArea);
}

void TrackEffectsPanel::updateModelUI(const juce::String &trackId)
{
	for (auto &fc : filterComponents)
	{
		if (fc->getTrackId() == trackId)
		{
			fc->updateModelUI();
			break;
		}
	}
}

void TrackEffectsPanel::setupUI()
{
	int index = 1;
	auto trackIds = audioProcessor.getAllTrackIds();

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

		int currentValue = 1;
		btn->setToggleState(index == currentValue, juce::dontSendNotification);

		btn->setLabelText(label);
		btn->setShowBackground(true);
		btn->setClickingTogglesState(true);
		btn->setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(Obsidian::ALPHA_02));
		btn->setColour(juce::TextButton::buttonOnColourId, modelColour);
		btn->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		btn->setColour(juce::TextButton::textColourOnId, textColour);

		btn->onClick = [this, trackId]()
		{
			if (auto *currentTrack = audioProcessor.getTrack(trackId))
			{
				filterComponent = std::make_unique<FilterComponent>(audioProcessor, currentTrack);
				addAndMakeVisible(*filterComponent);
				resized();
			}
		};

		addAndMakeVisible(*btn);
		trackSelectors.push_back(std::move(btn));

		filterComponent = std::make_unique<FilterComponent>(audioProcessor, track);
		addAndMakeVisible(*filterComponent);
		resized();

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
		filterComponent = nullptr;
		resized();
	};

	addAndMakeVisible(*btn);
	trackSelectors.push_back(std::move(btn));
}