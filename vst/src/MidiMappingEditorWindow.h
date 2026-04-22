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

class MidiMappingEditorWindow : public juce::DocumentWindow,
	public juce::Timer
{
public:
	MidiMappingEditorWindow(MidiLearnManager* manager);
	~MidiMappingEditorWindow() override;

	void closeButtonPressed() override;
	void timerCallback() override;

	std::function<void()> onWindowClosed;

private:
	class MidiMappingEditorContent : public juce::Component,
		public juce::Button::Listener
	{
	public:
		MidiMappingEditorContent(MidiLearnManager* manager);
		~MidiMappingEditorContent() override;

		void paint(juce::Graphics& g) override;
		void resized() override;
		void buttonClicked(juce::Button* button) override;

		void refreshMappingsList();
		void deleteMapping(const MidiMapping& mapping);
		void startLearningForMapping(const MidiMapping& mapping);

		juce::OwnedArray<MidiMappingRow> mappingRows;

	private:
		MidiLearnManager* midiLearnManager = nullptr;

		CustomLookAndFeel customLookAndFeel;

		juce::Label titleLabel;

		IconButtonSimple clearAllButton{ "ClearAll", "" };
		IconButtonSimple reloadDefaultsButton{ "ReloadDefaults", "" };

		juce::Viewport mappingsViewport;
		juce::Component mappingsContainer;

		void createMappingRows();

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMappingEditorContent)
	};

	MidiLearnManager* midiLearnManager = nullptr;
	std::unique_ptr<MidiMappingEditorContent> content;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMappingEditorWindow)
};