#include "StableAudioEngine.h"

#ifdef _WIN32
#include <direct.h>
#define GETCWD _getcwd
#define CHDIR _chdir
#else
#include <unistd.h>
#define GETCWD getcwd
#define CHDIR chdir
#endif

bool StableAudioEngine::initialize(const juce::String& modelsDir)
{
	try
	{
		DBG("Initializing Stable Audio Engine (subprocess mode)...");
		modelsDirectory = modelsDir;

		auto baseDir = juce::File(modelsDir);

#ifdef _WIN32
		audiogenExecutable = baseDir.getChildFile("audiogen.exe");
#else
		audiogenExecutable = baseDir.getChildFile("audiogen");
#endif

		if (!audiogenExecutable.exists())
		{
			return false;
		}

		if (!checkRequiredFiles())
		{
			return false;
		}

		isInitialized = true;
		return true;
	}
	catch (const std::exception& /*e*/)
	{
		DBG("Exception during Stable Audio Engine initialization");
		return false;
	}
}

bool StableAudioEngine::checkRequiredFiles()
{
	auto baseDir = juce::File(modelsDirectory);

	juce::StringArray missingFiles;

	if (!audiogenExecutable.exists())
		missingFiles.add("audiogen executable");
	if (!baseDir.getChildFile("conditioners_float32.tflite").exists())
		missingFiles.add("conditioners_float32.tflite");
	if (!baseDir.getChildFile("dit_model.tflite").exists())
		missingFiles.add("dit_model.tflite");
	if (!baseDir.getChildFile("autoencoder_model.tflite").exists())
		missingFiles.add("autoencoder_model.tflite");
	if (!baseDir.getChildFile("spiece.model").exists())
		missingFiles.add("spiece.model");

	if (!missingFiles.isEmpty())
	{
		DBG("Missing required files: " << missingFiles.joinIntoString(", "));
		return false;
	}

	return true;
}

StableAudioEngine::GenerationResult StableAudioEngine::generateSample(const GenerationParams& params)
{
	GenerationResult result;

	if (!isInitialized)
	{
		result.errorMessage = "Engine not initialized";
		return result;
	}

	try
	{
		DBG("Generating audio: '" << params.prompt << "' (" << params.duration << "s)");

		auto startTime = juce::Time::getMillisecondCounterHiRes();
		cleanupTempFiles();

		auto sanitizedPrompt = sanitizePrompt(params.prompt);
		auto seed = (params.seed == -1) ? generateRandomSeed() : params.seed;

		juce::StringArray command;

#ifdef _WIN32
		command.add(audiogenExecutable.getFullPathName());
		command.add(modelsDirectory);
		command.add(sanitizedPrompt);
		command.add(juce::String(params.numThreads));
		command.add(juce::String(seed));
#else
		command.add(audiogenExecutable.getFullPathName());
		command.add("-m");
		command.add(modelsDirectory);
		command.add("-p");
		command.add(sanitizedPrompt);
		command.add("-t");
		command.add(juce::String(params.numThreads));
		command.add("-s");
		command.add(juce::String(seed));
#endif

		DBG("Executing command: " << command.joinIntoString(" "));

		char originalDir[1024];
		GETCWD(originalDir, sizeof(originalDir));

		auto workingDir = juce::File(modelsDirectory);
		DBG("Changing working directory to: " << workingDir.getFullPathName());

		if (CHDIR(workingDir.getFullPathName().toRawUTF8()) != 0)
		{
			result.errorMessage = "Failed to change working directory";
			return result;
		}

		juce::ChildProcess childProcess;

		if (!childProcess.start(command))
		{
			CHDIR(originalDir);
			result.errorMessage = "Failed to start audiogen process";
			return result;
		}

		if (!childProcess.waitForProcessToFinish(600000))
		{
			childProcess.kill();
			CHDIR(originalDir);
			result.errorMessage = "Process timeout (10min)";
			return result;
		}

		CHDIR(originalDir);

		auto exitCode = childProcess.getExitCode();
		if (exitCode != 0)
		{
			result.errorMessage = "Process failed with exit code: " + juce::String(exitCode);
			return result;
		}

		auto outputFile = juce::File(modelsDirectory).getChildFile("output.wav");
		if (!outputFile.exists())
		{
			result.errorMessage = "Output file not found: " + outputFile.getFullPathName();
			return result;
		}

		auto audioData = loadAndResampleWavFile(outputFile, params.sampleRate);
		if (audioData.empty())
		{
			result.errorMessage = "Failed to load and resample generated audio file";
			return result;
		}

		auto endTime = juce::Time::getMillisecondCounterHiRes();
		result.audioData = audioData;
		result.leftChannel = audioData;
		result.rightChannel = audioData;
		result.actualDuration = static_cast<float>(audioData.size()) / params.sampleRate;
		result.success = true;
		result.performanceInfo = "Generated in " + juce::String(endTime - startTime, 0) + "ms";

		DBG("Generation successful: " << result.audioData.size() << " samples in "
			<< (endTime - startTime) << "ms");
		outputFile.deleteFile();

		return result;
	}
	catch (const std::exception& e)
	{
		result.errorMessage = juce::String("Generation failed: ") + e.what();
		DBG("Generation exception: " << e.what());
		return result;
	}
}

std::vector<float> StableAudioEngine::loadAndResampleWavFile(const juce::File& wavFile, double targetSampleRate)
{
	std::vector<float> audioData;

	try
	{
		juce::AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		auto* reader = formatManager.createReaderFor(wavFile);
		if (reader == nullptr)
		{
			DBG("Failed to create audio reader for: " << wavFile.getFullPathName());
			return audioData;
		}

		std::unique_ptr<juce::AudioFormatReader> readerPtr(reader);

		auto numSamples = static_cast<int>(reader->lengthInSamples);
		auto numChannels = static_cast<int>(reader->numChannels);
		double originalSampleRate = reader->sampleRate;

		juce::AudioBuffer<float> buffer(numChannels, numSamples);
		reader->read(&buffer, 0, numSamples, 0, true, true);

		juce::AudioBuffer<float> monoBuffer(1, numSamples);
		if (numChannels == 1)
		{
			monoBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
		}
		else
		{
			for (int sample = 0; sample < numSamples; ++sample)
			{
				float left = buffer.getSample(0, sample);
				float right = buffer.getSample(1, sample);
				monoBuffer.setSample(0, sample, (left + right) * 0.5f);
			}
		}

		DBG("Original: " << numSamples << " samples at " << originalSampleRate << "Hz");
		if (std::abs(originalSampleRate - targetSampleRate) > 1.0)
		{
			DBG("Resampling from " << originalSampleRate << "Hz to " << targetSampleRate << "Hz");

			juce::AudioBuffer<float> resampledBuffer = resampleBuffer(monoBuffer, originalSampleRate, targetSampleRate);

			audioData.reserve(resampledBuffer.getNumSamples());
			for (int sample = 0; sample < resampledBuffer.getNumSamples(); ++sample)
			{
				audioData.push_back(resampledBuffer.getSample(0, sample));
			}

			DBG("Resampled to: " << audioData.size() << " samples at " << targetSampleRate << "Hz");
		}
		else
		{
			audioData.reserve(numSamples);
			for (int sample = 0; sample < numSamples; ++sample)
			{
				audioData.push_back(monoBuffer.getSample(0, sample));
			}
			DBG("No resampling needed: " << audioData.size() << " samples");
		}
	}
	catch (const std::exception& /*e*/)
	{
		DBG("Exception loading/resampling WAV file in Stable Audio Engine");
		audioData.clear();
	}

	return audioData;
}

juce::AudioBuffer<float> StableAudioEngine::resampleBuffer(const juce::AudioBuffer<float>& inputBuffer,
	double inputSampleRate,
	double outputSampleRate)
{
	double ratio = outputSampleRate / inputSampleRate;
	int outputNumSamples = static_cast<int>(inputBuffer.getNumSamples() * ratio);

	juce::AudioBuffer<float> outputBuffer(1, outputNumSamples);
	juce::LagrangeInterpolator interpolator;
	interpolator.reset();

	const float* inputData = inputBuffer.getReadPointer(0);
	float* outputData = outputBuffer.getWritePointer(0);

	interpolator.process(1.0 / ratio, inputData, outputData, outputNumSamples, inputBuffer.getNumSamples(), 0);

	return outputBuffer;
}

std::vector<float> StableAudioEngine::generateAudio(const juce::String& prompt, float duration)
{
	GenerationParams params(prompt, duration);
	auto result = generateSample(params);
	return result.isValid() ? result.audioData : std::vector<float>();
}

juce::String StableAudioEngine::sanitizePrompt(const juce::String& prompt)
{
	auto sanitized = prompt.replace("\"", "\\\"")
		.replace("'", "\\'")
		.replace("|", "")
		.replace("&", "and")
		.replace(";", "");
	if (sanitized.length() > 200)
	{
		sanitized = sanitized.substring(0, 200);
	}

	return sanitized;
}

int StableAudioEngine::generateRandomSeed()
{
	return random.nextInt(1000000);
}

void StableAudioEngine::cleanupTempFiles()
{
	auto outputFile = juce::File(modelsDirectory).getChildFile("output.wav");
	if (outputFile.exists())
	{
		outputFile.deleteFile();
	}
}