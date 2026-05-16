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
	auto &processor = editor.audioProcessor;
	juce::String currentApiKey = processor.getApiKey();
	juce::String currentServerUrl = processor.getServerUrl();
	int timeout = processor.getRequestTimeout();

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

	processor.getApiClient().setApiKey(currentApiKey);
	processor.getApiClient().setBaseUrl(currentServerUrl);

	juce::Component::SafePointer<DjIaVstEditor> safeEditor(&editor);

	processor.threadPool.addJob(
	    [&processor, safeEditor, timeout]() mutable
	    {
		    if (safeEditor == nullptr)
			    return;
		    auto creditsInfo = processor.getApiClient().checkCredits(timeout);
		    if (processor.isShuttingDown.load())
			    return;
		    juce::MessageManager::callAsync(
		        [safeEditor, creditsInfo]()
		        {
			        if (auto *e = safeEditor.getComponent())
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