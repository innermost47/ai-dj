#pragma once
#include "JuceHeader.h"
#include "MidiLearnableComponents.h"
#include "CustomLookAndFeel.h"

class DjIaVstProcessor;

class SequencerComponent : public juce::Component
{
public:
	SequencerComponent(const juce::String& trackId, DjIaVstProcessor& processor);
	~SequencerComponent();

	void paint(juce::Graphics& g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;

	void setCurrentStep(int step);
	void setPlaying(bool playing);
	void setNumMeasures(int measures);
	void setCurrentMeasure(int measure);
	void updateSequenceButtonsDisplay();
	void updateFromTrackData();
	void setAccentColour(juce::Colour colour);

	bool isSequencerPlaying() const { return isPlaying; }

private:
	struct FlatButtonLF : public CustomLookAndFeel
	{
		void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
			bool, bool) override
		{
			g.setColour(btn.findColour(juce::TextButton::textColourOffId));
			g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
			g.drawText(btn.getButtonText(), btn.getLocalBounds(),
				juce::Justification::centred, false);
		}
	};

	FlatButtonLF flatLF;
	juce::String trackId;
	DjIaVstProcessor& audioProcessor;

	static const int MAX_STEPS_PER_MEASURE = 16;
	static const int MAX_MEASURES = 4;

	bool isEditing = false;

	int currentStep = 0;
	int currentMeasure = 0;
	int numMeasures = 1;
	int beatsPerMeasure = 4;
	bool isPlaying = false;

	juce::Slider measureSlider;
	juce::Slider timeSignatureSlider;
	std::array<MidiLearnableButton, 8> sequenceButtons;
	juce::Colour accentColour = ColourPalette::buttonDanger;

	juce::Timer* editingTimer = nullptr;

	juce::TextButton prevMeasureButton, nextMeasureButton;

	juce::Label measureLabel;
	juce::Label currentPlayingMeasureLabel;

	juce::Rectangle<int> getStepBounds(int step);

	void toggleStep(int step);
	void setupUI();

	int getTotalStepsForCurrentSignature() const;
	void setupSequenceButtons();
	void layoutSequenceButtons(juce::Rectangle<int> area);
	void onSequenceSelected(int seqIndex);

	double samplesPerStep;
	double stepAccumulator;
};