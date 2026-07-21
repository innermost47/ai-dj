#include "ObsidianEngine.h"

bool ObsidianEngine::initialize()
{
	appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                 .getChildFile(Obsidian::OBSIDIAN_BASE_DIR());
	auto stableAudioDir = appDataDir.getChildFile(Obsidian::STABLE_AUDIO_DIR());

	stableAudioEngine = std::make_unique<StableAudioEngine>();
	if (!stableAudioEngine->initialize(stableAudioDir.getFullPathName()))
	{
		return false;
	}
	return true;
}
