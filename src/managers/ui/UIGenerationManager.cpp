#include "UIGenerationManager.h"
#include "ColourPalette.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "TrackData.h"

UIGenerationManager::UIGenerationManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIGenerationManager::onGenerationComplete(const juce::String &trackId, const juce::String &message)
{
	bool isError = message.startsWith("ERROR:");
	stopGenerationUI(trackId, !isError, isError ? message : "");

	if (editor.isShowing())
	{
		editor.statusLabel.setText(message, juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();

		if (isError)
		{
			editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
			juce::Timer::callAfterDelay(5000,
			                            [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
			                            {
				                            if (auto *e = safeEditor.getComponent())
				                            {
					                            e->statusLabel.setText("Ready", juce::dontSendNotification);
					                            e->uiStatusManager->updateLCD();
					                            e->statusLabel.setColour(juce::Label::textColourId,
					                                                     ColourPalette::violet);
				                            }
			                            });
		}
		else
		{
			editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
			juce::Timer::callAfterDelay(3000,
			                            [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
			                            {
				                            if (auto *e = safeEditor.getComponent())
				                            {
					                            if (e->isShowing())
					                            {
						                            e->statusLabel.setText("Ready", juce::dontSendNotification);
						                            e->uiStatusManager->updateLCD();
						                            e->statusLabel.setColour(juce::Label::textColourId,
						                                                     ColourPalette::violet);
					                            }
				                            }
			                            });
		}
	}
	editor.uiStatusManager->refreshCredits();
}

void UIGenerationManager::startGenerationUI(const juce::String &trackId)
{
	setAllGenerateButtonsEnabled(false);
	editor.statusLabel.setText("Connecting to server...", juce::dontSendNotification);
	editor.uiStatusManager->updateLCD();

	for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->startGeneratingAnimation();
			break;
		}
	}
	if (editor.mixerPanel)
	{
		editor.mixerPanel->startGeneratingAnimationForTrack(trackId);
	}

	juce::Timer::callAfterDelay(100,
	                            [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor), trackId]()
	                            {
		                            if (auto *e = safeEditor.getComponent())
		                            {
			                            if (e->audioProcessor.getIsGenerating() &&
			                                e->audioProcessor.getGeneratingTrackId() == trackId)
			                            {
				                            e->statusLabel.setText("Generating loop (this may take a few minutes)...",
				                                                   juce::dontSendNotification);
				                            e->uiStatusManager->updateLCD();
			                            }
		                            }
	                            });
}

void UIGenerationManager::stopGenerationUI(const juce::String &trackId, bool success, const juce::String &errorMessage)
{
	setAllGenerateButtonsEnabled(true);

	for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->stopGeneratingAnimation();

			if (success)
			{
				trackComp->setSamplePending(true);

				if (editor.audioProcessor.getAutoLoadEnabled())
				{
					editor.statusLabel.setText("Sample ready - Loading automatically...", juce::dontSendNotification);
					editor.uiStatusManager->updateLCD();
				}
				else
				{
					editor.statusLabel.setText("Sample ready - Click 'Load Sample' to use it",
					                           juce::dontSendNotification);
					editor.uiStatusManager->updateLCD();
				}
			}

			trackComp->repaint();
			break;
		}
	}

	if (editor.mixerPanel)
	{
		editor.mixerPanel->stopGeneratingAnimationForTrack(trackId, success);
	}

	isGenerating_.store(false);
	wasGenerating_.store(false);
	generatingTrackId.clear();
	editor.stopTimer();

	if (!success && !errorMessage.isEmpty())
	{
		editor.statusLabel.setText("Error: " + errorMessage, juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
	}
}

void UIGenerationManager::generateFromTrackComponent(const juce::String &trackId)
{
	editor.audioProcessor.setIsGenerating(true);

	TrackData *track = editor.audioProcessor.getTrack(trackId);
	if (!track)
	{
		editor.statusLabel.setText("Error: Track not found", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
		editor.audioProcessor.setIsGenerating(false);
		return;
	}

	if (track->getCurrentPage().selectedPrompt.isEmpty())
	{
		editor.statusLabel.setText("Error: No prompt selected for this track", juce::dontSendNotification);
		editor.uiStatusManager->updateLCD();
		editor.audioProcessor.setIsGenerating(false);
		return;
	}

	juce::String currentGeneratingTrackId = trackId;
	editor.audioProcessor.setGeneratingTrackId(currentGeneratingTrackId);

	auto &currentPage = track->getCurrentPage();

	currentPage.setSelectedPrompt(track->getCurrentPage().selectedPrompt);
	currentPage.generationPrompt = track->getCurrentPage().selectedPrompt;
	currentPage.generationBpm = editor.audioProcessor.getGlobalBpm();
	currentPage.generationKey = editor.audioProcessor.getGlobalKey();
	currentPage.generationDuration = editor.audioProcessor.getGlobalDuration();
	if (currentPage.selectedModel.isEmpty())
		currentPage.selectedModel = "stable-audio-open-1.0";

	startGenerationUI(currentGeneratingTrackId);

	juce::Thread::launch(
	    [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor), currentGeneratingTrackId, track]()
	    {
		    try
		    {
			    auto request = track->createLoopRequest();

			    auto *e = safeEditor.getComponent();
			    if (!e)
				    return;

			    e->audioProcessor.getGenerationManager().generateLoop(request, currentGeneratingTrackId);
		    }
		    catch (const std::exception &ex)
		    {
			    juce::MessageManager::callAsync(
			        [safeEditor, currentGeneratingTrackId, error = juce::String(ex.what())]()
			        {
				        if (auto *e = safeEditor.getComponent())
				        {
					        e->uiGenerationManager->stopGenerationUI(currentGeneratingTrackId, false, error);
					        e->audioProcessor.setIsGenerating(false);
					        e->audioProcessor.setGeneratingTrackId("");
				        }
			        });
		    }
	    });
}

void UIGenerationManager::setAllGenerateButtonsEnabled(bool enabled)
{
	for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
	{
		trackComp->setGenerateButtonEnabled(enabled);
		trackComp->setCanvasGenerating(!enabled);
	}
}