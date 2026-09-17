#pragma once
#include "GlitchStepGrid.h"
#include "GlitchTypes.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
struct TrackData;

class GlitchSequenceRow : public ObsidianBaseMidiComponent
{
  public:
	GlitchSequenceRow(DjIaVstProcessor &processor, int sequenceIndex);
	~GlitchSequenceRow() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void setTrackData(TrackData *trackData);
	void refreshFromTrack();
	void setStepsPerBar(int stepsPerBar);
	void setCurrentPlaybackStep(int stepIndex);
	void setStepsPerBeat(int stepsPerBeat);
	void setPowerButtonEnabled(bool shouldBeEnabled);

	int getRequiredHeight() const;

	std::function<void()> onContentChanged;

  protected:
	juce::String getParameterPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "slot" + juce::String(t->slotIndex + 1);
	}
	juce::String getMidiLearnDescriptionPrefix() const override
	{
		auto *t = getTrack();
		if (!t || t->slotIndex == -1)
			return {};
		return "Slot " + juce::String(t->slotIndex + 1) + " ";
	}
	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

  private:
	int index;
	juce::String paramSuffixForThisRow;

	juce::Label seqLabel;
	IconButton activateButton{"Activate", ""};
	GlitchStepGrid stepGrid;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchSequenceRow)
};