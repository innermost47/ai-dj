#pragma once
#include "AssetDownloadManager.h"
#include "ObsidianBase.h"

class AssetDownloadContent : public ObsidianComponent
{
  public:
	AssetDownloadContent(const juce::File &destDir);

	void updateProgress(const AssetDownloadProgress &p);
	void setComplete(bool success, const juce::String &errorMessage);
	void resized() override;
	void paint(juce::Graphics &g) override;

  private:
	juce::File stableAudioDir;

	static juce::String formatBytes(juce::int64 bytes);
	static juce::String formatSpeed(double bps);
	static juce::String formatEta(double seconds);

	juce::Label statusLbl_, fileCountLbl_, currentFileLbl_, globalLbl_, speedLbl_, etaLbl_, warningLbl_, infoLbl_;
	juce::ProgressBar currentFileBar_{currentFileProgress_};
	juce::ProgressBar globalBar_{globalProgress_};
	juce::TextButton openFolderBtn_;
	juce::HyperlinkButton downloadLinkBtn_;

	double currentFileProgress_ = 0.0;
	double globalProgress_ = 0.0;
	double lastShownEta_ = -1.0;

	void setupUI();
};