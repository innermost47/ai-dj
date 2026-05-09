#pragma once
#include <JuceHeader.h>
#include <mutex>

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
		bool useImage = false;
		juce::String imageBase64 = "";
		juce::StringArray keywords;

		LoopRequest()
		    : prompt(""), model(""), generationDuration(6.0f), bpm(120.0f), key(""), useImage(false), imageBase64("")
		{
		}
	};

	struct LoopResponse
	{
		juce::File audioData;
		float duration;
		float bpm;
		float detectedBpm;
		juce::String key;
		juce::String errorMessage = "";
		int creditsRemaining = -1;
		bool isUnlimitedKey = false;
		int totalCredits = -1;
		int usedCredits = -1;

		LoopResponse() : duration(0.0f), bpm(120.0f), detectedBpm(-1.0f)
		{
		}
	};

	struct CreditsInfo
	{
		int creditsRemaining = 0;
		int creditsTotal = 0;
		bool canGenerateStandard = false;
		int costStandard = 0;
		bool success = false;
		juce::String errorMessage = "";
	};

	DjIaClient(const juce::String &apiKey = "", const juce::String &baseUrl = "http://localhost:8000");

	void setApiKey(const juce::String &newApiKey);
	juce::String getApiKey() const;
	void setBaseUrl(const juce::String &newBaseUrl);
	CreditsInfo checkCredits(int timeoutMS = 10000);
	LoopResponse generateLoop(const LoopRequest &request, double sampleRate, int requestTimeoutMS, bool bypassLLM);

  private:
	mutable std::mutex mutex;
	juce::String apiKey;
	juce::String baseUrl;
};