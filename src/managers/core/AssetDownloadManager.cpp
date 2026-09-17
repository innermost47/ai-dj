#include "AssetDownloadManager.h"

AssetDownloadManager::AssetDownloadManager() : juce::Thread("AssetDownloadThread")
{
}

AssetDownloadManager::~AssetDownloadManager()
{
	cancel();
}

void AssetDownloadManager::start(const juce::File &destinationDir, const std::vector<AssetFile> &files,
                                 ProgressCallback onProgress, CompletionCallback onComplete)
{
	jassert(!isThreadRunning());
	destDir_ = destinationDir;
	files_ = files;
	useManifest_ = false;
	onProgress_ = std::move(onProgress);
	onComplete_ = std::move(onComplete);
	cancelled_.store(false);
	startThread();
}

void AssetDownloadManager::startFromManifest(const juce::File &destinationDir, ProgressCallback onProgress,
                                             CompletionCallback onComplete)
{
	jassert(!isThreadRunning());
	destDir_ = destinationDir;
	files_.clear();
	useManifest_ = true;
	onProgress_ = std::move(onProgress);
	onComplete_ = std::move(onComplete);
	cancelled_.store(false);
	startThread();
}

void AssetDownloadManager::cancel()
{
	cancelled_.store(true);
	stopThread(5000);
}

bool AssetDownloadManager::isCancelled() const
{
	return cancelled_.load();
}

std::vector<AssetFile> AssetDownloadManager::fallbackAssets()
{
	return {{Obsidian::FP32_DIT_ONNX(), Obsidian::FP32_DIT_ONNX_URL(), 3878882},
	        {Obsidian::FP32_DIT_ONNX_DATA(), Obsidian::FP32_DIT_ONNX_DATA_URL(), 5813473856LL},
	        {Obsidian::DEC_DYNAMIC_BF16(), Obsidian::DEC_DYNAMIC_BF16_URL(), 218677278},
	        {Obsidian::ENC_DYNAMIC_BF16(), Obsidian::ENC_DYNAMIC_BF16_URL(), 215532694},
	        {Obsidian::ENCODER(), Obsidian::ENCODER_URL(), 620393530},
	        {Obsidian::TOKENIZER(), Obsidian::TOKENIZER_URL(), 34362429}};
}

std::vector<AssetFile> AssetDownloadManager::fetchManifestAssets()
{
	juce::URL url(Obsidian::MODEL_MANIFEST_URL());

	int statusCode = 0;
	auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
	                                        .withConnectionTimeoutMs(10000)
	                                        .withNumRedirectsToFollow(5)
	                                        .withStatusCode(&statusCode));

	if (stream == nullptr || statusCode < 200 || statusCode >= 300)
		return fallbackAssets();

	const auto text = stream->readEntireStreamAsString();
	const auto parsed = juce::JSON::parse(text);

	const auto *root = parsed.getDynamicObject();
	if (root == nullptr)
		return fallbackAssets();

	const auto *assetsArray = root->getProperty("assets").getArray();
	if (assetsArray == nullptr || assetsArray->isEmpty())
		return fallbackAssets();

	std::vector<AssetFile> result;
	result.reserve((size_t)assetsArray->size());

	for (const auto &entry : *assetsArray)
	{
		const auto *obj = entry.getDynamicObject();
		if (obj == nullptr)
			return fallbackAssets();

		const juce::String filename = obj->getProperty("filename").toString();
		const juce::String fileUrl = obj->getProperty("url").toString();
		const juce::int64 size = (juce::int64)obj->getProperty("size");

		if (filename.isEmpty() || !fileUrl.startsWith("http") || size < 0)
			return fallbackAssets();

		result.push_back({filename, fileUrl, size});
	}

	return result;
}

void AssetDownloadManager::run()
{
	constexpr int maxAttemptsPerFile = 4;
	constexpr int bufferSize = 1024 * 256;
	constexpr double progressIntervalMs = 100.0;
	constexpr double speedIntervalMs = 500.0;

	juce::HeapBlock<char> buffer(bufferSize);

	auto interruptibleWait = [this](int totalMs) -> bool
	{
		int waited = 0;
		while (waited < totalMs)
		{
			if (threadShouldExit() || cancelled_.load())
				return false;
			const int step = juce::jmin(100, totalMs - waited);
			wait(step);
			waited += step;
		}
		return !(threadShouldExit() || cancelled_.load());
	};

	if (useManifest_)
	{
		files_ = fetchManifestAssets();

		if (threadShouldExit() || cancelled_.load())
		{
			fireComplete(false, "Download cancelled.");
			return;
		}
	}

	if (files_.empty())
	{
		fireComplete(false, "No assets to download.");
		return;
	}

	juce::int64 totalExpected = 0;
	for (const auto &f : files_)
		totalExpected += f.expectedSizeBytes;

	if (!destDir_.exists() && !destDir_.createDirectory())
	{
		fireComplete(false, "Cannot create directory: " + destDir_.getFullPathName());
		return;
	}

	juce::int64 completedBytes = 0;
	double lastProgressFire = 0.0;

	auto lastSpeedUpdate = juce::Time::getMillisecondCounterHiRes();
	juce::int64 sessionTotalBytes = 0;
	juce::int64 bytesAtLastUpdate = 0;
	double speedBps = 0.0;

	for (int i = 0; i < (int)files_.size(); ++i)
	{
		if (threadShouldExit() || cancelled_.load())
		{
			fireComplete(false, "Download cancelled.");
			return;
		}

		const auto &asset = files_[i];
		juce::File destFile = destDir_.getChildFile(asset.filename);
		juce::File partFile = destDir_.getChildFile(asset.filename + ".part");

		if (destFile.existsAsFile() && asset.expectedSizeBytes > 0 && destFile.getSize() == asset.expectedSizeBytes)
		{
			completedBytes += asset.expectedSizeBytes;
			AssetDownloadProgress progress;
			progress.currentFileIndex = i;
			progress.totalFiles = (int)files_.size();
			progress.currentFilename = asset.filename;
			progress.currentFileBytes = asset.expectedSizeBytes;
			progress.currentFileTotalBytes = asset.expectedSizeBytes;
			progress.totalBytes = completedBytes;
			progress.totalExpectedBytes = totalExpected;
			progress.speedBytesPerSec = 0.0;
			progress.etaSeconds = -1.0;
			fireProgress(progress);

			continue;
		}

		if (destFile.existsAsFile() && !destFile.deleteFile())
		{
			fireComplete(false, "Cannot replace " + asset.filename +
			                        " - the file is in use. Close other plugin instances and try again.");
			return;
		}

		bool fileOk = false;
		juce::String lastError;

		for (int attempt = 1; attempt <= maxAttemptsPerFile && !fileOk; ++attempt)
		{
			if (threadShouldExit() || cancelled_.load())
			{
				fireComplete(false, "Download cancelled.");
				return;
			}

			juce::int64 resumeOffset = partFile.existsAsFile() ? partFile.getSize() : 0;

			if (asset.expectedSizeBytes > 0 && resumeOffset >= asset.expectedSizeBytes)
			{
				partFile.deleteFile();
				resumeOffset = 0;
			}

			juce::URL url(asset.url);
			int statusCode = 0;
			juce::StringPairArray responseHeaders;

			const juce::String extraHeaders =
			    resumeOffset > 0 ? "Range: bytes=" + juce::String(resumeOffset) + "-\r\n" : juce::String();

			const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
			                         .withConnectionTimeoutMs(30000)
			                         .withNumRedirectsToFollow(5)
			                         .withStatusCode(&statusCode)
			                         .withResponseHeaders(&responseHeaders)
			                         .withExtraHeaders(extraHeaders);

			auto stream = url.createInputStream(options);

			if (stream == nullptr)
			{
				lastError = "Failed to connect to: " + asset.url;
				if (attempt < maxAttemptsPerFile && !interruptibleWait(1000 * attempt))
				{
					fireComplete(false, "Download cancelled.");
					return;
				}
				continue;
			}

			if (resumeOffset > 0 && statusCode == 200)
			{
				partFile.deleteFile();
				resumeOffset = 0;
			}
			else if (statusCode == 416)
			{
				partFile.deleteFile();
				lastError = "HTTP 416 (invalid resume range) on: " + asset.url;
				continue;
			}
			else if (statusCode == 429 || statusCode >= 500)
			{
				lastError = "HTTP " + juce::String(statusCode) + " on: " + asset.url;
				if (attempt < maxAttemptsPerFile && !interruptibleWait(2000 * attempt))
				{
					fireComplete(false, "Download cancelled.");
					return;
				}
				continue;
			}
			else if (statusCode < 200 || statusCode >= 300)
			{
				fireComplete(false, "HTTP " + juce::String(statusCode) + " on: " + asset.url);
				return;
			}

			const juce::int64 remoteRemaining = stream->getTotalLength();
			const juce::int64 fileTotalBytes =
			    remoteRemaining > 0 ? resumeOffset + remoteRemaining : asset.expectedSizeBytes;

			juce::String contentEncoding;
			for (const auto &key : responseHeaders.getAllKeys())
				if (key.equalsIgnoreCase("Content-Encoding"))
					contentEncoding = responseHeaders.getValue(key, {});

			const bool sizeComparable = contentEncoding.isEmpty() || contentEncoding.equalsIgnoreCase("identity");

			if (sizeComparable && asset.expectedSizeBytes > 0 && fileTotalBytes > 0 &&
			    fileTotalBytes != asset.expectedSizeBytes)
			{
				partFile.deleteFile();
				lastError = "Unexpected file size for " + asset.filename + " (server: " + juce::String(fileTotalBytes) +
				            ", expected: " + juce::String(asset.expectedSizeBytes) + ")";
				if (attempt < maxAttemptsPerFile && !interruptibleWait(1000 * attempt))
				{
					fireComplete(false, "Download cancelled.");
					return;
				}
				continue;
			}

			auto out = std::make_unique<juce::FileOutputStream>(partFile);
			if (!out->openedOk())
			{
				fireComplete(false, "Cannot write to: " + partFile.getFullPathName());
				return;
			}

			juce::int64 fileDownloaded = resumeOffset;
			bool writeError = false;
			bool cancelledMidFile = false;

			while (!stream->isExhausted())
			{
				if (threadShouldExit() || cancelled_.load())
				{
					cancelledMidFile = true;
					break;
				}

				const int bytesRead = stream->read(buffer.getData(), bufferSize);
				if (bytesRead <= 0)
					break;

				if (!out->write(buffer.getData(), (size_t)bytesRead))
				{
					writeError = true;
					break;
				}

				fileDownloaded += bytesRead;
				sessionTotalBytes += bytesRead;

				const auto now = juce::Time::getMillisecondCounterHiRes();

				const double speedElapsed = now - lastSpeedUpdate;
				if (speedElapsed >= speedIntervalMs)
				{
					const double instantBps = (double)(sessionTotalBytes - bytesAtLastUpdate) / (speedElapsed / 1000.0);
					speedBps = speedBps > 0.0 ? speedBps * 0.7 + instantBps * 0.3 : instantBps;
					lastSpeedUpdate = now;
					bytesAtLastUpdate = sessionTotalBytes;
				}

				if (now - lastProgressFire >= progressIntervalMs)
				{
					lastProgressFire = now;

					const juce::int64 totalDownloaded = completedBytes + fileDownloaded;
					double eta = -1.0;
					const juce::int64 remaining = totalExpected - totalDownloaded;
					if (speedBps > 0.0 && remaining > 0)
						eta = (double)remaining / speedBps;

					AssetDownloadProgress progress;
					progress.currentFileIndex = i;
					progress.totalFiles = (int)files_.size();
					progress.currentFilename = asset.filename;
					progress.currentFileBytes = fileDownloaded;
					progress.currentFileTotalBytes = fileTotalBytes;
					progress.totalBytes = totalDownloaded;
					progress.totalExpectedBytes = totalExpected;
					progress.speedBytesPerSec = speedBps;
					progress.etaSeconds = eta;

					fireProgress(progress);
				}
			}

			out->flush();
			out.reset();

			if (cancelledMidFile)
			{
				fireComplete(false, "Download cancelled.");
				return;
			}

			if (writeError)
			{
				fireComplete(false, "Write error (disk full?) on: " + partFile.getFullPathName());
				return;
			}

			const juce::int64 finalSize = partFile.getSize();
			const juce::int64 mustBe = asset.expectedSizeBytes > 0 ? asset.expectedSizeBytes : fileTotalBytes;

			if (mustBe > 0 && finalSize != mustBe)
			{
				lastError = "Incomplete download of " + asset.filename + " (" + juce::String(finalSize) + " / " +
				            juce::String(mustBe) + " bytes)";
				if (attempt < maxAttemptsPerFile && !interruptibleWait(1000 * attempt))
				{
					fireComplete(false, "Download cancelled.");
					return;
				}
				continue;
			}

			if (destFile.existsAsFile())
				destFile.deleteFile();

			bool moved = false;
			for (int m = 0; m < 5 && !moved; ++m)
			{
				if (m > 0 && !interruptibleWait(500))
				{
					fireComplete(false, "Download cancelled.");
					return;
				}
				if (destFile.existsAsFile())
					destFile.deleteFile();
				moved = partFile.moveFileTo(destFile);
			}

			if (!moved)
			{
				fireComplete(false, "Cannot move file to: " + destFile.getFullPathName() +
				                        " - the file may be locked. Close other plugin instances and try again.");
				return;
			}

			completedBytes += finalSize;
			fileOk = true;
		}

		if (!fileOk)
		{
			fireComplete(false, lastError.isNotEmpty()
			                        ? lastError + " (after " + juce::String(maxAttemptsPerFile) + " attempts)"
			                        : "Download failed: " + asset.url);
			return;
		}
	}

	AssetDownloadProgress progress;
	progress.currentFileIndex = (int)files_.size() - 1;
	progress.totalFiles = (int)files_.size();
	progress.currentFilename = files_.empty() ? juce::String() : files_.back().filename;
	progress.currentFileBytes = files_.empty() ? 0 : files_.back().expectedSizeBytes;
	progress.currentFileTotalBytes = progress.currentFileBytes;
	progress.totalBytes = completedBytes;
	progress.totalExpectedBytes = totalExpected;
	progress.speedBytesPerSec = 0.0;
	progress.etaSeconds = 0.0;
	fireProgress(progress);

	fireComplete(true, {});
}

void AssetDownloadManager::fireProgress(const AssetDownloadProgress &p)
{
	if (onProgress_)
	{
		auto cb = onProgress_;
		juce::MessageManager::callAsync([cb, p]() { cb(p); });
	}
}

void AssetDownloadManager::fireComplete(bool success, const juce::String &error)
{
	if (onComplete_)
	{
		auto cb = onComplete_;
		onComplete_ = nullptr;
		onProgress_ = nullptr;
		juce::MessageManager::callAsync([cb, success, error]() { cb(success, error); });
	}
}