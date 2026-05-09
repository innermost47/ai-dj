#include "SendsPanel.h"
#include "PluginProcessor.h"

SendsPanel::SendsPanel(DjIaVstProcessor &processor) : ObsidianBaseMidiComponent(processor)
{
	delayTitleLbl.setText("DELAY", juce::dontSendNotification);
	delayTitleLbl.setFont(juce::FontOptions(11.0f, juce::Font::bold));
	delayTitleLbl.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	addAndMakeVisible(delayTitleLbl);

	auto styleSubLbl = [](juce::Label &l, const juce::String &txt)
	{
		l.setText(txt, juce::dontSendNotification);
		l.setFont(juce::FontOptions(9.0f, juce::Font::bold));
		l.setColour(juce::Label::textColourId, ColourPalette::textSecondary.withAlpha(0.8f));
	};

	styleSubLbl(delayTimeLbl, "Time");
	styleSubLbl(delayFeedbackLbl, "Feedback");
	styleSubLbl(delayModeLbl, "Mode");
	addAndMakeVisible(delayTimeLbl);
	addAndMakeVisible(delayFeedbackLbl);
	addAndMakeVisible(delayModeLbl);

	setupDelayDivisionButtons();
	setupDelayModeButtons();

	delayFeedbackKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
	delayFeedbackKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	styleKnob(delayFeedbackKnob, ColourPalette::violet);
	addAndMakeVisible(delayFeedbackKnob);

	reverbTitleLbl.setText("REVERB", juce::dontSendNotification);
	reverbTitleLbl.setFont(juce::FontOptions(11.0f, juce::Font::bold));
	reverbTitleLbl.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	addAndMakeVisible(reverbTitleLbl);

	styleSubLbl(reverbSizeLbl, "Size");
	styleSubLbl(reverbDampingLbl, "Damping");
	styleSubLbl(reverbWidthLbl, "Width");
	styleSubLbl(reverbMixLbl, "Mix");

	addAndMakeVisible(reverbSizeLbl);
	addAndMakeVisible(reverbDampingLbl);
	addAndMakeVisible(reverbWidthLbl);
	addAndMakeVisible(reverbMixLbl);

	wireParameters();

	auto setupKnob = [&](MidiLearnableSlider &s, juce::String paramId)
	{
		s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		styleKnob(s, ColourPalette::teal);
		syncSliderRange(s, paramId);
		addAndMakeVisible(s);
	};

	setupKnob(reverbSizeKnob, "reverbSize");
	setupKnob(reverbDampingKnob, "reverbDamping");
	setupKnob(reverbWidthKnob, "reverbWidth");
	setupKnob(reverbMixKnob, "reverbMix");

	syncSliderRange(delayFeedbackKnob, "delayFeedback");

	refreshRadioButtonsForParam("delayDivision");
	refreshRadioButtonsForParam("delayMode");
}

void SendsPanel::wireParameters()
{
	registerSliderParam("delayFeedback", delayFeedbackKnob);
	registerSliderParam("reverbSize", reverbSizeKnob);
	registerSliderParam("reverbDamping", reverbDampingKnob);
	registerSliderParam("reverbWidth", reverbWidthKnob);
	registerSliderParam("reverbMix", reverbMixKnob);

	registerMidiLearn("delayFeedback", &delayFeedbackKnob);
	registerMidiLearn("reverbSize", &reverbSizeKnob);
	registerMidiLearn("reverbDamping", &reverbDampingKnob);
	registerMidiLearn("reverbWidth", &reverbWidthKnob);
	registerMidiLearn("reverbMix", &reverbMixKnob);

	subscribeToParam("delayDivision");
	subscribeToParam("delayMode");

	auto curveCallback = [this](const juce::String &paramID, int targetIndex, int totalCount)
	{
		return [this, paramID, targetIndex, totalCount](float value)
		{
			if (value > 0.5f)
			{
				juce::MessageManager::callAsync(
				    [this, paramID, targetIndex, totalCount]()
				    {
					    if (auto *p = getProcessor().getParameterTreeState().getParameter(paramID))
					    {
						    p->beginChangeGesture();
						    p->setValueNotifyingHost((float)targetIndex / (float)(totalCount - 1));
						    p->endChangeGesture();
					    }
				    });
			}
		};
	};

	for (int i = 0; i < (int)delayDivisionButtons.size(); ++i)
	{
		registerMidiLearn("delayDivision", delayDivisionButtons[i].get(),
		                  curveCallback("delayDivision", i, (int)delayDivisionButtons.size()));
	}
	for (int i = 0; i < (int)delayModeButtons.size(); ++i)
	{
		registerMidiLearn("delayMode", delayModeButtons[i].get(),
		                  curveCallback("delayMode", i, (int)delayModeButtons.size()));
	}
}

void SendsPanel::onParameterChangedUI(const juce::String &paramSuffix, float /*normalizedValue*/)
{
	if (paramSuffix == "delayDivision" || paramSuffix == "delayMode")
		refreshRadioButtonsForParam(paramSuffix);
}

void SendsPanel::refreshRadioButtonsForParam(const juce::String &paramID)
{
	auto &apvts = getProcessor().getParameterTreeState();
	auto *p = apvts.getParameter(paramID);
	if (!p)
		return;

	auto &buttons = (paramID == "delayDivision") ? delayDivisionButtons : delayModeButtons;
	int max = (int)buttons.size() - 1;
	if (max < 0)
		return;

	int idx = juce::jlimit(0, max, (int)(p->getValue() * max + 0.5f));
	for (int i = 0; i < (int)buttons.size(); ++i)
		buttons[i]->setToggleState(i == idx, juce::dontSendNotification);
}

void SendsPanel::styleKnob(juce::Slider &s, juce::Colour col)
{
	s.setColour(juce::Slider::rotarySliderFillColourId, col);
	s.setColour(juce::Slider::thumbColourId, col);
	s.setColour(juce::Slider::rotarySliderOutlineColourId, ColourPalette::backgroundLight.withAlpha(0.3f));
}

void SendsPanel::setupDelayDivisionButtons()
{
	auto &apvts = getProcessor().getParameterTreeState();
	auto *param = apvts.getParameter("delayDivision");

	juce::StringArray labels{"1/16", "1/8.", "1/8", "1/4.", "1/4", "1/2", "1 bar", "2 bars"};

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::violet);
		btn->setRadioGroupId(0xDEAD);

		int currentValue = (int)(param->getValue() * (labels.size() - 1) + 0.5f);
		btn->setToggleState(i == currentValue, juce::dontSendNotification);

		int idx = i;
		int total = labels.size();
		btn->onClick = [param, idx, total]()
		{
			param->beginChangeGesture();
			param->setValueNotifyingHost((float)idx / (float)(total - 1));
			param->endChangeGesture();
		};

		addAndMakeVisible(*btn);
		delayDivisionButtons.push_back(std::move(btn));
	}
}

void SendsPanel::setupDelayModeButtons()
{
	auto &apvts = getProcessor().getParameterTreeState();
	auto *param = apvts.getParameter("delayMode");

	juce::StringArray labels{"Stereo", "Ping-Pong", "Mono"};

	for (int i = 0; i < labels.size(); ++i)
	{
		auto btn = std::make_unique<MidiLearnableLedRadioButton>(labels[i], ColourPalette::amber);
		btn->setRadioGroupId(0xBEEF);

		int currentValue = (int)(param->getValue() * (labels.size() - 1) + 0.5f);
		btn->setToggleState(i == currentValue, juce::dontSendNotification);

		int idx = i;
		int total = labels.size();
		btn->onClick = [param, idx, total]()
		{
			param->beginChangeGesture();
			param->setValueNotifyingHost((float)idx / (float)(total - 1));
			param->endChangeGesture();
		};

		addAndMakeVisible(*btn);
		delayModeButtons.push_back(std::move(btn));
	}
}

void SendsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.8f));
	g.fillRoundedRectangle(bounds, 6.0f);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

	int y = getHeight() / 2;
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
	g.drawLine(8.0f, (float)y, (float)getWidth() - 8.0f, (float)y, 0.5f);
}

void SendsPanel::resized()
{
	auto area = getLocalBounds().reduced(8, 6);

	int fxPart = area.getHeight() / 2;
	auto delayArea = area.removeFromTop(fxPart);
	auto reverbArea = area;

	delayTitleLbl.setBounds(delayArea.removeFromTop(14));
	delayArea.removeFromTop(4);
	auto mainRow = delayArea.removeFromTop(80);

	auto fbCol = mainRow.removeFromLeft(45);
	delayFeedbackKnob.setBounds(fbCol.reduced(2));

	mainRow.removeFromLeft(15);
	auto gridArea = mainRow;

	int btnsPerRow = 4;
	int btnH = 14;
	int btnW = (gridArea.getWidth() - (btnsPerRow - 1) * 4) / btnsPerRow;

	for (int i = 0; i < (int)delayDivisionButtons.size(); ++i)
	{
		int row = i / btnsPerRow;
		int col = i % btnsPerRow;
		int x = gridArea.getX() + col * (btnW + 4);
		int y = gridArea.getY() + row * (btnH + 4);
		delayDivisionButtons[i]->setBounds(x, y, btnW, btnH);
	}

	int numModes = (int)delayModeButtons.size();
	if (numModes > 0)
	{
		int modeBtnW = (gridArea.getWidth() - (numModes - 1) * 4) / numModes;
		int yMode = gridArea.getY() + (2 * (btnH + 4));

		for (int i = 0; i < numModes; ++i)
		{
			int xMode = gridArea.getX() + i * (modeBtnW + 4);
			delayModeButtons[i]->setBounds(xMode, yMode, modeBtnW, btnH);
		}
	}

	reverbArea.removeFromTop(4);
	reverbTitleLbl.setBounds(reverbArea.removeFromTop(14));
	reverbArea.removeFromTop(4);

	auto knobsRow = reverbArea.removeFromTop(65);
	int knobColW = knobsRow.getWidth() / 4;

	auto placeKnob = [](juce::Rectangle<int> col, juce::Label &lbl, juce::Slider &knob)
	{
		lbl.setBounds(col.removeFromTop(14));
		knob.setBounds(col.reduced(4, 2));
	};

	placeKnob(knobsRow.removeFromLeft(knobColW), reverbSizeLbl, reverbSizeKnob);
	placeKnob(knobsRow.removeFromLeft(knobColW), reverbDampingLbl, reverbDampingKnob);
	placeKnob(knobsRow.removeFromLeft(knobColW), reverbWidthLbl, reverbWidthKnob);
	placeKnob(knobsRow.removeFromLeft(knobColW), reverbMixLbl, reverbMixKnob);
}