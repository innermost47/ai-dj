#pragma once
#include "DataConst.h"
#include "ModulatorRow.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;

class ModulationPanel : public juce::Component
{
  public:
	ModulationPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~ModulationPanel() override;

	void showTrack(const juce::String &trackId);
	void refreshAfterStateLoad(const juce::String &trackId);
	void updateModelUI();
	void refreshTargetAvailability();

	int getPreferredHeight() const;

	std::function<void()> onContentChanged;

	void paint(juce::Graphics &g) override;
	void resized() override;

  private:
	void refreshRowsFromTrack();
	void handleVBlank();

	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	juce::Label helpLabel;
	std::array<std::unique_ptr<ModulatorRow>, kNumModSlots> modulatorRows;

	juce::String activeTrackId;
	TrackData *activeTrack = nullptr;

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationPanel)
};