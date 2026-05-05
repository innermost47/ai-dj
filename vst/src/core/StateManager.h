#pragma once
#include "JuceHeader.h"
#include "data/TrackData.h"

class DjIaVstProcessor;

class StateManager
{
public:
	StateManager(DjIaVstProcessor& processor);

	juce::ValueTree saveState() const;
	void loadState(const juce::ValueTree& state);
	void getStateInformation(juce::MemoryBlock& destData);
	void setStateInformation(const void* data, int sizeInBytes);

private:
	DjIaVstProcessor& audioProcessor;
};