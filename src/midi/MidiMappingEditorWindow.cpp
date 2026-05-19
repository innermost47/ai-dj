#include "MidiMappingEditorWindow.h"
#include "BinaryData.h"
#include "ObsidianAlertManager.h"

MidiMappingRow::MidiMappingRow(const MidiMapping &mapping, MidiLearnManager *manager)
    : mapping(mapping), midiLearnManager(manager)
{

	parameterLabel.setText(mapping.parameterName, juce::dontSendNotification);
	parameterLabel.setJustificationType(juce::Justification::centredLeft);
	parameterLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	parameterLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(13.5f));
	addAndMakeVisible(parameterLabel);

	midiInfoLabel.setText(getMidiInfoString(), juce::dontSendNotification);
	midiInfoLabel.setJustificationType(juce::Justification::centredLeft);
	midiInfoLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	midiInfoLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(12.0f));
	addAndMakeVisible(midiInfoLabel);

	deleteButton.loadIcon(BinaryData::trash_svg, BinaryData::trash_svgSize);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	deleteButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	deleteButton.addListener(this);
	addAndMakeVisible(deleteButton);

	learnButton.loadIcon(BinaryData::broadcast_svg, BinaryData::broadcast_svgSize);
	learnButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	learnButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	learnButton.addListener(this);
	addAndMakeVisible(learnButton);
}

MidiMappingRow::~MidiMappingRow()
{
}

void MidiMappingRow::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	const float corner = ObsidianSizes::CORNER;

	bool isEven = (getY() / juce::jmax(1, getHeight())) % 2 == 0;

	auto baseColour = isEven ? ColourPalette::backgroundDark : ColourPalette::backgroundMid.withAlpha(0.4f);

	auto rowBounds = bounds.reduced(4.0f, 3.0f);

	g.setColour(baseColour.brighter(0.02f));
	g.fillRoundedRectangle(rowBounds, corner);

	if (isLearning && blinkState)
	{
		g.setColour(ColourPalette::playArmed.withAlpha(0.25f));
		g.fillRoundedRectangle(rowBounds, corner);
		g.setColour(ColourPalette::playArmed.withAlpha(0.6f));
		g.drawRoundedRectangle(rowBounds, corner, 1.5f);
	}
	else
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawRoundedRectangle(rowBounds, corner, 0.6f);
	}

	auto accentBar =
	    juce::Rectangle<float>(rowBounds.getX() + 4.0f, rowBounds.getY() + 8.0f, 3.0f, rowBounds.getHeight() - 16.0f);

	auto accentColour = isLearning ? ColourPalette::playArmed : ColourPalette::lightGrey;
	g.setColour(accentColour.withAlpha(0.9f));
	g.fillRoundedRectangle(accentBar, 1.5f);

	auto badgeBounds = midiInfoBadgeBounds.toFloat();
	if (!badgeBounds.isEmpty())
	{
		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.18f));
		g.fillRoundedRectangle(badgeBounds, ObsidianSizes::CORNER);
		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.5f));
		g.drawRoundedRectangle(badgeBounds, ObsidianSizes::CORNER, 0.6f);

		g.setColour(ColourPalette::textAccent);
		g.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(10.5f));
		g.drawText(getMidiTypeShort(), badgeBounds, juce::Justification::centred);
	}
}

void MidiMappingRow::resized()
{
	auto bounds = getLocalBounds().reduced(8, 5);
	bounds.removeFromLeft(10);

	auto buttonArea = bounds.removeFromRight(56);
	deleteButton.setBounds(buttonArea.removeFromRight(40).withSizeKeepingCentre(26, 26));
	buttonArea.removeFromRight(ObsidianSizes::SPACER_XS);
	learnButton.setBounds(buttonArea.removeFromRight(40).withSizeKeepingCentre(26, 26));

	bounds.removeFromRight(8);

	const int badgeWidth = 42;
	const int badgeHeight = 18;
	const int midiTextWidth = 110;

	auto midiArea = bounds.removeFromRight(badgeWidth + 8 + midiTextWidth);
	midiInfoBadgeBounds = midiArea.removeFromLeft(badgeWidth).withSizeKeepingCentre(badgeWidth, badgeHeight);
	midiArea.removeFromLeft(8);
	midiInfoLabel.setBounds(midiArea);

	parameterLabel.setBounds(bounds);
}

void MidiMappingRow::buttonClicked(juce::Button *button)
{
	if (button == &deleteButton && onDeleteClicked)
		onDeleteClicked();
	else if (button == &learnButton && onLearnClicked)
	{
		if (midiLearnManager->isLearningActive())
		{
			midiLearnManager->stopLearning();
			setLearningActive(false);
			return;
		}
		onLearnClicked();
	}
}

void MidiMappingRow::setLearningActive(bool active)
{
	isLearning = active;
	if (!active)
	{
		blinkState = false;
		learnButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	}
	repaint();
}

void MidiMappingRow::toggleBlink()
{
	if (isLearning)
	{
		blinkState = !blinkState;
		learnButton.setColour(juce::TextButton::buttonColourId,
		                      blinkState ? ColourPalette::playArmed : ColourPalette::buttonSuccess);
		repaint();
	}
}

void MidiMappingRow::updateMapping(const MidiMapping &newMapping)
{
	mapping = newMapping;
	parameterLabel.setText(mapping.parameterName, juce::dontSendNotification);
	midiInfoLabel.setText(getMidiInfoString(), juce::dontSendNotification);
	repaint();
}

juce::String MidiMappingRow::getMidiInfoString() const
{
	return juce::String(mapping.midiNumber) + " · Ch" + juce::String(mapping.midiChannel + 1);
}

juce::String MidiMappingRow::getMidiTypeShort() const
{
	switch (mapping.midiType)
	{
	case 1:
		return "CC";
	case 0:
		return "NOTE";
	case 2:
		return "PB";
	default:
		return "?";
	}
}

MidiMappingEditorWindow::MidiMappingEditorWindow(MidiLearnManager *manager) : midiLearnManager(manager)
{
	subtitleLabel.setText("Manage mappings or ReLearn to reassign.", juce::dontSendNotification);
	subtitleLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_REGULAR));
	subtitleLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	subtitleLabel.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(subtitleLabel);

	countLabel.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(ObsidianSizes::TEXT_INFO));
	countLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	countLabel.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(countLabel);

	clearAllButton.loadIcon(BinaryData::trash_svg, BinaryData::trash_svgSize);
	clearAllButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	clearAllButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	clearAllButton.setCompactMode(true);
	clearAllButton.addListener(this);
	addAndMakeVisible(clearAllButton);

	reloadDefaultsButton.loadIcon(BinaryData::treestructure_svg, BinaryData::treestructure_svgSize);
	reloadDefaultsButton.setTooltip("Load default MIDI mappings (for OBSIDIAN Neural - Mobile Controller)");
	reloadDefaultsButton.setColour(juce::TextButton::buttonColourId, ColourPalette::indigo);
	reloadDefaultsButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	reloadDefaultsButton.setCompactMode(true);
	reloadDefaultsButton.addListener(this);
	addAndMakeVisible(reloadDefaultsButton);

	mappingsViewport.setViewedComponent(&mappingsContainer, false);
	mappingsViewport.setScrollBarsShown(true, false);
	mappingsViewport.setLookAndFeel(&customLookAndFeel);
	addAndMakeVisible(mappingsViewport);

	refreshMappingsList();
	startTimerHz(2);
}

MidiMappingEditorWindow::~MidiMappingEditorWindow()
{
	stopTimer();
	mappingsViewport.setLookAndFeel(nullptr);
}

void MidiMappingEditorWindow::timerCallback()
{
	if (midiLearnManager->isLearningActive())
	{
		for (auto *row : mappingRows)
			row->toggleBlink();
	}
	else
	{
		for (auto *row : mappingRows)
			row->setLearningActive(false);
	}
}

void MidiMappingEditorWindow::paint(juce::Graphics &g)
{
	auto listF = listBackgroundBounds.toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.4f));
	g.fillRoundedRectangle(listF, ObsidianSizes::CORNER);
	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.4f));
	g.fillRoundedRectangle(listF.reduced(0.5f), ObsidianSizes::CORNER);
}

void MidiMappingEditorWindow::resized()
{
	auto bounds = getLocalBounds();

	headerBounds = bounds.removeFromTop(48);
	auto headerContent = headerBounds;

	int buttonHeight = 26;
	int buttonWidth = 26;
	auto buttonsArea = headerContent.removeFromRight(buttonWidth * 2 + ObsidianSizes::SPACER_XS);

	buttonsArea.removeFromTop(headerContent.getHeight() / 2 - buttonHeight / 2);
	reloadDefaultsButton.setBounds(buttonsArea.removeFromRight(buttonWidth).removeFromTop(buttonHeight));
	buttonsArea.removeFromRight(ObsidianSizes::SPACER_XS);
	clearAllButton.setBounds(buttonsArea.removeFromRight(buttonWidth).removeFromTop(buttonHeight));

	auto subtitleArea = headerContent.removeFromTop(headerContent.getHeight() / 2 + 2);
	subtitleLabel.setBounds(subtitleArea);
	countLabel.setBounds(headerContent);

	bounds.removeFromTop(ObsidianSizes::GAP);

	listBackgroundBounds = bounds;
	mappingsViewport.setBounds(bounds.reduced(2));

	int scrollBarWidth =
	    mappingsViewport.getVerticalScrollBar().isVisible() ? mappingsViewport.getScrollBarThickness() : 0;
	int rowWidth = mappingsViewport.getWidth() - scrollBarWidth;
	int rowHeight = 42;

	mappingsContainer.setSize(rowWidth, mappingRows.size() * rowHeight);

	for (int i = 0; i < mappingRows.size(); ++i)
		mappingRows[i]->setBounds(0, i * rowHeight, rowWidth, rowHeight);
}

void MidiMappingEditorWindow::buttonClicked(juce::Button *button)
{
	if (button == &clearAllButton)
	{
		ObsidianAlertManager::showConfirm(this, "Confirmation", "Are you sure you want to clear all MIDI mappings?",
		                                  "Yes", "No",
		                                  [this](bool confirmed)
		                                  {
			                                  if (confirmed)
			                                  {
				                                  midiLearnManager->clearAllMappings();
				                                  refreshMappingsList();
			                                  }
		                                  });
	}
	else if (button == &reloadDefaultsButton)
	{
		ObsidianAlertManager::showConfirm(
		    this, "Load Default Mappings",
		    "Load default MIDI mappings configured for the OBSIDIAN Neural - Mobile Controller? "
		    "This will replace your current mappings.",
		    "Load", "Cancel",
		    [this](bool confirmed)
		    {
			    if (confirmed)
			    {
				    midiLearnManager->clearAllMappings();
				    midiLearnManager->loadDefaultMappings(midiLearnManager->getProcessor());
				    refreshMappingsList();
			    }
		    });
	}
}

void MidiMappingEditorWindow::refreshMappingsList()
{
	mappingRows.clear();
	mappingsContainer.removeAllChildren();

	auto mappings = midiLearnManager->getAllMappings();

	for (const auto &mapping : mappings)
	{
		auto *row = new MidiMappingRow(mapping, midiLearnManager);
		row->onDeleteClicked = [this, mapping] { deleteMapping(mapping); };
		row->onLearnClicked = [this, mapping] { startLearningForMapping(mapping); };

		mappingRows.add(row);
		mappingsContainer.addAndMakeVisible(row);
	}

	int count = mappingRows.size();
	countLabel.setText(juce::String(count) + (count <= 1 ? " mapping active" : " mappings active"),
	                   juce::dontSendNotification);

	resized();
	repaint();
}

void MidiMappingEditorWindow::deleteMapping(const MidiMapping &mapping)
{
	ObsidianAlertManager::showConfirm(this, "Confirmation", "Delete mapping for \"" + mapping.parameterName + "\"?",
	                                  "Yes", "No",
	                                  [this, mapping](bool confirmed)
	                                  {
		                                  if (confirmed)
		                                  {
			                                  midiLearnManager->removeMapping(mapping.parameterName);
			                                  refreshMappingsList();
		                                  }
	                                  });
}

void MidiMappingEditorWindow::startLearningForMapping(const MidiMapping &mapping)
{
	auto onLearningComplete = [this, paramName = mapping.parameterName](float /* value */)
	{
		juce::MessageManager::callAsync(
		    [this, paramName]()
		    {
			    auto updatedMappings = midiLearnManager->getAllMappings();
			    for (const auto &updated : updatedMappings)
			    {
				    if (updated.parameterName == paramName)
				    {
					    for (auto *row : mappingRows)
					    {
						    if (row->getMapping().parameterName == paramName)
						    {
							    row->updateMapping(updated);
							    row->setLearningActive(false);
							    break;
						    }
					    }
					    break;
				    }
			    }
		    });
	};

	midiLearnManager->startLearning(mapping.parameterName, mapping.processor, onLearningComplete, mapping.description);

	for (auto *row : mappingRows)
		row->setLearningActive(false);
	for (auto *row : mappingRows)
	{
		if (row->getMapping().parameterName == mapping.parameterName)
		{
			row->setLearningActive(true);
			break;
		}
	}
}

void ObsidianAlertManager::showMidiMappingEditor(juce::Component *parent, MidiLearnManager *manager)
{
	auto modal = std::make_unique<ObsidianModalWindow>("MIDI Mappings", 700, 600);
	modal->setContent(std::make_unique<MidiMappingEditorWindow>(manager));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	juce::Component::SafePointer<ObsidianModalOverlay> safeOverlay(overlay);

	overlay->modalWindow->addButton("Close", crossSvg, ColourPalette::buttonInactive,
	                                [safeOverlay]()
	                                {
		                                if (safeOverlay != nullptr)
			                                safeOverlay->close();
	                                });
}