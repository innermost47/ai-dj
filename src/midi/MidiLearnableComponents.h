#pragma once
#include "ColourPalette.h"
#include <JuceHeader.h>

class MidiLearnableBase
{
  public:
	virtual ~MidiLearnableBase() = default;
	virtual void setLearningMode(bool isLearning) = 0;
	virtual bool isLearning() const = 0;

	std::function<void()> onMidiLearn;
	std::function<void()> onMidiRemove;
};

template <typename ComponentType>
class MidiLearnable : public ComponentType, public MidiLearnableBase, private juce::Timer
{
  public:
	MidiLearnable()
	{
		learningMode = false;
		blinkState = false;
		this->setAccessible(false);
	}

	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return juce::Component::createIgnoredAccessibilityHandler(*this);
	}

	~MidiLearnable()
	{
		onMidiLearn = nullptr;
		onMidiRemove = nullptr;
		stopTimer();
		this->setVisible(false);
	}

	void setLearningMode(bool isLearning) override
	{
		learningMode = isLearning;
		if (learningMode)
		{
			startTimer(300);
		}
		else
		{
			stopTimer();
			blinkState = false;

			if (juce::MessageManager::getInstance()->isThisTheMessageThread())
			{
				this->repaint();
			}
			else
			{
				juce::MessageManager::callAsync([this]() { this->repaint(); });
			}
		}
	}

	bool isLearning() const override
	{
		return learningMode;
	}

	void mouseDown(const juce::MouseEvent &e) override
	{
		if (e.mods.isRightButtonDown() && !e.mods.isCtrlDown())
		{
			juce::PopupMenu menu;
			menu.addItem(1, "MIDI Learn");
			menu.addItem(2, "Remove MIDI", onMidiRemove != nullptr);
			menu.showMenuAsync(juce::PopupMenu::Options(),
			                   [this](int result)
			                   {
				                   if (result == 1 && onMidiLearn)
				                   {
					                   setLearningMode(true);
					                   onMidiLearn();
				                   }
				                   else if (result == 2 && onMidiRemove)
				                   {
					                   onMidiRemove();
				                   }
			                   });
		}
		else
		{
			ComponentType::mouseDown(e);
		}
	}

	void paint(juce::Graphics &g) override
	{
		ComponentType::paint(g);

		if (learningMode && blinkState)
		{
			auto bounds = this->getLocalBounds();
			g.setColour(ColourPalette::textSecondary);
			g.drawRect(bounds, 3);

			g.setColour(ColourPalette::withAlpha(ColourPalette::textSecondary, 0.2f));
			g.fillRect(bounds);
		}
	}

  private:
	void timerCallback() override
	{
		blinkState = !blinkState;
		this->repaint();
	}

	bool learningMode;
	bool blinkState;
};

using MidiLearnableButton = MidiLearnable<juce::TextButton>;
using MidiLearnableSlider = MidiLearnable<juce::Slider>;
using MidiLearnableComboBox = MidiLearnable<juce::ComboBox>;
using MidiLearnableToggleButton = MidiLearnable<juce::ToggleButton>;