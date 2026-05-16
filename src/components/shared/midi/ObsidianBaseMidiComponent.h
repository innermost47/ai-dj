#pragma once
#include "MidiLearnableComponents.h"
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

	void syncSliderRange(juce::Slider &s, juce::String paramId);

	void subscribeToParam(const juce::String &paramSuffix);

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

  private:
	struct Binding
	{
		juce::String suffix;
		juce::Slider *slider = nullptr;
		juce::Button *button = nullptr;
		bool momentary = false;
	};

	std::vector<juce::String> listenedParams;
	std::vector<std::unique_ptr<Binding>> bindings;

	Binding *findBindingByParamId(const juce::String &fullId);
	void pushSliderToParam(Binding &b);
	void pushButtonToParam(Binding &b);
	void applyParamToBinding(Binding &b, float normalizedValue);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObsidianBaseMidiComponent)
};