#if JucePlugin_Build_Standalone
#include "ObsidianStandaloneApp.h"
#include "BinaryData.h"
#include "SplashScreen.h"
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
	mainWindow->setVisible(false);

	splashScreen = std::make_unique<SplashScreen>();
	splashWindow = std::make_unique<juce::Component>();
	splashWindow->addAndMakeVisible(splashScreen.get());
	splashWindow->setSize(500, 400);
	splashScreen->setSize(500, 400);
	splashWindow->addToDesktop(juce::ComponentPeer::StyleFlags::windowIsTemporary |
	                           juce::ComponentPeer::StyleFlags::windowHasDropShadow);
	splashWindow->setAlwaysOnTop(true);
	splashWindow->centreWithSize(500, 400);
	splashWindow->setVisible(true);

	splashStartTime = juce::Time::getMillisecondCounter();

	juce::Timer::callAfterDelay(100, [this] { checkInit(); });
}

void ObsidianStandaloneApp::checkInit()
{
	const auto elapsed = juce::Time::getMillisecondCounter() - splashStartTime;
	auto *p = getProcessor();
	const bool ready = (p && p->heavyInitDone.load()) || elapsed > 10000;

	if (ready && elapsed > 1500)
	{
		if (mainWindow)
			mainWindow->setVisible(true);
		splashWindow.reset();
		return;
	}

	juce::Timer::callAfterDelay(100, [this] { checkInit(); });
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