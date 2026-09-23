#include "DjIaClient.h"

DjIaClient::DjIaClient(const juce::String &url) : baseUrl(url.trimCharactersAtEnd("/"))
{
}

void DjIaClient::setBaseUrl(const juce::String &newBaseUrl)
{
	std::lock_guard<std::mutex> lock(mutex);
	baseUrl = newBaseUrl.trimCharactersAtEnd("/");
}

std::shared_ptr<juce::WebInputStream> DjIaClient::createTrackedStream(const juce::URL &url,
                                                                      const juce::URL::InputStreamOptions &options)
{
	if (cancelled.load())
		return nullptr;

	auto stream = std::make_shared<juce::WebInputStream>(url, options.getParameterHandling() ==
	                                                              juce::URL::ParameterHandling::inPostData);

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
		if (auto s = weak.lock())
			s->cancel();
	activeStreams.clear();
}

juce::String DjIaClient::extractErrorDetail(const juce::String &body)
{
	auto parsed = juce::JSON::parse(body);
	if (auto *obj = parsed.getDynamicObject())
	{
		auto detail = obj->getProperty("detail");
		if (detail.isString())
			return detail.toString();

		if (auto *arr = detail.getArray(); arr != nullptr && !arr->isEmpty())
			if (auto *first = (*arr)[0].getDynamicObject())
				return first->getProperty("msg").toString();
	}
	return {};
}

DjIaClient::LoopResponse DjIaClient::generateLoop(const LoopRequest &request, double sampleRate, int requestTimeoutMS)
{
	juce::ignoreUnused(sampleRate); 
	cancelled.store(false);

	juce::File tempFile;

	try
	{
		const int bpm = juce::roundToInt(request.bpm < 0.0f ? 110.0f : request.bpm);
		const int duration = juce::roundToInt(request.generationDuration);

		auto *obj = new juce::DynamicObject();
		juce::var jsonRequest(obj);
		obj->setProperty("prompt", request.prompt);
		obj->setProperty("bpm", bpm);
		obj->setProperty("duration", duration);
		if (request.model.isNotEmpty())
			obj->setProperty("model", request.model);
		if (request.key.isNotEmpty())
			obj->setProperty("key", request.key);

		const auto jsonString = juce::JSON::toString(jsonRequest);

		juce::String currentBaseUrl;
		{
			std::lock_guard<std::mutex> lock(mutex);
			currentBaseUrl = baseUrl;
		}

		if (currentBaseUrl.isEmpty())
			throw std::runtime_error("Server URL not configured. Please set server URL in settings.");

		if (!currentBaseUrl.startsWithIgnoreCase("http"))
			throw std::runtime_error("Invalid server URL format. Must start with http:// or https://");

		auto url = juce::URL(currentBaseUrl + "/process").withPOSTData(jsonString);
		auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
		                   .withExtraHeaders("Content-Type: application/json\n")
		                   .withConnectionTimeoutMs(requestTimeoutMS);

		auto stream = createTrackedStream(url, options);
		if (!stream)
			throw std::runtime_error("Request cancelled.");

		if (!stream->connect(nullptr))
		{
			if (cancelled.load())
				throw std::runtime_error("Request cancelled.");
			throw std::runtime_error(("Cannot connect to server at " + currentBaseUrl +
			                          ". Please check: Server is running, URL is correct, Network connection")
			                             .toStdString());
		}

		const int statusCode = stream->getStatusCode();
		const auto responseHeaders = stream->getResponseHeaders();

		if (statusCode != 200)
		{
			const auto detail = extractErrorDetail(stream->readEntireStreamAsString());
			juce::String message;

			if (statusCode == 422)
				message = "Invalid request";
			else if (statusCode == 503)
				message = "Server busy or model not available";
			else if (statusCode == 500)
				message = "Server error during generation";
			else
				message = "HTTP Error " + juce::String(statusCode);

			if (detail.isNotEmpty())
				message += ": " + detail;

			throw std::runtime_error(message.toStdString());
		}

		tempFile = juce::File::createTempFile(".wav");
		{
			juce::FileOutputStream out(tempFile);
			if (!out.openedOk())
				throw std::runtime_error("Cannot create temporary file for audio data.");

			out.writeFromInputStream(*stream, -1);
			out.flush();
		}

		if (cancelled.load())
			throw std::runtime_error("Request cancelled.");

		if (tempFile.getSize() == 0)
			throw std::runtime_error("Server returned empty response. Server may be overloaded or misconfigured.");

		LoopResponse result;
		result.audioData = tempFile;
		result.bpm = static_cast<float>(bpm);
		result.key = request.key;

		const auto durationStr = responseHeaders["X-Duration"];
		result.duration = durationStr.isNotEmpty() ? durationStr.getFloatValue() : static_cast<float>(duration);

		const auto snappedBpmStr = responseHeaders["X-Snapped-BPM"];
		if (snappedBpmStr.isNotEmpty())
			result.snappedBpm = snappedBpmStr.getFloatValue();

		const auto seedStr = responseHeaders["X-Seed"];
		if (seedStr.isNotEmpty())
			result.seed = seedStr.getIntValue();

		return result;
	}
	catch (const std::exception &e)
	{
		if (tempFile.existsAsFile())
			tempFile.deleteFile();

		LoopResponse emptyResponse;
		emptyResponse.errorMessage = e.what();
		return emptyResponse;
	}
}