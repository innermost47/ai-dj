#pragma once
#include "DataConst.h"
#include "GlitchMetaStepButton.h"
#include "GlitchTypes.h"
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class GlitchMetaSequenceRow : public ObsidianBaseMidiComponent
{
  public:
	explicit GlitchMetaSequenceRow(DjIaVstProcessor &processor);
	~GlitchMetaSequenceRow() override;

	void setTrackData(TrackData *trackData);
	void refreshFromTrack();
	void setCurrentPlaybackStep(int stepIndex);
	bool isChainActive() const;

	int getRequiredHeight() const
	{
		return 26;
	}

	std::function<void()> onBypassChanged;

	void paint(juce::Graphics &g) override;
	void resized() override;

  protected:
	juce::String getParameterPrefix() const override
	{
		if (!track || track->slotIndex < 0)
			return {};
		return "slot" + juce::String(track->slotIndex + 1);
	}

	juce::String getMidiLearnDescriptionPrefix() const override
	{
		if (!track || track->slotIndex < 0)
			return {};
		return "Slot " + juce::String(track->slotIndex + 1) + " ";
	}

	void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue) override;

  private:
	TrackData *track = nullptr;

	IconButton bypassButton{"BypassGlitchChain", ""};
	juce::Label titleLabel;
	std::array<std::unique_ptr<GlitchMetaStepButton>, GlitchMetaSequence::MAX_STEPS> stepButtons;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlitchMetaSequenceRow)
};