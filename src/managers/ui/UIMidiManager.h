#pragma once
#include <JuceHeader.h>

class DjIaVstEditor;

class UIMidiManager
{
  public:
	explicit UIMidiManager(DjIaVstEditor &editor);
	~UIMidiManager() = default;

	void updateMidiIndicator(const juce::String &noteInfo);
	bool keyPressed(const juce::KeyPress &key);
	bool keyStateChanged(bool isKeyDown);
	bool keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected);

  private:
	DjIaVstEditor &editor;

	enum KeyboardLayout
	{
		QWERTY,
		AZERTY,
		QWERTZ
	};

	KeyboardLayout detectKeyboardLayout();
};