#include "TrackSelectorBar.h"
#include "PluginProcessor.h"

TrackSelectorBar::TrackSelectorBar(DjIaVstProcessor &processor, bool includeMaster)
    : audioProcessor(processor), includeMasterButton(includeMaster)
{
	refresh();
}

void TrackSelectorBar::refresh()
{
	removeAllChildren();
	buttons.clear();

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
		btn->setLabelText(label);
		btn->setName("trackSelector" + trackId);
		btn->setShowBackground(true);
		btn->setClickingTogglesState(true);
		btn->setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(Obsidian::ALPHA_02));
		btn->setColour(juce::TextButton::buttonOnColourId, modelColour);
		btn->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		btn->setColour(juce::TextButton::textColourOnId, textColour);

		auto *raw = btn.get();
		btn->onClick = [this, trackId, raw]()
		{
			if (raw->getToggleState() && onTrackSelected)
				onTrackSelected(trackId);
		};

		addAndMakeVisible(*btn);
		buttons.push_back(std::move(btn));

		if (track->isSelected.load())
			buttons.back()->setToggleState(true, juce::dontSendNotification);

		index++;
	}

	if (includeMasterButton)
	{
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

		auto *raw = btn.get();
		btn->onClick = [this, raw]()
		{
			if (raw->getToggleState() && onMasterSelected)
				onMasterSelected();
		};

		addAndMakeVisible(*btn);
		buttons.push_back(std::move(btn));
	}

	resized();
}

void TrackSelectorBar::resized()
{
	juce::FlexBox selectors;
	selectors.flexDirection = juce::FlexBox::Direction::row;
	selectors.justifyContent = juce::FlexBox::JustifyContent::center;
	selectors.alignContent = juce::FlexBox::AlignContent::center;

	for (auto &btn : buttons)
		if (btn->isVisible())
			selectors.items.add(juce::FlexItem(*btn).withFlex(1.f).withMargin(juce::FlexItem::Margin(1.f)));

	selectors.performLayout(getLocalBounds());
}

void TrackSelectorBar::selectTrack(const juce::String &trackId)
{
	for (auto &btn : buttons)
		btn->setToggleState(btn->getName() == "trackSelector" + trackId, juce::dontSendNotification);
}

void TrackSelectorBar::selectMaster()
{
	for (auto &btn : buttons)
		btn->setToggleState(btn->getButtonText() == "M", juce::dontSendNotification);
}

void TrackSelectorBar::updateModelColour(const juce::String &trackId)
{
	auto *track = audioProcessor.getTrack(trackId);
	if (!track)
		return;
	for (auto &btn : buttons)
	{
		if (btn->getName() == "trackSelector" + trackId)
		{
			auto &currentPage = track->getCurrentPage();
			auto modelColour = AiModelDefinitions::getColourForModel(currentPage.selectedModel);
			bool darkText = modelColour.getBrightness() > 0.6f;
			auto textColour = darkText ? juce::Colours::black : juce::Colours::white;
			btn->setColour(juce::TextButton::buttonColourId, modelColour.withAlpha(Obsidian::ALPHA_02));
			btn->setColour(juce::TextButton::buttonOnColourId, modelColour);
			btn->setColour(juce::TextButton::textColourOnId, textColour);
		}
	}
}

void TrackSelectorBar::setMasterButtonVisible(bool shouldBeVisible)
{
	if (!includeMasterButton || buttons.empty())
		return;

	auto &masterBtn = buttons.back();
	if (masterBtn->isVisible() == shouldBeVisible)
		return;

	masterBtn->setVisible(shouldBeVisible);
	resized();
}