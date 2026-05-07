#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;

class UILayoutManager
{
  public:
	explicit UILayoutManager(DjIaVstEditor &editor);
	~UILayoutManager() = default;

	void resized();
	void layoutPromptSection(juce::Rectangle<int> area, int spacing, int controlsZoneW);
	void layoutTracksGrid();

	static constexpr int TRACK_CELL_H = 140;
	static constexpr int TRACK_ROWS = 4;
	static constexpr int TRACK_COLS = 2;

  private:
	DjIaVstEditor &editor;
};