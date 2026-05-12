#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;
class ScaleAndDurationPanel;

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

	std::unique_ptr<ScaleAndDurationPanel> scaleAndDurationPanel;

	IconButtonSimple bypassSequencerButton{"BypassSeq", ""};
	IconButtonSimple configButton{"Config", ""};
	IconButtonSimple openMidiEditorButton{"MidiEditor", ""};
	IconButtonSimple helpButton{"Help", ""};
	IconButtonSimple bypassLLMButton{"BypassLLM", ""};

	juce::Image logoImage;

	juce::Label configLabel;
	juce::Label versionLabel;
	juce::Label buildLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigComponent)
};