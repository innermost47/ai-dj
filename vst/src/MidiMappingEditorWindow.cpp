#include "MidiMappingEditorWindow.h"
#include "ColourPalette.h"


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

	deleteButton.setButtonText("Delete");
	deleteButton.addListener(this);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addAndMakeVisible(deleteButton);

	learnButton.setButtonText("Re-Learn");
	learnButton.addListener(this);
	learnButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	learnButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addAndMakeVisible(learnButton);

	setSize(800, 50);
}

MidiMappingRow::~MidiMappingRow()
{
}

void MidiMappingRow::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::transparentBlack);
	g.setColour(ColourPalette::textSecondary);
	g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, 0.5f);
}

void MidiMappingRow::resized()
{
	auto bounds = getLocalBounds().reduced(5);

	auto buttonArea = bounds.removeFromRight(180);
	deleteButton.setBounds(buttonArea.removeFromRight(80).reduced(2));
	learnButton.setBounds(buttonArea.removeFromRight(90).reduced(2));

	auto labelArea = bounds;
	parameterLabel.setBounds(labelArea.removeFromLeft(300));
	midiInfoLabel.setBounds(labelArea);
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
	case 1: typeStr = "CC"; break;
	case 0: typeStr = "Note"; break;
	case 2: typeStr = "Pitch Bend"; break;
	default: typeStr = "Unknown"; break;
	}
	return typeStr + " " + juce::String(mapping.midiNumber) + " (Ch " + juce::String(mapping.midiChannel + 1) + ")";
}

MidiMappingEditorWindow::MidiMappingEditorContent::MidiMappingEditorContent(MidiLearnManager* manager)
	: midiLearnManager(manager)
{
	titleLabel.setText("MIDI Mappings", juce::dontSendNotification);
	auto titleFont = juce::Font(24.0f);
	titleFont.setBold(true);
	titleLabel.setFont(titleFont);
	titleLabel.setJustificationType(juce::Justification::centred);
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	addAndMakeVisible(titleLabel);

	clearAllButton.setButtonText("Clear All");
	clearAllButton.addListener(this);
	clearAllButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	clearAllButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addAndMakeVisible(clearAllButton);

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
	g.fillAll(ColourPalette::backgroundDeep);
	g.setColour(ColourPalette::textSecondary);
	g.drawRect(mappingsViewport.getBounds(), 1);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::resized()
{
	auto bounds = getLocalBounds().reduced(10);

	titleLabel.setBounds(bounds.removeFromTop(40));
	bounds.removeFromTop(10);

	auto toolbarBounds = bounds.removeFromTop(35);
	clearAllButton.setBounds(toolbarBounds.removeFromLeft(100).reduced(2));

	bounds.removeFromTop(10);
	mappingsViewport.setBounds(bounds);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::buttonClicked(juce::Button* button)
{
	if (button == &clearAllButton)
	{
		showConfirmationDialog("Are you sure you want to clear all MIDI mappings?",
			[this] {
				midiLearnManager->clearAllMappings();
				refreshMappingsList();
			});
	}
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
		row->onDeleteClicked = [this, mapping] { deleteMapping(mapping); };
		row->onLearnClicked = [this, mapping] { startLearningForMapping(mapping); };

		mappingRows.add(row);
		mappingsContainer.addAndMakeVisible(row);
		row->setBounds(0, y, 800, 50);
		y += 50;
	}

	mappingsContainer.setSize(800, y);
}

void MidiMappingEditorWindow::MidiMappingEditorContent::deleteMapping(const MidiMapping& mapping)
{
	showConfirmationDialog("Delete mapping for \"" + mapping.parameterName + "\"?",
		[this, mapping] {
			midiLearnManager->removeMapping(mapping.parameterName);
			refreshMappingsList();
		});
}

void MidiMappingEditorWindow::MidiMappingEditorContent::startLearningForMapping(const MidiMapping& mapping)
{
	auto onLearningComplete = [this, paramName = mapping.parameterName](float value)
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
					}
				});
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

void MidiMappingEditorWindow::MidiMappingEditorContent::showConfirmationDialog(
	const juce::String& message, std::function<void()> onConfirm)
{
	auto options = juce::MessageBoxOptions()
		.withIconType(juce::MessageBoxIconType::QuestionIcon)
		.withTitle("Confirmation")
		.withMessage(message)
		.withButton("Yes")
		.withButton("No");

	juce::AlertWindow::showAsync(options, [onConfirm](int result)
		{
			if (result == 1 && onConfirm)
				onConfirm();
		});
}

MidiMappingEditorWindow::MidiMappingEditorWindow(MidiLearnManager* manager)
	: DocumentWindow("MIDI Mappings",
		ColourPalette::backgroundDark,
		DocumentWindow::allButtons),
	midiLearnManager(manager)
{
	content = std::make_unique<MidiMappingEditorContent>(manager);
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
	if (auto* content = dynamic_cast<MidiMappingEditorContent*>(getContentComponent()))
	{
		if (midiLearnManager->isLearningActive())
		{
			for (auto* row : content->mappingRows)
				row->toggleBlink();
		}
		else
		{
			for (auto* row : content->mappingRows)
				row->setLearningActive(false);
		}
	}
}