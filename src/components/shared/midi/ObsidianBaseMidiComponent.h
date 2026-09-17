#pragma once
#include "GlitchSequencerEngine.h"
#include "MidiLearnableComponents.h"
#include "ModulationEngine.h"
#include "ObsidianBase.h"
#include "TrackData.h"
#include <JuceHeader.h>
#include <functional>

class DjIaVstProcessor;

class ObsidianBaseMidiComponent : public ObsidianComponent, public juce::AudioProcessorParameter::Listener
{
  public:
	explicit ObsidianBaseMidiComponent(DjIaVstProcessor &processor);
	~ObsidianBaseMidiComponent() override;

	void parameterValueChanged(int parameterIndex, float newValue) final;
	void parameterGestureChanged(int /*parameterIndex*/, bool /*gestureIsStarting*/) override {};

	TrackData *getTrack() const
	{
		return track.get();
	}

	juce::WeakReference<TrackData> track;

  protected:
	virtual juce::String getParameterPrefix() const
	{
		return {};
	}

	virtual juce::String getMidiLearnDescriptionPrefix() const
	{
		return {};
	}

	virtual void onParameterChangedUI(const juce::String &paramSuffix, float normalizedValue);

	void registerSliderParam(const juce::String &paramSuffix, juce::Slider &slider);

	void registerButtonParam(const juce::String &paramSuffix, juce::Button &button, bool momentary = false);

	void registerMidiLearn(const juce::String &paramSuffix, MidiLearnableBase *component,
	                       std::function<void(float)> uiCallback = nullptr);

	void triggerMomentaryParam(const juce::String &paramSuffix);

	void syncBindingsFromParameters();

	void syncSliderRange(juce::Slider &s, const juce::String &paramId);

	void subscribeToParam(const juce::String &paramSuffix);

	void clearAllBindings();

	void markForDestruction()
	{
		isDestroyed.store(true);
	}

	juce::String fullParamId(const juce::String &paramSuffix) const;
	juce::AudioProcessorParameter *getParam(const juce::String &paramSuffix) const;
	DjIaVstProcessor &getProcessor() const
	{
		return audioProcessor;
	}

	std::atomic<bool> isDestroyed{false};

	DjIaVstProcessor &audioProcessor;

	void syncModulationRing(juce::Slider &slider, const juce::String &paramSuffix)
	{
		auto *t = getTrack();
		if (!t)
			return;

		int targetIndex = 0;
		for (int i = 1; i < getNumModTargets(); ++i)
		{
			if (getModTargets()[i].paramSuffix == nullptr)
				continue;
			if (paramSuffix == getModTargets()[i].paramSuffix)
			{
				targetIndex = i;
				break;
			}
		}

		const bool isModulated = targetIndex > 0 && ModulationEngine::isTargetModulated(*t, targetIndex);
		const float amount = isModulated ? ModulationEngine::getModulationForTarget(*t, targetIndex) : 0.0f;

		auto &props = slider.getProperties();
		const bool wasActive = (bool)props[CustomLookAndFeel::getModActivePropertyId()];
		const float wasAmount = (float)props[CustomLookAndFeel::getModAmountPropertyId()];

		if (wasActive == isModulated && std::abs(wasAmount - amount) < 0.001f)
			return;

		props.set(CustomLookAndFeel::getModActivePropertyId(), isModulated);
		props.set(CustomLookAndFeel::getModAmountPropertyId(), amount);
		slider.repaint();
	}

	bool isFxOwnedByGlitch(GlitchEffectType fx) const
	{
		auto *t = getTrack();
		if (!t)
			return false;
		return (t->glitchReservedMask.load() & GlitchSequencerEngine::effectBit(fx)) != 0;
	}

	bool isGlitchDrivingFx(GlitchEffectType fx) const
	{
		auto *t = getTrack();
		if (!t)
			return false;

		const int bit = GlitchSequencerEngine::effectBit(fx);
		return (t->glitchFxEnabledMask.load() & t->glitchOwnedMask.load() & bit) != 0;
	}

	void paintGlitchLed(juce::Graphics &g, GlitchEffectType fx)
	{
		const bool owned = isFxOwnedByGlitch(fx);
		if (!owned)
			return;

		auto led = getLocalBounds().reduced(6).removeFromTop(14).removeFromRight(10);
		const bool driving = isGlitchDrivingFx(fx);

		g.setColour(driving ? ColourPalette::buttonSuccess : ColourPalette::buttonSuccess.withAlpha(0.2f));
		g.fillEllipse(led.withSizeKeepingCentre(7, 7).toFloat());
	}

	void syncGlitchLock(GlitchEffectType fx, juce::Component *mixKnob, juce::Component *bypassButton)
	{
		const bool locked = isFxOwnedByGlitch(fx);

		if (glitchLockApplied == locked && glitchLockInitialised)
			return;

		glitchLockApplied = locked;
		glitchLockInitialised = true;

		if (mixKnob != nullptr)
		{
			mixKnob->setEnabled(!locked);
			mixKnob->setAlpha(locked ? 0.4f : 1.0f);
		}

		if (bypassButton != nullptr)
		{
			bypassButton->setEnabled(!locked);
			bypassButton->setAlpha(locked ? 0.4f : 1.0f);
		}
	}

	bool syncGlitchLedState(GlitchEffectType fx)
	{
		const bool driving = isGlitchDrivingFx(fx);
		const bool owned = isFxOwnedByGlitch(fx);

		if (driving == lastGlitchDriving && owned == lastGlitchOwned)
			return false;

		lastGlitchDriving = driving;
		lastGlitchOwned = owned;
		return true;
	}

  private:
	struct Binding
	{
		juce::String suffix;
		juce::Slider *slider = nullptr;
		juce::Button *button = nullptr;
		bool momentary = false;
	};

	bool lastGlitchDriving = false;
	bool lastGlitchOwned = false;
	bool glitchLockApplied = false;
	bool glitchLockInitialised = false;

	std::vector<juce::String> listenedParams;
	std::vector<std::unique_ptr<Binding>> bindings;

	Binding *findBindingByParamId(const juce::String &fullId);
	void pushSliderToParam(Binding &b);
	void pushButtonToParam(Binding &b);
	void applyParamToBinding(Binding &b, float normalizedValue);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianBaseMidiComponent)
};