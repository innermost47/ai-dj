#pragma once
#include "JuceHeader.h"
#include "data/TrackData.h"
#include "engines/DjIaClient.h"

class DjIaVstProcessor;

class GenerationManager
{
public:
	GenerationManager(DjIaVstProcessor& processor);

	void generateLoop(const DjIaClient::LoopRequest& request, const juce::String& targetTrackId);
	void generateLoopAPI(const DjIaClient::LoopRequest& request, const juce::String& trackId);
	void generateLoopLocal(const DjIaClient::LoopRequest& request, const juce::String& trackId);
	void notifyGenerationComplete(const juce::String& trackId, const juce::String& message);
	void generateSampleWithImage(const juce::String& trackId, const juce::String& base64Image, const juce::StringArray& keywords);
	void generateLoopWithImage(const DjIaClient::LoopRequest& request, const juce::String& trackId, int timeoutMS);
	void reEnableCanvasGenerate();
	void generateLoopFromMidi(const juce::String& trackId);
	void generateLoopFromGlobalSettings();
	void triggerGlobalGeneration();
	void syncSelectedTrackWithGlobalPrompt();
	void handleGenerate();
	void clearPendingAudio();

private:
	DjIaVstProcessor& audioProcessor;

	juce::CriticalSection apiLock;

	juce::File createTempAudioFile(const std::vector<float>& audioData, float /*duration*/);
};