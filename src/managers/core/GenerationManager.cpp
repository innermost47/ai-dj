#include "GenerationManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "StableAudioEngine.h"
#include "TrackData.h"

GenerationManager::GenerationManager(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

void GenerationManager::generateLoop(const DjIaClient::LoopRequest &request, const juce::String &targetTrackId)
{
	TrackData *track = audioProcessor.getTrack(targetTrackId);
	if (track)
	{
		track->stagingTargetPageIndex.store(track->currentPageIndex.load());
		track->isInGeneratingProcess.store(true);
	}

	try
	{
		if (audioProcessor.getUseLocalModel())
		{
			generateLoopLocal(request, targetTrackId);
		}
		else
		{
			DjIaClient::LoopRequest apiRequest = request;
			generateLoopAPI(apiRequest, targetTrackId);
		}
	}
	catch (const std::exception &e)
	{
		audioProcessor.setHasPendingAudioData(false);
		audioProcessor.setWaitingForMidiToLoad(false);
		audioProcessor.clearTrackIdWaitingForLoad();
		audioProcessor.setCorrectMidiNoteReceived(false);
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		if (track)
		{
			track->stagingTargetPageIndex.store(-1);
		}
		reEnableCanvasGenerate();
		notifyGenerationComplete(targetTrackId, "Error: " + juce::String(e.what()));
	}
}

void GenerationManager::generateLoopAPI(const DjIaClient::LoopRequest &request, const juce::String &trackId)
{
	if (TrackData *t = audioProcessor.getTrack(trackId))
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackGenerate(t->slotIndex + 1),
		                                                 MidiMapping::feedbackActive);

	auto response = audioProcessor.getApiClient().generateLoop(
	    request, audioProcessor.getHostSampleRate(), audioProcessor.getRequestTimeout(), audioProcessor.getBypassLLM());

	try
	{
		if (!response.errorMessage.isEmpty())
		{
			audioProcessor.setIsGenerating(false);
			audioProcessor.setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "ERROR: " + response.errorMessage);
			return;
		}

		if (response.audioData.getFullPathName().isEmpty() || !response.audioData.exists() ||
		    response.audioData.getSize() == 0)
		{
			audioProcessor.setIsGenerating(false);
			audioProcessor.setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "Invalid response from API");
			return;
		}
	}
	catch (const std::exception & /*e*/)
	{
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "Response validation failed");
		return;
	}

	auto preprocessResult =
	    audioProcessor.getAudioManager().preprocessAudioFile(response.audioData, response.snappedBpm, trackId);

	{
		const juce::ScopedLock lock(apiLock);
		audioProcessor.setPendingTrackId(trackId);
		audioProcessor.setPendingAudioFile(preprocessResult.success ? preprocessResult.stretchedFile
		                                                            : response.audioData);
		audioProcessor.setPendingDetectedBpm(response.detectedBpm);
		audioProcessor.setPendingSnappedBpm(-1.0f);
		audioProcessor.setHasPendingAudioData(true);
		audioProcessor.setWaitingForMidiToLoad(true);
		audioProcessor.setTrackIdWaitingForLoad(trackId);
		audioProcessor.setCorrectMidiNoteReceived(false);
	}
	if (!audioProcessor.getIsLoadingState())
		if (TrackData *track = audioProcessor.getTrack(trackId))
		{
			track->preprocessHasOriginal.store(preprocessResult.hasOriginalVersion);
			track->preprocessOriginalBpm.store(preprocessResult.originalBpm);
			track->skipBpmSync.store(true);
			track->skipDiskReload.store(preprocessResult.stagingFilled);
			track->hasSamplePending.store(true);

			auto &currentPage = track->getCurrentPage();
			currentPage.prompt = request.prompt;
			currentPage.bpm = request.bpm;
		}

	audioProcessor.setIsGenerating(false);
	audioProcessor.setGeneratingTrackId("");
	reEnableCanvasGenerate();

	juce::String successMessage = "Loop generated successfully! Press Play to listen.";
	if (response.isUnlimitedKey)
	{
		successMessage += " - Unlimited API key";
	}
	else if (response.creditsRemaining >= 0)
	{
		successMessage += " - " + juce::String(response.creditsRemaining) + " credits remaining";
	}

	notifyGenerationComplete(trackId, successMessage);
}

void GenerationManager::generateLoopLocal(const DjIaClient::LoopRequest &request, const juce::String &trackId)
{
	if (TrackData *t = audioProcessor.getTrack(trackId))
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackGenerate(t->slotIndex + 1),
		                                                 MidiMapping::feedbackActive);

	auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                      .getChildFile(Obsidian::OBSIDIAN_BASE_DIR);
	auto stableAudioDir = appDataDir.getChildFile(Obsidian::STABLE_AUDIO_DIR);

	StableAudioEngine localEngine;
	if (!localEngine.initialize(stableAudioDir.getFullPathName()))
	{
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "ERROR: Local models not found. Please check setup instructions.");
		return;
	}

	StableAudioEngine::GenerationParams params(request.prompt, 6.0f);
	params.sampleRate = static_cast<int>(audioProcessor.getHostSampleRate());
	params.numThreads = 4;

	auto result = localEngine.generateSample(params);

	if (!result.success || result.audioData.empty())
	{
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "ERROR: Local generation failed - " + result.errorMessage);
		return;
	}

	juce::File tempFile = juce::File::createTempFile(".wav");
	juce::FileOutputStream stream(tempFile);
	if (stream.openedOk())
		stream.write(result.audioData.data(), result.audioData.size());
	else
	{
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		reEnableCanvasGenerate();
		notifyGenerationComplete(trackId, "ERROR: Failed to create audio file");
		return;
	}
	stream.flush();

	auto preprocessResult = audioProcessor.getAudioManager().preprocessAudioFile(tempFile, -1.0f, trackId);

	{
		const juce::ScopedLock lock(apiLock);
		audioProcessor.setPendingTrackId(trackId);
		audioProcessor.setPendingAudioFile(preprocessResult.success ? preprocessResult.stretchedFile : tempFile);
		audioProcessor.setPendingDetectedBpm(request.bpm);
		audioProcessor.setPendingSnappedBpm(-1.0f);
		audioProcessor.setHasPendingAudioData(true);
		audioProcessor.setWaitingForMidiToLoad(true);
		audioProcessor.setTrackIdWaitingForLoad(trackId);
		audioProcessor.setCorrectMidiNoteReceived(false);
	}

	if (!audioProcessor.getIsLoadingState())
		if (TrackData *track = audioProcessor.getTrack(trackId))
		{
			track->preprocessHasOriginal.store(preprocessResult.hasOriginalVersion);
			track->preprocessOriginalBpm.store(preprocessResult.originalBpm);
			track->skipBpmSync.store(true);
			track->skipDiskReload.store(preprocessResult.stagingFilled);
			track->hasSamplePending.store(true);

			auto &currentPage = track->getCurrentPage();
			currentPage.prompt = request.prompt;
			currentPage.bpm = request.bpm;
		}

	audioProcessor.setIsGenerating(false);
	audioProcessor.setGeneratingTrackId("");
	reEnableCanvasGenerate();

	juce::String successMessage =
	    juce::String::formatted("Loop generated locally! (%.1fs) Press Play to listen.", result.actualDuration);

	notifyGenerationComplete(trackId, successMessage);
}

void GenerationManager::notifyGenerationComplete(const juce::String &trackId, const juce::String &message)
{

	audioProcessor.setLastGeneratedTrackId(trackId);
	audioProcessor.setPendingMessage(message);
	audioProcessor.setHasPendingNotification(true);
	audioProcessor.triggerAsyncUpdate();
	if (TrackData *t = audioProcessor.getTrack(trackId))
		audioProcessor.getMidiManager().sendMidiFeedback(MidiMapping::ccFeedbackGenerate(t->slotIndex + 1),
		                                                 MidiMapping::feedbackIdle);
}

void GenerationManager::generateSampleWithImage(const juce::String &trackId, const juce::String &base64Image,
                                                const juce::StringArray &keywords)
{
	if (audioProcessor.getIsGenerating() || audioProcessor.getIsLoadingState())
		return;

	TrackData *track = audioProcessor.getTrack(trackId);
	if (!track)
		return;

	track->stagingTargetPageIndex.store(track->currentPageIndex.load());

	audioProcessor.setIsGenerating(true);
	audioProcessor.setGeneratingTrackId(trackId);

	juce::MessageManager::callAsync(
	    [this, trackId]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		    {
			    editor->uiGenerationManager->startGenerationUI(trackId);
			    editor->statusLabel.setText("Analyzing image and generating audio...", juce::dontSendNotification);
			    editor->uiStatusManager->updateLCD();
		    }
	    });

	juce::Thread::launch(
	    [this, trackId, base64Image, keywords]()
	    {
		    try
		    {
			    TrackData *track = audioProcessor.getTrack(trackId);
			    if (!track)
				    throw std::runtime_error("Track not found");

			    DjIaClient::LoopRequest request;
			    float hostBpm = static_cast<float>(audioProcessor.getHostBpm());
			    float fallbackBpm = hostBpm > 0 ? hostBpm : 127.0f;

			    auto &currentPage = track->getCurrentPage();
			    request.model = currentPage.selectedModel;
			    request.bpm = fallbackBpm;
			    request.key =
			        !currentPage.generationKey.isEmpty() ? currentPage.generationKey : audioProcessor.getGlobalKey();
			    request.generationDuration = currentPage.generationDuration > 0
			                                     ? (float)currentPage.generationDuration
			                                     : static_cast<float>(audioProcessor.getGlobalDuration());

			    if (request.model.isEmpty())
				    request.model = Obsidian::STABLE_AUDIO_OPEN_V1;

			    if (request.bpm <= 0)
				    request.bpm = 127.0f;
			    if (request.key.isEmpty())
				    request.key = "C Minor";
			    if (request.generationDuration <= 0)
				    request.generationDuration = 6.0f;

			    request.prompt = "";
			    request.useImage = true;
			    request.imageBase64 = base64Image;
			    request.keywords = keywords;

			    generateLoopWithImage(request, trackId, 300000);
		    }
		    catch (const std::exception &e)
		    {
			    audioProcessor.setIsGenerating(false);
			    audioProcessor.setGeneratingTrackId("");

			    if (TrackData *track = audioProcessor.getTrack(trackId))
			    {
				    track->stagingTargetPageIndex.store(-1);
			    }

			    juce::String errorMessage = juce::String(e.what());

			    juce::MessageManager::callAsync(
			        [this, trackId, errorMessage]()
			        {
				        if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				        {
					        editor->uiGenerationManager->stopGenerationUI(trackId, false, errorMessage);
				        }
			        });
		    }
	    });
}

void GenerationManager::generateLoopWithImage(const DjIaClient::LoopRequest &request, const juce::String &trackId,
                                              int timeoutMS)
{
	auto response =
	    audioProcessor.getApiClient().generateLoop(request, audioProcessor.getHostSampleRate(), timeoutMS, false);

	try
	{
		if (!response.errorMessage.isEmpty())
		{
			audioProcessor.setIsGenerating(false);
			audioProcessor.setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "ERROR: " + response.errorMessage);
			return;
		}

		if (response.audioData.getFullPathName().isEmpty() || !response.audioData.exists() ||
		    response.audioData.getSize() == 0)
		{
			audioProcessor.setIsGenerating(false);
			audioProcessor.setGeneratingTrackId("");
			reEnableCanvasGenerate();
			notifyGenerationComplete(trackId, "Invalid response from API");
			return;
		}
	}
	catch (const std::exception & /*e*/)
	{
		audioProcessor.setIsGenerating(false);
		audioProcessor.setGeneratingTrackId("");
		notifyGenerationComplete(trackId, "Response validation failed");
		return;
	}

	{
		const juce::ScopedLock lock(apiLock);
		audioProcessor.setPendingTrackId(trackId);
		audioProcessor.setPendingAudioFile(response.audioData);
		audioProcessor.setPendingDetectedBpm(response.detectedBpm);
		audioProcessor.setPendingSnappedBpm(response.snappedBpm);
		audioProcessor.setHasPendingAudioData(true);
		audioProcessor.setWaitingForMidiToLoad(true);
		audioProcessor.setTrackIdWaitingForLoad(trackId);
		audioProcessor.setCorrectMidiNoteReceived(false);
	}

	if (TrackData *track = audioProcessor.getTrack(trackId))
	{
		juce::String generatedPrompt = "Generated from image";

		auto &currentPage = track->getCurrentPage();
		currentPage.prompt = generatedPrompt;
		currentPage.generationPrompt = generatedPrompt;
		currentPage.generationKey = response.key;
	}

	audioProcessor.setIsGenerating(false);
	audioProcessor.setGeneratingTrackId("");
	reEnableCanvasGenerate();

	juce::String successMessage = "Audio generated from image! Press Play to listen.";

	if (response.isUnlimitedKey)
	{
		successMessage += " - Unlimited API key";
	}
	else if (response.creditsRemaining >= 0)
	{
		successMessage += " - " + juce::String(response.creditsRemaining) + " credits remaining";
	}

	notifyGenerationComplete(trackId, successMessage);
}

void GenerationManager::reEnableCanvasGenerate()
{
	juce::MessageManager::callAsync(
	    [this]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		    {
			    editor->uiGenerationManager->setAllGenerateButtonsEnabled(true);
		    }
	    });
}

void GenerationManager::generateLoopFromMidi(const juce::String &trackId)
{
	if (audioProcessor.getIsGenerating() || audioProcessor.getIsLoadingState())
		return;

	TrackData *track = audioProcessor.getTrack(trackId);
	if (!track)
		return;

	track->stagingTargetPageIndex.store(track->currentPageIndex.load());

	audioProcessor.setIsGenerating(true);
	audioProcessor.setGeneratingTrackId(trackId);

	juce::MessageManager::callAsync(
	    [this, trackId]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
		    {
			    editor->uiGenerationManager->startGenerationUI(trackId);
		    }
	    });

	juce::Thread::launch(
	    [this, trackId]()
	    {
		    try
		    {
			    TrackData *track = audioProcessor.getTrack(trackId);
			    if (!track)
			    {
				    throw std::runtime_error("Track not found");
			    }

			    DjIaClient::LoopRequest request;
			    request.generationDuration = static_cast<float>(audioProcessor.getGlobalDuration());
			    float currentHostBpm = static_cast<float>(audioProcessor.getHostBpm());

			    auto &currentPage = track->getCurrentPage();
			    request.model = currentPage.selectedModel;
			    if (!currentPage.selectedPrompt.isEmpty())
			    {
				    request.prompt = currentPage.selectedPrompt;
				    request.bpm = currentHostBpm;
				    request.key = !currentPage.generationKey.isEmpty() ? currentPage.generationKey
				                                                       : audioProcessor.getGlobalKey();
			    }
			    else
			    {
				    request = audioProcessor.createGlobalLoopRequest();
				    if (currentPage.selectedModel.isNotEmpty())
					    request.model = currentPage.selectedModel;
				    currentPage.setSelectedPrompt(request.prompt);
				    currentPage.generationBpm = currentHostBpm;
				    currentPage.generationKey = request.key;
			    }

			    if (request.model.isEmpty())
				    request.model = Obsidian::STABLE_AUDIO_OPEN_V1;

			    juce::String promptSource = !request.prompt.isEmpty()
			                                    ? "track prompt: " + request.prompt.substring(0, 20) + "..."
			                                    : "global prompt";
			    juce::MessageManager::callAsync(
			        [this, promptSource]()
			        {
				        if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				        {
					        editor->statusLabel.setText("Generating with " + promptSource, juce::dontSendNotification);
					        editor->uiStatusManager->updateLCD();
				        }
			        });
			    generateLoop(request, trackId);
		    }
		    catch (const std::exception &e)
		    {
			    audioProcessor.setIsGenerating(false);
			    audioProcessor.setGeneratingTrackId("");

			    if (TrackData *track = audioProcessor.getTrack(trackId))
			    {
				    track->stagingTargetPageIndex.store(-1);
			    }

			    juce::MessageManager::callAsync(
			        [this, trackId, error = juce::String(e.what())]()
			        {
				        if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
				        {
					        editor->uiGenerationManager->stopGenerationUI(trackId, false, error);
				        }
			        });
		    }
	    });
}

void GenerationManager::handleGenerate()
{
	if (audioProcessor.getIsGenerating())
		return;
	int changedSlot = audioProcessor.getMidiLearnManager().changedGenerateSlotIndex.load();
	if (changedSlot >= 0)
	{
		auto trackIds = audioProcessor.getAllTrackIds();
		for (const auto &trackId : trackIds)
		{
			TrackData *track = audioProcessor.getTrack(trackId);
			if (track->slotIndex == changedSlot)
			{
				bool paramGenerate = audioProcessor.getParameterManager().getGenerate(track->slotIndex) > 0.5f;
				if (paramGenerate)
				{
					generateLoopFromMidi(trackId);
					audioProcessor.needsUIUpdate.store(true);
				}
				break;
			}
		}
		audioProcessor.getMidiLearnManager().changedGenerateSlotIndex.store(-1);
	}
}

void GenerationManager::clearPendingAudio()
{
	const juce::ScopedLock lock(apiLock);
	audioProcessor.setPendingAudioFile(juce::File());
	audioProcessor.clearPendingTrackId();
	audioProcessor.setHasPendingAudioData(false);
}
