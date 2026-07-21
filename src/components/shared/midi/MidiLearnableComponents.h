#pragma once
#include "ColourPalette.h"
#include "DataConst.h"
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

template <typename ComponentType> class MidiLearnable : public ComponentType, public MidiLearnableBase
{
  public:
	template <typename... Args> MidiLearnable(Args &&...args) : ComponentType(std::forward<Args>(args)...)
	{
		learningMode = false;
		blinkState = false;
		this->setAccessible(false);

		vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
	}

	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
	{
		return juce::Component::createIgnoredAccessibilityHandler(*this);
	}

	~MidiLearnable()
	{
		onMidiLearn = nullptr;
		onMidiRemove = nullptr;
		vBlankAttachment.reset();
		this->setVisible(false);
	}

	void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
	{
		if (wheel.deltaX == 0.0f && wheel.deltaY == 0.0f)
			return;
		ComponentType::mouseWheelMove(e, wheel);
	}

	void setLearningMode(bool isLearning) override
	{
		if (learningMode == isLearning)
			return;

		learningMode = isLearning;
		blinkState = false;

		if (learningMode)
			lastBlinkTime = juce::Time::getMillisecondCounter();

		if (juce::MessageManager::getInstance()->isThisTheMessageThread())
			this->repaint();
		else
		{
			juce::Component::SafePointer<MidiLearnable> safeThis(this);
			juce::MessageManager::callAsync(
			    [safeThis]()
			    {
				    if (safeThis)
					    safeThis->repaint();
			    });
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
	void handleVBlank()
	{
		if (!learningMode)
			return;

		auto currentTime = juce::Time::getMillisecondCounter();
		if (currentTime - lastBlinkTime < Obsidian::BLINKING_DURATION_TIME)
			return;

		lastBlinkTime = currentTime;
		blinkState = !blinkState;
		this->repaint();
	}

	bool learningMode;
	bool blinkState;

	std::unique_ptr<juce::VBlankAttachment> vBlankAttachment;
	uint32_t lastBlinkTime = 0;
};

using MidiLearnableButton = MidiLearnable<juce::TextButton>;
using MidiLearnableSlider = MidiLearnable<juce::Slider>;
using MidiLearnableComboBox = MidiLearnable<juce::ComboBox>;
using MidiLearnableToggleButton = MidiLearnable<juce::ToggleButton>;