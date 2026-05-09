#include "UITrackManager.h"
#include "AiModelDefinitions.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"

UITrackManager::UITrackManager(DjIaVstEditor &editor) : editor(editor)
{
}

UITrackManager::~UITrackManager()
{
	for (auto &tc : trackComponents)
		if (tc)
			tc->setVisible(false);
	trackComponents.clear();
}

void UITrackManager::refreshTracks()
{
	trackComponents.clear();
	editor.uiLayoutManager->getTracksContainer()->removeAllChildren();
	refreshTrackComponents();
	for (auto &trackComp : trackComponents)
		trackComp->loadPromptPresets();
	editor.repaint();
}

void UITrackManager::refreshTrackComponents()
{
	if (editor.isBeingDestroyed.load())
		return;
	if (editor.audioProcessor.getTrackManager().isInitializing.load())
	{
		juce::Timer::callAfterDelay(50, [this]() { refreshTrackComponents(); });
		return;
	}
	auto trackIds = editor.audioProcessor.getAllTrackIds();

	if (trackComponents.size() == trackIds.size())
	{
		bool allVisible = true;
		for (auto &comp : trackComponents)
		{
			if (!comp->isVisible() || comp->getParentComponent() == nullptr)
			{
				allVisible = false;
				break;
			}
		}
		if (allVisible)
		{
			for (int i = 0; i < (int)trackComponents.size() && i < (int)trackIds.size(); ++i)
			{
				trackComponents[i]->setTrackData(editor.audioProcessor.getTrack(trackIds[i]));

				trackComponents[i]->updateFromTrackData();
				if (auto *sequencer = trackComponents[i]->getSequencer())
					sequencer->updateFromTrackData();
			}
			return;
		}
	}

	juce::String generatingId = editor.audioProcessor.getGeneratingTrackId();
	bool wasGeneratingLocal = editor.audioProcessor.getIsGenerating();

	trackComponents.clear();
	editor.uiLayoutManager->getTracksContainer()->removeAllChildren();

	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = editor.audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;

		auto trackComp = std::make_unique<TrackComponent>(trackId, editor.audioProcessor);
		trackComp->setTrackData(trackData);

		trackComp->onGenerateWithImage =
		    [this](const juce::String &trackId, const juce::String &image, const juce::StringArray &keywords)
		{ editor.audioProcessor.getGenerationManager().generateSampleWithImage(trackId, image, keywords); };

		trackComp->onTrackRenamed = [this](const juce::String &id, const juce::String &newName)
		{
			if (editor.mixerPanel)
			{
				editor.mixerPanel->updateTrackName(id, newName);
			}
		};

		trackComp->onModelChanged = [this](const juce::String &id)
		{
			if (editor.mixerPanel)
			{
				editor.mixerPanel->updateModelUI(id);
			}
		};

		trackComp->onGenerateForTrack = [this](const juce::String &id)
		{ editor.uiGenerationManager->generateFromTrackComponent(id); };

		trackComp->onPreviewTrack = [this](const juce::String &trackId)
		{ editor.audioProcessor.previewTrack(trackId); };

		trackComp->onTrackPromptChanged = [this](const juce::String /*&trackId*/, const juce::String &prompt)
		{
			editor.uiStatusManager->setStatusWithTimeout("Track prompt updated: " + prompt.substring(0, 20) + "...",
			                                             3000);
		};

		trackComp->onStatusMessage = [this](const juce::String &message)
		{ editor.uiStatusManager->setStatusWithTimeout(message, 3000); };

		trackComp->onStopPreview = [this](const juce::String &trackId)
		{ editor.audioProcessor.getAudioManager().stopTrackPreview(trackId); };

		if (wasGeneratingLocal && trackId == generatingId)
		{
			trackComp->startGeneratingAnimation();
		}

		editor.uiLayoutManager->getTracksContainer()->addAndMakeVisible(trackComp.get());
		trackComponents.push_back(std::move(trackComp));
	}

	editor.uiLayoutManager->getTracksContainer()->resized();

	if (editor.mixerPanel)
	{
		editor.mixerPanel->refreshMixerChannels();
	}

	juce::MessageManager::callAsync(
	    [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
	    {
		    if (auto *e = safeEditor.getComponent())
		    {
			    e->resized();
			    e->repaint();
		    }
	    });
	editor.uiLayoutManager->getTracksContainer()->repaint();
}

void UITrackManager::onSampleLoaded(const juce::String &trackId)
{
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->setSamplePending(false);
			trackComp->updateFromTrackData();
			trackComp->repaint();
			if (editor.mixerPanel)
				editor.mixerPanel->clearSamplePending(trackId);
			break;
		}
	}
}

void UITrackManager::refreshUIForMode()
{
	bool isLocalMode = editor.audioProcessor.getUseLocalModel();
	auto modelsForMode = AiModelDefinitions::getModelsForMode(isLocalMode);

	for (auto &tc : trackComponents)
	{
		if (!tc || !tc->getTrack())
			continue;
		auto *track = tc->getTrack();

		for (int i = 0; i < 4; ++i)
		{
			auto &page = track->pages[i];

			if (isLocalMode)
			{
				if (page.selectedModel != AiModelDefinitions::LOCAL_MODEL_NAME)
					page.savedModelBeforeLocal = page.selectedModel;
				page.selectedModel = AiModelDefinitions::LOCAL_MODEL_NAME;
			}
			else
			{
				if (!page.savedModelBeforeLocal.isEmpty())
					page.selectedModel = page.savedModelBeforeLocal;
			}
		}

		tc->modelSelector.clear();
		for (int i = 0; i < modelsForMode.size(); ++i)
			tc->modelSelector.addItem(modelsForMode[i], i + 1);

		juce::String currentModel = track->getCurrentPage().selectedModel;
		int idx = modelsForMode.indexOf(currentModel);
		if (idx >= 0)
			tc->modelSelector.setSelectedId(idx + 1, juce::sendNotification);
		else
			tc->modelSelector.setSelectedId(1, juce::sendNotification);

		if (editor.mixerPanel)
			editor.mixerPanel->updateModelUI(tc->getTrackId());
	}

	editor.resized();
}

void UITrackManager::checkLocalModelsAndNotify()
{
	auto appDataDir =
	    juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("OBSIDIAN-Neural");
	auto stableAudioDir = appDataDir.getChildFile("stable-audio");

	StableAudioEngine tempEngine;
	bool modelsPresent = tempEngine.initialize(stableAudioDir.getFullPathName());

	if (modelsPresent)
	{
		editor.statusLabel.setText("Local models found! Configuration saved.", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
		editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
	}
	else
	{
		ObsidianAlertManager::showConfirm(
		    &editor, "Local Models Required",
		    "Local models not found!\n\nExpected location: " + stableAudioDir.getFullPathName(),
		    "Open GitHub Instructions", "OK",
		    [](bool confirmed)
		    {
			    if (confirmed)
				    juce::URL("https://github.com/innermost47/ai-dj/blob/main/README.md").launchInDefaultBrowser();
		    });

		editor.statusLabel.setText("Local mode selected - Models setup required", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
		editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
	}
}

void UITrackManager::updateUIComponents()
{
	if (!editor.uiGenerationManager->isGenerating() && editor.audioProcessor.getIsGenerating())
	{
		editor.uiGenerationManager->setIsGenerating(true);
		editor.uiGenerationManager->setWasGenerating(true);
		editor.startTimer(200);
	}
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->isShowing())
		{
			TrackData *track = editor.audioProcessor.getTrack(trackComp->getTrackId());
			if (track && !trackComp->isEditingLabel)
			{
				trackComp->updateFromTrackData();
			}
		}
	}
	if (editor.mixerPanel)
	{
		editor.mixerPanel->updateAllMixerComponents();
	}

	if (!editor.lastMidiNote.isEmpty())
	{
		static int midiBlinkCounter = 0;
		if (++midiBlinkCounter > 6)
		{
			editor.midiIndicator.setColour(juce::Label::backgroundColourId, ColourPalette::backgroundDeep);
			editor.lastMidiNote.clear();
			editor.uiStatusManager->updateLCD();
			midiBlinkCounter = 0;
		}
	}

	for (auto &trackComp : trackComponents)
	{
		TrackData *track = editor.audioProcessor.getTrack(trackComp->getTrackId());
		if (track && track->isPlaying.load() && track->getCurrentPage().numSamples > 0)
		{
			double startSample = track->getCurrentPage().loopStart * track->getCurrentPage().sampleRate;
			double currentTimeInSection =
			    (startSample + track->readPosition.load()) / track->getCurrentPage().sampleRate;

			trackComp->updatePlaybackPosition(currentTimeInSection);
		}
	}

	static bool currentWasGenerating = false;
	bool isCurrentlyGenerating = editor.audioProcessor.getIsGenerating();
	if (currentWasGenerating && !isCurrentlyGenerating)
	{
		for (auto &trackComp : trackComponents)
		{
			trackComp->refreshWaveformIfNeeded();
		}
	}
	currentWasGenerating = isCurrentlyGenerating;
}

TrackComponent *UITrackManager::getTrackComponent(const juce::String &trackId)
{
	for (auto &tc : trackComponents)
	{
		if (tc && tc->getTrackId() == trackId)
			return tc.get();
	}
	return nullptr;
}

void UITrackManager::detachAllListeners()
{
	for (auto &tc : trackComponents)
	{
		if (tc)
		{
			auto *track = tc->getTrack();
			if (track && track->slotIndex != -1)
			{
				tc->removeListener("Generate");
				tc->removeListener("RandomRetrigger");
				tc->removeListener("RetriggerInterval");
				tc->removeListener("AdsrAttack");
				tc->removeListener("AdsrDecay");
				tc->removeListener("AdsrSustain");
				tc->removeListener("AdsrRelease");
			}
			tc->detachWaveformTrack();
		}
	}
}

void UITrackManager::forceFullRefresh()
{
	trackComponents.clear();
	editor.uiLayoutManager->getTracksContainer()->removeAllChildren();
}