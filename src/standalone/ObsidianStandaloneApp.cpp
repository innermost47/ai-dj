#if JucePlugin_Build_Standalone
#include "ObsidianStandaloneApp.h"
#include "StandaloneTransportComponent.h"
#include "config/version.h"
#include <JuceHeader.h>

using juce::StandaloneFilterWindow;

class ObsidianStandaloneApp : public juce::JUCEApplication
{
  public:
	const juce::String getApplicationName() override
	{
		return "OBSIDIAN Neural";
	}
	const juce::String getApplicationVersion() override
	{
		return Version::FULL;
	}
	bool moreThanOneInstanceAllowed() override
	{
		return false;
	}

	void initialise(const juce::String &) override
	{
		mainWindow =
		    std::make_unique<StandaloneFilterWindow>(getApplicationName(), juce::Colours::black, nullptr, true);
		mainWindow->setVisible(true);
	}

	void shutdown() override
	{
		mainWindow = nullptr;
	}
	void systemRequestedQuit() override
	{
		quit();
	}

  private:
	std::unique_ptr<StandaloneFilterWindow> mainWindow;
};

START_JUCE_APPLICATION(ObsidianStandaloneApp)
#endif