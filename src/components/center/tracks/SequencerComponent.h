#pragma once
#include "MidiLearnableComponents.h"
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class SequencerComponent : public ObsidianComponent, public juce::SettableTooltipClient
{
  public:
	SequencerComponent(const juce::String &trackId, DjIaVstProcessor &processor);
	~SequencerComponent();

	void paint(juce::Graphics &g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent &event) override;

	void setPlaying(bool playing);
	void setNumMeasures(int measures);
	void setCurrentMeasure(int measure);
	void updateSequenceButtonsDisplay();
	void updateFromTrackData();
	void setAccentColour(juce::Colour colour);

  private:
	struct RoundedLabelLF : public juce::LookAndFeel_V4
	{
		float radius = Obsidian::CORNER;
		void drawLabel(juce::Graphics &g, juce::Label &label) override
		{
			g.setColour(label.findColour(juce::Label::backgroundColourId));
			g.fillRoundedRectangle(label.getLocalBounds().toFloat(), radius);
			g.setColour(label.findColour(juce::Label::textColourId));
			g.setFont(label.getFont());
			g.drawFittedText(label.getText(), label.getLocalBounds().reduced(1), label.getJustificationType(), 1);
		}
	};
	RoundedLabelLF roundedLabelLF;
	juce::String trackId;
	DjIaVstProcessor &audioProcessor;

	static const int MAX_STEPS_PER_MEASURE = 16;
	static const int MAX_MEASURES = 4;

	bool isEditing = false;

	int currentStep = 0;
	int currentMeasure = 0;
	int numMeasures = 1;
	int beatsPerMeasure = 4;
	bool isPlaying = false;

	std::array<juce::TextButton, 4> measureButtons;
	juce::Slider timeSignatureSlider;
	std::array<MidiLearnableButton, 8> sequenceButtons;
	juce::Colour accentColour = ColourPalette::buttonDanger;

	juce::Timer *editingTimer = nullptr;

	IconButtonSimple prevMeasureButton{"PrevMeasure", ""};
	IconButtonSimple nextMeasureButton{"NextMeasure", ""};

	juce::Label measureLabel;
	juce::Label currentPlayingMeasureLabel;

	juce::Rectangle<int> getStepBounds(int step);

	void toggleStep(int step);
	void setupUI();

	int getTotalStepsForCurrentSignature() const;
	void setupSequenceButtons();
	void layoutSequenceButtons(juce::Rectangle<int> area);
	void onSequenceSelected(int seqIndex);
	void updateMeasureButtonsDisplay();

	double samplesPerStep;
	double stepAccumulator;
};