#pragma once
#include "TrackData.h"
#include <JuceHeader.h>

class DjIaVstProcessor;

class StateManager
{
  public:
	StateManager(DjIaVstProcessor &processor);

	juce::ValueTree saveState() const;
	void loadState(const juce::ValueTree &state);
	void getStateInformation(juce::MemoryBlock &destData);
	void setStateInformation(const void *data, int sizeInBytes);
	bool saveToFile(const juce::File &file);
	bool loadFromFile(const juce::File &file);

	juce::File getLineageSidecarFile(const juce::String &projectId) const;

	static juce::File getDefaultSessionsFolder();

  private:
	DjIaVstProcessor &audioProcessor;
};