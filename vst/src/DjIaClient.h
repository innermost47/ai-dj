#pragma once
#include "./JuceHeader.h"
#include <mutex>

class DjIaClient
{
public:
	struct LoopRequest
	{
		juce::String prompt;
		float generationDuration;
		float bpm;
		juce::String key;
		bool useImage = false;
		juce::String imageBase64 = "";
		juce::StringArray keywords;

		LoopRequest()
			: prompt(""),
			generationDuration(6.0f),
			bpm(120.0f),
			key(""),
			useImage(false),
			imageBase64("")
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

		LoopResponse()
			: duration(0.0f), bpm(120.0f), detectedBpm(-1.0f)
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

	DjIaClient(const juce::String& apiKey = "", const juce::String& baseUrl = "http://localhost:8000")
		: apiKey(apiKey), baseUrl(baseUrl + "/api/v1")
	{
	}

	void setApiKey(const juce::String& newApiKey)
	{
		std::lock_guard<std::mutex> lock(mutex);
		apiKey = newApiKey;
		DBG("DjIaClient: API key updated");
	}

	juce::String getApiKey() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return apiKey;
	}

	juce::String getBaseUrl() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return baseUrl;
	}

	void setBaseUrl(const juce::String& newBaseUrl)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (newBaseUrl.endsWith("/"))
		{
			baseUrl = newBaseUrl.dropLastCharacters(1) + "/api/v1";
		}
		else
		{
			baseUrl = newBaseUrl + "/api/v1";
		}
		DBG("DjIaClient: Base URL updated to: " + baseUrl);
	}

	CreditsInfo checkCredits(int timeoutMS = 10000)
	{
		CreditsInfo result;

		try
		{
			juce::String currentBaseUrl;
			juce::String currentApiKey;

			{
				std::lock_guard<std::mutex> lock(mutex);
				currentBaseUrl = baseUrl;
				currentApiKey = apiKey;
			}

			if (currentBaseUrl.isEmpty())
			{
				throw std::runtime_error("Server URL not configured");
			}

			juce::String headerString = "Content-Type: application/json\n";
			if (currentApiKey.isNotEmpty())
			{
				headerString += "X-API-Key: " + currentApiKey + "\n";
			}

			int statusCode = 0;
			juce::StringPairArray responseHeaders;

			auto url = juce::URL(currentBaseUrl + "/auth/credits/check/vst");
			auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
				.withStatusCode(&statusCode)
				.withResponseHeaders(&responseHeaders)
				.withExtraHeaders(headerString)
				.withConnectionTimeoutMs(timeoutMS);

			auto response = url.createInputStream(options);

			if (!response)
			{
				throw std::runtime_error("Cannot connect to server");
			}

			if (statusCode != 200)
			{
				throw std::runtime_error("HTTP Error " + std::to_string(statusCode));
			}

			juce::String responseText = response->readEntireStreamAsString();

			auto jsonResponse = juce::JSON::parse(responseText);

			if (jsonResponse.isObject())
			{
				auto obj = jsonResponse.getDynamicObject();

				result.creditsRemaining = obj->getProperty("credits_remaining");
				result.creditsTotal = obj->getProperty("credits_total");
				result.canGenerateStandard = obj->getProperty("can_generate_standard");
				result.costStandard = obj->getProperty("cost_standard");
				result.success = true;
			}
			else
			{
				throw std::runtime_error("Invalid JSON response");
			}
		}
		catch (const std::exception& e)
		{
			result.success = false;
			result.errorMessage = e.what();
		}

		return result;
	}

	LoopResponse generateLoop(const LoopRequest& request, double sampleRate, int requestTimeoutMS)
	{
		try
		{
			juce::var jsonRequest(new juce::DynamicObject());
			float bpm = request.bpm;
			if (bpm < 0.0f)
			{
				bpm = 110.0f;
			}
			jsonRequest.getDynamicObject()->setProperty("prompt", request.prompt);
			jsonRequest.getDynamicObject()->setProperty("bpm", bpm);
			jsonRequest.getDynamicObject()->setProperty("key", request.key);
			jsonRequest.getDynamicObject()->setProperty("sample_rate", sampleRate);
			jsonRequest.getDynamicObject()->setProperty("generation_duration", request.generationDuration);
			if (request.useImage && !request.imageBase64.isEmpty())
			{
				jsonRequest.getDynamicObject()->setProperty("use_image", true);
				jsonRequest.getDynamicObject()->setProperty("image_base64", request.imageBase64);
			}

			if (request.keywords.size() > 0)
			{
				juce::Array<juce::var> keywordsArray;
				for (const auto& keyword : request.keywords)
				{
					keywordsArray.add(juce::var(keyword));
				}
				jsonRequest.getDynamicObject()->setProperty("keywords", juce::var(keywordsArray));
			}

			auto jsonString = juce::JSON::toString(jsonRequest);

			juce::String currentBaseUrl;
			juce::String currentApiKey;

			{
				std::lock_guard<std::mutex> lock(mutex);
				currentBaseUrl = baseUrl;
				currentApiKey = apiKey;
			}

			juce::String headerString = "Content-Type: application/json\n";
			if (currentApiKey.isNotEmpty())
			{
				headerString += "X-API-Key: " + currentApiKey + "\n";
			}

			if (currentBaseUrl.isEmpty())
			{
				DBG("ERROR: Base URL is empty!");
				throw std::runtime_error("Server URL not configured. Please set server URL in settings.");
			}

			if (!currentBaseUrl.startsWithIgnoreCase("http"))
			{
				DBG("ERROR: Invalid URL format: " + currentBaseUrl);
				throw std::runtime_error("Invalid server URL format. Must start with http:// or https://");
			}

			int statusCode = 0;
			juce::StringPairArray responseHeaders;
			auto url = juce::URL(currentBaseUrl + "/generate")
				.withPOSTData(jsonString);
			auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
				.withStatusCode(&statusCode)
				.withResponseHeaders(&responseHeaders)
				.withExtraHeaders(headerString)
				.withConnectionTimeoutMs(requestTimeoutMS);

			auto response = url.createInputStream(options);
			if (!response)
			{
				DBG("ERROR: Failed to connect to server");
				throw std::runtime_error(("Cannot connect to server at " + currentBaseUrl +
					". Please check: Server is running, URL is correct, Network connection")
					.toStdString());
			}

			DBG("HTTP Status Code: " + juce::String(statusCode));

			if (statusCode == 403)
			{
				DBG("ERROR: HTTP 403 Forbidden");
				throw std::runtime_error("Authentication failed: Invalid or expired API key. Please check your credentials.");
			}
			else if (statusCode == 401)
			{
				DBG("ERROR: HTTP 401 Unauthorized");
				throw std::runtime_error("Authentication failed: API key required or invalid.");
			}
			else if (statusCode == 422)
			{
				DBG("ERROR: HTTP 422 Unprocessable Entity");
				throw std::runtime_error("Invalid request: The server could not process your request. Please check your prompt and parameters.");
			}
			else if (statusCode == 500)
			{
				DBG("ERROR: HTTP 500 Internal Server Error");
				throw std::runtime_error("Server error: The audio generation service is temporarily unavailable. Please try again later.");
			}
			else if (statusCode == 503)
			{
				DBG("ERROR: HTTP 503 Service Unavailable");
				throw std::runtime_error("Service unavailable: All GPU providers are currently busy. Please try again in a few moments.");
			}
			else if (statusCode != 200)
			{
				DBG("ERROR: HTTP " + juce::String(statusCode));
				throw std::runtime_error("HTTP Error " + std::to_string(statusCode) + ": Request failed.");
			}

			if (response->isExhausted())
			{
				DBG("ERROR: Empty response from server");
				throw std::runtime_error("Server returned empty response. Server may be overloaded or misconfigured.");
			}

			LoopResponse result;
			result.audioData = juce::File::createTempFile(".wav");
			juce::FileOutputStream stream(result.audioData);
			if (stream.openedOk())
			{
				stream.writeFromInputStream(*response, response->getTotalLength());
			}
			else
			{
				DBG("ERROR: Cannot create temp file");
				throw std::runtime_error("Cannot create temporary file for audio data.");
			}
			result.duration = request.generationDuration;
			result.bpm = bpm;
			result.key = request.key;
			juce::String creditsRemaining = responseHeaders["X-Credits-Remaining"];
			juce::String duration = responseHeaders["X-Duration"];
			if (creditsRemaining.isNotEmpty())
			{
				if (creditsRemaining == "unlimited")
				{
					result.isUnlimitedKey = true;
					result.creditsRemaining = -1;
				}
				else
				{
					result.creditsRemaining = creditsRemaining.getIntValue();
					result.isUnlimitedKey = false;
				}
			}

			auto detectedBpmStr = responseHeaders["X-Detected-BPM"];
			if (detectedBpmStr.isNotEmpty())
			{
				result.detectedBpm = detectedBpmStr.getFloatValue();
				DBG("BPM detected server: " + juce::String(result.detectedBpm));
			}
			else
			{
				DBG("No X-Detected-BPM header from server");
			}

			DBG("WAV file created: " + result.audioData.getFullPathName() +
				" (" + juce::String(result.audioData.getSize()) + " bytes)");

			return result;
		}
		catch (const std::exception& e)
		{
			DBG("API Error: " + juce::String(e.what()));
			LoopResponse emptyResponse;
			emptyResponse.errorMessage = e.what();
			return emptyResponse;
		}
	}

private:
	mutable std::mutex mutex;
	juce::String apiKey;
	juce::String baseUrl;
};