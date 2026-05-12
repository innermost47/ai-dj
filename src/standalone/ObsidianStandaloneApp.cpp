#if JucePlugin_Build_Standalone
#include "ObsidianStandaloneApp.h"
#include "StandaloneTransportComponent.h"
#include "config/version.h"

ObsidianStandaloneApp *ObsidianStandaloneApp::instance = nullptr;

const juce::String ObsidianStandaloneApp::getApplicationName()
{
	return "OBSIDIAN Neural";
}

const juce::String ObsidianStandaloneApp::getApplicationVersion()
{
	return Version::FULL;
}

bool ObsidianStandaloneApp::moreThanOneInstanceAllowed()
{
	return false;
}

void ObsidianStandaloneApp::initialise(const juce::String &)
{
	instance = this;
	mainWindow =
	    std::make_unique<juce::StandaloneFilterWindow>(getApplicationName(), juce::Colours::black, nullptr, true);
	mainWindow->setVisible(true);
}

void ObsidianStandaloneApp::shutdown()
{
	mainWindow = nullptr;
	instance = nullptr;
}

void ObsidianStandaloneApp::systemRequestedQuit()
{
	quit();
}

juce::AudioDeviceManager *ObsidianStandaloneApp::getSharedDeviceManager()
{
	if (instance && instance->mainWindow)
		if (auto *holder = instance->mainWindow->getPluginHolder())
			return &holder->deviceManager;
	return nullptr;
}

START_JUCE_APPLICATION(ObsidianStandaloneApp)
#endif