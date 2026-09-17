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
