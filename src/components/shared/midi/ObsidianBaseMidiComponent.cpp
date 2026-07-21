#include "ObsidianBaseMidiComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

ObsidianBaseMidiComponent::ObsidianBaseMidiComponent(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

ObsidianBaseMidiComponent::~ObsidianBaseMidiComponent()
{
	isDestroyed.store(true);

	auto &apvts = audioProcessor.getParameterTreeState();
	for (auto &id : listenedParams)
	{
		if (auto *p = apvts.getParameter(id))
			p->removeListener(this);
	}
}

juce::String ObsidianBaseMidiComponent::fullParamId(const juce::String &paramSuffix) const
{
	auto prefix = getParameterPrefix();
	return prefix.isEmpty() ? paramSuffix : (prefix + paramSuffix);
}

juce::AudioProcessorParameter *ObsidianBaseMidiComponent::getParam(const juce::String &paramSuffix) const
{
	return audioProcessor.getParameterTreeState().getParameter(fullParamId(paramSuffix));
}

void ObsidianBaseMidiComponent::registerSliderParam(const juce::String &paramSuffix, juce::Slider &slider)
{
	bindings.push_back(std::make_unique<Binding>(Binding{paramSuffix, &slider, nullptr, false}));
	Binding *b = bindings.back().get();

	slider.onValueChange = [this, b]()
	{
		if (isDestroyed.load())
			return;
		pushSliderToParam(*b);
	};

	subscribeToParam(paramSuffix);
}

void ObsidianBaseMidiComponent::registerButtonParam(const juce::String &paramSuffix, juce::Button &button,
                                                    bool momentary)
{
	bindings.push_back(std::make_unique<Binding>(Binding{paramSuffix, nullptr, &button, momentary}));
	Binding *b = bindings.back().get();

	button.onClick = [this, b, paramSuffix]()
	{
		if (isDestroyed.load())
			return;
		if (track == nullptr)
			return;
		if (paramSuffix == "Play" && track->getCurrentPage().numSamples == 0)
			return;
		pushButtonToParam(*b);
	};

	subscribeToParam(paramSuffix);
}

ObsidianBaseMidiComponent::Binding *ObsidianBaseMidiComponent::findBindingByParamId(const juce::String &fullId)
{
	for (auto &b : bindings)
	{
		if (!b)
			continue;
		if (fullParamId(b->suffix) == fullId)
			return b.get();
	}
	return nullptr;
}

void ObsidianBaseMidiComponent::triggerMomentaryParam(const juce::String &paramSuffix)
{
	if (auto *p = getParam(paramSuffix))
	{
		p->setValueNotifyingHost(1.0f);
		juce::Timer::callAfterDelay(100, [p]() { p->setValueNotifyingHost(0.0f); });
	}
}

void ObsidianBaseMidiComponent::pushSliderToParam(Binding &b)
{
	if (!b.slider)
		return;
	auto *p = getParam(b.suffix);
	if (!p)
		return;

	auto &apvts = audioProcessor.getParameterTreeState();
	auto range = apvts.getParameterRange(fullParamId(b.suffix));

	float value = (float)b.slider->getValue();
	if (std::isnan(value) || std::isinf(value))
		return;

	p->setValueNotifyingHost(range.convertTo0to1(value));
}

void ObsidianBaseMidiComponent::syncSliderRange(juce::Slider &s, const juce::String &paramId)
{
	auto range = audioProcessor.getParameterTreeState().getParameterRange(paramId);
	auto param = audioProcessor.getParameterTreeState().getParameter(paramId);

	if (param == nullptr)
		return;
	float normalizedValue = param->getValue();
	float actualValue = range.convertFrom0to1(normalizedValue);

	float normalizedDefault = param->getDefaultValue();
	float actualDefaultValue = range.convertFrom0to1(normalizedDefault);

	s.setRange(range.start, range.end, range.interval);
	s.setSkewFactor(range.skew, range.symmetricSkew);
	s.setValue(actualValue, juce::dontSendNotification);
	s.setDoubleClickReturnValue(true, actualDefaultValue);
}

void ObsidianBaseMidiComponent::pushButtonToParam(Binding &b)
{
	if (!b.button)
		return;
	auto *p = getParam(b.suffix);
	if (!p)
		return;

	if (b.momentary)
	{
		p->setValueNotifyingHost(1.0f);
		juce::Timer::callAfterDelay(100, [p]() { p->setValueNotifyingHost(0.0f); });
	}
	else
	{
		p->setValueNotifyingHost(b.button->getToggleState() ? 1.0f : 0.0f);
	}
}

void ObsidianBaseMidiComponent::applyParamToBinding(Binding &b, float normalizedValue)
{
	auto &apvts = audioProcessor.getParameterTreeState();

	if (b.slider)
	{
		if (b.slider->isMouseButtonDown())
			return;
		auto range = apvts.getParameterRange(fullParamId(b.suffix));
		float newVal = range.convertFrom0to1(normalizedValue);
		if (std::abs((float)b.slider->getValue() - newVal) < range.interval * .5f)
			return;
		b.slider->setValue(newVal, juce::dontSendNotification);
	}
	else if (b.button)
	{
		if (b.momentary)
			return;
		b.button->setToggleState(normalizedValue > 0.5f, juce::dontSendNotification);
	}
}

void ObsidianBaseMidiComponent::subscribeToParam(const juce::String &paramSuffix)
{
	auto fullId = fullParamId(paramSuffix);
	if (std::find(listenedParams.begin(), listenedParams.end(), fullId) != listenedParams.end())
		return;

	if (auto *p = audioProcessor.getParameterTreeState().getParameter(fullId))
	{
		p->addListener(this);
		listenedParams.push_back(fullId);
	}
}

void ObsidianBaseMidiComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	if (isDestroyed.load())
		return;

	juce::WeakReference<juce::Component> safeThis(this);
	juce::MessageManager::callAsync(
	    [this, safeThis, parameterIndex, newValue]()
	    {
		    if (safeThis == nullptr || isDestroyed.load())
			    return;

		    auto &allParams = audioProcessor.AudioProcessor::getParameters();
		    if (parameterIndex < 0 || parameterIndex >= allParams.size())
			    return;
		    auto *paramByID = dynamic_cast<juce::RangedAudioParameter *>(allParams[parameterIndex]);
		    if (!paramByID)
			    return;
		    juce::String fullId = paramByID->paramID;

		    if (auto *b = findBindingByParamId(fullId))
			    applyParamToBinding(*b, newValue);

		    auto prefix = getParameterPrefix();
		    auto suffix = prefix.isEmpty() ? fullId : fullId.fromFirstOccurrenceOf(prefix, false, false);
		    onParameterChangedUI(suffix, newValue);
	    });
}

void ObsidianBaseMidiComponent::onParameterChangedUI(const juce::String &, float)
{
}

void ObsidianBaseMidiComponent::registerMidiLearn(const juce::String &paramSuffix, MidiLearnableBase *component,
                                                  std::function<void(float)> uiCallback)
{
	if (!component)
		return;

	auto fullId = fullParamId(paramSuffix);
	auto description = getMidiLearnDescriptionPrefix() + paramSuffix;

	component->onMidiLearn = [this, fullId, description, component, uiCallback]()
	{
		if (isDestroyed.load())
			return;
		if (!audioProcessor.getActiveEditor())
			return;

		juce::MessageManager::callAsync(
		    [this, description]()
		    {
			    if (isDestroyed.load())
				    return;
			    if (auto *editor = dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			    {
				    editor->statusLabel.setText("Learning MIDI for " + description + "...", juce::dontSendNotification);
				    editor->uiStatusManager->updateLCD();
			    }
		    });

		audioProcessor.getMidiLearnManager().startLearning(fullId, &audioProcessor, uiCallback, description, component);
	};

	component->onMidiRemove = [this, fullId]()
	{
		if (isDestroyed.load())
			return;
		audioProcessor.getMidiLearnManager().removeMappingForParameter(fullId);
	};

	if (uiCallback)
		audioProcessor.getMidiLearnManager().registerUICallback(fullId, uiCallback);
}

void ObsidianBaseMidiComponent::syncBindingsFromParameters()
{
	for (auto &b : bindings)
	{
		if (auto *p = getParam(b->suffix))
			applyParamToBinding(*b, p->getValue());
	}
}

void ObsidianBaseMidiComponent::clearAllBindings()
{
	auto &apvts = audioProcessor.getParameterTreeState();
	for (auto &id : listenedParams)
		if (auto *p = apvts.getParameter(id))
			p->removeListener(this);
	listenedParams.clear();

	for (auto &b : bindings)
	{
		if (b->slider)
			b->slider->onValueChange = nullptr;
		if (b->button)
			b->button->onClick = nullptr;
	}
	bindings.clear();
}
