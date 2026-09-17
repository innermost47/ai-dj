#include "UIModalManager.h"
#include "ConfigComponent.h"
#include "Fonts.h"
#include "LeftPanelWrapper.h"
#include "OnboardingFlow.h"
#include "OnboardingStepData.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "RightPanelWrapper.h"
#include "config/version.h"

static void applyScaleTo(ObsidianModalOverlay *o, float scale)
{
	o->setTransform(scale != 1.0f ? juce::AffineTransform::scale(scale) : juce::AffineTransform());
	o->setBounds(0, 0, Obsidian::BASE_PLUGIN_WIDTH, Obsidian::BASE_PLUGIN_HEIGHT);
}

UIModalManager::UIModalManager(DjIaVstEditor &editor) : editor(editor)
{
}

void UIModalManager::addModal(std::unique_ptr<ObsidianModalOverlay> overlay)
{
	auto *raw = overlay.get();
	editor.addAndMakeVisible(raw);
	applyScaleTo(raw, editor.getUIScale());
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
	ObsidianAlertManager::showConfigDialog(
	    &editor, "OBSIDIAN-Neural Configuration " + Version::VERSION, editor.audioProcessor.getServerUrl(),
	    editor.audioProcessor.getApiKey(), editor.audioProcessor.getUseLocalModel(),
	    editor.audioProcessor.getRequestTimeout(), true,
	    [this](const ObsidianAlertManager::ConfigDialogResult &res)
	    {
		    if (res.confirmed)
		    {
			    bool modeChanged = (res.useLocalModel != editor.audioProcessor.getUseLocalModel());
			    editor.audioProcessor.setUseLocalModel(res.useLocalModel);
			    if (!res.useLocalModel)
			    {
				    editor.audioProcessor.setServerUrl(res.serverUrl);
				    editor.audioProcessor.setApiKey(res.apiKey);
			    }
			    editor.audioProcessor.setRequestTimeout(res.timeoutMs);
			    editor.audioProcessor.saveGlobalConfig();
			    editor.uiTrackManager->refreshUIForMode();
			    if (modeChanged && editor.uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel() != nullptr)
				    editor.uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel()->refreshList();
		    }
		    juce::Timer::callAfterDelay(400,
		                                [this, useLocal = res.confirmed && res.useLocalModel]()
		                                {
			                                if (useLocal)
				                                showModelDownloader([this]() { showOnboardingTour(); });
			                                else
				                                showOnboardingTour();
		                                });
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
		    if (modeChanged)
		    {
			    editor.uiTrackManager->refreshUIForMode();
			    if (editor.uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel() != nullptr)
				    editor.uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel()->refreshList();
		    }
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
	auto &animator = juce::Desktop::getInstance().getAnimator();
	for (auto &overlay : activeModals)
	{
		overlay->closing = true;
		animator.cancelAnimation(overlay.get(), false);
		if (auto *p = overlay->getParentComponent())
			p->removeChildComponent(overlay.get());
	}
	activeModals.clear();
}

void UIModalManager::checkForUpdates()
{
	auto &proc = editor.audioProcessor;

	const juce::int64 now = juce::Time::currentTimeMillis();
	const juce::int64 last = proc.getLastUpdateCheckTime();
	const juce::int64 oneDay = 24 * 60 * 60 * 1000LL;

	if (last > now)
		proc.setLastUpdateCheckTime(0);
	else if (last > 0 && (now - last) < oneDay)
		return;

	juce::Thread::launch(
	    [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor)]()
	    {
		    int statusCode = 0;
		    juce::URL url(Obsidian::GITHUB_LATEST_RELEASE_API_URL());
		    std::unique_ptr<juce::InputStream> stream(
		        url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
		                                  .withExtraHeaders("User-Agent: OBSIDIAN-Neural\r\n"
		                                                    "Accept: application/vnd.github+json")
		                                  .withConnectionTimeoutMs(5000)
		                                  .withStatusCode(&statusCode)));

		    if (stream == nullptr || statusCode != 200)
			    return;

		    auto response = stream->readEntireStreamAsString();
		    if (response.isEmpty())
			    return;

		    juce::MessageManager::callAsync(
		        [safeEditor, response]()
		        {
			        auto *ed = safeEditor.getComponent();
			        if (ed == nullptr || ed->isBeingDestroyed.load() || ed->uiModalManager == nullptr)
				        return;
			        ed->uiModalManager->handleUpdateCheckResponse(response);
		        });
	    });
}

void UIModalManager::handleUpdateCheckResponse(const juce::String &jsonResponse)
{
	auto json = juce::JSON::parse(jsonResponse);
	auto *obj = json.getDynamicObject();
	if (obj == nullptr)
		return;

	editor.audioProcessor.setLastUpdateCheckTime(juce::Time::currentTimeMillis());
	editor.audioProcessor.saveGlobalConfig();

	const juce::String tag = obj->getProperty("tag_name").toString().trim();
	const juce::String releaseUrl = obj->getProperty("html_url").toString();

	if (tag.isEmpty() || releaseUrl.isEmpty())
		return;

	const int latestBuild = tag.getTrailingIntValue();
	const int currentBuild = juce::String(BUILD_NUMBER).getIntValue();

	if (latestBuild <= 0 || currentBuild <= 0)
		return;
	if (latestBuild <= currentBuild)
		return;
	if (latestBuild <= editor.audioProcessor.getLastDismissedUpdateBuild())
		return;
	if (!editor.isInitialized.load())
		return;

	juce::Timer::callAfterDelay(
	    2000,
	    [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor), latestBuild, currentBuild, releaseUrl]()
	    {
		    auto *ed = safeEditor.getComponent();
		    if (ed == nullptr || ed->isBeingDestroyed.load() || ed->uiModalManager == nullptr)
			    return;
		    if (ed->uiModalManager->hasActiveModals())
			    return;

		    ObsidianAlertManager::showUpdateAvailable(
		        ed, juce::String(currentBuild), juce::String(latestBuild), releaseUrl,
		        [safeEditor, latestBuild](bool dontShowAgain)
		        {
			        if (!dontShowAgain)
				        return;
			        if (auto *e = safeEditor.getComponent())
			        {
				        e->audioProcessor.setLastDismissedUpdateBuild(latestBuild);
				        e->audioProcessor.saveGlobalConfig();
			        }
		        });
	    });
}

void UIModalManager::showModelDownloader(std::function<void()> onComplete)
{
	auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                      .getChildFile(Obsidian::OBSIDIAN_BASE_DIR());
	auto stableAudioDir = appDataDir.getChildFile(Obsidian::STABLE_AUDIO_DIR());

	juce::StringArray required{Obsidian::FP32_DIT_ONNX(),    Obsidian::FP32_DIT_ONNX_DATA(),
	                           Obsidian::DEC_DYNAMIC_BF16(), Obsidian::ENC_DYNAMIC_BF16(),
	                           Obsidian::ENCODER(),          Obsidian::TOKENIZER()};

	bool allPresent = true;
	for (const auto &asset : AssetDownloadManager::fallbackAssets())
	{
		auto f = stableAudioDir.getChildFile(asset.filename);
		if (!f.existsAsFile() || (asset.expectedSizeBytes > 0 && f.getSize() != asset.expectedSizeBytes))
		{
			allPresent = false;
			break;
		}
	}

	if (allPresent)
	{
		if (onComplete)
			onComplete();
		return;
	}

	std::vector<LicenseEntry> licenses{{"Stability AI Community License",
	                                    "https://stability.ai/community-license-agreement",
	                                    "Free for non-commercial use and commercial use under $1M annual revenue.\n"
	                                    "Cannot be used to train other foundational AI models."},
	                                   {"Gemma Terms of Use (T5Gemma tokenizer)", "https://ai.google.dev/gemma/terms",
	                                    "Free for research and commercial use.\n"
	                                    "Cannot be used to train models competing with Google's Gemma products."}};

	ObsidianAlertManager::showLicenseAgreement(&editor, licenses,
	                                           [this, stableAudioDir, onComplete](bool accepted)
	                                           {
		                                           if (!accepted)
		                                           {
			                                           if (onComplete)
				                                           onComplete();
			                                           return;
		                                           }
		                                           ObsidianAlertManager::showAssetDownloader(&editor, stableAudioDir,
		                                                                                     [onComplete](bool success)
		                                                                                     {
			                                                                                     juce::ignoreUnused(
			                                                                                         success);
			                                                                                     if (onComplete)
				                                                                                     onComplete();
		                                                                                     });
	                                           });
}

void UIModalManager::showCredits()
{
	ObsidianAlertManager::showCredits(&editor);
}

void UIModalManager::applyScale(float scale)
{
	for (auto &o : activeModals)
		if (o != nullptr)
			applyScaleTo(o.get(), scale);
}
