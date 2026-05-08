#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;

class UIPresetManager
{
  public:
	explicit UIPresetManager(DjIaVstEditor &editor);
	~UIPresetManager() = default;

	void notifyTracksPromptUpdate();

  private:
	DjIaVstEditor &editor;
};