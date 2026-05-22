#include "UIModalManager.h"
#include "Fonts.h"
#include "OnboardingFlow.h"
#include "OnboardingStepData.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "config/version.h"

UIModalManager::UIModalManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIModalManager::addModal(std::unique_ptr<ObsidianModalOverlay> overlay)
{
	auto *raw = overlay.get();
	editor.addAndMakeVisible(raw);
	raw->setBounds(editor.getLocalBounds());
	raw->toFront(false);
	activeModals.push_back(std::move(overlay));
	raw->startFadeIn();
}

void UIModalManager::removeModal(ObsidianModalOverlay *overlay)
{
	activeModals.erase(std::remove_if(activeModals.begin(), activeModals.end(),
	                                  [overlay](const std::unique_ptr<ObsidianModalOverlay> &p)
	                                  { return p.get() == overlay; }),
	                   activeModals.end());
}

void UIModalManager::showFirstTimeSetup()
{
	ObsidianAlertManager::showConfigDialog(&editor, "OBSIDIAN-Neural Configuration " + Version::VERSION,
	                                       editor.audioProcessor.getServerUrl(), editor.audioProcessor.getApiKey(),
	                                       editor.audioProcessor.getUseLocalModel(),
	                                       editor.audioProcessor.getRequestTimeout(), true,
	                                       [this](const ObsidianAlertManager::ConfigDialogResult &res)
	                                       {
		                                       if (res.confirmed)
		                                       {
			                                       editor.audioProcessor.setUseLocalModel(res.useLocalModel);
			                                       if (res.useLocalModel)
				                                       editor.uiTrackManager->checkLocalModelsAndNotify();
			                                       else
			                                       {
				                                       editor.audioProcessor.setServerUrl(res.serverUrl);
				                                       editor.audioProcessor.setApiKey(res.apiKey);
			                                       }
			                                       editor.audioProcessor.setRequestTimeout(res.timeoutMs);
			                                       editor.audioProcessor.saveGlobalConfig();
			                                       editor.uiTrackManager->refreshUIForMode();
		                                       }
		                                       juce::Timer::callAfterDelay(400, [this]() { showOnboardingTour(); });
	                                       });
}

void UIModalManager::showConfigDialog()
{
	ObsidianAlertManager::showConfigDialog(
	    &editor, "OBSIDIAN-Neural Configuration " + Version::VERSION, editor.audioProcessor.getServerUrl(),
	    editor.audioProcessor.getApiKey(), editor.audioProcessor.getUseLocalModel(),
	    editor.audioProcessor.getRequestTimeout(), false,
	    [this](const ObsidianAlertManager::ConfigDialogResult &res)
	    {
		    if (!res.confirmed)
			    return;
		    bool modeChanged = (res.useLocalModel != editor.audioProcessor.getUseLocalModel());
		    editor.audioProcessor.setUseLocalModel(res.useLocalModel);
		    if (res.useLocalModel)
			    editor.uiTrackManager->checkLocalModelsAndNotify();
		    else
		    {
			    editor.audioProcessor.setServerUrl(res.serverUrl);
			    if (res.apiKey.isNotEmpty())
				    editor.audioProcessor.setApiKey(res.apiKey);
		    }
		    editor.audioProcessor.setRequestTimeout(res.timeoutMs);
		    editor.audioProcessor.saveGlobalConfig();
		    if (modeChanged)
			    editor.uiTrackManager->refreshUIForMode();
		    editor.uiStatusManager->setStatusWithTimeout(
		        modeChanged ? "Mode changed! Configuration updated." : "Configuration updated.", 3000);
	    });
}

void UIModalManager::showOnboardingTour()
{
	if (editor.audioProcessor.getOnboardingDone())
		return;

	const auto variant = editor.audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone
	                         ? OnboardingVariant::Standalone
	                         : OnboardingVariant::VST;
	showOnboarding(variant);
}

void UIModalManager::showOnboarding(OnboardingVariant variant)
{
	onboardingFlow = std::make_unique<OnboardingFlow>(editor, *this, variant);
	onboardingFlow->start();
}

void UIModalManager::advanceOnboardingTo(int stepIndex)
{
	if (onboardingFlow != nullptr)
		onboardingFlow->showStep(stepIndex);
}

void UIModalManager::openMidiMappingEditor()
{
	ObsidianAlertManager::showMidiMappingEditor(&editor, &editor.audioProcessor.getMidiLearnManager());
}

void UIModalManager::clearAll()
{
	activeModals.clear();
}

void UIModalManager::checkForUpdates()
{
	juce::Thread::launch(
	    [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
	    {
		    juce::URL url("https://api.github.com/repos/innermost47/ai-dj/releases/latest");
		    auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
		                                            .withExtraHeaders("User-Agent: OBSIDIAN-Neural-Plugin")
		                                            .withConnectionTimeoutMs(5000));

		    if (stream == nullptr)
			    return;

		    auto json = juce::JSON::parse(stream->readEntireStreamAsString());
		    if (auto *obj = json.getDynamicObject())
		    {
			    auto tagName = obj->getProperty("tag_name").toString();
			    int latestNum = tagName.trimCharactersAtStart("v").getIntValue();
			    int currentNum = juce::String(BUILD_NUMBER).getIntValue();

			    if (latestNum > currentNum)
			    {
				    juce::MessageManager::callAsync(
				        [safeEditor, tagName]()
				        {
					        if (auto *editor = safeEditor.getComponent())
					        {
						        if (editor->isInitialized.load())
						        {
							        juce::Timer::callAfterDelay(2000,
							                                    [safeEditor, tagName]()
							                                    {
								                                    if (auto *editor = safeEditor.getComponent())
								                                    {
									                                    ObsidianAlertManager::showUpdateAvailable(
									                                        safeEditor, tagName,
									                                        juce::String(BUILD_NUMBER));
								                                    }
							                                    });
						        }
					        }
				        });
			    }
		    }
	    });
}