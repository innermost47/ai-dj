#include "UIMidiManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "RightPanelWrapper.h"
#include "StandaloneTransportComponent.h"
#if JUCE_WINDOWS
#include <windows.h>
#include <winuser.h>
#endif

UIMidiManager::UIMidiManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIMidiManager::updateMidiIndicator(const juce::String &noteInfo)
{
	editor.lastMidiNote = noteInfo;
	juce::Component::SafePointer<DjIaVstEditor> safeEditor(&editor);
	juce::MessageManager::callAsync(
	    [safeEditor, noteInfo]()
	    {
		    if (!safeEditor)
			    return;
		    safeEditor->midiIndicator.setText(noteInfo, juce::dontSendNotification);
		    safeEditor->uiStatusManager->updateLCD();
		    juce::Timer::callAfterDelay(800,
		                                [safeEditor]()
		                                {
			                                if (!safeEditor)
				                                return;
			                                safeEditor->midiIndicator.setText("", juce::dontSendNotification);
			                                safeEditor->uiStatusManager->updateLCD();
		                                });
	    });
}

bool UIMidiManager::keyPressed(const juce::KeyPress &key)
{
	KeyboardLayout layout = detectKeyboardLayout();

	std::vector<std::vector<juce::KeyPress>> layoutKeys(8);

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (key == juce::KeyPress::spaceKey)
		{
			if (editor.audioProcessor.getStandaloneTransport())
			{
				editor.audioProcessor.getStandaloneTransport()->togglePlayStop();
				editor.uiLayoutManager->getRightPanelWrapper()
				    ->getStandaloneTransportComponent()
				    ->udpatePlayButtonDisplay(editor.audioProcessor.getStandaloneTransport()->isPlaying());
			}
			return true;
		}
	}

	switch (layout)
	{
	case AZERTY:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('z'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('q'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('w'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress(','), juce::KeyPress(';')}};
		break;

	case QWERTY:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('z'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')}};
		break;

	case QWERTZ:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('y'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('z'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')}};
		break;
	}

	for (int slotIndex = 0; slotIndex < Obsidian::MAX_TRACKS; ++slotIndex)
	{
		for (int page = 0; page < Obsidian::MAX_PAGES; ++page)
		{
			if (keyMatches(key, layoutKeys[slotIndex][page]))
			{
				for (auto &trackComp : editor.uiTrackManager->getTrackComponents())
				{
					if (auto *track = trackComp->getTrack())
					{
						if (track->slotIndex == slotIndex)
						{
							if (editor.audioProcessor.getIsGenerating() &&
							    editor.audioProcessor.getGeneratingTrackId() == track->trackId)
							{
								editor.uiStatusManager->setStatusWithTimeout(
								    "Cannot switch pages during generation...");
								return false;
							}
							else
							{
								trackComp->onPageSelected(page);
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool UIMidiManager::keyStateChanged(bool isKeyDown)
{
	if (isKeyDown && !editor.hasKeyboardFocus(true))
	{
		editor.grabKeyboardFocus();
	}
	return false;
}

UIMidiManager::KeyboardLayout UIMidiManager::detectKeyboardLayout()
{
#if JUCE_WINDOWS
	HKL layout = GetKeyboardLayout(0);
	WORD primaryLang = PRIMARYLANGID(LOWORD(layout));

	if (primaryLang == LANG_FRENCH)
		return AZERTY;
	if (primaryLang == LANG_GERMAN)
		return QWERTZ;
#endif
	return QWERTY;
}

bool UIMidiManager::keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected)
{
	if (pressed == expected)
		return true;
	if (pressed.getKeyCode() == expected.getKeyCode())
		return true;
	if (expected.getKeyCode() >= '1' && expected.getKeyCode() <= '4')
	{
		int expectedNum = expected.getKeyCode() - '0';
		if (pressed.getKeyCode() >= '1' && pressed.getKeyCode() <= '4')
		{
			int pressedNum = pressed.getKeyCode() - '0';
			return pressedNum == expectedNum;
		}
	}

	return false;
}