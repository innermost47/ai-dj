#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class FilterComponent;
class EqualizerComponent;
class CompressorComponent;
class LimiterComponent;
class DistortionComponent;

class TrackEffectsPanel : public ObsidianComponent
{
  public:
	TrackEffectsPanel(DjIaVstProcessor &processor);
	~TrackEffectsPanel() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void updateModelUI(const juce::String &trackId);
	void refresh();
	void setupUI();

  private:
	DjIaVstProcessor &audioProcessor;

	juce::String activeTrackId;

	std::vector<std::unique_ptr<IconButtonSimple>> trackSelectors;

	std::unique_ptr<FilterComponent> filterComponent;
	std::unique_ptr<EqualizerComponent> equalizerComponent;
	std::unique_ptr<CompressorComponent> compressorComponent;
	std::unique_ptr<LimiterComponent> limiterComponent;
	std::unique_ptr<DistortionComponent> distortionComponent;

	void addComponents(const juce::String &trackId);
	void resetComponents();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackEffectsPanel)
};