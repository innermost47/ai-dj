#pragma once
#include <JuceHeader.h>
#include "MidiMapping.h"
#include "midi/MidiLearnableComponents.h"
#include <vector>
#include <functional>

class DjIaVstEditor;
class DjIaVstProcessor;

class MidiLearnManager : public juce::Timer
{
public:
	MidiLearnManager();
	~MidiLearnManager();

	void startLearning(const juce::String &parameterName,
					   DjIaVstProcessor *processor,
					   std::function<void(float)> uiCallback,
					   const juce::String &description,
					   MidiLearnableBase *component = nullptr);
	void stopLearning();
	bool processMidiForLearning(const juce::MidiMessage &message);
	void processMidiMappings(const juce::MidiMessage &message);
	void removeMapping(juce::String parameterName);
	void clearAllMappings();
	std::vector<MidiMapping> getAllMappings() const { return mappings; }
	bool isLearningActive() const { return isLearning; }
	void registerUICallback(const juce::String &parameterName,
							std::function<void(float)> callback);
	void restoreUICallbacks();
	void addMapping(const MidiMapping &midiMapping);
	bool isBooleanParameter(const juce::String &parameterName);
	std::atomic<bool> mustCheckForMidiEvent{false};
	std::atomic<int> changedPlaySlotIndex{-1};
	std::atomic<int> changedGenerateSlotIndex{-1};
	void setEditor(DjIaVstEditor *editor) { currentEditor = editor; }
	bool removeMappingForParameter(const juce::String &parameterName);
	void setProcessor(DjIaVstProcessor *p) { learningProcessor = p; }
	DjIaVstProcessor *getProcessor() const { return learningProcessor; }
	void loadDefaultMappings(DjIaVstProcessor *processor);

private:
	void timerCallback() override;
	juce::CriticalSection learnLock;
	bool isLearning = false;
	std::map<juce::String, std::function<void(float)>> registeredUICallbacks;
	std::function<void(float)> learningUiCallback;
	DjIaVstProcessor *learningProcessor = nullptr;
	juce::String learningDescription;
	juce::String learningParameter;
	std::vector<MidiMapping> mappings;
	bool flashState = false;
	juce::Colour originalColour;
	int originalColourId = 0;
	static constexpr int LEARN_TIMEOUT_MS = 10000;
	juce::int64 learnStartTime = 0;
	MidiMapping learningMapping;
	DjIaVstEditor *currentEditor = nullptr;
	MidiLearnableBase *currentLearningComponent = nullptr;
	void showStatus(const MidiMapping &mapping, const juce::String &text, bool isWarning);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnManager)
};