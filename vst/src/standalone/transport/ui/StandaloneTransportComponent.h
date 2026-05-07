#pragma once
#include "BinaryData.h"
#include "ColourPalette.h"
#include "CustomLookAndFeel.h"
#include "IconButton.h"
#include "StandaloneTransport.h"
#include <JuceHeader.h>

class StandaloneTransportComponent : public juce::Component, private juce::Timer
{
  public:
	StandaloneTransportComponent(StandaloneTransport &transport);
	~StandaloneTransportComponent() override;

	void resized() override;
	void paint(juce::Graphics &g) override;
	void udpatePlayButtonDisplay(bool isPlaying);

	std::function<void(double)> onBpmChanged;
	std::function<void(int, int)> onTimeSignatureChanged;

  private:
	void setupUI();
	void timerCallback() override;
	void updateBeatDisplay();
	void onBpmEditorChanged();
	void registerTapTempo();

	class BeatLcd : public juce::Component
	{
	  public:
		void paint(juce::Graphics &g) override;
		void setBarBeatSub(int b, int beat, int sub);
		void setPlaying(bool p);
		void setBeatPulse(float intensity);
		void setIsDownbeat(bool d);
		void setTimeSignature(int num, int den);
		void setPaused(bool p);

	  private:
		int bar = 1, beat = 1, sub = 1;
		int sigNum = 4, sigDen = 4;
		bool playing = false;
		bool downbeat = false;
		float pulse = 0.0f;
		bool paused = false;
	};

	class BpmField : public juce::Component
	{
	  public:
		BpmField();
		void resized() override;
		void paint(juce::Graphics &g) override;
		void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;
		void mouseDoubleClick(const juce::MouseEvent &) override;

		std::function<void(double)> onValueChanged;
		std::function<void()> onResetRequested;

		void setBpmValue(double bpm);
		double getBpmValue() const;

		juce::TextEditor editor;
	};

	StandaloneTransport &transport;

	IconButton playButton{"play"};
	IconButton stopButton{"stop"};
	IconButton tapButton{"tap", "TAP"};
	IconButtonRepeat bpmDownButton{"bpmDown", "-"};
	IconButtonRepeat bpmUpButton{"bpmUp", "+"};

	BeatLcd lcd;
	BpmField bpmField;

	juce::ComboBox timeSigNumerator;
	juce::Label timeSigSeparator;
	juce::ComboBox timeSigDenominator;
	juce::Label timeSigLabel;

	int currentBeat = 0;
	int currentSubBeat = 0;
	bool beatFlash = false;
	float currentPulse = 0.0f;
	bool isPaused = false;
	float pauseBlinkPhase = 0.0f;
	bool wasBlinking = false;

	juce::Array<juce::int64> tapTimes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneTransportComponent)
};