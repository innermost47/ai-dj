#pragma once
#include "ObsidianBase.h"

struct LicenseEntry
{
	juce::String title;
	juce::String url;
	juce::String summary;
};

class LicenseAgreementContent : public ObsidianComponent
{
  public:
	explicit LicenseAgreementContent(const std::vector<LicenseEntry> &licenses);

	bool allAccepted() const;

	std::function<void(bool)> onAcceptanceChanged;

	void resized() override;

	void paint(juce::Graphics &g) override;

  private:
	std::vector<LicenseEntry> licenses_;
	juce::Label introLbl_;
	juce::OwnedArray<juce::Label> titleLabels_;
	juce::OwnedArray<juce::Label> summaryLabels_;
	juce::OwnedArray<juce::TextButton> linkButtons_;
	juce::OwnedArray<juce::TextButton> checkboxes_;
	std::vector<juce::Rectangle<int>> separatorRects_;
};