#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBaseMidiComponent.h"
#include "PluginProcessor.h"
#include "VuMeter.h"
#include <JuceHeader.h>

class MasterChannel : public ObsidianBaseMidiComponent
{
  public:
	MasterChannel(DjIaVstProcessor &processor);
	~MasterChannel();
	void setRealAudioLevelStereo(float levelLeft, float levelRight);
	void updateMasterLevels();

	std::function<void(float)> onMasterVolumeChanged;
	std::function<void(float)> onMasterPanChanged;
	std::function<void(float, float, float)> onMasterEQChanged;

  private:
	VuMeter vuMeter;
	MidiLearnableSlider masterVolumeSlider;
	MidiLearnableSlider masterPanKnob;
	MidiLearnableSlider highKnob, midKnob, lowKnob;

	juce::Rectangle<int> masterVUBounds;

	bool hasRealAudio = false;

	juce::Label masterLabel;
	juce::Label highLabel, midLabel, lowLabel, panLabel;

	float masterLevel = 0.0f;
	float masterPeakHold = 0.0f;
	float masterLevelLeft = 0.0f;
	float masterLevelRight = 0.0f;
	float masterPeakHoldLeft = 0.0f;
	float realAudioLevelLeft = 0.0f;
	float realAudioLevelRight = 0.0f;
	float masterPeakHoldRight = 0.0f;
	float realAudioLevel = 0.0f;

	int masterPeakHoldTimerLeft = 0;
	int masterPeakHoldTimerRight = 0;
	int masterPeakHoldTimer = 0;

	void setupUI();
	void paint(juce::Graphics &g) override;
	void resized() override;
	void wireParameters();

  protected:
	juce::String getMidiLearnDescriptionPrefix() const override
	{
		return "Master ";
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannel)
};
