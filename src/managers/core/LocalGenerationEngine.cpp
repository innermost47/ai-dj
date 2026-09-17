#include "LocalGenerationEngine.h"
#include "PluginProcessor.h"
#include "SA3PipeLine.h"
#include <filesystem>

class SA3LocalGenerationEngine : public LocalGenerationEngine
{
  public:
	SA3LocalGenerationEngine(DjIaVstProcessor &processor) : audioProcessor(processor)
	{
	}

	~SA3LocalGenerationEngine() override
	{
		destroyLocalModel();
	}

	LocalGenerationResult generate(const DjIaClient::LoopRequest &request, double hostSampleRate) override
	{
		initLocalModel();

		LocalGenerationResult result;
		std::vector<uint8_t> wavBytes;
		if (sa3Pipeline && sa3Pipeline->isReady())
		{
			juce::String fullPrompt = request.prompt + ", " + request.key + ", " + juce::String(request.bpm);
			std::string prompt = fullPrompt.toStdString();
			int hostRate = (int)hostSampleRate;
			wavBytes = sa3Pipeline->generate(prompt, request.generationDuration, hostRate);
		}

		if (wavBytes.empty())
		{
			result.errorMessage = "Local generation failed";
			return result;
		}

		result.audioData = std::move(wavBytes);
		result.actualDuration = request.generationDuration;
		result.success = true;
		return result;
	}

  private:
	void initLocalModel()
	{
		if (audioProcessor.isLocalModelInitialized())
			return;
		auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		                      .getChildFile(Obsidian::OBSIDIAN_BASE_DIR());
		auto stableAudioDir = appDataDir.getChildFile(Obsidian::STABLE_AUDIO_DIR());
		sa3Pipeline = std::make_unique<SA3PipeLine>();
		std::filesystem::path modelsPath = std::filesystem::u8path(stableAudioDir.getFullPathName().toStdString());
		if (audioProcessor.getUseLocalModel())
		{
			sa3Pipeline->initModel(modelsPath);
			audioProcessor.setLocalModelInitialized(true);
		}
	}

	void destroyLocalModel()
	{
		if (sa3Pipeline)
		{
			sa3Pipeline = nullptr;
			audioProcessor.setLocalModelInitialized(false);
		}
	}

	DjIaVstProcessor &audioProcessor;
	std::unique_ptr<SA3PipeLine> sa3Pipeline;
};

std::unique_ptr<LocalGenerationEngine> createLocalGenerationEngine(DjIaVstProcessor &processor)
{
	return std::make_unique<SA3LocalGenerationEngine>(processor);
}