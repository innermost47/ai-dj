#pragma once
#include "DataConst.h"
#include <JuceHeader.h>
#include <functional>
#include <map>
#include <memory>
#include <vector>

struct PromptBankEntry
{
	juce::String id;
	juce::String text;
	juce::String modelName;
	juce::String category;
	juce::Time creationTime;
	int usageCount = 0;
	bool isBuiltIn = false;
};

struct PromptCategoryInfo
{
	int id = 0;
	juce::String name;
	juce::Colour colour;
	bool isBuiltIn = false;
};

struct PromptInfo
{
	juce::String text;
	juce::String category;
};

class PromptBank
{
  public:
	PromptBank();
	~PromptBank();

	PromptBankEntry *addPrompt(const juce::String &text, const juce::String &modelName,
	                           const juce::String &category = {});
	bool removePrompt(const juce::String &id);
	bool updatePrompt(const juce::String &id, const juce::String &text, const juce::String &modelName,
	                  const juce::String &category);
	void incrementUsage(const juce::String &id);

	PromptBankEntry *getPrompt(const juce::String &id);
	std::vector<PromptBankEntry *> getAllPrompts();
	std::vector<PromptBankEntry *> getPromptsByCategory(const juce::String &category);

	void addCategory(const juce::String &name, juce::Colour colour);
	bool renameCategory(const juce::String &oldName, const juce::String &newName, juce::Colour colour);
	bool removeCategory(const juce::String &name);
	const std::vector<PromptCategoryInfo> &getCategories() const
	{
		return categories;
	}
	bool isCategoryEditable(int id) const
	{
		return id >= 1;
	}

	void loadFromFile();
	void saveToFile();

	void migrateFromCustomPrompts(const juce::StringArray &existing,
	                              const juce::String &defaultModel = Obsidian::STABLE_AUDIO_OPEN_V1());

	bool hasMigrated() const
	{
		return migrated;
	}

	bool hasSeeded() const
	{
		return seeded;
	}
	bool seedDefaultPromptsAndCategories();
	bool hasSA3Seeded() const
	{
		return seededSA3;
	}
	bool seedStableAudio3Medium();

	std::function<void()> onBankChanged;

  private:
	struct DefaultPrompt
	{
		const char *text;
		const char *model;
		const char *category;
	};

	static juce::File getPromptBankFile();
	static juce::String generateId();

	bool addDefaultPromptsArray(const DefaultPrompt *array, int arraySize);

	int getNextCategoryId();

	std::map<juce::String, std::unique_ptr<PromptBankEntry>> prompts;

	std::vector<PromptCategoryInfo> categories;

	bool migrated = false;
	bool seeded = false;
	bool seededSA3 = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromptBank)
};