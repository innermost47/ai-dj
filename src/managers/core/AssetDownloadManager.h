#pragma once
#include "DataConst.h"
#include <JuceHeader.h>

struct AssetFile
{
	juce::String filename;
	juce::String url;
	juce::int64 expectedSizeBytes = 0;
};

struct AssetDownloadProgress
{
	int currentFileIndex = 0;
	int totalFiles = 0;
	juce::String currentFilename;
	juce::int64 currentFileBytes = 0;
	juce::int64 currentFileTotalBytes = 0;
	juce::int64 totalBytes = 0;
	juce::int64 totalExpectedBytes = 0;
	double speedBytesPerSec = 0.0;
	double etaSeconds = -1.0;

	float currentFileProgress() const
	{
		if (currentFileTotalBytes <= 0)
			return 0.0f;
		return (float)((double)currentFileBytes / (double)currentFileTotalBytes);
	}

	float globalProgress() const
	{
		if (totalExpectedBytes <= 0)
			return 0.0f;
		return (float)((double)totalBytes / (double)totalExpectedBytes);
	}
};

class AssetDownloadManager : private juce::Thread
{
  public:
	using ProgressCallback = std::function<void(const AssetDownloadProgress &)>;
	using CompletionCallback = std::function<void(bool success, const juce::String &errorMessage)>;

	AssetDownloadManager();
	~AssetDownloadManager() override;

	void start(const juce::File &destinationDir, const std::vector<AssetFile> &files, ProgressCallback onProgress,
	           CompletionCallback onComplete);
	void cancel();
	void startFromManifest(const juce::File &destinationDir, ProgressCallback onProgress,
	                       CompletionCallback onComplete);

	bool isCancelled() const;

	static std::vector<AssetFile> fallbackAssets();

  private:
	bool useManifest_ = false;

	juce::File destDir_;

	std::vector<AssetFile> files_;

	ProgressCallback onProgress_;
	CompletionCallback onComplete_;

	std::atomic<bool> cancelled_{false};

	void run() override;
	void fireProgress(const AssetDownloadProgress &p);
	void fireComplete(bool success, const juce::String &error);

	std::vector<AssetFile> fetchManifestAssets();
};