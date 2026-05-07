#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;

class UIPresetManager
{
  public:
	explicit UIPresetManager(DjIaVstEditor &editor);
	~UIPresetManager() = default;

	void loadPromptPresets();
	void onPresetSelected();
	void onSavePreset();
	void notifyTracksPromptUpdate();
	void refreshAllPromptLists();
	juce::StringArray getAllPrompts() const;

  private:
	DjIaVstEditor &editor;
};