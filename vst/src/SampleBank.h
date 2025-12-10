#pragma once
#include "JuceHeader.h"
#include <vector>
#include <memory>

struct SampleBankEntry
{
	juce::String id;
	juce::String filename;
	juce::String originalPrompt;
	juce::String filePath;
	juce::Time creationTime;
	float duration;
	float bpm;
	juce::String key;
	std::vector<juce::String> stems;
	std::vector<juce::String> usedInProjects;

	std::vector<juce::String> categories;

	double sampleRate;
	int numChannels;
	int numSamples;

	SampleBankEntry() : duration(0.0f), bpm(126.0f), sampleRate(48000.0),
		numChannels(2), numSamples(0)
	{
	}
};

class SampleBank
{
public:
	SampleBank();
	~SampleBank() = default;

	juce::String addSample(const juce::String& prompt,
		const juce::File& audioFile,
		float bpm = 126.0f,
		const juce::String& key = "");

	bool removeSample(const juce::String& sampleId);
	SampleBankEntry* getSample(const juce::String& sampleId);
	std::vector<SampleBankEntry*> getAllSamples();

	std::vector<juce::String> getUnusedSamples() const;
	int removeUnusedSamples();
	void markSampleAsUsed(const juce::String& sampleId, const juce::String& projectId);
	void markSampleAsUnused(const juce::String& sampleId, const juce::String& projectId);

	void saveBankData();
	void loadBankData();

	std::function<void()> onBankChanged;

private:
	std::vector<std::unique_ptr<SampleBankEntry>> samples;
	juce::File bankDirectory;
	juce::File bankIndexFile;
	juce::CriticalSection bankLock;

	juce::String createSafeFilename(const juce::String& prompt, const juce::Time& timestamp);
	juce::String promptToSnakeCase(const juce::String& prompt);
	void analyzeSampleFile(SampleBankEntry* entry, const juce::File& audioFile);
	juce::File getBankDirectory();
	void ensureBankDirectoryExists();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBank)
};