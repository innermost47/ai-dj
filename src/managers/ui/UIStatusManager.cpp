#include "UIStatusManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

UIStatusManager::UIStatusManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIStatusManager::setStatusWithTimeout(const juce::String &message, int timeoutMs)
{
	editor.statusLabel.setText(message, juce::dontSendNotification);
	updateLCD();
	juce::Timer::callAfterDelay(timeoutMs,
	                            [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
	                            {
		                            if (auto *e = safeEditor.getComponent())
		                            {
			                            e->statusLabel.setText("Ready", juce::dontSendNotification);
			                            e->uiStatusManager->updateLCD();
		                            }
	                            });
}

void UIStatusManager::updateLCD()
{
	editor.lcdScreen->setLines(editor.creditsLabel.getText(), editor.statusLabel.getText(),
	                           editor.midiIndicator.getText());
}

void UIStatusManager::refreshCredits()
{
	refreshCreditsAsync();
}

void UIStatusManager::refreshCreditsAsync()
{
	juce::String currentApiKey = editor.audioProcessor.getApiKey();
	juce::String currentServerUrl = editor.audioProcessor.getServerUrl();
	int timeout = editor.audioProcessor.getRequestTimeout();

	if (currentApiKey.isEmpty())
	{
		editor.creditsLabel.setText("Credits: No API Key", juce::dontSendNotification);
		return;
	}

	if (currentServerUrl.isEmpty())
	{
		editor.creditsLabel.setText("Credits: No Server", juce::dontSendNotification);
		return;
	}

	editor.creditsLabel.setText("Credits: Loading...", juce::dontSendNotification);

	editor.audioProcessor.getApiClient().setApiKey(currentApiKey);
	editor.audioProcessor.getApiClient().setBaseUrl(currentServerUrl);

	juce::Thread::launch(
	    [timeout, safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
	    {
		    auto *e = safeEditor.getComponent();
		    if (!e)
			    return;
		    if (e->audioProcessor.isShuttingDown.load())
			    return;
		    auto creditsInfo = e->audioProcessor.getApiClient().checkCredits(timeout);
		    juce::MessageManager::callAsync(
		        [safeEditor, creditsInfo]()
		        {
			        if (auto *editor = safeEditor.getComponent())
			        {
				        if (creditsInfo.success)
				        {
					        juce::String creditsText;
					        if (creditsInfo.creditsRemaining == -1 || creditsInfo.creditsTotal == -1)
					        {
						        creditsText = "Credits: Unlimited";
					        }
					        else
					        {
						        creditsText = "Credits: " + juce::String(creditsInfo.creditsRemaining) + " / " +
						                      juce::String(creditsInfo.creditsTotal);
					        }

					        safeEditor->creditsLabel.setText(creditsText, juce::dontSendNotification);
					        safeEditor->audioProcessor.setCreditsRemaining(creditsInfo.creditsRemaining);
					        safeEditor->audioProcessor.canGenerateStandard = creditsInfo.canGenerateStandard;
				        }
				        else
				        {
					        safeEditor->creditsLabel.setText("Credits: Error", juce::dontSendNotification);
				        }
			        }
		        });
	    });
}