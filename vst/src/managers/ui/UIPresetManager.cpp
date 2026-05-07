#include "UIPresetManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

UIPresetManager::UIPresetManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIPresetManager::loadPromptPresets()
{
	editor.promptPresetSelector.clear();
	juce::StringArray allPrompts = editor.audioProcessor.promptPresets;
	auto customPrompts = editor.audioProcessor.getCustomPrompts();
	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);

	for (int i = 0; i < allPrompts.size(); ++i)
	{
		editor.promptPresetSelector.addItem(allPrompts[i], i + 1);
	}
	int lastPresetIndex = editor.audioProcessor.getLastPresetIndex();
	if (lastPresetIndex >= 1 && lastPresetIndex <= allPrompts.size())
	{
		editor.promptPresetSelector.setSelectedId(lastPresetIndex + 1, juce::dontSendNotification);
	}
	else
	{
		editor.promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
	}
	juce::String selectedPresetText = editor.promptPresetSelector.getText();
	editor.promptInput.setText(selectedPresetText, juce::dontSendNotification);
}

void UIPresetManager::onPresetSelected()
{
	int selectedId = editor.promptPresetSelector.getSelectedId();
	editor.audioProcessor.setLastPresetIndex(selectedId);
	juce::String selectedPrompt = editor.promptPresetSelector.getText();
	if (!selectedPrompt.isEmpty())
	{
		editor.promptInput.setText(selectedPrompt);
		editor.statusLabel.setText("Preset loaded: " + selectedPrompt, juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
	}
	else
	{
		editor.promptInput.clear();
		editor.statusLabel.setText("Custom prompt mode", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
	}
}

void UIPresetManager::onSavePreset()
{
	juce::String currentPrompt = editor.promptInput.getText().trim();
	if (currentPrompt.isNotEmpty())
	{
		editor.audioProcessor.addCustomPrompt(currentPrompt);
		loadPromptPresets();
		notifyTracksPromptUpdate();
		int totalItems = editor.promptPresetSelector.getNumItems();
		for (int i = 0; i < totalItems; ++i)
		{
			if (editor.promptPresetSelector.getItemText(i) == currentPrompt)
			{
				editor.promptPresetSelector.setSelectedId(i + 1, juce::dontSendNotification);
				break;
			}
		}

		editor.statusLabel.setText("Preset saved: " + currentPrompt, juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
	}
	else
	{
		editor.statusLabel.setText("Enter a prompt first!", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
	}
}

void UIPresetManager::notifyTracksPromptUpdate()
{
	juce::StringArray allPrompts = editor.audioProcessor.promptPresets;
	auto customPrompts = editor.audioProcessor.getCustomPrompts();

	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);
	for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
	{
		trackComp->updatePromptPresets(allPrompts);
	}
}

void UIPresetManager::refreshAllPromptLists()
{
	loadPromptPresets();
	notifyTracksPromptUpdate();
}

juce::StringArray UIPresetManager::getAllPrompts() const
{
	juce::StringArray allPrompts = editor.audioProcessor.promptPresets;
	auto customPrompts = editor.audioProcessor.getCustomPrompts();

	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}

	return allPrompts;
}