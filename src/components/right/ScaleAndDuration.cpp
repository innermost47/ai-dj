#include "ScaleAndDuration.h"
#include "PluginProcessor.h"

ScaleAndDurationPanel::ScaleAndDurationPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	addAndMakeVisible(keySelector);
	keySelector.addItem("C Ionian", 1);
	keySelector.addItem("C# Ionian", 2);
	keySelector.addItem("D Ionian", 3);
	keySelector.addItem("D# Ionian", 4);
	keySelector.addItem("E Ionian", 5);
	keySelector.addItem("F Ionian", 6);
	keySelector.addItem("F# Ionian", 7);
	keySelector.addItem("G Ionian", 8);
	keySelector.addItem("G# Ionian", 9);
	keySelector.addItem("A Ionian", 10);
	keySelector.addItem("A# Ionian", 11);
	keySelector.addItem("B Ionian", 12);
	keySelector.addItem("C Dorian", 13);
	keySelector.addItem("C# Dorian", 14);
	keySelector.addItem("D Dorian", 15);
	keySelector.addItem("D# Dorian", 16);
	keySelector.addItem("E Dorian", 17);
	keySelector.addItem("F Dorian", 18);
	keySelector.addItem("F# Dorian", 19);
	keySelector.addItem("G Dorian", 20);
	keySelector.addItem("G# Dorian", 21);
	keySelector.addItem("A Dorian", 22);
	keySelector.addItem("A# Dorian", 23);
	keySelector.addItem("B Dorian", 24);
	keySelector.addItem("C Phrygian", 25);
	keySelector.addItem("C# Phrygian", 26);
	keySelector.addItem("D Phrygian", 27);
	keySelector.addItem("D# Phrygian", 28);
	keySelector.addItem("E Phrygian", 29);
	keySelector.addItem("F Phrygian", 30);
	keySelector.addItem("F# Phrygian", 31);
	keySelector.addItem("G Phrygian", 32);
	keySelector.addItem("G# Phrygian", 33);
	keySelector.addItem("A Phrygian", 34);
	keySelector.addItem("A# Phrygian", 35);
	keySelector.addItem("B Phrygian", 36);
	keySelector.addItem("C Lydian", 37);
	keySelector.addItem("C# Lydian", 38);
	keySelector.addItem("D Lydian", 39);
	keySelector.addItem("D# Lydian", 40);
	keySelector.addItem("E Lydian", 41);
	keySelector.addItem("F Lydian", 42);
	keySelector.addItem("F# Lydian", 43);
	keySelector.addItem("G Lydian", 44);
	keySelector.addItem("G# Lydian", 45);
	keySelector.addItem("A Lydian", 46);
	keySelector.addItem("A# Lydian", 47);
	keySelector.addItem("B Lydian", 48);
	keySelector.addItem("C Mixolydian", 49);
	keySelector.addItem("C# Mixolydian", 50);
	keySelector.addItem("D Mixolydian", 51);
	keySelector.addItem("D# Mixolydian", 52);
	keySelector.addItem("E Mixolydian", 53);
	keySelector.addItem("F Mixolydian", 54);
	keySelector.addItem("F# Mixolydian", 55);
	keySelector.addItem("G Mixolydian", 56);
	keySelector.addItem("G# Mixolydian", 57);
	keySelector.addItem("A Mixolydian", 58);
	keySelector.addItem("A# Mixolydian", 59);
	keySelector.addItem("B Mixolydian", 60);
	keySelector.addItem("C Aeolian", 61);
	keySelector.addItem("C# Aeolian", 62);
	keySelector.addItem("D Aeolian", 63);
	keySelector.addItem("D# Aeolian", 64);
	keySelector.addItem("E Aeolian", 65);
	keySelector.addItem("F Aeolian", 66);
	keySelector.addItem("F# Aeolian", 67);
	keySelector.addItem("G Aeolian", 68);
	keySelector.addItem("G# Aeolian", 69);
	keySelector.addItem("A Aeolian", 70);
	keySelector.addItem("A# Aeolian", 71);
	keySelector.addItem("B Aeolian", 72);
	keySelector.addItem("C Locrian", 73);
	keySelector.addItem("C# Locrian", 74);
	keySelector.addItem("D Locrian", 75);
	keySelector.addItem("D# Locrian", 76);
	keySelector.addItem("E Locrian", 77);
	keySelector.addItem("F Locrian", 78);
	keySelector.addItem("F# Locrian", 79);
	keySelector.addItem("G Locrian", 80);
	keySelector.addItem("G# Locrian", 81);
	keySelector.addItem("A Locrian", 82);
	keySelector.addItem("A# Locrian", 83);
	keySelector.addItem("B Locrian", 84);
	keySelector.addItem("C Major", 85);
	keySelector.addItem("C# Major", 86);
	keySelector.addItem("D Major", 87);
	keySelector.addItem("D# Major", 88);
	keySelector.addItem("E Major", 89);
	keySelector.addItem("F Major", 90);
	keySelector.addItem("F# Major", 91);
	keySelector.addItem("G Major", 92);
	keySelector.addItem("G# Major", 93);
	keySelector.addItem("A Major", 94);
	keySelector.addItem("A# Major", 95);
	keySelector.addItem("B Major", 96);
	keySelector.addItem("C Minor", 97);
	keySelector.addItem("C# Minor", 98);
	keySelector.addItem("D Minor", 99);
	keySelector.addItem("D# Minor", 100);
	keySelector.addItem("E Minor", 101);
	keySelector.addItem("F Minor", 102);
	keySelector.addItem("F# Minor", 103);
	keySelector.addItem("G Minor", 104);
	keySelector.addItem("G# Minor", 105);
	keySelector.addItem("A Minor", 106);
	keySelector.addItem("A# Minor", 107);
	keySelector.addItem("B Minor", 108);
	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);

	addAndMakeVisible(durationSelector);
	for (int s : {2, 4, 6, 8, 10, 12, 16, 20, 24, 30})
		durationSelector.addItem(juce::String(s) + " s", s);
	int currentDur = juce::roundToInt(audioProcessor.getGlobalDuration());
	durationSelector.setSelectedId(currentDur, juce::dontSendNotification);
	if (durationSelector.getSelectedId() == 0)
		durationSelector.setSelectedId(6, juce::dontSendNotification);

	durationSelector.setTooltip("Generation duration in seconds");
	keySelector.setTooltip("Select musical key and mode for generation");

	addAndMakeVisible(titleLabel);
	titleLabel.setText("Key and duration", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_REGULAR));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	titleLabel.setJustificationType(juce::Justification::centredLeft);

	addAndMakeVisible(helpLabel);
	helpLabel.setText("These settings apply to every generation. They will be saved with the project.",
	                  juce::dontSendNotification);
	helpLabel.setFont(juce::FontOptions(Obsidian::TEXT_INFO));
	helpLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	helpLabel.setJustificationType(juce::Justification::topLeft);

	keySelector.onChange = [this]()
	{
		audioProcessor.setLastKeyIndex(keySelector.getSelectedId());
		audioProcessor.setGlobalKey(keySelector.getText());
	};

	durationSelector.onChange = [this]()
	{
		int val = durationSelector.getSelectedId();
		if (val > 0)
		{
			audioProcessor.setLastDuration((float)val);
			audioProcessor.setGlobalDuration(val);
		}
	};
}

void ScaleAndDurationPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);
}

void ScaleAndDurationPanel::resized()
{
	auto area = getLocalBounds();
	titleLabel.setBounds(area.removeFromTop((int)Obsidian::TEXT_REGULAR));
	area.removeFromTop(Obsidian::GAP_4);
	helpLabel.setBounds(area.removeFromTop(46));
	area.removeFromTop(Obsidian::GAP_4);

	keySelector.setBounds(area.removeFromTop(Obsidian::COMBO_BOX_BASE_HEIGHT));
	area.removeFromTop(Obsidian::GAP_4);
	durationSelector.setBounds(area.removeFromTop(Obsidian::COMBO_BOX_BASE_HEIGHT));
}

void ScaleAndDurationPanel::update()
{
	int currentDur = juce::roundToInt(audioProcessor.getGlobalDuration());
	durationSelector.setSelectedId(currentDur, juce::dontSendNotification);
	if (durationSelector.getSelectedId() == 0)
		durationSelector.setSelectedId(6, juce::dontSendNotification);

	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);
}