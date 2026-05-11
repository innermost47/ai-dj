#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;

class ConfigComponent : public ObsidianComponent
{
  public:
	ConfigComponent(DjIaVstProcessor &processor, DjIaVstEditor &editor);

	void paint(juce::Graphics &g) override;
	void resized() override;
	void setupUI();
	void addEventListeners();
	void updateFromProcessor();

  private:
	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	IconButtonSimple bypassSequencerButton{"BypassSeq", ""};
	IconButtonSimple configButton{"Config", ""};
	IconButtonSimple openMidiEditorButton{"MidiEditor", ""};
	IconButtonSimple helpButton{"Help", ""};
	IconButtonSimple bypassLLMButton{"BypassLLM", ""};

	juce::Image logoImage;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigComponent)
};