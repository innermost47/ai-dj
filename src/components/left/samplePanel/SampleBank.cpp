#include "SampleBank.h"
#include "config/version.h"

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

juce::String SampleBank::addSample(const juce::String &prompt, const juce::File &audioFile, float bpm,
                                   const juce::String &key, const juce::String &modelName, const juce::String &category)
{
	juce::ScopedLock lock(bankLock);

	auto entry = std::make_unique<SampleBankEntry>();
	entry->id = juce::Uuid().toString();
	entry->originalPrompt = prompt;
	entry->modelName = modelName;
	entry->creationTime = juce::Time::getCurrentTime();
	entry->bpm = bpm;
	entry->key = key;

	if (category.isNotEmpty())
	{
		entry->category = category;
	}
	else
	{
		const juce::String lowerPrompt = prompt.toLowerCase();
		if (lowerPrompt.contains("ambient") || lowerPrompt.contains("pad"))
			entry->category = "Ambient";
		else if (lowerPrompt.contains("house"))
			entry->category = "House";
		else if (lowerPrompt.contains("techno"))
			entry->category = "Techno";
		else if (lowerPrompt.contains("hip hop") || lowerPrompt.contains("hiphop"))
			entry->category = "Hip-Hop";
		else if (lowerPrompt.contains("jazz"))
			entry->category = "Jazz";
		else if (lowerPrompt.contains("rock"))
			entry->category = "Rock";
	}

	entry->filename = createSafeFilename(prompt, entry->creationTime);

	juce::File destinationFile = bankDirectory.getChildFile(entry->filename);
	if (!audioFile.copyFileTo(destinationFile))
	{
		return {};
	}

	entry->filePath = destinationFile.getFullPathName();

	analyzeSampleFile(entry.get(), destinationFile);

	juce::String sampleId = entry->id;
	samples.push_back(std::move(entry));

	saveBankData();

	if (onBankChanged)
		onBankChanged();

	return sampleId;
}

bool SampleBank::removeSample(const juce::String &sampleId)
{
	juce::File fileToDelete;
	juce::StringArray cachePathsToDelete;
	bool needsCallback = false;

	{
		juce::ScopedLock lock(bankLock);

		auto it =
		    std::find_if(samples.begin(), samples.end(),
		                 [&sampleId](const std::unique_ptr<SampleBankEntry> &entry) { return entry->id == sampleId; });

		if (it == samples.end())
			return false;

		fileToDelete = juce::File((*it)->filePath);

		cachePathsToDelete = (*it)->cacheFiles;

		samples.erase(it);
		needsCallback = true;

		saveBankData();
	}

	if (fileToDelete.exists())
		fileToDelete.deleteFile();

	for (const auto &p : cachePathsToDelete)
	{
		juce::File f(p);
		if (f.existsAsFile())
			f.deleteFile();
	}

	if (needsCallback && onBankChanged)
		onBankChanged();

	return true;
}

SampleBankEntry *SampleBank::getSample(const juce::String &sampleId)
{
	juce::ScopedLock lock(bankLock);

	auto it = std::find_if(samples.begin(), samples.end(), [&sampleId](const std::unique_ptr<SampleBankEntry> &entry)
	                       { return entry->id == sampleId; });

	return (it != samples.end()) ? it->get() : nullptr;
}

std::vector<SampleBankEntry *> SampleBank::getAllSamples()
{
	juce::ScopedLock lock(bankLock);

	std::vector<SampleBankEntry *> result;
	for (auto &entry : samples)
	{
		result.push_back(entry.get());
	}
	return result;
}

std::vector<juce::String> SampleBank::getUnusedSamples() const
{
	juce::ScopedLock lock(bankLock);

	std::vector<juce::String> unused;
	for (const auto &entry : samples)
	{
		if (entry->usedInProjects.empty())
		{
			unused.push_back(entry->id);
		}
	}
	return unused;
}

int SampleBank::removeSamples(const juce::StringArray &sampleIds)
{
	juce::StringArray pathsToDelete;
	int removedCount = 0;

	{
		juce::ScopedLock lock(bankLock);

		for (const auto &sampleId : sampleIds)
		{
			auto it =
			    std::find_if(samples.begin(), samples.end(), [&sampleId](const std::unique_ptr<SampleBankEntry> &entry)
			                 { return entry->id == sampleId; });
			if (it == samples.end())
				continue;

			pathsToDelete.add((*it)->filePath);
			pathsToDelete.addArray((*it)->cacheFiles);

			samples.erase(it);
			++removedCount;
		}

		if (removedCount > 0)
			saveBankData();
	}

	for (const auto &p : pathsToDelete)
	{
		juce::File f(p);
		if (f.existsAsFile())
			f.deleteFile();
	}

	if (removedCount > 0 && onBankChanged)
	{
		juce::WeakReference<SampleBank> safeThis(this);
		juce::MessageManager::callAsync(
		    [safeThis]()
		    {
			    if (safeThis && safeThis->onBankChanged)
				    safeThis->onBankChanged();
		    });
	}

	return removedCount;
}

int SampleBank::removeUnusedSamples()
{
	juce::StringArray ids;
	for (const auto &id : getUnusedSamples())
		ids.add(id);
	return removeSamples(ids);
}

void SampleBank::markSampleAsUsed(const juce::String &sampleId, const juce::String &projectId)
{
	bool needsSave = false;

	{
		juce::ScopedLock lock(bankLock);

		auto it =
		    std::find_if(samples.begin(), samples.end(),
		                 [&sampleId](const std::unique_ptr<SampleBankEntry> &entry) { return entry->id == sampleId; });

		if (it != samples.end())
		{
			auto &projects = (*it)->usedInProjects;
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

void SampleBank::markSampleAsUnused(const juce::String &sampleId, const juce::String &projectId)
{
	juce::ScopedLock lock(bankLock);

	auto *entry = getSample(sampleId);
	if (entry)
	{
		auto &projects = entry->usedInProjects;
		projects.erase(std::remove(projects.begin(), projects.end(), projectId), projects.end());
		saveBankData();
	}
}

juce::String SampleBank::createSafeFilename(const juce::String &prompt, const juce::Time &timestamp)
{
	juce::String snakePrompt = promptToSnakeCase(prompt);
	juce::String timeString = timestamp.formatted("%Y%m%d_%H%M%S");
	return snakePrompt + "_" + timeString + ".wav";
}

juce::String SampleBank::promptToSnakeCase(const juce::String &prompt)
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

void SampleBank::analyzeSampleFile(SampleBankEntry *entry, const juce::File &audioFile)
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
	    .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
	    .getChildFile(Obsidian::SAMPLE_BANK_DIR());
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

		for (const auto &entry : samples)
		{
			if (!entry)
				continue;

			juce::DynamicObject::Ptr sampleData = new juce::DynamicObject();

			sampleData->setProperty("id", entry->id.isEmpty() ? juce::Uuid().toString() : entry->id);
			sampleData->setProperty("filename", entry->filename);
			sampleData->setProperty("originalPrompt", entry->originalPrompt);
			sampleData->setProperty("description", entry->description);
			sampleData->setProperty("modelName", entry->modelName);
			sampleData->setProperty("filePath", entry->filePath);
			juce::Array<juce::var> cacheArray;
			for (const auto &p : entry->cacheFiles)
				cacheArray.add(p);
			sampleData->setProperty("cacheFiles", cacheArray);
			sampleData->setProperty("creationTime", entry->creationTime.toMilliseconds());
			sampleData->setProperty("duration", static_cast<double>(entry->duration));
			sampleData->setProperty("bpm", static_cast<double>(entry->bpm));
			sampleData->setProperty("key", entry->key);
			sampleData->setProperty("sampleRate", static_cast<double>(entry->sampleRate));
			sampleData->setProperty("numChannels", static_cast<int>(entry->numChannels));
			sampleData->setProperty("numSamples", static_cast<int>(entry->numSamples));

			sampleData->setProperty("category", entry->category);

			juce::Array<juce::var> projectsArray;
			for (const auto &project : entry->usedInProjects)
			{
				if (!project.isEmpty())
					projectsArray.add(project);
			}
			sampleData->setProperty("usedInProjects", projectsArray);

			samplesArray.add(sampleData.get());
		}

		bankData->setProperty("samples", samplesArray);
		bankData->setProperty("version", juce::String(Version::VERSION));

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

void SampleBank::runLegacyCategoriesMigration()
{
	juce::File legacyFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                            .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
	                            .getChildFile(Obsidian::CATEGORIES_FILE());

	if (!legacyFile.exists())
		return;

	if (onMigrateLegacyCategory == nullptr)
		return;

	auto json = juce::JSON::parse(legacyFile);
	if (!json.isObject())
	{
		legacyFile.moveFileTo(legacyFile.withFileExtension(".migrated"));
		return;
	}

	auto *obj = json.getDynamicObject();
	if (obj == nullptr)
		return;

	auto arrVar = obj->getProperty("categories");
	if (!arrVar.isArray())
	{
		legacyFile.moveFileTo(legacyFile.withFileExtension(".migrated"));
		return;
	}

	auto *arr = arrVar.getArray();
	int migratedCount = 0;

	for (int i = 0; i < arr->size(); ++i)
	{
		auto v = arr->getUnchecked(i);
		if (!v.isObject())
			continue;

		auto *catObj = v.getDynamicObject();
		if (catObj == nullptr)
			continue;

		const juce::String name = catObj->getProperty("name").toString();
		if (name.isEmpty())
			continue;

		if (onCheckCategoryExists && onCheckCategoryExists(name))
			continue;

		juce::Colour colour;
		auto colourVar = catObj->getProperty("colour");
		if (!colourVar.isVoid())
			colour = juce::Colour((juce::uint32)(int)colourVar);
		else
			colour = deriveColourFromName(name);

		onMigrateLegacyCategory(name, colour);
		++migratedCount;
	}

	juce::File archived = legacyFile.withFileExtension(".json.migrated");
	legacyFile.moveFileTo(archived);
}

juce::Colour SampleBank::deriveColourFromName(const juce::String &name)
{
	juce::uint32 hash = 2166136261u;
	for (int i = 0; i < name.length(); ++i)
	{
		hash ^= (juce::uint32)name[i];
		hash *= 16777619u;
	}

	const float hue = (float)(hash % 360u) / 360.0f;

	const float saturation = 0.55f;
	const float brightness = 0.75f;

	return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

void SampleBank::loadBankData()
{
	juce::ScopedLock lock(bankLock);
	if (!bankIndexFile.exists())
		return;

	juce::var bankJson = juce::JSON::parse(bankIndexFile);
	if (!bankJson.isObject())
		return;

	auto *bankObj = bankJson.getDynamicObject();
	if (!bankObj)
		return;

	auto samplesVar = bankObj->getProperty("samples");
	if (!samplesVar.isArray())
		return;

	auto *samplesArray = samplesVar.getArray();
	samples.clear();

	for (int i = 0; i < samplesArray->size(); ++i)
	{
		auto sampleVar = samplesArray->getUnchecked(i);
		if (!sampleVar.isObject())
			continue;

		auto *sampleObj = sampleVar.getDynamicObject();
		if (!sampleObj)
			continue;

		auto entry = std::make_unique<SampleBankEntry>();
		entry->id = sampleObj->getProperty("id").toString();
		entry->filename = sampleObj->getProperty("filename").toString();
		entry->originalPrompt = sampleObj->getProperty("originalPrompt").toString();
		entry->description = sampleObj->getProperty("description").toString();
		entry->modelName = sampleObj->getProperty("modelName").toString();
		entry->filePath = sampleObj->getProperty("filePath").toString();
		auto cacheVar = sampleObj->getProperty("cacheFiles");
		if (cacheVar.isArray())
			for (const auto &v : *cacheVar.getArray())
				entry->cacheFiles.add(v.toString());
		auto creationTimeVar = sampleObj->getProperty("creationTime");
		entry->creationTime = juce::Time(creationTimeVar.isVoid() ? 0 : (juce::int64)creationTimeVar);
		entry->duration = static_cast<float>(sampleObj->getProperty("duration"));
		entry->bpm = static_cast<float>(sampleObj->getProperty("bpm"));
		entry->key = sampleObj->getProperty("key").toString();
		entry->sampleRate = sampleObj->getProperty("sampleRate");
		entry->numChannels = sampleObj->getProperty("numChannels");
		entry->numSamples = sampleObj->getProperty("numSamples");

		auto categoryVar = sampleObj->getProperty("category");
		if (!categoryVar.isVoid() && categoryVar.toString().isNotEmpty())
			entry->category = categoryVar.toString();
		else
		{
			auto categoriesVar = sampleObj->getProperty("categories");
			if (categoriesVar.isArray())
			{
				auto *categoriesArray = categoriesVar.getArray();
				if (categoriesArray->size() > 0)
					entry->category = categoriesArray->getUnchecked(0).toString();
			}
		}

		auto projectsVar = sampleObj->getProperty("usedInProjects");
		if (projectsVar.isArray())
		{
			auto *projectsArray = projectsVar.getArray();
			for (int j = 0; j < projectsArray->size(); ++j)
				entry->usedInProjects.push_back(projectsArray->getUnchecked(j).toString());
		}

		juce::File sampleFile(entry->filePath);
		if (sampleFile.exists())
			samples.push_back(std::move(entry));
	}
}

void SampleBank::addCacheFiles(const juce::String &sampleId, const juce::String &cachePath,
                               const juce::String &originalPath)
{
	juce::ScopedLock lock(bankLock);
	if (auto *entry = getSample(sampleId))
	{
		if (cachePath.isNotEmpty())
			entry->cacheFiles.addIfNotAlreadyThere(cachePath);
		if (originalPath.isNotEmpty())
			entry->cacheFiles.addIfNotAlreadyThere(originalPath);
		saveBankData();
	}
}
