#pragma once
#include "ObsidianBase.h"
#include "SendsPanel.h"
#include "TrackEffectsPanel.h"
#include "TrackRecapPanel.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class RightPanelWrapper : public ObsidianComponent
{
  public:
	RightPanelWrapper(DjIaVstProcessor &processor);
	~RightPanelWrapper() override = default;

	void paint(juce::Graphics &g) override;
	void resized() override;

	TrackRecapPanel *getTrackRecapPanel()
	{
		return trackRecap.get();
	}
	TrackEffectsPanel *getTrackEffectsPanel()
	{
		return trackEffects.get();
	}
	SendsPanel *getSendsPanel()
	{
		return sendsPanel.get();
	}

  private:
	DjIaVstProcessor &audioProcessor;

	std::unique_ptr<TrackRecapPanel> trackRecap;
	std::unique_ptr<TrackEffectsPanel> trackEffects;
	std::unique_ptr<SendsPanel> sendsPanel;

	// Container qui contient recap + effects, scrollable
	juce::Component scrollContent;
	juce::Viewport contentViewport;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanelWrapper)
};