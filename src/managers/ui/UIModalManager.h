#pragma once
#include "ObsidianAlertManager.h"
#include "ObsidianModal.h"
#include "OnboardingFlow.h"
#include "OnboardingStepData.h"
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
	void showOnboardingTour();
	void openMidiMappingEditor();
	void checkForUpdates();
	void clearAll();
	void showOnboarding(OnboardingVariant variant);
	void advanceOnboardingTo(int stepIndex);
	void showModelDownloader(std::function<void()> onComplete = nullptr);
	void showCredits();
	void applyScale(float scale);

	bool hasActiveModals() const noexcept
	{
		return !activeModals.empty();
	}

  private:
	DjIaVstEditor &editor;
	std::vector<std::unique_ptr<ObsidianModalOverlay>> activeModals;
	std::unique_ptr<OnboardingFlow> onboardingFlow;

	void handleUpdateCheckResponse(const juce::String &jsonResponse);

	static constexpr juce::int64 updateCheckIntervalMs = 24 * 60 * 60 * 1000LL;
};