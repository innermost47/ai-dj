#include "MidiMappingEditorWindow.h"
#include "components/ObsidianAlertManager.h"
#include "BinaryData.h"

MidiMappingRow::MidiMappingRow(const MidiMapping &mapping, MidiLearnManager *manager)
	: mapping(mapping), midiLearnManager(manager)
{
	parameterLabel.setText(mapping.parameterName, juce::dontSendNotification);
	parameterLabel.setJustificationType(juce::Justification::centredLeft);
	parameterLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	addAndMakeVisible(parameterLabel);

	midiInfoLabel.setText(getMidiInfoString(), juce::dontSendNotification);
	midiInfoLabel.setJustificationType(juce::Justification::centredLeft);
	midiInfoLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	addAndMakeVisible(midiInfoLabel);

	deleteButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	deleteButton.addListener(this);
	addAndMakeVisible(deleteButton);

	learnButton.loadIcon(BinaryData::broadcast_svg, BinaryData::broadcast_svgSize);
	learnButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	learnButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	learnButton.addListener(this);
	addAndMakeVisible(learnButton);
}

MidiMappingRow::~MidiMappingRow() {}

void MidiMappingRow::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	bool isEven = (getY() / getHeight()) % 2 == 0;
	g.setColour(isEven ? ColourPalette::backgroundDark : ColourPalette::backgroundMid.withAlpha(0.5f));
	g.fillRect(bounds);

	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.6f));
	g.fillRect(0.0f, 4.0f, 3.0f, bounds.getHeight() - 8.0f);

	g.setColour(ColourPalette::trackSelected.withAlpha(0.2f));
	g.drawLine(10.0f, bounds.getBottom() - 0.5f, bounds.getWidth() - 10.0f, bounds.getBottom() - 0.5f, 0.5f);
}

void MidiMappingRow::resized()
{
	auto bounds = getLocalBounds().reduced(5);
	auto buttonArea = bounds.removeFromRight(90);
	deleteButton.setBounds(buttonArea.removeFromRight(40).withSizeKeepingCentre(36, 36));
	learnButton.setBounds(buttonArea.removeFromRight(44).withSizeKeepingCentre(36, 36));

	auto labelArea = bounds;
	parameterLabel.setBounds(labelArea.removeFromLeft(300));
	midiInfoLabel.setBounds(labelArea);
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
		repaint();
	}
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
	juce::String typeStr;
	switch (mapping.midiType)
	{
	case 1:
		typeStr = "CC";
		break;
	case 0:
		typeStr = "Note";
		break;
	case 2:
		typeStr = "Pitch Bend";
		break;
	default:
		typeStr = "Unknown";
		break;
	}
	return typeStr + " " + juce::String(mapping.midiNumber) + " (Ch " + juce::String(mapping.midiChannel + 1) + ")";
}

MidiMappingEditorWindow::MidiMappingEditorWindow(MidiLearnManager *manager)
	: midiLearnManager(manager)
{
	subtitleLabel.setText("Manage custom mappings or click 'ReLearn' to assign a new MIDI control.", juce::dontSendNotification);
	subtitleLabel.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
	subtitleLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	subtitleLabel.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(subtitleLabel);

	clearAllButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	clearAllButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	clearAllButton.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonDanger);
	clearAllButton.addListener(this);
	addAndMakeVisible(clearAllButton);

	reloadDefaultsButton.loadIcon(BinaryData::refresh_svg, BinaryData::refresh_svgSize);
	reloadDefaultsButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	reloadDefaultsButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
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
	g.fillAll(ColourPalette::backgroundDark);

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(headerBounds.toFloat(), 6.0f);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawRoundedRectangle(headerBounds.toFloat(), 6.0f, 1.0f);
}

void MidiMappingEditorWindow::resized()
{
	auto bounds = getLocalBounds().reduced(10);
	headerBounds = bounds.removeFromTop(44);
	auto headerContent = headerBounds.reduced(8, 5);
	reloadDefaultsButton.setBounds(headerContent.removeFromRight(34).withSizeKeepingCentre(34, 34));
	headerContent.removeFromRight(10);
	clearAllButton.setBounds(headerContent.removeFromRight(34).withSizeKeepingCentre(34, 34));

	subtitleLabel.setBounds(headerContent);

	bounds.removeFromTop(10);

	mappingsViewport.setBounds(bounds);

	int scrollBarWidth = mappingsViewport.getVerticalScrollBar().isVisible() ? mappingsViewport.getScrollBarThickness() : 0;
	int rowWidth = mappingsViewport.getWidth() - scrollBarWidth;
	int rowHeight = 50;

	mappingsContainer.setSize(rowWidth, mappingRows.size() * rowHeight);

	for (int i = 0; i < mappingRows.size(); ++i)
	{
		mappingRows[i]->setBounds(0, i * rowHeight, rowWidth, rowHeight);
	}
}

void MidiMappingEditorWindow::buttonClicked(juce::Button *button)
{
	if (button == &clearAllButton)
	{
		ObsidianAlertManager::showConfirm(this,
										  "Confirmation", "Are you sure you want to clear all MIDI mappings?", "Yes", "No",
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
		ObsidianAlertManager::showConfirm(this,
										  "Confirmation", "Reset mappings to default configuration?", "Yes", "No",
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
		row->onDeleteClicked = [this, mapping]
		{ deleteMapping(mapping); };
		row->onLearnClicked = [this, mapping]
		{ startLearningForMapping(mapping); };

		mappingRows.add(row);
		mappingsContainer.addAndMakeVisible(row);
	}

	resized();
}

void MidiMappingEditorWindow::deleteMapping(const MidiMapping &mapping)
{
	ObsidianAlertManager::showConfirm(this, "Confirmation", "Delete mapping for \"" + mapping.parameterName + "\"?", "Yes", "No",
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
		juce::MessageManager::callAsync([this, paramName]()
										{
				auto updatedMappings = midiLearnManager->getAllMappings();
				for (const auto& updated : updatedMappings)
				{
					if (updated.parameterName == paramName)
					{
						for (auto* row : mappingRows)
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
				} });
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
	auto modal = std::make_unique<ObsidianModalWindow>("MIDI Mappings");

	modal->setContent(std::make_unique<MidiMappingEditorWindow>(manager));

	auto *overlay = new ObsidianModalOverlay(parent, std::move(modal));

	overlay->modalWindow->addButton("Close", crossSvg, ColourPalette::buttonInactive, [overlay]()
									{ overlay->close(); });
}
