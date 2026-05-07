#include "StandaloneMenuBar.h"
#include "ColourPalette.h"
#include "StandaloneTransport.h"

StandaloneMenuBar::StandaloneMenuBar(StandaloneTransport &t) : transport(t)
{
#if JUCE_MAC
	juce::MenuBarModel::setMacMainMenu(this);
#else
	menuBarComponent = std::make_unique<juce::MenuBarComponent>(this);
	addAndMakeVisible(*menuBarComponent);
#endif
}

StandaloneMenuBar::~StandaloneMenuBar()
{
#if JUCE_MAC
	juce::MenuBarModel::setMacMainMenu(nullptr);
#endif
}

juce::StringArray StandaloneMenuBar::getMenuBarNames()
{
	return {"File", "Settings", "Help"};
}

juce::PopupMenu StandaloneMenuBar::getMenuForIndex(int menuIndex, const juce::String &)
{
	juce::PopupMenu menu;

	if (menuIndex == 0)
	{
		menu.addItem(newSession, "New Session", true, false);
		menu.addItem(loadSession, "Open Session...", true, false);
		menu.addSeparator();
		menu.addItem(saveSession, "Save Session", true, false);
		menu.addItem(saveSessionAs, "Save Session As...", true, false);
		menu.addSeparator();
		menu.addItem(exportSession, "Export Audio...", true, false);
	}
	else if (menuIndex == 1)
	{
		menu.addItem(audioSettings, "Audio Settings...", true, false);
		menu.addItem(midiSettings, "MIDI Settings...", true, false);
	}
	else if (menuIndex == 2)
	{
		menu.addItem(showAbout, "About OBSIDIAN Neural", true, false);
	}

	return menu;
}

void StandaloneMenuBar::menuItemSelected(int menuItemID, int)
{
	switch (menuItemID)
	{
	case newSession:
		if (onNewSession)
			onNewSession();
		break;
	case saveSession:
		if (onSaveSession)
			onSaveSession();
		break;
	case saveSessionAs:
		if (onSaveSession)
			onSaveSession();
		break;
	case loadSession:
		if (onLoadSession)
			onLoadSession();
		break;
	case exportSession:
		if (onExportSession)
			onExportSession();
		break;
	case audioSettings:
		if (onShowSettings)
			onShowSettings();
		break;
	case showAbout:
		if (onShowAbout)
			onShowAbout();
		break;
	default:
		break;
	}
}

void StandaloneMenuBar::resized()
{
#if !JUCE_MAC
	if (menuBarComponent)
		menuBarComponent->setBounds(getLocalBounds());
#endif
}