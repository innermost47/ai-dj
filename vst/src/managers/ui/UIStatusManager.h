#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;

class UIStatusManager
{
  public:
	explicit UIStatusManager(DjIaVstEditor &editor);
	~UIStatusManager() = default;

	void setStatusWithTimeout(const juce::String &message, int timeoutMs = 2000);
	void updateLCD();
	void refreshCredits();
	void refreshCreditsAsync();

  private:
	DjIaVstEditor &editor;
};