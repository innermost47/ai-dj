#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

class DjIaClient
{
  public:
	struct LoopRequest
	{
		juce::String prompt;
		juce::String model;     
		float generationDuration;
		float bpm;              
		juce::String key;       

		LoopRequest() : prompt(""), model(""), generationDuration(6.0f), bpm(120.0f), key("")
		{
		}
	};

	struct LoopResponse
	{
		juce::File audioData;
		float duration;
		float bpm;
		float snappedBpm = -1.0f;
		int seed = -1;
		juce::String key;
		juce::String errorMessage = "";

		LoopResponse() : duration(0.0f), bpm(120.0f)
		{
		}
	};

	explicit DjIaClient(const juce::String &baseUrl = "http://localhost:8000");

	LoopResponse generateLoop(const LoopRequest &request, double sampleRate, int requestTimeoutMS);

	void setBaseUrl(const juce::String &newBaseUrl);
	void cancelPendingRequests();

  private:
	mutable std::mutex mutex;
	juce::String baseUrl;

	std::mutex streamsMutex;
	std::vector<std::weak_ptr<juce::WebInputStream>> activeStreams;
	std::atomic<bool> cancelled{false};

	std::shared_ptr<juce::WebInputStream> createTrackedStream(const juce::URL &url,
	                                                          const juce::URL::InputStreamOptions &options);

	static juce::String extractErrorDetail(const juce::String &body);
};