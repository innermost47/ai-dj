#include "LocalGenerationEngine.h"
#include "PluginProcessor.h"
#include "StableAudioEngine.h"

class StableAudioLocalGenerationEngine : public LocalGenerationEngine
{
  public:
	StableAudioLocalGenerationEngine(DjIaVstProcessor &processor) : audioProcessor(processor)
	{
	}

	LocalGenerationResult generate(const DjIaClient::LoopRequest &request, double hostSampleRate) override
	{
		LocalGenerationResult result;

		auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		                      .getChildFile(Obsidian::OBSIDIAN_BASE_DIR());
		auto stableAudioDir = appDataDir.getChildFile(Obsidian::STABLE_AUDIO_DIR());

		StableAudioEngine engine;
		if (!engine.initialize(stableAudioDir.getFullPathName()))
		{
			result.errorMessage = "Local models not found. Please check setup instructions.";
			return result;
		}

		StableAudioEngine::GenerationParams params(request.prompt, 6.0f);
		params.sampleRate = static_cast<int>(hostSampleRate);
		params.numThreads = 4;

		auto genResult = engine.generateSample(params);
		if (!genResult.success || genResult.audioData.empty())
		{
			result.errorMessage = genResult.errorMessage;
			return result;
		}

		juce::File tempWavFile = juce::File::createTempFile(".wav");

		juce::WavAudioFormat wavFormat;
		juce::FileOutputStream *fileStream = new juce::FileOutputStream(tempWavFile);
		if (!fileStream->openedOk())
		{
			delete fileStream;
			result.errorMessage = "Cannot create temp WAV file";
			return result;
		}

		std::unique_ptr<juce::AudioFormatWriter> writer(
		    wavFormat.createWriterFor(fileStream, hostSampleRate, 1, 16, {}, 0));
		if (writer == nullptr)
		{
			delete fileStream;
			result.errorMessage = "Cannot create WAV writer";
			return result;
		}

		juce::AudioBuffer<float> buffer(1, static_cast<int>(genResult.audioData.size()));
		buffer.copyFrom(0, 0, genResult.audioData.data(), static_cast<int>(genResult.audioData.size()));

		if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
		{
			writer.reset();
			result.errorMessage = "Failed to write WAV data";
			return result;
		}
		writer.reset();

		juce::MemoryBlock wavBytes;
		tempWavFile.loadFileAsData(wavBytes);
		tempWavFile.deleteFile();

		result.audioData.assign(static_cast<const uint8_t *>(wavBytes.getData()),
		                        static_cast<const uint8_t *>(wavBytes.getData()) + wavBytes.getSize());
		result.actualDuration = genResult.actualDuration;
		result.success = true;
		return result;
	}

  private:
	DjIaVstProcessor &audioProcessor;
};

std::unique_ptr<LocalGenerationEngine> createLocalGenerationEngine(DjIaVstProcessor &processor)
{
	return std::make_unique<StableAudioLocalGenerationEngine>(processor);
}