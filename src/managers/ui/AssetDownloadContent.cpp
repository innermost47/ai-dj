#include "AssetDownloadContent.h"

AssetDownloadContent::AssetDownloadContent(const juce::File &destDir) : stableAudioDir(destDir)
{
	setupUI();
}

void AssetDownloadContent::setupUI()
{
	statusLbl_.setText("Preparing download...", juce::dontSendNotification);
	statusLbl_.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	statusLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
	statusLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(statusLbl_);

	fileCountLbl_.setText("", juce::dontSendNotification);
	fileCountLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	fileCountLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
	fileCountLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(fileCountLbl_);

	currentFileLbl_.setText("Current file:", juce::dontSendNotification);
	currentFileLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	currentFileLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
	addAndMakeVisible(currentFileLbl_);

	currentFileBar_.setColour(juce::ProgressBar::backgroundColourId, ColourPalette::backgroundDeep);
	currentFileBar_.setColour(juce::ProgressBar::foregroundColourId, ColourPalette::slate);
	addAndMakeVisible(currentFileBar_);

	globalLbl_.setText("Overall:", juce::dontSendNotification);
	globalLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	globalLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
	addAndMakeVisible(globalLbl_);

	globalBar_.setColour(juce::ProgressBar::backgroundColourId, ColourPalette::backgroundDeep);
	globalBar_.setColour(juce::ProgressBar::foregroundColourId, ColourPalette::buttonPrimary);
	addAndMakeVisible(globalBar_);

	speedLbl_.setText("", juce::dontSendNotification);
	speedLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	speedLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
	speedLbl_.setJustificationType(juce::Justification::centredRight);
	addAndMakeVisible(speedLbl_);

	etaLbl_.setText("", juce::dontSendNotification);
	etaLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	etaLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
	etaLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(etaLbl_);

	warningLbl_.setText(
	    "Warning: This download is ~7 GB. Make sure you have a stable connection and enough disk space.",
	    juce::dontSendNotification);
	warningLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary.withAlpha(0.7f));
	warningLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
	warningLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(warningLbl_);

	infoLbl_.setText("Do not close the plugin window during download.\n"
	                 "Files are saved to: " +
	                     stableAudioDir.getFullPathName(),
	                 juce::dontSendNotification);
	infoLbl_.setColour(juce::Label::textColourId, ColourPalette::textSecondary.withAlpha(0.6f));
	infoLbl_.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
	infoLbl_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(infoLbl_);

	openFolderBtn_.setButtonText("Open folder");
	openFolderBtn_.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep.brighter(0.04f));
	openFolderBtn_.setColour(juce::TextButton::textColourOffId, ColourPalette::buttonPrimary);
	openFolderBtn_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
	openFolderBtn_.onClick = [this]() { stableAudioDir.revealToUser(); };
	addAndMakeVisible(openFolderBtn_);

	downloadLinkBtn_.setButtonText("Download manually");
	downloadLinkBtn_.setURL(juce::URL(Obsidian::MODELS_DOWNLOAD_URL()));
	downloadLinkBtn_.setColour(juce::HyperlinkButton::textColourId, ColourPalette::buttonPrimary);
	downloadLinkBtn_.setJustificationType(juce::Justification::centredLeft);
	downloadLinkBtn_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
	addAndMakeVisible(downloadLinkBtn_);
}

void AssetDownloadContent::updateProgress(const AssetDownloadProgress &p)
{
	currentFileProgress_ = (double)p.currentFileProgress();
	globalProgress_ = (double)p.globalProgress();

	statusLbl_.setText("Downloading: " + p.currentFilename, juce::dontSendNotification);

	fileCountLbl_.setText("File " + juce::String(p.currentFileIndex + 1) + " of " + juce::String(p.totalFiles) +
	                          "  -  " + formatBytes(p.currentFileBytes) + " / " + formatBytes(p.currentFileTotalBytes),
	                      juce::dontSendNotification);

	if (p.speedBytesPerSec > 0.0)
		speedLbl_.setText(formatSpeed(p.speedBytesPerSec), juce::dontSendNotification);

	if (p.etaSeconds >= 0.0)
	{
		const bool firstEstimate = lastShownEta_ < 0.0;
		const bool bigChange = lastShownEta_ > 0.0 && std::abs(p.etaSeconds - lastShownEta_) / lastShownEta_ > 0.15;
		const bool goodNews = lastShownEta_ > 0.0 && p.etaSeconds < lastShownEta_ - 30.0;

		if (firstEstimate || bigChange || goodNews)
		{
			lastShownEta_ = p.etaSeconds;
			etaLbl_.setText("ETA: " + formatEta(p.etaSeconds), juce::dontSendNotification);
		}
	}

	currentFileBar_.repaint();
	globalBar_.repaint();
}

void AssetDownloadContent::setComplete(bool success, const juce::String &errorMessage)
{
	if (success)
	{
		currentFileProgress_ = 1.0;
		globalProgress_ = 1.0;
		statusLbl_.setText("Download complete!", juce::dontSendNotification);
		statusLbl_.setColour(juce::Label::textColourId, ColourPalette::slate);
		fileCountLbl_.setText("All files downloaded successfully.", juce::dontSendNotification);
		speedLbl_.setText("", juce::dontSendNotification);
		etaLbl_.setText("", juce::dontSendNotification);
		warningLbl_.setVisible(false);
	}
	else
	{
		statusLbl_.setText("Download failed.", juce::dontSendNotification);
		statusLbl_.setColour(juce::Label::textColourId, ColourPalette::buttonDangerDark);
		fileCountLbl_.setText(errorMessage, juce::dontSendNotification);
		fileCountLbl_.setColour(juce::Label::textColourId, ColourPalette::buttonDangerDark);
	}

	lastShownEta_ = -1.0;
	currentFileBar_.repaint();
	globalBar_.repaint();
}

void AssetDownloadContent::resized()
{
	auto area = getLocalBounds().reduced(Obsidian::PADDING);

	warningLbl_.setBounds(area.removeFromTop(36));
	area.removeFromTop(Obsidian::GAP_4 * 2);

	statusLbl_.setBounds(area.removeFromTop(20));
	area.removeFromTop(4);
	fileCountLbl_.setBounds(area.removeFromTop(18));
	area.removeFromTop(Obsidian::GAP_4 * 2);

	currentFileLbl_.setBounds(area.removeFromTop(16));
	area.removeFromTop(4);
	currentFileBar_.setBounds(area.removeFromTop(18));
	area.removeFromTop(Obsidian::GAP_4 * 2);

	globalLbl_.setBounds(area.removeFromTop(16));
	area.removeFromTop(4);
	globalBar_.setBounds(area.removeFromTop(18));
	area.removeFromTop(8);

	auto speedRow = area.removeFromTop(18);
	etaLbl_.setBounds(speedRow.removeFromLeft(speedRow.getWidth() / 2));
	speedLbl_.setBounds(speedRow);

	area.removeFromTop(8);
	infoLbl_.setBounds(area.removeFromTop(32));

	area.removeFromTop(Obsidian::GAP_4 * 2);
	auto actionRow = area.removeFromTop(24);
	openFolderBtn_.setBounds(actionRow.removeFromLeft(120));
	actionRow.removeFromLeft(Obsidian::GAP_4 * 2);
	downloadLinkBtn_.setBounds(actionRow.removeFromLeft(160));
}

void AssetDownloadContent::paint(juce::Graphics &)
{
}

juce::String AssetDownloadContent::formatBytes(juce::int64 bytes)
{
	if (bytes < 0)
		return "?";
	if (bytes < 1024)
		return juce::String(bytes) + " B";
	if (bytes < 1024 * 1024)
		return juce::String(bytes / 1024) + " KB";
	if (bytes < 1024 * 1024 * 1024)
		return juce::String(bytes / (1024 * 1024)) + " MB";
	return juce::String::formatted("%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
}

juce::String AssetDownloadContent::formatSpeed(double bps)
{
	if (bps < 1024.0)
		return juce::String::formatted("%.0f B/s", bps);
	if (bps < 1024.0 * 1024.0)
		return juce::String::formatted("%.1f KB/s", bps / 1024.0);
	return juce::String::formatted("%.1f MB/s", bps / (1024.0 * 1024.0));
}

juce::String AssetDownloadContent::formatEta(double seconds)
{
	if (seconds < 0)
		return "...";
	int s = (int)seconds;

	if (s < 30)
		return "less than 30s";
	if (s < 90)
		return "about 1 min";
	if (s < 600)
		return "about " + juce::String((s + 30) / 60) + " min";
	if (s < 3600)
		return "about " + juce::String(((s + 150) / 300) * 5) + " min";
	int halves = (s + 900) / 1800;
	return "about " + juce::String::formatted("%.1f", halves * 0.5) + " h";
}