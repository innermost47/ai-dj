#include "MidiMappingEditorWindow.h"
#include "ColourPalette.h"
#include "ObsidianAlertManager.h"
#include "BinaryData.h"

MidiMappingRow::MidiMappingRow(const MidiMapping& mapping, MidiLearnManager* manager)
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

	setSize(800, 50);
}

MidiMappingRow::~MidiMappingRow()
{
}

void MidiMappingRow::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	bool isEven = (getY() / getHeight()) % 2 == 0;
	g.setColour(isEven
		? ColourPalette::backgroundDark
		: ColourPalette::backgroundMid.withAlpha(0.5f));
	g.fillRect(bounds);
	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.6f));
	g.fillRect(0.0f, 4.0f, 3.0f, bounds.getHeight() - 8.0f);
	g.setColour(ColourPalette::trackSelected.withAlpha(0.2f));
	g.drawLine(10.0f, bounds.getBottom() - 0.5f,
		bounds.getWidth() - 10.0f, bounds.getBottom() - 0.5f, 0.5f);
}

void MidiMappingRow::buttonClicked(juce::Button* button)
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

void MidiMappingEditorWindow::MidiMappingEditorContent::buttonClicked(juce::Button* button)
{
	if (button == &clearAllButton)
	{
		ObsidianAlertManager::showConfirm(
			"Confirmation", "Are you sure you want to clear all MIDI mappings?",
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
			"Confirmation", "Reset mappings to default configuration?",
			"Yes", "No",
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

MidiMappingEditorWindow::MidiMappingEditorContent::MidiMappingEditorContent(MidiLearnManager* manager)
	: midiLearnManager(manager)
{
	titleLabel.setText("MIDI Mappings", juce::dontSendNotification);
	titleLabel.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
	titleLabel.setJustificationType(juce::Justification::centred);
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	addAndMakeVisible(titleLabel);

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
}

MidiMappingEditorWindow::MidiMappingEditorContent::~MidiMappingEditorContent()
{
	mappingsViewport.setLookAndFeel(nullptr);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::paint(juce::Graphics& g)
{
	g.fillAll(ColourPalette::backgroundDark);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::resized()
{
	auto bounds = getLocalBounds().reduced(10);

	auto headerRow = bounds.removeFromTop(40);
	reloadDefaultsButton.setBounds(headerRow.removeFromRight(38).withSizeKeepingCentre(34, 34));
	headerRow.removeFromRight(6);
	clearAllButton.setBounds(headerRow.removeFromRight(38).withSizeKeepingCentre(34, 34));
	titleLabel.setBounds(headerRow);

	bounds.removeFromTop(8);
	mappingsViewport.setBounds(bounds);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::refreshMappingsList()
{
	auto mappings = midiLearnManager->getAllMappings();
	createMappingRows();
}

void MidiMappingEditorWindow::MidiMappingEditorContent::createMappingRows()
{
	mappingRows.clear();

	auto mappings = midiLearnManager->getAllMappings();
	int y = 0;

	for (const auto& mapping : mappings)
	{
		auto* row = new MidiMappingRow(mapping, midiLearnManager);
		row->onDeleteClicked = [this, mapping]
			{ deleteMapping(mapping); };
		row->onLearnClicked = [this, mapping]
			{ startLearningForMapping(mapping); };

		mappingRows.add(row);
		mappingsContainer.addAndMakeVisible(row);
		row->setBounds(0, y, 800, 50);
		y += 50;
	}

	mappingsContainer.setSize(800, y);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::deleteMapping(const MidiMapping& mapping)
{
	ObsidianAlertManager::showConfirm(
		"Confirmation", "Delete mapping for \"" + mapping.parameterName + "\"?",
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

void MidiMappingEditorWindow::MidiMappingEditorContent::startLearningForMapping(const MidiMapping& mapping)
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

	midiLearnManager->startLearning(
		mapping.parameterName,
		mapping.processor,
		onLearningComplete,
		mapping.description);

	for (auto* row : mappingRows)
		row->setLearningActive(false);

	for (auto* row : mappingRows)
	{
		if (row->getMapping().parameterName == mapping.parameterName)
		{
			row->setLearningActive(true);
			break;
		}
	}
}

void MidiMappingRow::updateMapping(const MidiMapping& newMapping)
{
	mapping = newMapping;
	parameterLabel.setText(mapping.parameterName, juce::dontSendNotification);
	midiInfoLabel.setText(getMidiInfoString(), juce::dontSendNotification);
	repaint();
}

MidiMappingEditorWindow::MidiMappingEditorWindow(MidiLearnManager* manager)
	: DocumentWindow("MIDI Mappings",
		ColourPalette::backgroundDark,
		DocumentWindow::allButtons),
	midiLearnManager(manager)
{
	content = std::make_unique<MidiMappingEditorContent>(manager);
	setUsingNativeTitleBar(true);
	setOpaque(true);
	setContentOwned(content.release(), true);
	setResizable(true, true);
	setResizeLimits(600, 300, 1200, 800);
	setBounds(100, 100, 850, 500);
	setVisible(true);
	startTimerHz(2);
}

MidiMappingEditorWindow::~MidiMappingEditorWindow()
{
	stopTimer();
}

void MidiMappingEditorWindow::closeButtonPressed()
{
	if (onWindowClosed)
		onWindowClosed();
	setVisible(false);
	delete this;
}

void MidiMappingEditorWindow::timerCallback()
{
	if (auto* contentLocal = dynamic_cast<MidiMappingEditorContent*>(getContentComponent()))
	{
		if (midiLearnManager->isLearningActive())
		{
			for (auto* row : contentLocal->mappingRows)
				row->toggleBlink();
		}
		else
		{
			for (auto* row : contentLocal->mappingRows)
				row->setLearningActive(false);
		}
	}
}