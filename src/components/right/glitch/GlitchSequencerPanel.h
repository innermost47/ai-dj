#pragma once
#include "DataConst.h"
#include "GlitchLegend.h"
#include "GlitchMetaSequenceRow.h"
#include "GlitchSequenceRow.h"
#include "ObsidianBase.h"
#include "Sizes.h"
#include <JuceHeader.h>
#include <array>

class DjIaVstProcessor;
class DjIaVstEditor;

struct TrackData;

class GlitchSequencerPanel : public ObsidianComponent
{
  public:
	GlitchSequencerPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~GlitchSequencerPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	int getPreferredHeight() const;
	void refreshAfterStateLoad(const juce::String &trackId);
	void showTrack(const juce::String &trackId);

	std::function<void()> onContentChanged;

  private:
	DjIaVstProcessor &audioProcessor;
	DjIaVstEditor &editor;

	juce::String activeTrackId;
	TrackData *activeTrack = nullptr;

	GlitchLegend legend;
	GlitchMetaSequenceRow metaRow;

	juce::Label chainHelpLabel;
	juce::Label seqHelpLabel;

	std::array<std::unique_ptr<GlitchSequenceRow>, 8> sequenceRows;

	int lastKnownNumerator = -1;
	int lastKnownDenominator = -1;
	int lastNotifiedHeight = -1;

	bool lastKnownChainState = false;

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;

	juce::ComboBox presetSelector;
	IconButton savePresetButton{"SaveGlitchPreset", ""};
	IconButton deletePresetButton{"DeleteGlitchPreset", ""};
	juce::Label presetLabel;

	void refreshPresetList();
	void applySelectedPreset();
	void saveCurrentAsPreset();
	void deleteSelectedPreset();
	void updatePresetButtons();
	void notifyContentChanged();
	void handleVBlank();
	void refreshRowsFromTrack();

	static constexpr int kUserPresetIdOffset = 1000;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSequencerPanel)
};