#include "UIModalManager.h"
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
	ObsidianAlertManager::showConfigDialog(&editor, "OBSIDIAN-Neural Configuration " + Version::FULL,
	                                       editor.audioProcessor.getServerUrl(), editor.audioProcessor.getApiKey(),
	                                       editor.audioProcessor.getUseLocalModel(),
	                                       editor.audioProcessor.getRequestTimeout(), true,
	                                       [this](const ObsidianAlertManager::ConfigDialogResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;
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
		                                       juce::Timer::callAfterDelay(400, [this]() { showOnboardingTour(); });
	                                       });
}

void UIModalManager::showConfigDialog()
{
	ObsidianAlertManager::showConfigDialog(
	    &editor, "OBSIDIAN-Neural Configuration " + Version::FULL, editor.audioProcessor.getServerUrl(),
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

void UIModalManager::editCustomPromptDialog(const juce::String &selectedPrompt)
{
	ObsidianAlertManager::showEditPrompt(&editor, selectedPrompt,
	                                     [this, selectedPrompt](const juce::String &newPrompt)
	                                     {
		                                     editor.audioProcessor.editCustomPrompt(selectedPrompt, newPrompt);
		                                     int index = editor.audioProcessor.promptPresets.indexOf(selectedPrompt);
		                                     if (index >= 0)
			                                     editor.audioProcessor.promptPresets.set(index, newPrompt);
		                                     editor.uiPresetManager->loadPromptPresets();
	                                     });
}

void UIModalManager::showOnboardingTour()
{
	if (editor.audioProcessor.getOnboardingDone())
		return;

	showOnboardingStep(1);
}

void UIModalManager::showOnboardingStep(int step)
{
	struct StepInfo
	{
		juce::String title;
		juce::String message;
		juce::String buttonNext;
		juce::String buttonSkip;
		juce::String illustrationSvg;
	};

	juce::String lightningSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="#D96850" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"></polygon></svg>)";
	juce::String diskSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="#D96850" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>)";
	juce::String playSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="#D96850" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>)";
	juce::String mapSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="#D96850" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="7"></rect><rect x="14" y="3" width="7" height="7"></rect><rect x="14" y="14" width="7" height="7"></rect><rect x="3" y="14" width="7" height="7"></rect></svg>)";

	std::vector<StepInfo> steps = {
	    {"OBSIDIAN Neural  -  1 of 5  -  Welcome",
	     "Welcome to OBSIDIAN Neural.\n\nThis is an AI sound engine. You describe a sound,\nthe engine generates it as "
	     "audio you can play, loop\nand sequence in your DAW.\n\nThe power lies in the [ GEN ] buttons.\nPress GEN, "
	     "get audio.\nEverything else is just sculpting what comes out.",
	     "Show me how", "Skip tour", lightningSvg},

	    {"OBSIDIAN Neural  -  2 of 5  -  The Global Prompt",
	     "To create sounds, start at the very top:\n\n1. PROMPT INPUT   Type your idea (e.g. 'Acid Bass')\n2. SAVE "
	     "(Disk)    Click the disk icon to save it.\n\nOnce saved, your prompt is added to the global list\nand "
	     "becomes available to every track in the plugin.\n\nOn each track, use the dropdowns to pick your "
	     "saved\nprompt and the AI Model you want to use.\nEach model has its own color and personality.",
	     "Got it", "Skip tour", diskSvg},

	    {"OBSIDIAN Neural  -  3 of 5  -  Generate",
	     "Two ways to trigger a generation:\n\n1. TRACK GEN: Click the [ GEN ] lightning bolt on a\n   specific track "
	     "to generate audio for that page.\n\n2. GLOBAL GEN: Click a track to select it\n   (grey frame), then use the "
	     "large lightning bolt\n   at the top of the VST.\n\nThe track pulses while the AI is thinking. "
	     "When\nfinished, the waveform appears. Don't like it?\nHit GEN again for a fresh roll of the dice.",
	     "OK", "Skip tour", lightningSvg},

	    {"OBSIDIAN Neural  -  4 of 5  -  Make it play",
	     "How to play and shape your sounds:\n\n1. PREVIEW: Instant audition of the raw sample.\n\n2. MIXER PLAY "
	     "(Bottom panel): Arm the track. If\n   your DAW is playing, the sound starts at the next\n   bar and loops "
	     "perfectly.\n\n3. WAVEFORM: Edit loop points directly on the\n   waveform. You can also DRAG & DROP the "
	     "waveform\n   directly into your DAW.\n\n4. SEQUENCER: Use the grid to set retrigger points.",
	     "Almost done", "Skip tour", playSvg},

	    {"OBSIDIAN Neural  -  5 of 5  -  The rest",
	     "Quick map of the interface:\n\nABCD    4 pages per track. Store variations here.\n\nREPEAT  Beat-repeat "
	     "effect. Use RND to randomize.\n\nMIXER   Located at the BOTTOM. Controls volume,\n        pitch, pan, and EQ "
	     "for the Master.\n\nBANK    Left panel. Every generation is saved here\n        automatically. Drag files "
	     "back to reload.\n\nNow go make noise.",
	     "Let's go !", "Skip", mapSvg}};

	if (step < 1 || step > (int)steps.size())
		return;

	const auto &info = steps[step - 1];
	bool isLastStep = (step == (int)steps.size());

	auto modal = std::make_unique<ObsidianModalWindow>(info.title);

	class OnboardingContent : public juce::Component
	{
	  public:
		juce::Label textLabel;
		std::unique_ptr<juce::Drawable> svgIllustration;

		OnboardingContent(const juce::String &text, const juce::String &svgData)
		{
			textLabel.setText(text, juce::dontSendNotification);
			textLabel.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
			textLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			textLabel.setJustificationType(juce::Justification::topLeft);
			addAndMakeVisible(textLabel);

			if (svgData.isNotEmpty())
			{
				auto xml = juce::XmlDocument::parse(svgData);
				if (xml)
					svgIllustration = juce::Drawable::createFromSVG(*xml);
			}
		}

		void paint(juce::Graphics &g) override
		{
			if (svgIllustration != nullptr)
			{
				auto bounds = getLocalBounds().toFloat();
				auto iconArea = bounds.removeFromRight(120.0f).withSizeKeepingCentre(100.0f, 100.0f);
				svgIllustration->drawWithin(g, iconArea, juce::RectanglePlacement::centred, 0.35f);
			}
		}

		void resized() override
		{
			auto bounds = getLocalBounds();
			if (svgIllustration != nullptr)
				bounds.removeFromRight(120);
			textLabel.setBounds(bounds);
		}
	};

	modal->setContent(std::make_unique<OnboardingContent>(info.message, info.illustrationSvg));

	auto overlayOwned = std::make_unique<ObsidianModalOverlay>(std::move(modal));
	auto *overlay = overlayOwned.get();
	addModal(std::move(overlayOwned));

	juce::String arrowSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"></line><polyline points="12 5 19 12 12 19"></polyline></svg>)";
	juce::String skipSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><polyline points="13 17 18 12 13 7"></polyline><polyline points="6 17 11 12 6 7"></polyline></svg>)";

	overlay->modalWindow->addButton(info.buttonSkip, skipSvg, ColourPalette::buttonInactive,
	                                [this, overlay]()
	                                {
		                                overlay->close();
		                                editor.audioProcessor.setOnboardingDone(true);
		                                editor.audioProcessor.saveGlobalConfig();
	                                });

	overlay->modalWindow->addButton(
	    info.buttonNext, arrowSvg, ColourPalette::buttonPrimary,
	    [this, overlay, step, isLastStep]()
	    {
		    overlay->close();

		    if (!isLastStep)
		    {
			    juce::Timer::callAfterDelay(400,
			                                [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor), step]()
			                                {
				                                if (auto *e = safeEditor.getComponent())
					                                e->uiModalManager->showOnboardingStep(step + 1);
			                                });
		    }
		    else
		    {
			    editor.audioProcessor.setOnboardingDone(true);
			    editor.audioProcessor.saveGlobalConfig();

			    editor.statusLabel.setText(
			        juce::String::fromUTF8("Ready - pick a prompt, hit GEN and let's hear what comes out."),
			        juce::dontSendNotification);
			    editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
			    editor.uiStatusManager->updateLCD();
		    }
	    });
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