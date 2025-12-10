#pragma once
#include <JuceHeader.h>

class DjIaVstProcessor;

struct MidiMapping
{
    int midiType;
    int midiNumber;
    int midiChannel;
    juce::String parameterName;
    juce::String description;
    DjIaVstProcessor *processor;
    std::function<void(float)> uiCallback;
};