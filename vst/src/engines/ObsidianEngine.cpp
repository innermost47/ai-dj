#pragma once
#include "ObsidianEngine.h"


bool ObsidianEngine::initialize()
{
	appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural");
	auto stableAudioDir = appDataDir.getChildFile("stable-audio");

	stableAudioEngine = std::make_unique<StableAudioEngine>();
	if (!stableAudioEngine->initialize(stableAudioDir.getFullPathName()))
	{
		return false;
	}
	return true;
}
