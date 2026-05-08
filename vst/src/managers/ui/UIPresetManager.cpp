#include "UIPresetManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

UIPresetManager::UIPresetManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIPresetManager::notifyTracksPromptUpdate()
{
	for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
	{
		juce::String currentModel;
		if (auto *track = editor.audioProcessor.getTrack(trackComp->getTrackId()))
			currentModel = track->getCurrentPage().selectedModel;

		juce::StringArray prompts = editor.audioProcessor.getAvailablePromptsForModel(currentModel);
		trackComp->updatePromptPresets(prompts);
	}
}