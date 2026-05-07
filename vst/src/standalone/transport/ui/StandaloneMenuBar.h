#pragma once
#include <JuceHeader.h>

class StandaloneTransport;

class StandaloneMenuBar : public juce::Component, public juce::MenuBarModel
{
  public:
	explicit StandaloneMenuBar(StandaloneTransport &transport);
	~StandaloneMenuBar() override;

	juce::StringArray getMenuBarNames() override;
	juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String &menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

	std::function<void()> onNewSession;
	std::function<void()> onSaveSession;
	std::function<void()> onLoadSession;
	std::function<void()> onExportSession;
	std::function<void()> onShowSettings;
	std::function<void()> onShowAbout;

	void resized() override;

  private:
	StandaloneTransport &transport;
	std::unique_ptr<juce::MenuBarComponent> menuBarComponent;

	enum MenuIDs
	{
		newSession = 1,
		saveSession,
		saveSessionAs,
		loadSession,
		exportSession,

		audioSettings = 100,
		midiSettings,
		showAbout,
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneMenuBar)
};