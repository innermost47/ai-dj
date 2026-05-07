#pragma once
#include "ObsidianModal.h"
#include <JuceHeader.h>

class DjIaVstEditor;

class UIModalManager
{
  public:
	explicit UIModalManager(DjIaVstEditor &editor);
	~UIModalManager() = default;

	void addModal(std::unique_ptr<ObsidianModalOverlay> overlay);
	void removeModal(ObsidianModalOverlay *overlay);
	void showFirstTimeSetup();
	void showConfigDialog();
	void editCustomPromptDialog(const juce::String &selectedPrompt);
	void showOnboardingStep(int step);
	void showOnboardingTour();
	void openMidiMappingEditor();
	void checkForUpdates();
	void clearAll();

  private:
	DjIaVstEditor &editor;
	std::vector<std::unique_ptr<ObsidianModalOverlay>> activeModals;
};