#include "SampleBank.h"

SampleBank::SampleBank()
{
	bankDirectory = getBankDirectory();
	bankIndexFile = bankDirectory.getChildFile("sample_bank.json");
	ensureBankDirectoryExists();
	loadBankData();
	if (!bankIndexFile.exists())
	{
		saveBankData();
	}
}

juce::String SampleBank::addSample(const juce::String& prompt,
	const juce::File& audioFile,
	float bpm,
	const juce::String& key)
{
	juce::ScopedLock lock(bankLock);

	auto entry = std::make_unique<SampleBankEntry>();
	entry->id = juce::Uuid().toString();
	entry->originalPrompt = prompt;
	entry->creationTime = juce::Time::getCurrentTime();
	entry->bpm = bpm;
	entry->key = key;

	auto& categories = entry->categories;


	juce::String lowerPrompt = prompt.toLowerCase();
	if (lowerPrompt.contains("ambient") || lowerPrompt.contains("pad"))
		categories.push_back("Ambient");
	if (lowerPrompt.contains("house"))
		categories.push_back("House");
	if (lowerPrompt.contains("techno"))
		categories.push_back("Techno");
	if (lowerPrompt.contains("hip hop") || lowerPrompt.contains("hiphop"))
		categories.push_back("Hip-Hop");
	if (lowerPrompt.contains("jazz"))
		categories.push_back("Jazz");
	if (lowerPrompt.contains("rock"))
		categories.push_back("Rock");

	if (categories.empty())
		categories.push_back("Electronic");

	entry->filename = createSafeFilename(prompt, entry->creationTime);

	juce::File destinationFile = bankDirectory.getChildFile(entry->filename);
	if (!audioFile.copyFileTo(destinationFile))
	{
		DBG("Failed to copy sample to bank: " + destinationFile.getFullPathName());
		return {};
	}

	entry->filePath = destinationFile.getFullPathName();

	analyzeSampleFile(entry.get(), destinationFile);

	juce::String sampleId = entry->id;
	samples.push_back(std::move(entry));

	saveBankData();

	if (onBankChanged)
		onBankChanged();

	DBG("Sample added to bank: " + sampleId + " -> " + destinationFile.getFileName());
	return sampleId;
}

bool SampleBank::removeSample(const juce::String& sampleId)
{
	juce::File fileToDelete;
	bool needsCallback = false;

	{
		juce::ScopedLock lock(bankLock);

		auto it = std::find_if(samples.begin(), samples.end(),
			[&sampleId](const std::unique_ptr<SampleBankEntry>& entry)
			{
				return entry->id == sampleId;
			});

		if (it == samples.end())
			return false;

		fileToDelete = juce::File((*it)->filePath);

		samples.erase(it);
		needsCallback = true;

		saveBankData();

	}

	if (fileToDelete.exists())
	{
		fileToDelete.deleteFile();
	}

	if (needsCallback && onBankChanged)
	{
		onBankChanged();
	}

	return true;
}

SampleBankEntry* SampleBank::getSample(const juce::String& sampleId)
{
	juce::ScopedLock lock(bankLock);

	auto it = std::find_if(samples.begin(), samples.end(),
		[&sampleId](const std::unique_ptr<SampleBankEntry>& entry)
		{
			return entry->id == sampleId;
		});

	return (it != samples.end()) ? it->get() : nullptr;
}

std::vector<SampleBankEntry*> SampleBank::getAllSamples()
{
	juce::ScopedLock lock(bankLock);

	std::vector<SampleBankEntry*> result;
	for (auto& entry : samples)
	{
		result.push_back(entry.get());
	}
	return result;
}

std::vector<juce::String> SampleBank::getUnusedSamples() const
{
	juce::ScopedLock lock(bankLock);

	std::vector<juce::String> unused;
	for (const auto& entry : samples)
	{
		if (entry->usedInProjects.empty())
		{
			unused.push_back(entry->id);
		}
	}
	return unused;
}

int SampleBank::removeUnusedSamples()
{
	auto unusedIds = getUnusedSamples();
	int removedCount = 0;

	for (const auto& id : unusedIds)
	{
		if (removeSample(id))
			removedCount++;
	}

	if (removedCount > 0 && onBankChanged)
	{
		juce::MessageManager::callAsync([this]()
			{
				if (onBankChanged)
					onBankChanged();
			});
	}

	return removedCount;
}


void SampleBank::markSampleAsUsed(const juce::String& sampleId, const juce::String& projectId)
{
	bool needsSave = false;

	{
		juce::ScopedLock lock(bankLock);

		auto it = std::find_if(samples.begin(), samples.end(),
			[&sampleId](const std::unique_ptr<SampleBankEntry>& entry)
			{
				return entry->id == sampleId;
			});

		if (it != samples.end())
		{
			auto& projects = (*it)->usedInProjects;
			if (std::find(projects.begin(), projects.end(), projectId) == projects.end())
			{
				projects.push_back(projectId);
				needsSave = true;
			}
		}
	}

	if (needsSave)
	{
		saveBankData();
	}
}

void SampleBank::markSampleAsUnused(const juce::String& sampleId, const juce::String& projectId)
{
	juce::ScopedLock lock(bankLock);

	auto* entry = getSample(sampleId);
	if (entry)
	{
		auto& projects = entry->usedInProjects;
		projects.erase(std::remove(projects.begin(), projects.end(), projectId), projects.end());
		saveBankData();
	}
}

juce::String SampleBank::createSafeFilename(const juce::String& prompt, const juce::Time& timestamp)
{
	juce::String snakePrompt = promptToSnakeCase(prompt);
	juce::String timeString = timestamp.formatted("%Y%m%d_%H%M%S");
	return snakePrompt + "_" + timeString + ".wav";
}

juce::String SampleBank::promptToSnakeCase(const juce::String& prompt)
{
	juce::String result = prompt.toLowerCase();

	juce::String invalidChars = " !@#$%^&*()+-=[]{}|;':\",./<>?";
	for (int i = 0; i < invalidChars.length(); ++i)
	{
		result = result.replaceCharacter(invalidChars[i], '_');
	}

	while (result.contains("__"))
	{
		result = result.replace("__", "_");
	}

	if (result.startsWith("_"))
		result = result.substring(1);
	if (result.endsWith("_"))
		result = result.dropLastCharacters(1);

	if (result.length() > 50)
	{
		result = result.substring(0, 50);
	}

	return result.isEmpty() ? "sample" : result;
}

void SampleBank::analyzeSampleFile(SampleBankEntry* entry, const juce::File& audioFile)
{
	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();

	std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
	if (reader)
	{
		entry->duration = static_cast<float>(reader->lengthInSamples / reader->sampleRate);
		entry->sampleRate = reader->sampleRate;
		entry->numChannels = reader->numChannels;
		entry->numSamples = static_cast<int>(reader->lengthInSamples);
	}
}

juce::File SampleBank::getBankDirectory()
{
	return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("SampleBank");
}

void SampleBank::ensureBankDirectoryExists()
{
	if (!bankDirectory.exists())
	{
		bankDirectory.createDirectory();
	}
}

void SampleBank::saveBankData()
{
	try
	{
		if (!bankDirectory.exists())
		{
			auto result = bankDirectory.createDirectory();
			if (!result.wasOk())
				return;
		}

		juce::DynamicObject::Ptr bankData = new juce::DynamicObject();
		juce::Array<juce::var> samplesArray;

		for (const auto& entry : samples)
		{
			if (!entry) continue;

			juce::DynamicObject::Ptr sampleData = new juce::DynamicObject();

			sampleData->setProperty("id", entry->id.isEmpty() ? juce::Uuid().toString() : entry->id);
			sampleData->setProperty("filename", entry->filename);
			sampleData->setProperty("originalPrompt", entry->originalPrompt);
			sampleData->setProperty("filePath", entry->filePath);
			sampleData->setProperty("creationTime", entry->creationTime.toMilliseconds());
			sampleData->setProperty("duration", static_cast<double>(entry->duration));
			sampleData->setProperty("bpm", static_cast<double>(entry->bpm));
			sampleData->setProperty("key", entry->key);
			sampleData->setProperty("sampleRate", static_cast<double>(entry->sampleRate));
			sampleData->setProperty("numChannels", static_cast<int>(entry->numChannels));
			sampleData->setProperty("numSamples", static_cast<int>(entry->numSamples));

			juce::Array<juce::var> categoriesArray;
			for (const auto& category : entry->categories)
			{
				if (!category.isEmpty())
					categoriesArray.add(category);
			}
			sampleData->setProperty("categories", categoriesArray);

			juce::Array<juce::var> projectsArray;
			for (const auto& project : entry->usedInProjects)
			{
				if (!project.isEmpty())
					projectsArray.add(project);
			}
			sampleData->setProperty("usedInProjects", projectsArray);

			samplesArray.add(sampleData.get());
		}

		bankData->setProperty("samples", samplesArray);
		bankData->setProperty("version", "1.0");

		juce::String jsonString = juce::JSON::toString(juce::var(bankData.get()), true);

		if (jsonString.isEmpty())
			return;

		bankIndexFile.replaceWithText(jsonString);

	}
	catch (...)
	{
		return;
	}
}

void SampleBank::loadBankData()
{
	juce::ScopedLock lock(bankLock);
	if (!bankIndexFile.exists())
		return;

	juce::var bankJson = juce::JSON::parse(bankIndexFile);
	if (!bankJson.isObject())
		return;

	auto* bankObj = bankJson.getDynamicObject();
	if (!bankObj)
		return;

	auto samplesVar = bankObj->getProperty("samples");
	if (!samplesVar.isArray())
		return;

	auto* samplesArray = samplesVar.getArray();
	samples.clear();

	for (int i = 0; i < samplesArray->size(); ++i)
	{
		auto sampleVar = samplesArray->getUnchecked(i);
		if (!sampleVar.isObject())
			continue;

		auto* sampleObj = sampleVar.getDynamicObject();
		if (!sampleObj)
			continue;

		auto entry = std::make_unique<SampleBankEntry>();
		entry->id = sampleObj->getProperty("id").toString();
		entry->filename = sampleObj->getProperty("filename").toString();
		entry->originalPrompt = sampleObj->getProperty("originalPrompt").toString();
		entry->filePath = sampleObj->getProperty("filePath").toString();
		auto creationTimeVar = sampleObj->getProperty("creationTime");
		entry->creationTime = juce::Time(creationTimeVar.isVoid() ? 0 : (juce::int64)creationTimeVar);
		entry->duration = static_cast<float>(sampleObj->getProperty("duration"));
		entry->bpm = static_cast<float>(sampleObj->getProperty("bpm"));
		entry->key = sampleObj->getProperty("key").toString();
		entry->sampleRate = sampleObj->getProperty("sampleRate");
		entry->numChannels = sampleObj->getProperty("numChannels");
		entry->numSamples = sampleObj->getProperty("numSamples");

		auto categoriesVar = sampleObj->getProperty("categories");
		if (categoriesVar.isArray())
		{
			auto* categoriesArray = categoriesVar.getArray();
			for (int j = 0; j < categoriesArray->size(); ++j)
				entry->categories.push_back(categoriesArray->getUnchecked(j).toString());
		}

		auto projectsVar = sampleObj->getProperty("usedInProjects");
		if (projectsVar.isArray())
		{
			auto* projectsArray = projectsVar.getArray();
			for (int j = 0; j < projectsArray->size(); ++j)
				entry->usedInProjects.push_back(projectsArray->getUnchecked(j).toString());
		}

		juce::File sampleFile(entry->filePath);
		if (sampleFile.exists())
		{
			samples.push_back(std::move(entry));
		}
	}

	DBG("Loaded " + juce::String(samples.size()) + " samples from bank");
}