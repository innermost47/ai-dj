#pragma once
#include <JuceHeader.h>

struct EscapableTextEditor : public juce::TextEditor
{
	bool keyPressed(const juce::KeyPress &key) override
	{
		if (key == juce::KeyPress::escapeKey)
		{
			giveAwayKeyboardFocus();
		}
		return juce::TextEditor::keyPressed(key);
	}
};