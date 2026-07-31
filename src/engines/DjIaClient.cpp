#include "DjIaClient.h"

DjIaClient::DjIaClient(const juce::String &apiKey, const juce::String &baseUrl)
    : apiKey(apiKey), baseUrl(baseUrl + "/api/v1")
{
}

void DjIaClient::setApiKey(const juce::String &newApiKey)
{
	std::lock_guard<std::mutex> lock(mutex);
	apiKey = newApiKey;
}

juce::String DjIaClient::getApiKey() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return apiKey;
}

void DjIaClient::setBaseUrl(const juce::String &newBaseUrl)
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
}

std::shared_ptr<juce::WebInputStream> DjIaClient::createTrackedStream(const juce::URL &url,
                                                                      const juce::URL::InputStreamOptions &options)
{
	if (cancelled.load())
		return nullptr;

	auto stream = std::shared_ptr<juce::WebInputStream>(
	    new juce::WebInputStream(url, options.getParameterHandling() == juce::URL::ParameterHandling::inPostData));

	stream->withExtraHeaders(options.getExtraHeaders());
	stream->withConnectionTimeout(options.getConnectionTimeoutMs());

	{
		std::lock_guard<std::mutex> lock(streamsMutex);
		activeStreams.erase(std::remove_if(activeStreams.begin(), activeStreams.end(),
		                                   [](const std::weak_ptr<juce::WebInputStream> &w) { return w.expired(); }),
		                    activeStreams.end());
		activeStreams.push_back(stream);
	}

	if (cancelled.load())
	{
		stream->cancel();
		return nullptr;
	}

	return stream;
}

void DjIaClient::cancelPendingRequests()
{
	cancelled.store(true);
	std::lock_guard<std::mutex> lock(streamsMutex);
	for (auto &weak : activeStreams)
	{
		if (auto s = weak.lock())
			s->cancel();
	}
	activeStreams.clear();
}

DjIaClient::CreditsInfo DjIaClient::checkCredits(int timeoutMS)
{
	CreditsInfo result;
	try
	{
		if (cancelled.load())
			throw std::runtime_error("Cancelled");

		juce::String currentBaseUrl, currentApiKey;
		{
			std::lock_guard<std::mutex> lock(mutex);
			currentBaseUrl = baseUrl;
			currentApiKey = apiKey;
		}
		if (currentBaseUrl.isEmpty())
			throw std::runtime_error("Server URL not configured");

		juce::String headerString = "Content-Type: application/json\n";
		if (currentApiKey.isNotEmpty())
			headerString += "X-API-Key: " + currentApiKey + "\n";

		auto url = juce::URL(currentBaseUrl + "/auth/credits/check/vst");
		auto stream = std::shared_ptr<juce::WebInputStream>(new juce::WebInputStream(url, false));
		stream->withExtraHeaders(headerString);
		stream->withConnectionTimeout(timeoutMS);

		{
			std::lock_guard<std::mutex> lock(streamsMutex);
			if (cancelled.load())
				throw std::runtime_error("Cancelled");
			activeStreams.push_back(stream);
		}

		if (!stream->connect(nullptr))
			throw std::runtime_error("Cannot connect to server");

		int statusCode = stream->getStatusCode();
		if (statusCode != 200)
			throw std::runtime_error("HTTP Error " + std::to_string(statusCode));

		juce::String responseText = stream->readEntireStreamAsString();

		if (cancelled.load())
			throw std::runtime_error("Cancelled during read");

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
			throw std::runtime_error("Invalid JSON response");
	}
	catch (const std::exception &e)
	{
		result.success = false;
		result.errorMessage = e.what();
	}
	return result;
}

DjIaClient::LoopResponse DjIaClient::generateLoop(const LoopRequest &request, double sampleRate, int requestTimeoutMS)
{
	try
	{
		juce::var jsonRequest(new juce::DynamicObject());
		float bpm = request.bpm;
		if (bpm < 0.0f)
			bpm = 110.0f;
		jsonRequest.getDynamicObject()->setProperty("prompt", request.prompt);
		jsonRequest.getDynamicObject()->setProperty("bpm", bpm);
		jsonRequest.getDynamicObject()->setProperty("key", request.key);
		jsonRequest.getDynamicObject()->setProperty("model", request.model);
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
			for (const auto &keyword : request.keywords)
				keywordsArray.add(juce::var(keyword));
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
			headerString += "X-API-Key: " + currentApiKey + "\n";

		if (currentBaseUrl.isEmpty())
			throw std::runtime_error("Server URL not configured. Please set server URL in settings.");

		if (!currentBaseUrl.startsWithIgnoreCase("http"))
			throw std::runtime_error("Invalid server URL format. Must start with http:// or https://");

		int statusCode = 0;
		juce::StringPairArray responseHeaders;
		auto url = juce::URL(currentBaseUrl + "/generate").withPOSTData(jsonString);
		auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
		                   .withStatusCode(&statusCode)
		                   .withResponseHeaders(&responseHeaders)
		                   .withExtraHeaders(headerString)
		                   .withConnectionTimeoutMs(requestTimeoutMS);

		auto response = url.createInputStream(options);
		if (!response)
			throw std::runtime_error(("Cannot connect to server at " + currentBaseUrl +
			                          ". Please check: Server is running, URL is correct, Network connection")
			                             .toStdString());

		if (statusCode == 403)
			throw std::runtime_error(
			    "Authentication failed: Invalid or expired API key. Please check your credentials.");
		else if (statusCode == 401)
			throw std::runtime_error("Authentication failed: API key required or invalid.");
		else if (statusCode == 422)
			throw std::runtime_error(
			    "Invalid request: The server could not process your request. Please check your prompt and parameters.");
		else if (statusCode == 500)
			throw std::runtime_error(
			    "Server error: The audio generation service is temporarily unavailable. Please try again later.");
		else if (statusCode == 503)
			throw std::runtime_error(
			    "Service unavailable: All GPU providers are currently busy. Please try again in a few moments.");
		else if (statusCode != 200)
			throw std::runtime_error("HTTP Error " + std::to_string(statusCode) + ": Request failed.");

		if (response->isExhausted())
			throw std::runtime_error("Server returned empty response. Server may be overloaded or misconfigured.");

		LoopResponse result;
		result.audioData = juce::File::createTempFile(".wav");
		juce::FileOutputStream stream(result.audioData);

		if (stream.openedOk())
			stream.writeFromInputStream(*response, response->getTotalLength());
		else
			throw std::runtime_error("Cannot create temporary file for audio data.");

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

		auto snappedBpmStr = responseHeaders["X-Snapped-BPM"];
		if (snappedBpmStr.isNotEmpty())
			result.snappedBpm = snappedBpmStr.getFloatValue();

		return result;
	}
	catch (const std::exception &e)
	{
		LoopResponse emptyResponse;
		emptyResponse.errorMessage = e.what();
		return emptyResponse;
	}
}
