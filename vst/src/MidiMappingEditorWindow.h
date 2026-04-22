#pragma once
#include "ObsidianBase.h"
#include "MidiMapping.h"
#include "MidiLearnManager.h"
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "IconButton.h"

class MidiMappingRow : public ObsidianComponent,
	public juce::Button::Listener
{
public:
	MidiMappingRow(const MidiMapping& mapping, MidiLearnManager* manager);
	~MidiMappingRow() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void buttonClicked(juce::Button* button) override;

	std::function<void()> onDeleteClicked;
	std::function<void()> onLearnClicked;

	const MidiMapping& getMapping() const { return mapping; }
	void setLearningActive(bool active);
	void toggleBlink();
	void updateMapping(const MidiMapping& newMapping);

private:
	MidiMapping mapping;
	MidiLearnManager* midiLearnManager = nullptr;

	juce::Label parameterLabel;
	juce::Label midiInfoLabel;

	IconButtonSimple deleteButton{ "Delete", "" };
	IconButtonSimple learnButton{ "ReLearn", "" };

	bool isLearning = false;
	bool blinkState = false;

	juce::String getMidiInfoString() const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMappingRow)
};

class MidiMappingEditorWindow : public juce::Component,
	public juce::Button::Listener,
	public juce::Timer
{
public:
	MidiMappingEditorWindow(MidiLearnManager* manager);
	~MidiMappingEditorWindow() override;

	void paint(juce::Graphics& g) override;
	void resized() override;
	void buttonClicked(juce::Button* button) override;
	void timerCallback() override;

	void refreshMappingsList();

private:
	void deleteMapping(const MidiMapping& mapping);
	void startLearningForMapping(const MidiMapping& mapping);

	MidiLearnManager* midiLearnManager = nullptr;

	CustomLookAndFeel customLookAndFeel;

	IconButtonSimple clearAllButton{ "ClearAll", "" };
	IconButtonSimple reloadDefaultsButton{ "ReloadDefaults", "" };

	juce::Label subtitleLabel;
	juce::Rectangle<int> headerBounds;

	juce::Viewport mappingsViewport;
	juce::Component mappingsContainer;

	juce::OwnedArray<MidiMappingRow> mappingRows;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMappingEditorWindow)
};