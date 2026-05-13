#include "StandaloneTransportComponent.h"
#include "DataConst.h"
#include "PluginProcessor.h"

void StandaloneTransportComponent::BeatLcd::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	const float corner = ObsidianSizes::CORNER;

	g.setColour(juce::Colours::black.withAlpha(0.4f));
	g.fillRoundedRectangle(bounds.translated(0, 1.5f), corner);

	juce::ColourGradient lcdGradient(ColourPalette::backgroundDark.darker(0.2f), bounds.getX(), bounds.getY(),
	                                 ColourPalette::backgroundDark.darker(0.25f), bounds.getX(), bounds.getBottom(),
	                                 false);
	g.setGradientFill(lcdGradient);
	g.fillRoundedRectangle(bounds, corner);

	auto borderColour = ColourPalette::lightGrey.withAlpha(0.6f);
	g.setColour(borderColour);
	g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 0.8f);

	auto inner = bounds.reduced(1.0f, 4.0f);

	const int numLeds = juce::jmin(sigNum, 12);
	const float ledSize = 6.0f;
	const float ledGap = 4.0f;
	const float ledRowH = 10.0f;
	auto ledsRow = inner.removeFromTop(ledRowH);

	const float ledsTotalW = ledSize * numLeds + ledGap * (numLeds - 1);
	const float ledsStartX = ledsRow.getCentreX() - ledsTotalW * 0.5f;

	for (int i = 0; i < numLeds; ++i)
	{
		auto ledX = ledsStartX + i * (ledSize + ledGap);
		auto ledY = ledsRow.getCentreY() - ledSize * 0.5f;
		juce::Rectangle<float> ledRect(ledX, ledY, ledSize, ledSize);

		bool isCurrentBeat = (playing || paused) && (i == (beat - 1));
		bool isFirst = (i == 0);

		auto ledColour = isCurrentBeat ? (isFirst ? ColourPalette::playActive : ColourPalette::textAccent)
		                               : ColourPalette::backgroundLight.withAlpha(0.5f);
		if (isCurrentBeat && pulse > 0.01f)
			ledColour = ledColour.brighter(pulse * 0.3f);

		g.setColour(ledColour.darker(0.3f));
		g.fillEllipse(ledRect.translated(0, 0.5f));
		g.setColour(ledColour);
		g.fillEllipse(ledRect);

		if (isCurrentBeat)
		{
			g.setColour(ledColour.withAlpha(0.4f * pulse));
			g.fillEllipse(ledRect.expanded(2.0f));
		}
	}

	inner.removeFromTop(2.0f);

	const float h = inner.getHeight();
	const float totalW = inner.getWidth();
	const float barColW = totalW * 0.45f;
	const float beatColW = totalW * 0.30f;

	auto barCol = inner.removeFromLeft(barColW);
	auto beatCol = inner.removeFromLeft(beatColW);
	auto subCol = inner;

	auto drawLabelValue = [&](juce::Rectangle<float> col, const juce::String &lbl, const juce::String &val, bool isHero)
	{
		g.setColour(ColourPalette::textPrimary.withAlpha(0.8f));
		g.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO, juce::Font::plain));
		auto labelArea = col.removeFromLeft(col.getWidth() * 0.4f);
		g.drawFittedText(lbl, labelArea.toNearestInt(), juce::Justification::centredRight, 1);

		col.removeFromLeft(4.0f);

		auto valueColour = isHero ? ColourPalette::textAccent : ColourPalette::textPrimary;
		if (isHero && playing && downbeat && pulse > 0.01f)
			valueColour = valueColour.brighter(pulse * 0.45f);

		g.setColour(valueColour);
		const float valSize =
		    isHero ? juce::jmin(h * 0.95f, ObsidianSizes::TEXT_XXL) : juce::jmin(h * 0.8f, ObsidianSizes::TEXT_XL);
		g.setFont(juce::FontOptions(valSize, juce::Font::bold));
		g.drawFittedText(val, col.toNearestInt(), juce::Justification::centredLeft, 1);
	};

	drawLabelValue(barCol, "BAR", juce::String(bar), false);
	drawLabelValue(beatCol, "BEAT", juce::String(beat), true);
	drawLabelValue(subCol, "SUB", juce::String(sub), false);
}

void StandaloneTransportComponent::BeatLcd::setBarBeatSub(int b, int beatV, int subV)
{
	bar = b;
	beat = beatV;
	sub = subV;
	repaint();
}

void StandaloneTransportComponent::BeatLcd::setPlaying(bool p)
{
	playing = p;
	repaint();
}

void StandaloneTransportComponent::BeatLcd::setPaused(bool p)
{
	paused = p;
	repaint();
}

void StandaloneTransportComponent::BeatLcd::setBeatPulse(float intensity)
{
	pulse = intensity;
	repaint();
}

void StandaloneTransportComponent::BeatLcd::setIsDownbeat(bool d)
{
	downbeat = d;
}

void StandaloneTransportComponent::BeatLcd::setTimeSignature(int num, int den)
{
	sigNum = num;
	sigDen = den;
	repaint();
}

StandaloneTransportComponent::BpmField::BpmField()
{
	addAndMakeVisible(editor);
	editor.setInputRestrictions(6, "0123456789.");
	editor.setJustification(juce::Justification::centred);
	editor.setColour(EscapableTextEditor::textColourId, ColourPalette::textAccent);
	editor.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_XXL));
	editor.setTooltip("BPM\nScroll: +/-1\nShift+Scroll: +/-5\nCmd+Scroll: +/-0.1\nDouble-clic: reset 120");
}

void StandaloneTransportComponent::BpmField::resized()
{
	auto area = getLocalBounds();
	editor.setBounds(area);
}

void StandaloneTransportComponent::BpmField::paint(juce::Graphics & /*g*/)
{
}

void StandaloneTransportComponent::BpmField::mouseWheelMove(const juce::MouseEvent &e,
                                                            const juce::MouseWheelDetails &wheel)
{
	const double step = e.mods.isShiftDown() ? 5.0 : (e.mods.isCommandDown() ? 0.1 : 1.0);
	double newBpm = getBpmValue() + (wheel.deltaY > 0 ? step : -step);
	newBpm = juce::jlimit(20.0, 999.0, newBpm);
	setBpmValue(newBpm);
	if (onValueChanged)
		onValueChanged(newBpm);
}

void StandaloneTransportComponent::BpmField::mouseDoubleClick(const juce::MouseEvent &)
{
	if (onResetRequested)
		onResetRequested();
}

void StandaloneTransportComponent::BpmField::setBpmValue(double bpm)
{
	editor.setText(juce::String(bpm, 1), juce::dontSendNotification);
}

double StandaloneTransportComponent::BpmField::getBpmValue() const
{
	return editor.getText().getDoubleValue();
}

StandaloneTransportComponent::StandaloneTransportComponent(StandaloneTransport &t, DjIaVstProcessor &processor)
    : transport(t), audioProcessor(processor)
{
	setupUI();
	syncFromTransport();
	startTimerHz(30);
}

StandaloneTransportComponent::~StandaloneTransportComponent()
{
	stopTimer();
}

void StandaloneTransportComponent::setupUI()
{
	addAndMakeVisible(lcd);
	lcd.setBarBeatSub(1, 1, 1);
	lcd.setTimeSignature(transport.getTimeSigNumerator(), transport.getTimeSigDenominator());

	addAndMakeVisible(playButton);
	playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
	playButton.setClickingTogglesState(true);
	playButton.setShowBorder(true);
	playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
	playButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	playButton.setColour(juce::TextButton::textColourOnId, ColourPalette::textSecondary);
	playButton.setCustomIconColour(ColourPalette::textPrimary);
	playButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	playButton.setTooltip("Play / Pause");
	playButton.onClick = [this]()
	{
		const bool wantPlay = playButton.getToggleState();

		if (wantPlay)
			transport.play();
		else
			transport.stop();

		if (audioProcessor.getIsLinkActive())
		{
			if (wantPlay)
				audioProcessor.requestLinkStart();
			else
				audioProcessor.requestLinkStop();
		}

		udpatePlayButtonDisplay(wantPlay);
	};

	addAndMakeVisible(stopButton);
	stopButton.loadIcon(BinaryData::square_svg, BinaryData::square_svgSize);
	stopButton.setShowBorder(true);
	stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	stopButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	stopButton.setCustomIconColour(ColourPalette::textPrimary);
	stopButton.setCustomIconColourToggled(ColourPalette::textPrimary);
	stopButton.setTooltip("Stop & Rewind");
	stopButton.onClick = [this]()
	{
		transport.stop();
		transport.rewind();
		playButton.setToggleState(false, juce::dontSendNotification);
		playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
		isPaused = false;
		pauseBlinkPhase = 0.0f;
		wasBlinking = false;
		currentBeat = 0;
		currentSubBeat = 0;
		currentPulse = 0.0f;
		lcd.setPlaying(false);
		lcd.setPaused(false);
		lcd.setBeatPulse(0.0f);
		updateBeatDisplay();

		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		playButton.repaint();
		stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		stopButton.repaint();

		if (audioProcessor.getIsLinkActive())
		{
			audioProcessor.requestLinkStop();
		}
	};

	addAndMakeVisible(bpmField);
	bpmField.setBpmValue(transport.getBpm());
	bpmField.editor.onReturnKey = [this]()
	{
		onBpmEditorChanged();
		giveAwayKeyboardFocus();
	};
	bpmField.editor.onFocusLost = [this]() { onBpmEditorChanged(); };
	bpmField.onValueChanged = [this](double bpm)
	{
		transport.setBpm(bpm);
		if (onBpmChanged)
			onBpmChanged(transport.getBpm());
	};
	bpmField.onResetRequested = [this]()
	{
		transport.setBpm(120.0);
		bpmField.setBpmValue(transport.getBpm());
		if (onBpmChanged)
			onBpmChanged(transport.getBpm());
	};

	addAndMakeVisible(linkButton);
	linkButton.setShowBorder(true);
	linkButton.setClickingTogglesState(true);
	linkButton.setToggleState(audioProcessor.getIsLinkActive(), juce::dontSendNotification);
	linkButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
	linkButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
	linkButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	linkButton.setColour(juce::TextButton::textColourOnId, ColourPalette::textAccent);
	linkButton.setTooltip("Ableton Link - sync tempo and start/stop with other Link-enabled apps on your network");
	linkButton.onClick = [this]()
	{
		audioProcessor.setLinkActive(linkButton.getToggleState());
		juce::Component::SafePointer<StandaloneTransportComponent> safeThis(this);
		juce::Timer::callAfterDelay(500,
		                            [safeThis]()
		                            {
			                            if (safeThis != nullptr)
				                            safeThis->syncFromTransport();
		                            });
	};

	addAndMakeVisible(timeSigEditor);

	timeSigEditor.setJustification(juce::Justification::centred);
	timeSigEditor.setIndents(0, 0);
	timeSigEditor.setColour(EscapableTextEditor::backgroundColourId, juce::Colours::transparentBlack);
	timeSigEditor.setColour(EscapableTextEditor::outlineColourId, juce::Colours::transparentBlack);
	timeSigEditor.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
	timeSigEditor.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));

	timeSigEditor.setInputRestrictions(5, "0123456789/");

	timeSigEditor.onReturnKey = [this]()
	{
		handleTimeSigChange();
		giveAwayKeyboardFocus();
	};

	timeSigEditor.onFocusLost = [this]() { handleTimeSigChange(); };
}

void StandaloneTransportComponent::udpatePlayButtonDisplay(bool isPlaying)
{
	if (isPlaying)
	{
		playButton.loadIcon(BinaryData::pause_svg, BinaryData::pause_svgSize);
		isPaused = false;
		pauseBlinkPhase = 0.0f;
		lcd.setPlaying(true);
		lcd.setPaused(false);
		auto playingBg = ColourPalette::playActive;
		playButton.setColour(juce::TextButton::buttonColourId, playingBg);
		playButton.setColour(juce::TextButton::buttonOnColourId, playingBg);
		playButton.repaint();
		auto stopBg = ColourPalette::stopActive;
		stopButton.setColour(juce::TextButton::buttonColourId, stopBg);
		stopButton.repaint();
	}
	else
	{
		playButton.loadIcon(BinaryData::pause_svg, BinaryData::pause_svgSize);
		isPaused = true;
		pauseBlinkPhase = 0.0f;
		lcd.setPlaying(false);
		lcd.setPaused(true);
		stopButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		stopButton.repaint();
	}
}

void StandaloneTransportComponent::handleTimeSigChange()
{
	auto text = timeSigEditor.getText();
	juce::StringArray tokens;
	tokens.addTokens(text, "/", "");
	if (tokens.size() == 2)
	{
		int num = tokens[0].trim().getIntValue();
		int den = tokens[1].trim().getIntValue();

		num = juce::jlimit(1, 32, num);
		den = juce::jlimit(2, 32, den);

		if (!juce::isPowerOfTwo(den))
			den = juce::nextPowerOfTwo(den) / 2;

		int stepsPerBeat;
		if (den == 8)
			stepsPerBeat = 2;
		else if (den == 4)
			stepsPerBeat = 4;
		else if (den == 2)
			stepsPerBeat = 8;
		else
			stepsPerBeat = 4;

		const int maxNumerator = ObsidianDataConst::MAX_STEPS_PER_MEASURE / stepsPerBeat;
		num = juce::jlimit(1, maxNumerator, num);

		transport.setTimeSignature(num, den);
		lcd.setTimeSignature(num, den);
		if (audioProcessor.getIsLinkActive())
			audioProcessor.setLinkQuantum(static_cast<double>(num));
		if (onTimeSignatureChanged)
			onTimeSignatureChanged();
	}
	syncFromTransport();
}

void StandaloneTransportComponent::resized()
{
	auto area = getLocalBounds();

	const int lcdHeight = 60;
	const int transportH = 26;

	auto lcdRow = area.removeFromTop(lcdHeight);
	auto lcdWidth = (lcdRow.getWidth() / 3) * 2;

	lcd.setBounds(lcdRow.removeFromLeft(lcdWidth));
	lcdRow.removeFromLeft(ObsidianSizes::GAP_4);
	bpmField.setBounds(lcdRow.removeFromTop(juce::roundToInt(lcdHeight / 1.5f)));
	lcdRow.removeFromTop(ObsidianSizes::GAP_4);
	linkButton.setBounds(lcdRow);

	area.removeFromTop(ObsidianSizes::GAP_4);

	auto transportRow = area.removeFromTop(transportH);
	auto transportWidth = (transportRow.getWidth() / 3) * 2;

	const int btnGap = ObsidianSizes::GAP_4;
	const int transportBtnWidth = (transportWidth - btnGap) / 2;

	playButton.setBounds(transportRow.removeFromLeft(transportBtnWidth));
	transportRow.removeFromLeft(btnGap);
	stopButton.setBounds(transportRow.removeFromLeft(transportBtnWidth));
	transportRow.removeFromLeft(btnGap);
	timeSigEditor.setBounds(transportRow);
}

void StandaloneTransportComponent::paint(juce::Graphics & /*g*/)
{
}

void StandaloneTransportComponent::timerCallback()
{
	if (isPaused)
	{
		pauseBlinkPhase += 0.18f;
		pauseBlinkPhase = std::fmod(pauseBlinkPhase, juce::MathConstants<float>::twoPi);

		float intensity = 0.5f + 0.5f * std::sin(pauseBlinkPhase);
		auto baseColour = ColourPalette::backgroundMid;
		auto blinkColour = baseColour.interpolatedWith(ColourPalette::playArmed, intensity * 0.8f);

		playButton.setColour(juce::TextButton::buttonColourId, blinkColour);
		playButton.setColour(juce::TextButton::buttonOnColourId, blinkColour);
		playButton.repaint();
		wasBlinking = true;
	}
	else if (wasBlinking)
	{
		auto playingBg = ColourPalette::playActive;
		playButton.setColour(juce::TextButton::buttonColourId, playingBg);
		playButton.setColour(juce::TextButton::buttonOnColourId, playingBg);
		playButton.repaint();
		wasBlinking = false;
	}
	if (currentPulse > 0.0f)
	{
		currentPulse = juce::jmax(0.0f, currentPulse - 0.08f);
		lcd.setBeatPulse(currentPulse);
		repaint(0, 0, 4, getHeight());
	}

	if (!transport.isPlaying())
	{
		if (beatFlash)
		{
			beatFlash = false;
			currentPulse = 0.0f;
			lcd.setBeatPulse(0.0f);
		}
		return;
	}

	double ppq = transport.getCurrentPpq();
	int timeSigNum = transport.getTimeSigNumerator();

	int newBeat = (int)ppq % timeSigNum;
	int newSubBeat = (int)(ppq * 4.0) % (timeSigNum * 4);

	if (newBeat != currentBeat || newSubBeat != currentSubBeat)
	{
		bool isNewBeat = (newBeat != currentBeat);
		bool isDownbeat = (newBeat == 0 && isNewBeat);

		currentBeat = newBeat;
		currentSubBeat = newSubBeat;
		beatFlash = (newSubBeat % 4 == 0);

		if (isNewBeat)
		{
			currentPulse = 1.0f;
			lcd.setIsDownbeat(isDownbeat);
			lcd.setBeatPulse(1.0f);
		}

		updateBeatDisplay();
	}
}

void StandaloneTransportComponent::updateBeatDisplay()
{
	int bar = (int)(transport.getCurrentPpq() / transport.getTimeSigNumerator()) + 1;
	int beat = currentBeat + 1;
	int subBeat = (currentSubBeat % 4) + 1;
	lcd.setBarBeatSub(bar, beat, subBeat);
}

void StandaloneTransportComponent::onBpmEditorChanged()
{
	double newBpm = bpmField.getBpmValue();
	if (newBpm > 0.0)
	{
		transport.setBpm(newBpm);
		bpmField.setBpmValue(transport.getBpm());
		if (onBpmChanged)
			onBpmChanged(transport.getBpm());
		if (audioProcessor.getIsLinkActive())
			audioProcessor.setLinkTempo(newBpm);
	}
}

void StandaloneTransportComponent::syncFromTransport()
{
	bpmField.setBpmValue(transport.getBpm());

	juce::String sigText =
	    juce::String(transport.getTimeSigNumerator()) + "/" + juce::String(transport.getTimeSigDenominator());

	timeSigEditor.setText(sigText, juce::dontSendNotification);
}