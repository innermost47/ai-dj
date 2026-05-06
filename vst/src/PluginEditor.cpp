#include "PluginEditor.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "ObsidianAlertManager.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#include "config/version.h"
#if JUCE_WINDOWS
#include <windows.h>
#include <winuser.h>
#endif

DjIaVstEditor::DjIaVstEditor(DjIaVstProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
	setResizable(true, true);
	setResizeLimits(1100, 800, 2400, 1600);
	setSize(1620, 840);
	setScaleFactor(1.0f);
	juce::LookAndFeel::setDefaultLookAndFeel(&CustomLookAndFeel::getInstance());
	ObsidianAlertManager::initialize();
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(false);
	setFocusContainerType(FocusContainerType::focusContainer);
	setInterceptsMouseClicks(true, true);
	tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
	logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);
	audioProcessor.setGenerationListener(this);

	if (audioProcessor.isStateReady())
	{
		initUI();
		juce::Timer::callAfterDelay(300,
		                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		                            {
			                            if (safeThis == nullptr || safeThis->isBeingDestroyed.load())
				                            return;
			                            safeThis->finalizeInit();
		                            });
	}
	else
	{
		startTimer(50);
	}

	juce::WeakReference<DjIaVstEditor> weakThis(this);

	audioProcessor.setMidiIndicatorCallback(
	    [weakThis](const juce::String &noteInfo)
	    {
		    if (weakThis != nullptr)
			    weakThis->updateMidiIndicator(noteInfo);
	    });

	audioProcessor.onUIUpdateNeeded = [weakThis]()
	{
		juce::MessageManager::callAsync(
		    [weakThis]()
		    {
			    if (weakThis != nullptr)
				    weakThis->updateUIComponents();
		    });
	};
	juce::Component::SafePointer<DjIaVstEditor> safeEditor(this);
	audioProcessor.getSequencerManager().onSequencerUpdateNeeded = [safeEditor](const juce::String &trackId)
	{
		juce::MessageManager::callAsync(
		    [safeEditor, trackId]()
		    {
			    if (safeEditor.getComponent() == nullptr)
				    return;
			    if (safeEditor->isBeingDestroyed.load())
				    return;
			    if (auto *sequencer = static_cast<SequencerComponent *>(safeEditor->getSequencerForTrack(trackId)))
				    sequencer->updateFromTrackData();
		    });
	};

	juce::Timer::callAfterDelay(4000,
	                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
	                            {
		                            if (safeThis == nullptr)
			                            return;
		                            if (!safeThis->audioProcessor.updateCheckDone)
		                            {
			                            safeThis->audioProcessor.updateCheckDone = true;
			                            safeThis->checkForUpdates();
		                            }
	                            });
}

DjIaVstEditor::~DjIaVstEditor()
{
	isBeingDestroyed.store(true);
	stopTimer();
	tracksViewport.setViewedComponent(nullptr, false);
	audioProcessor.onMasterOutput = nullptr;
	audioProcessor.setMidiIndicatorCallback(nullptr);
	audioProcessor.onUIUpdateNeeded = nullptr;
	audioProcessor.setGenerationListener(nullptr);
	activeModals.clear();

	for (auto &tc : trackComponents)
		if (tc)
			tc->setVisible(false);
	trackComponents.clear();

	if (mixerPanel)
	{
		mixerViewport.setViewedComponent(nullptr, false);
		mixerViewport.setVisible(false);
		mixerPanel.reset();
	}

	setLookAndFeel(nullptr);
	ObsidianAlertManager::shutdown();
}

void DjIaVstEditor::finalizeInit()
{
	if (isBeingDestroyed.load())
		return;
	if (audioProcessor.getIsLoadingState())
	{
		juce::Timer::callAfterDelay(100,
		                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		                            {
			                            if (safeThis == nullptr || safeThis->isBeingDestroyed.load())
				                            return;
			                            safeThis->finalizeInit();
		                            });
		return;
	}

	loadPromptPresets();
	refreshTracks();
	refreshCreditsAsync();

	if (audioProcessor.getIsGenerating())
	{
		generateButton.setEnabled(false);
		setAllGenerateButtonsEnabled(false);
		statusLabel.setText("Generation in progress...", juce::dontSendNotification);
		updateLCD();
		juce::String generatingId = audioProcessor.getGeneratingTrackId();
		for (auto &trackComp : trackComponents)
		{
			if (trackComp && trackComp->getTrackId() == generatingId)
			{
				trackComp->startGeneratingAnimation();
				break;
			}
		}
	}
}

bool DjIaVstEditor::keyStateChanged(bool isKeyDown)
{
	if (isKeyDown && !hasKeyboardFocus(true))
	{
		if (!promptInput.hasKeyboardFocus(true))
		{
			grabKeyboardFocus();
		}
	}
	return false;
}

void DjIaVstEditor::updateMidiIndicator(const juce::String &noteInfo)
{
	lastMidiNote = noteInfo;
	juce::Component::SafePointer<DjIaVstEditor> safeThis(this);
	juce::MessageManager::callAsync(
	    [safeThis, noteInfo]()
	    {
		    if (!safeThis)
			    return;
		    safeThis->midiIndicator.setText(noteInfo, juce::dontSendNotification);
		    safeThis->updateLCD();
		    juce::Timer::callAfterDelay(800,
		                                [safeThis]()
		                                {
			                                if (!safeThis)
				                                return;
			                                safeThis->midiIndicator.setText("", juce::dontSendNotification);
			                                safeThis->updateLCD();
		                                });
	    });
}

void DjIaVstEditor::updateUIComponents()
{
	if (!isGenerating.load() && audioProcessor.getIsGenerating())
	{
		isGenerating.store(true);
		wasGenerating.store(true);
		startGenerationButtonAnimation();
		startTimer(200);
	}
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->isShowing())
		{
			TrackData *track = audioProcessor.getTrack(trackComp->getTrackId());
			if (track && !trackComp->isEditingLabel)
			{
				trackComp->updateFromTrackData();
			}
		}
	}
	if (mixerPanel)
	{
		mixerPanel->updateAllMixerComponents();
	}

	if (!lastMidiNote.isEmpty())
	{
		static int midiBlinkCounter = 0;
		if (++midiBlinkCounter > 6)
		{
			midiIndicator.setColour(juce::Label::backgroundColourId, ColourPalette::backgroundDeep);
			lastMidiNote.clear();
			updateLCD();
			midiBlinkCounter = 0;
		}
	}

	if (!autoLoadButton.getToggleState())
	{
		updateLoadButtonState();
	}

	for (auto &trackComp : trackComponents)
	{
		TrackData *track = audioProcessor.getTrack(trackComp->getTrackId());
		if (track && track->isPlaying.load() && track->getCurrentPage().numSamples > 0)
		{
			double startSample = track->getCurrentPage().loopStart * track->getCurrentPage().sampleRate;
			double currentTimeInSection =
			    (startSample + track->readPosition.load()) / track->getCurrentPage().sampleRate;

			trackComp->updatePlaybackPosition(currentTimeInSection);
		}
	}

	static bool currentWasGenerating = false;
	bool isCurrentlyGenerating = generateButton.isEnabled() == false;
	if (currentWasGenerating && !isCurrentlyGenerating)
	{
		for (auto &trackComp : trackComponents)
		{
			trackComp->refreshWaveformIfNeeded();
		}
	}
	currentWasGenerating = isCurrentlyGenerating;
}

void DjIaVstEditor::onGenerationComplete(const juce::String &trackId, const juce::String &message)
{
	bool isError = message.startsWith("ERROR:");
	stopGenerationUI(trackId, !isError, isError ? message : "");

	if (isShowing())
	{
		statusLabel.setText(message, juce::dontSendNotification);
		updateLCD();

		if (isError)
		{
			statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
			juce::Timer::callAfterDelay(5000,
			                            [this]()
			                            {
				                            if (isShowing())
				                            {
					                            statusLabel.setText("Ready", juce::dontSendNotification);
					                            updateLCD();
					                            statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
				                            }
			                            });
		}
		else
		{
			statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
			juce::Timer::callAfterDelay(3000,
			                            [this]()
			                            {
				                            if (isShowing())
				                            {
					                            statusLabel.setText("Ready", juce::dontSendNotification);
					                            updateLCD();
					                            statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
				                            }
			                            });
		}
	}
	refreshCredits();
}

void DjIaVstEditor::refreshTracks()
{
	trackComponents.clear();
	tracksContainer.removeAllChildren();
	refreshTrackComponents();
	for (auto &trackComp : trackComponents)
		trackComp->loadPromptPresets();
	updateSelectedTrack();
	repaint();
}

void DjIaVstEditor::initUI()
{
	if (isInitialized.load())
		return;

	setupUI();
	refreshUIForMode();
	serverUrlInput.setText(audioProcessor.getServerUrl(), juce::dontSendNotification);
	apiKeyInput.setText(audioProcessor.getApiKey(), juce::dontSendNotification);
	if (audioProcessor.getServerUrl().isEmpty())
	{
		juce::Timer::callAfterDelay(500, [this]() { showFirstTimeSetup(); });
	}
	isInitialized.store(true);
	juce::WeakReference<DjIaVstEditor> weakThis(this);

	audioProcessor.setMidiIndicatorCallback(
	    [weakThis](const juce::String &noteInfo)
	    {
		    if (weakThis != nullptr)
			    weakThis->updateMidiIndicator(noteInfo);
	    });

	audioProcessor.onUIUpdateNeeded = [weakThis]()
	{
		juce::MessageManager::callAsync(
		    [weakThis]()
		    {
			    if (weakThis != nullptr)
				    weakThis->updateUIComponents();
		    });
	};
}

void DjIaVstEditor::showFirstTimeSetup()
{
	ObsidianAlertManager::showConfigDialog(this, "OBSIDIAN-Neural Configuration " + Version::FULL,
	                                       audioProcessor.getServerUrl(), audioProcessor.getApiKey(),
	                                       audioProcessor.getUseLocalModel(), audioProcessor.getRequestTimeout(), true,
	                                       [this](const ObsidianAlertManager::ConfigDialogResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;

		                                       audioProcessor.setUseLocalModel(res.useLocalModel);
		                                       if (res.useLocalModel)
			                                       checkLocalModelsAndNotify();
		                                       else
		                                       {
			                                       audioProcessor.setServerUrl(res.serverUrl);
			                                       audioProcessor.setApiKey(res.apiKey);
		                                       }
		                                       audioProcessor.setRequestTimeout(res.timeoutMs);
		                                       audioProcessor.saveGlobalConfig();
		                                       refreshUIForMode();
		                                       juce::Timer::callAfterDelay(400, [this]() { showOnboardingTour(); });
	                                       });
}

void DjIaVstEditor::showOnboardingTour()
{
	if (audioProcessor.getOnboardingDone())
		return;

	showOnboardingStep(1);
}

void DjIaVstEditor::showOnboardingStep(int step)
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
		                                audioProcessor.setOnboardingDone(true);
		                                audioProcessor.saveGlobalConfig();
	                                });

	overlay->modalWindow->addButton(
	    info.buttonNext, arrowSvg, ColourPalette::buttonPrimary,
	    [this, overlay, step, isLastStep]()
	    {
		    overlay->close();

		    if (!isLastStep)
		    {
			    juce::Component::SafePointer<DjIaVstEditor> safeThis(this);
			    juce::MessageManager::callAsync(
			        [safeThis, step]()
			        {
				        if (safeThis != nullptr)
					        safeThis->showOnboardingStep(step + 1);
			        });
		    }
		    else
		    {
			    audioProcessor.setOnboardingDone(true);
			    audioProcessor.saveGlobalConfig();

			    statusLabel.setText(
			        juce::String::fromUTF8("Ready - pick a prompt, hit GEN and let's hear what comes out."),
			        juce::dontSendNotification);
			    statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
			    updateLCD();
		    }
	    });
}

void DjIaVstEditor::addModal(std::unique_ptr<ObsidianModalOverlay> overlay)
{
	auto *raw = overlay.get();
	addAndMakeVisible(raw);
	raw->setBounds(getLocalBounds());
	raw->toFront(false);
	activeModals.push_back(std::move(overlay));
	raw->startFadeIn();
}

void DjIaVstEditor::removeModal(ObsidianModalOverlay *overlay)
{
	activeModals.erase(std::remove_if(activeModals.begin(), activeModals.end(),
	                                  [overlay](const std::unique_ptr<ObsidianModalOverlay> &p)
	                                  { return p.get() == overlay; }),
	                   activeModals.end());
}

void DjIaVstEditor::refreshUIForMode()
{
	bool isLocalMode = audioProcessor.getUseLocalModel();
	durationSelector.setEnabled(!isLocalMode);
	resized();
}

void DjIaVstEditor::showConfigDialog()
{
	ObsidianAlertManager::showConfigDialog(
	    this, "OBSIDIAN-Neural Configuration " + Version::FULL, audioProcessor.getServerUrl(),
	    audioProcessor.getApiKey(), audioProcessor.getUseLocalModel(), audioProcessor.getRequestTimeout(), false,
	    [this](const ObsidianAlertManager::ConfigDialogResult &res)
	    {
		    if (!res.confirmed)
			    return;

		    bool modeChanged = (res.useLocalModel != audioProcessor.getUseLocalModel());
		    audioProcessor.setUseLocalModel(res.useLocalModel);

		    if (res.useLocalModel)
			    checkLocalModelsAndNotify();
		    else
		    {
			    audioProcessor.setServerUrl(res.serverUrl);
			    if (res.apiKey.isNotEmpty())
				    audioProcessor.setApiKey(res.apiKey);
		    }
		    audioProcessor.setRequestTimeout(res.timeoutMs);
		    audioProcessor.saveGlobalConfig();

		    if (modeChanged)
			    refreshUIForMode();
		    setStatusWithTimeout(modeChanged ? "Mode changed! Configuration updated." : "Configuration updated.", 3000);
	    });
}

void DjIaVstEditor::checkLocalModelsAndNotify()
{
	auto appDataDir =
	    juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("OBSIDIAN-Neural");
	auto stableAudioDir = appDataDir.getChildFile("stable-audio");

	StableAudioEngine tempEngine;
	bool modelsPresent = tempEngine.initialize(stableAudioDir.getFullPathName());

	if (modelsPresent)
	{
		statusLabel.setText("Local models found! Configuration saved.", juce::dontSendNotification);
		updateLCD();
		statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
	}
	else
	{
		ObsidianAlertManager::showConfirm(
		    this, "Local Models Required",
		    "Local models not found!\n\nExpected location: " + stableAudioDir.getFullPathName(),
		    "Open GitHub Instructions", "OK",
		    [](bool confirmed)
		    {
			    if (confirmed)
				    juce::URL("https://github.com/innermost47/ai-dj/blob/main/README.md").launchInDefaultBrowser();
		    });

		statusLabel.setText("Local mode selected - Models setup required", juce::dontSendNotification);
		updateLCD();
		statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
	}
}

void DjIaVstEditor::timerCallback()
{
	if (!isInitialized.load())
	{
		if (audioProcessor.isStateReady())
		{
			stopTimer();
			initUI();
			juce::Timer::callAfterDelay(300,
			                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
			                            {
				                            if (safeThis == nullptr || safeThis->isBeingDestroyed.load())
					                            return;
				                            safeThis->finalizeInit();
			                            });
			return;
		}
	}

	bool anyTrackPlaying = false;
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->isShowing())
		{
			TrackData *track = audioProcessor.getTrack(trackComp->getTrackId());
			if (track && track->isPlaying.load())
			{
				trackComp->updateFromTrackData();
				anyTrackPlaying = true;
			}
		}
	}
	if (!anyTrackPlaying)
	{
		static int skipFrames = 0;
		skipFrames++;
		if (skipFrames < 10)
			return;
		skipFrames = 0;
	}

	static double lastHostBpm = 0.0;
	double currentHostBpm = audioProcessor.getHostBpm();
	if (std::abs(currentHostBpm - lastHostBpm) > 0.1)
	{
		lastHostBpm = currentHostBpm;
		for (auto &trackComp : trackComponents)
		{
			TrackData *track = audioProcessor.getTrack(trackComp->getTrackId());
			if (track && (track->timeStretchMode == 3 || track->timeStretchMode == 4))
				trackComp->updateWaveformWithTimeStretch();
		}
	}
}

void DjIaVstEditor::startGenerationButtonAnimation()
{
	generateButton.setEnabled(false);
}

void DjIaVstEditor::stopGenerationButtonAnimation()
{
	generateButton.setEnabled(true);
	generatingTrackId.clear();
}

void DjIaVstEditor::setupUI()
{
	addAndMakeVisible(pluginNameLabel);
	pluginNameLabel.setText("NEURAL SOUND ENGINE", juce::dontSendNotification);
	pluginNameLabel.setFont(juce::FontOptions("Courier New", 18.0f, juce::Font::bold));
	pluginNameLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	pluginNameLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(developerLabel);
	developerLabel.setText("Developed by InnerMost47", juce::dontSendNotification);
	developerLabel.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::italic));
	developerLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	developerLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(stabilityLabel);
	stabilityLabel.setText("Powered by Stability AI", juce::dontSendNotification);
	stabilityLabel.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::plain));
	stabilityLabel.setColour(juce::Label::textColourId, ColourPalette::credits);
	stabilityLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(promptPresetSelector);

	addAndMakeVisible(savePresetButton);
	savePresetButton.loadIcon(BinaryData::save_svg, BinaryData::save_svgSize);

	addAndMakeVisible(promptInput);
	promptInput.setMultiLine(false);
	promptInput.setReturnKeyStartsNewLine(false);
	promptInput.setTextToShowWhenEmpty("Enter custom prompt or select preset...", ColourPalette::textSecondary);
	promptInput.setText(audioProcessor.getGlobalPrompt(), juce::dontSendNotification);
	promptInput.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundMid);
	promptInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::trackSelected.withAlpha(0.4f));
	promptInput.setColour(juce::TextEditor::focusedOutlineColourId, ColourPalette::trackSelected);

	addAndMakeVisible(keySelector);
	keySelector.addItem("C Ionian", 1);
	keySelector.addItem("C# Ionian", 2);
	keySelector.addItem("D Ionian", 3);
	keySelector.addItem("D# Ionian", 4);
	keySelector.addItem("E Ionian", 5);
	keySelector.addItem("F Ionian", 6);
	keySelector.addItem("F# Ionian", 7);
	keySelector.addItem("G Ionian", 8);
	keySelector.addItem("G# Ionian", 9);
	keySelector.addItem("A Ionian", 10);
	keySelector.addItem("A# Ionian", 11);
	keySelector.addItem("B Ionian", 12);
	keySelector.addItem("C Dorian", 13);
	keySelector.addItem("C# Dorian", 14);
	keySelector.addItem("D Dorian", 15);
	keySelector.addItem("D# Dorian", 16);
	keySelector.addItem("E Dorian", 17);
	keySelector.addItem("F Dorian", 18);
	keySelector.addItem("F# Dorian", 19);
	keySelector.addItem("G Dorian", 20);
	keySelector.addItem("G# Dorian", 21);
	keySelector.addItem("A Dorian", 22);
	keySelector.addItem("A# Dorian", 23);
	keySelector.addItem("B Dorian", 24);
	keySelector.addItem("C Phrygian", 25);
	keySelector.addItem("C# Phrygian", 26);
	keySelector.addItem("D Phrygian", 27);
	keySelector.addItem("D# Phrygian", 28);
	keySelector.addItem("E Phrygian", 29);
	keySelector.addItem("F Phrygian", 30);
	keySelector.addItem("F# Phrygian", 31);
	keySelector.addItem("G Phrygian", 32);
	keySelector.addItem("G# Phrygian", 33);
	keySelector.addItem("A Phrygian", 34);
	keySelector.addItem("A# Phrygian", 35);
	keySelector.addItem("B Phrygian", 36);
	keySelector.addItem("C Lydian", 37);
	keySelector.addItem("C# Lydian", 38);
	keySelector.addItem("D Lydian", 39);
	keySelector.addItem("D# Lydian", 40);
	keySelector.addItem("E Lydian", 41);
	keySelector.addItem("F Lydian", 42);
	keySelector.addItem("F# Lydian", 43);
	keySelector.addItem("G Lydian", 44);
	keySelector.addItem("G# Lydian", 45);
	keySelector.addItem("A Lydian", 46);
	keySelector.addItem("A# Lydian", 47);
	keySelector.addItem("B Lydian", 48);
	keySelector.addItem("C Mixolydian", 49);
	keySelector.addItem("C# Mixolydian", 50);
	keySelector.addItem("D Mixolydian", 51);
	keySelector.addItem("D# Mixolydian", 52);
	keySelector.addItem("E Mixolydian", 53);
	keySelector.addItem("F Mixolydian", 54);
	keySelector.addItem("F# Mixolydian", 55);
	keySelector.addItem("G Mixolydian", 56);
	keySelector.addItem("G# Mixolydian", 57);
	keySelector.addItem("A Mixolydian", 58);
	keySelector.addItem("A# Mixolydian", 59);
	keySelector.addItem("B Mixolydian", 60);
	keySelector.addItem("C Aeolian", 61);
	keySelector.addItem("C# Aeolian", 62);
	keySelector.addItem("D Aeolian", 63);
	keySelector.addItem("D# Aeolian", 64);
	keySelector.addItem("E Aeolian", 65);
	keySelector.addItem("F Aeolian", 66);
	keySelector.addItem("F# Aeolian", 67);
	keySelector.addItem("G Aeolian", 68);
	keySelector.addItem("G# Aeolian", 69);
	keySelector.addItem("A Aeolian", 70);
	keySelector.addItem("A# Aeolian", 71);
	keySelector.addItem("B Aeolian", 72);
	keySelector.addItem("C Locrian", 73);
	keySelector.addItem("C# Locrian", 74);
	keySelector.addItem("D Locrian", 75);
	keySelector.addItem("D# Locrian", 76);
	keySelector.addItem("E Locrian", 77);
	keySelector.addItem("F Locrian", 78);
	keySelector.addItem("F# Locrian", 79);
	keySelector.addItem("G Locrian", 80);
	keySelector.addItem("G# Locrian", 81);
	keySelector.addItem("A Locrian", 82);
	keySelector.addItem("A# Locrian", 83);
	keySelector.addItem("B Locrian", 84);
	keySelector.addItem("C Major", 85);
	keySelector.addItem("C# Major", 86);
	keySelector.addItem("D Major", 87);
	keySelector.addItem("D# Major", 88);
	keySelector.addItem("E Major", 89);
	keySelector.addItem("F Major", 90);
	keySelector.addItem("F# Major", 91);
	keySelector.addItem("G Major", 92);
	keySelector.addItem("G# Major", 93);
	keySelector.addItem("A Major", 94);
	keySelector.addItem("A# Major", 95);
	keySelector.addItem("B Major", 96);
	keySelector.addItem("C Minor", 97);
	keySelector.addItem("C# Minor", 98);
	keySelector.addItem("D Minor", 99);
	keySelector.addItem("D# Minor", 100);
	keySelector.addItem("E Minor", 101);
	keySelector.addItem("F Minor", 102);
	keySelector.addItem("F# Minor", 103);
	keySelector.addItem("G Minor", 104);
	keySelector.addItem("G# Minor", 105);
	keySelector.addItem("A Minor", 106);
	keySelector.addItem("A# Minor", 107);
	keySelector.addItem("B Minor", 108);
	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);

	addAndMakeVisible(durationSelector);
	for (int s : {2, 4, 6, 8, 10, 12, 16, 20, 24, 30})
		durationSelector.addItem(juce::String(s) + " s", s);
	int currentDur = juce::roundToInt(audioProcessor.getGlobalDuration());
	durationSelector.setSelectedId(currentDur, juce::dontSendNotification);
	if (durationSelector.getSelectedId() == 0)
		durationSelector.setSelectedId(6, juce::dontSendNotification);
	durationSelector.setTooltip("Generation duration in seconds");

	addAndMakeVisible(generateButton);
	generateButton.loadIcon(BinaryData::zap_svg, BinaryData::zap_svgSize);

	addAndMakeVisible(configButton);
	configButton.loadIcon(BinaryData::gear_svg, BinaryData::gear_svgSize);
	configButton.setTooltip("Configure settings globally");
	configButton.onClick = [this]() { showConfigDialog(); };

	addAndMakeVisible(autoLoadButton);
	autoLoadButton.loadIcon(BinaryData::refresh_svg, BinaryData::refresh_svgSize);
	autoLoadButton.setClickingTogglesState(true);
	autoLoadButton.setToggleState(audioProcessor.getAutoLoadEnabled(), juce::dontSendNotification);
	autoLoadButton.setTooltip("Toggle between automatic and manual sample loading");

	addAndMakeVisible(loadSampleButton);
	loadSampleButton.loadIcon(BinaryData::upload_svg, BinaryData::upload_svgSize);
	loadSampleButton.setEnabled(!audioProcessor.getAutoLoadEnabled());
	loadSampleButton.setTooltip("Manually load pending generated sample");

	addAndMakeVisible(tracksViewport);
	tracksViewport.setViewedComponent(&tracksContainer, false);
	tracksViewport.setScrollBarsShown(true, true);
	tracksContainer.setWantsKeyboardFocus(false);
	tracksViewport.setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(true);

	if (!mixerPanel)
	{
		mixerPanel = std::make_unique<MixerPanel>(audioProcessor);
		mixerViewport.setViewedComponent(mixerPanel.get(), false);
		addAndMakeVisible(mixerViewport);
		mixerPanel->onTrackRenamedFromMixer = [this](const juce::String &trackId, const juce::String &newName)
		{
			for (auto &trackComp : trackComponents)
			{
				if (trackComp->getTrackId() == trackId)
				{
					trackComp->syncTrackName(newName);
					break;
				}
			}
		};
	}

	statusLabel.setColour(juce::Label::backgroundColourId, ColourPalette::backgroundDeep);
	statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);

	addAndMakeVisible(bypassSequencerButton);
	bypassSequencerButton.setClickingTogglesState(true);
	bypassSequencerButton.setToggleState(audioProcessor.getBypassSequencer(), juce::dontSendNotification);
	if (audioProcessor.getBypassSequencer())
	{
		bypassSequencerButton.loadIcon(BinaryData::cpuregular_svg, BinaryData::cpuregular_svgSize);
	}
	else
	{
		bypassSequencerButton.loadIcon(BinaryData::cpu_svg, BinaryData::cpu_svgSize);
	}
	bypassSequencerButton.setTooltip("Global bypass - direct MIDI playback for composition mode");

	addAndMakeVisible(bypassLLMButton);
	bypassLLMButton.setClickingTogglesState(true);
	bypassLLMButton.setToggleState(audioProcessor.getBypassLLM(), juce::dontSendNotification);
	if (audioProcessor.getBypassLLM())
	{
		bypassLLMButton.loadIcon(BinaryData::robotregular_svg, BinaryData::robotregular_svgSize);
	}
	else
	{
		bypassLLMButton.loadIcon(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
	}
	bypassLLMButton.setTooltip("Disables prompt enhancement for faster, raw generation");

	promptPresetSelector.setTooltip(
	    "Select a preset prompt (Right-click for MIDI learn, Ctrl+Right-click to edit custom prompts)");
	promptInput.setTooltip("Enter your custom prompt for audio generation");
	savePresetButton.setTooltip("Save current prompt as custom preset");
	keySelector.setTooltip("Select musical key and mode for generation");
	generateButton.setTooltip("Generate audio loop for selected track (Right-click for MIDI learn)");
	configButton.setTooltip("Configure API settings and generation mode");
	autoLoadButton.setTooltip("Automatically load generated samples (disable for manual control)");
	loadSampleButton.setTooltip("Manually load pending generated sample");

	if (!sampleBankPanel)
	{
		sampleBankPanel = std::make_unique<SampleBankPanel>(audioProcessor);
		addChildComponent(*sampleBankPanel);
	}

	addAndMakeVisible(openMidiEditorButton);
	openMidiEditorButton.loadIcon(BinaryData::piano_svg, BinaryData::piano_svgSize);
	openMidiEditorButton.setTooltip("Open MIDI mappings editor");

	addAndMakeVisible(helpButton);
	helpButton.loadIcon(BinaryData::info_svg, BinaryData::info_svgSize);
	helpButton.setTooltip("Open the Quick Start tour");

	addAndMakeVisible(lcdScreen);

	logoComponent.setImage(logoImage);
	logoComponent.setImagePlacement(juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
	addAndMakeVisible(logoComponent);

	addAndMakeVisible(masterWaveformDisplay);
	mixerPanel->setMasterWaveform(&masterWaveformDisplay);
	mixerPanel->setLCDScreen(&lcdScreen);
	audioProcessor.onMasterOutput = [this](const float *l, const float *r, int n, double ppq)
	{
		masterWaveformDisplay.pushSamples(l, r, n);
		masterWaveformDisplay.setPositionInBeats(ppq);
	};

	addAndMakeVisible(toggleBankButton);
	toggleBankButton.loadIcon(BinaryData::search_svg, BinaryData::search_svgSize);
	toggleBankButton.setClickingTogglesState(true);
	toggleBankButton.setTooltip("Toggle sample bank panel");

	auto setupControlBtn = [](IconButtonSimple &btn)
	{
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
		btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		btn.setHasAccentBar(true);
	};

	setupControlBtn(bypassSequencerButton);
	setupControlBtn(autoLoadButton);
	setupControlBtn(loadSampleButton);
	setupControlBtn(openMidiEditorButton);
	setupControlBtn(configButton);
	setupControlBtn(helpButton);
	setupControlBtn(toggleBankButton);
	setupControlBtn(bypassLLMButton);

	savePresetButton.setShowBorder(true);
	generateButton.setShowBorder(true);
	bypassSequencerButton.setShowBorder(true);
	openMidiEditorButton.setShowBorder(true);
	configButton.setShowBorder(true);
	helpButton.setShowBorder(true);
	toggleBankButton.setShowBorder(true);
	autoLoadButton.setShowBorder(true);
	loadSampleButton.setShowBorder(true);
	bypassLLMButton.setShowBorder(true);

	setSize(audioProcessor.getSavedWindowWidth(), audioProcessor.getSavedWindowHeight());

	bool bankVisible = audioProcessor.getSavedBankVisible();
	toggleBankButton.setToggleState(bankVisible, juce::dontSendNotification);
	if (sampleBankPanel)
		sampleBankPanel->setVisible(bankVisible);

	refreshTrackComponents();
	addEventListeners();
}

void DjIaVstEditor::addEventListeners()
{
	autoLoadButton.onClick = [this] { onAutoLoadToggled(); };
	loadSampleButton.onClick = [this] { onLoadSampleClicked(); };
	savePresetButton.onClick = [this] { onSavePreset(); };
	promptPresetSelector.onChange = [this] { onPresetSelected(); };
	promptPresetSelector.addMouseListener(this, false);

	promptInput.onTextChange = [this]()
	{
		audioProcessor.setLastPrompt(promptInput.getText());
		audioProcessor.setGlobalPrompt(promptInput.getText());
	};

	keySelector.onChange = [this]()
	{
		audioProcessor.setLastKeyIndex(keySelector.getSelectedId());
		audioProcessor.setGlobalKey(keySelector.getText());
	};

	durationSelector.onChange = [this]()
	{
		int val = durationSelector.getSelectedId();
		if (val > 0)
		{
			audioProcessor.setLastDuration((float)val);
			audioProcessor.setGlobalDuration(val);
		}
	};

	promptPresetSelector.onChange = [this]()
	{
		onPresetSelected();
		audioProcessor.setLastPresetIndex(promptPresetSelector.getSelectedId() - 1);
	};

	promptPresetSelector.onMidiLearn = [this]()
	{
		statusLabel.setText("Learning MIDI for prompt selector...", juce::dontSendNotification);
		updateLCD();
		audioProcessor.getMidiLearnManager().startLearning(
		    "promptPresetSelector", &audioProcessor,
		    [this](float value)
		    {
			    juce::MessageManager::callAsync(
			        [this, value]()
			        {
				        int numItems = promptPresetSelector.getNumItems();
				        if (numItems > 0)
				        {
					        int selectedIndex = (int)(value * (numItems - 1));
					        promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
				        }
			        });
		    },
		    "Prompt Preset Selector", &promptPresetSelector);
	};

	promptPresetSelector.onMidiRemove = [this]()
	{ audioProcessor.getMidiLearnManager().removeMappingForParameter("promptPresetSelector"); };

	audioProcessor.getMidiLearnManager().registerUICallback(
	    "promptPresetSelector",
	    [this](float value)
	    {
		    juce::MessageManager::callAsync(
		        [this, value]()
		        {
			        int numItems = promptPresetSelector.getNumItems();
			        if (numItems > 0)
			        {
				        int selectedIndex = (int)(value * (numItems - 1));
				        promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
			        }
		        });
	    });

	promptInput.onReturnKey = [this]()
	{
		juce::String currentPrompt = promptInput.getText().trim();
		if (currentPrompt.isNotEmpty())
		{
			audioProcessor.addCustomPrompt(currentPrompt);
			loadPromptPresets();
			notifyTracksPromptUpdate();
			int totalItems = promptPresetSelector.getNumItems();
			for (int i = 0; i < totalItems; ++i)
			{
				if (promptPresetSelector.getItemText(i) == currentPrompt)
				{
					promptPresetSelector.setSelectedId(i + 1, juce::dontSendNotification);
					break;
				}
			}

			statusLabel.setText("Preset saved: " + currentPrompt, juce::dontSendNotification);
			updateLCD();
		}
		grabKeyboardFocus();
	};

	bypassSequencerButton.onClick = [this]()
	{
		bool isBypassed = bypassSequencerButton.getToggleState();
		audioProcessor.setBypassSequencer(isBypassed);

		if (isBypassed)
		{
			bypassSequencerButton.setButtonText("Composition Mode");
			bypassSequencerButton.loadIcon(BinaryData::cpuregular_svg, BinaryData::cpuregular_svgSize);
			statusLabel.setText("Composition mode - Direct MIDI playback", juce::dontSendNotification);
			updateLCD();
		}
		else
		{
			bypassSequencerButton.setButtonText("Sequencer Mode");
			bypassSequencerButton.loadIcon(BinaryData::cpu_svg, BinaryData::cpu_svgSize);
			statusLabel.setText("Sequencer mode - Armed playback", juce::dontSendNotification);
			updateLCD();
		}
	};

	bypassLLMButton.onClick = [this]()
	{
		bool isBypassed = bypassLLMButton.getToggleState();
		audioProcessor.setBypassLLM(isBypassed);

		if (isBypassed)
		{
			bypassLLMButton.setButtonText("Direct Mode");
			bypassLLMButton.loadIcon(BinaryData::robotregular_svg, BinaryData::robotregular_svgSize);
			statusLabel.setText("Direct Mode: Raw input, direct generation", juce::dontSendNotification);
			updateLCD();
		}
		else
		{
			bypassLLMButton.setButtonText("Enhanced Mode");
			bypassLLMButton.loadIcon(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
			statusLabel.setText("Enhanced Mode: AI-optimized prompt routing", juce::dontSendNotification);
			updateLCD();
		}
	};

	generateButton.onMidiLearn = [this]()
	{
		statusLabel.setText("Learning MIDI for generate button...", juce::dontSendNotification);
		updateLCD();
		audioProcessor.getMidiLearnManager().startLearning("generate", &audioProcessor, nullptr, "Generate Loop",
		                                                   &generateButton);
	};

	generateButton.onMidiRemove = [this]()
	{ audioProcessor.getMidiLearnManager().removeMappingForParameter("generate"); };

	generateButton.onClick = [this]() { onGenerateButtonClicked(); };

	sampleBankPanel->onSampleDroppedToTrack = [this](const juce::String &sampleId, const juce::String &trackId)
	{
		audioProcessor.getAudioManager().loadSampleFromBank(sampleId, trackId);
		setStatusWithTimeout("Sample loaded from bank: " + sampleId.substring(0, 8) + "...", 3000);
	};

	openMidiEditorButton.onClick = [this] { openMidiMappingEditor(); };

	helpButton.onClick = [this]() { showOnboardingStep(1); };

	toggleBankButton.onClick = [this]()
	{
		bool visible = toggleBankButton.getToggleState();
		if (sampleBankPanel)
			sampleBankPanel->setVisible(visible);
		audioProcessor.setBankVisible(visible);
		resized();
	};
}

void DjIaVstEditor::notifyTracksPromptUpdate()
{
	juce::StringArray allPrompts = audioProcessor.promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();

	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);
	for (auto &trackComp : trackComponents)
	{
		trackComp->updatePromptPresets(allPrompts);
	}
}

void DjIaVstEditor::mouseDown(const juce::MouseEvent &event)
{
	if (event.eventComponent == &promptPresetSelector && event.mods.isPopupMenu())
	{
		juce::String selectedPrompt = promptPresetSelector.getText();
		auto customPrompts = audioProcessor.getCustomPrompts();

		if (event.mods.isCtrlDown() && customPrompts.contains(selectedPrompt))
		{
			juce::PopupMenu menu;
			menu.addItem(1, "Edit");
			menu.addItem(2, "Delete");

			menu.showMenuAsync(juce::PopupMenu::Options(),
			                   [this, selectedPrompt](int result)
			                   {
				                   if (result == 1)
				                   {
					                   editCustomPromptDialog(selectedPrompt);
				                   }
				                   else if (result == 2)
				                   {
					                   ObsidianAlertManager::showConfirm(
					                       this, "Delete Custom Prompt",
					                       "Are you sure you want to delete this prompt?\n\n'" + selectedPrompt + "'",
					                       "Delete", "Cancel",
					                       [this, selectedPrompt](bool confirmed)
					                       {
						                       if (confirmed)
						                       {
							                       audioProcessor.removeCustomPrompt(selectedPrompt);
							                       audioProcessor.promptPresets.removeString(selectedPrompt);
							                       audioProcessor.setLastPresetIndex(
							                           audioProcessor.getLastPresetIndex() - 1);
							                       loadPromptPresets();
							                       notifyTracksPromptUpdate();
						                       }
					                       });
				                   }
			                   });
		}
	}
}

void DjIaVstEditor::editCustomPromptDialog(const juce::String &selectedPrompt)
{
	ObsidianAlertManager::showEditPrompt(this, selectedPrompt,
	                                     [this, selectedPrompt](const juce::String &newPrompt)
	                                     {
		                                     audioProcessor.editCustomPrompt(selectedPrompt, newPrompt);
		                                     int index = audioProcessor.promptPresets.indexOf(selectedPrompt);
		                                     if (index >= 0)
			                                     audioProcessor.promptPresets.set(index, newPrompt);
		                                     loadPromptPresets();
	                                     });
}

void DjIaVstEditor::updateUIFromProcessor()
{
	serverUrlInput.setText(audioProcessor.getServerUrl(), juce::dontSendNotification);
	apiKeyInput.setText(audioProcessor.getApiKey(), juce::dontSendNotification);

	promptInput.setText(audioProcessor.getGlobalPrompt(), juce::dontSendNotification);
	int currentDur = juce::roundToInt(audioProcessor.getGlobalDuration());
	durationSelector.setSelectedId(currentDur, juce::dontSendNotification);
	if (durationSelector.getSelectedId() == 0)
		durationSelector.setSelectedId(6, juce::dontSendNotification);

	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);

	bool autoLoadOn = audioProcessor.getAutoLoadEnabled();
	autoLoadButton.setToggleState(autoLoadOn, juce::dontSendNotification);
	loadSampleButton.setEnabled(!autoLoadOn);

	if (autoLoadOn)
	{
		autoLoadButton.setButtonText("Auto-Load Mode");
	}
	else
	{
		autoLoadButton.setButtonText("Manual Mode");
	}

	bool bypassOn = audioProcessor.getBypassSequencer();
	bypassSequencerButton.setToggleState(bypassOn, juce::dontSendNotification);

	if (bypassOn)
	{
		bypassSequencerButton.setButtonText("Composition Mode");
		bypassSequencerButton.loadIcon(BinaryData::cpuregular_svg, BinaryData::cpuregular_svgSize);
	}
	else
	{
		bypassSequencerButton.setButtonText("Sequencer Mode");
		bypassSequencerButton.loadIcon(BinaryData::cpu_svg, BinaryData::cpu_svgSize);
	}

	bool bypassLLMOn = audioProcessor.getBypassLLM();
	bypassLLMButton.setToggleState(bypassLLMOn, juce::dontSendNotification);

	if (bypassLLMOn)
	{
		bypassLLMButton.setButtonText("Direct Mode");
		bypassLLMButton.loadIcon(BinaryData::robotregular_svg, BinaryData::robotregular_svgSize);
	}
	else
	{
		bypassLLMButton.setButtonText("Enhanced Mode");
		bypassLLMButton.loadIcon(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
	}

	int presetIndex = audioProcessor.getLastPresetIndex();
	if (presetIndex >= 0 && presetIndex < audioProcessor.promptPresets.size())
	{
		promptPresetSelector.setSelectedId(presetIndex + 1, juce::dontSendNotification);
	}
	else
	{
		promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
	}

	refreshTrackComponents();
}

void DjIaVstEditor::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDeep);
}

void DjIaVstEditor::layoutPromptSection(juce::Rectangle<int> area, int spacing, int controlsZoneW)
{
	const int itemH = 28;
	const int vPad = (area.getHeight() - itemH) / 2;
	area = area.reduced(0, vPad);

	constexpr int numCtrl = 8;
	const int totalCtrlSpacing = numCtrl * spacing;
	const int ctrlBtnW = juce::jmax(24, (controlsZoneW - totalCtrlSpacing) / numCtrl);

	configButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	toggleBankButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	helpButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	openMidiEditorButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	loadSampleButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	autoLoadButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	bypassSequencerButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	bypassLLMButton.setBounds(area.removeFromLeft(ctrlBtnW));
	area.removeFromLeft(spacing);
	if (sampleBankPanel && sampleBankPanel->isVisible())
	{
		area.removeFromLeft(2);
	}

	const int genBtnW = 50;
	const int saveBtnW = 34;

	area.removeFromRight(2);
	generateButton.setBounds(area.removeFromRight(genBtnW));
	area.removeFromRight(spacing);
	savePresetButton.setBounds(area.removeFromRight(saveBtnW));
	area.removeFromRight(spacing);

	const int idealKeyW = 180;
	const int idealDurW = 100;
	const int idealPresetW = 600;
	const int idealPromptW = 600;

	const int minKeyW = 120;
	const int minDurW = 80;
	const int minPresetW = 280;
	const int minPromptW = 280;

	const int remaining = area.getWidth();
	const int idealTotal = idealKeyW + idealDurW + idealPresetW + idealPromptW + spacing * 3;

	int keyW, durW, presetW, promptW;

	if (remaining >= idealTotal)
	{
		keyW = idealKeyW;
		durW = idealDurW;
		const int extra = remaining - idealTotal;
		presetW = idealPresetW + extra / 2;
		promptW = remaining - keyW - durW - presetW - spacing * 3;
	}
	else
	{
		const float scale = static_cast<float>(remaining) / static_cast<float>(idealTotal);
		keyW = juce::jmax(minKeyW, static_cast<int>(idealKeyW * scale));
		durW = juce::jmax(minDurW, static_cast<int>(idealDurW * scale));
		presetW = juce::jmax(minPresetW, static_cast<int>(idealPresetW * scale));
		promptW = juce::jmax(minPromptW, remaining - keyW - durW - presetW - spacing * 3);
	}

	keySelector.setBounds(area.removeFromRight(keyW));
	area.removeFromRight(spacing);
	durationSelector.setBounds(area.removeFromRight(durW));
	area.removeFromRight(spacing);
	promptPresetSelector.setBounds(area.removeFromRight(presetW));
	area.removeFromRight(spacing);

	promptInput.setBounds(area);
}

void DjIaVstEditor::resized()
{
	static bool resizing = false;
	if (resizing)
		return;
	resizing = true;
	const int spacing = 4;
	const int padding = 6;
	auto fullBounds = getLocalBounds();
	const int bannerHeight = 40;

	const int bankWidth =
	    (sampleBankPanel && sampleBankPanel->isVisible()) ? juce::jmax(290, fullBounds.getWidth() / 6) : 0;

	auto headerArea = fullBounds.removeFromTop(bannerHeight);
	headerArea.reduce(padding, 0);

	const int ctrlZoneW = 290;

	layoutPromptSection(headerArea, spacing, ctrlZoneW);

	if (sampleBankPanel && sampleBankPanel->isVisible())
	{
		auto bankArea = fullBounds.removeFromLeft(bankWidth);
		sampleBankPanel->setBounds(bankArea);
	}
	fullBounds.removeFromLeft(padding);
	fullBounds.removeFromRight(padding);
	auto area = fullBounds;
	const int totalHeight = area.getHeight();
	const int maxMixerHeight = 220;
	const int minMixerHeight = 220;
	int mixerHeight = juce::jlimit(minMixerHeight, maxMixerHeight, static_cast<int>(totalHeight * 0.28f));
	int tracksHeight = totalHeight - mixerHeight - spacing;
	auto tracksArea = area.removeFromTop(tracksHeight);
	tracksViewport.setBounds(tracksArea);
	tracksViewport.setViewedComponent(&tracksContainer, false);
	const int totalContentHeight = TRACK_CELL_H * TRACK_ROWS + spacing * (TRACK_ROWS - 1);
	const int totalContentWidth = TRACK_COLS * 600 + spacing * (TRACK_COLS - 1);
	bool needsHorizontal = totalContentWidth > tracksArea.getWidth();
	bool needsVertical = totalContentHeight + (needsHorizontal ? 12 : 0) > tracksArea.getHeight();
	tracksViewport.setScrollBarsShown(needsVertical, needsHorizontal);
	layoutTracksGrid();
	area.removeFromTop(spacing);

	auto bottomRow = area;
	const int minMixerWidth = 1300;

	if (mixerPanel)
	{
		int contentWidth = juce::jmax(minMixerWidth, bottomRow.getWidth());
		bool needsHorizontalScroll = (contentWidth > bottomRow.getWidth());
		int scrollbarH = needsHorizontalScroll ? (mixerViewport.getScrollBarThickness() + 6) : 6;
		mixerViewport.setBounds(bottomRow);
		mixerPanel->setSize(contentWidth, bottomRow.getHeight() - scrollbarH);
		mixerViewport.setScrollBarsShown(false, needsHorizontalScroll);
		mixerViewport.setVisible(true);
	}
	resizing = false;
	audioProcessor.setWindowSize(getWidth(), getHeight());
}

void DjIaVstEditor::updateLCD()
{
	lcdScreen.setLines(creditsLabel.getText(), statusLabel.getText(), midiIndicator.getText());
}

void DjIaVstEditor::layoutTracksGrid()
{
	const int spacing = 5;
	const int minCellW = 600;
	const int minTotalWidth = TRACK_COLS * minCellW + spacing * (TRACK_COLS - 1);

	auto viewportBounds = tracksViewport.getBounds();
	if (viewportBounds.isEmpty())
		return;

	const int scrollbarAllowance = tracksViewport.isVerticalScrollBarShown() ? 12 : 0;
	const int scrollbarBottomAllowance = tracksViewport.isHorizontalScrollBarShown() ? 4 : 0;
	const int availableWidth = viewportBounds.getWidth() - scrollbarAllowance;
	const int totalWidth = juce::jmax(minTotalWidth, availableWidth);
	const int cellW = (totalWidth - spacing * (TRACK_COLS - 1)) / TRACK_COLS;
	const int totalHeight = TRACK_CELL_H * TRACK_ROWS + spacing * (TRACK_ROWS - 1) + scrollbarBottomAllowance;
	tracksContainer.setSize(totalWidth, totalHeight);

	for (auto &comp : trackComponents)
	{
		TrackData *trackData = audioProcessor.getTrack(comp->getTrackId());
		if (trackData == nullptr)
			continue;

		int slot = trackData->slotIndex;
		if (slot < 0 || slot >= 8)
			continue;

		int col = (trackData->getDeckSide() == TrackData::DeckSide::A) ? 0 : 1;
		int row = slot % 4;

		int x = col * (cellW + spacing);
		int y = row * (TRACK_CELL_H + spacing);

		comp->setBounds(x, y, cellW, TRACK_CELL_H);
	}
}

void DjIaVstEditor::openMidiMappingEditor()
{
	ObsidianAlertManager::showMidiMappingEditor(this, &audioProcessor.getMidiLearnManager());
}

void DjIaVstEditor::setAllGenerateButtonsEnabled(bool enabled)
{
	for (auto &trackComp : trackComponents)
	{
		trackComp->setGenerateButtonEnabled(enabled);
		trackComp->setCanvasGenerating(!enabled);
	}
}

void DjIaVstEditor::startGenerationUI(const juce::String &trackId)
{
	generateButton.setEnabled(false);
	setAllGenerateButtonsEnabled(false);
	statusLabel.setText("Connecting to server...", juce::dontSendNotification);
	updateLCD();

	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->startGeneratingAnimation();
			break;
		}
	}
	if (mixerPanel)
	{
		mixerPanel->startGeneratingAnimationForTrack(trackId);
	}

	juce::Timer::callAfterDelay(
	    100,
	    [this, trackId]()
	    {
		    if (audioProcessor.getIsGenerating() && audioProcessor.getGeneratingTrackId() == trackId)
		    {
			    statusLabel.setText("Generating loop (this may take a few minutes)...", juce::dontSendNotification);
			    updateLCD();
		    }
	    });
}

void DjIaVstEditor::stopGenerationUI(const juce::String &trackId, bool success, const juce::String &errorMessage)
{
	generateButton.setEnabled(true);
	setAllGenerateButtonsEnabled(true);

	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->stopGeneratingAnimation();

			if (success)
			{
				trackComp->setSamplePending(true);

				if (audioProcessor.getAutoLoadEnabled())
				{
					statusLabel.setText("Sample ready - Loading automatically...", juce::dontSendNotification);
					updateLCD();
				}
				else
				{
					statusLabel.setText("Sample ready - Click 'Load Sample' to use it", juce::dontSendNotification);
					updateLCD();
				}
			}

			trackComp->repaint();
			break;
		}
	}

	if (mixerPanel)
	{
		mixerPanel->stopGeneratingAnimationForTrack(trackId);
	}

	isGenerating.store(false);
	wasGenerating.store(false);
	stopGenerationButtonAnimation();
	stopTimer();

	if (!success && !errorMessage.isEmpty())
	{
		statusLabel.setText("Error: " + errorMessage, juce::dontSendNotification);
		updateLCD();
	}
}

void DjIaVstEditor::onSampleLoaded(const juce::String &trackId)
{
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->setSamplePending(false);
			trackComp->updateFromTrackData();
			trackComp->repaint();
			if (mixerPanel)
				mixerPanel->clearSamplePending(trackId);
			break;
		}
	}
}

void DjIaVstEditor::onGenerateButtonClicked()
{
	juce::String serverUrl = audioProcessor.getServerUrl();
	juce::String apiKey = audioProcessor.getApiKey();
	if (serverUrl.isEmpty())
	{
		statusLabel.setText("Error: Server URL is required", juce::dontSendNotification);
		updateLCD();
		return;
	}
	bool isLocalServer = serverUrl.contains("localhost") || serverUrl.contains("127.0.0.1");
	if (apiKey.isEmpty() && !isLocalServer)
	{
		statusLabel.setText("Error: API Key is required", juce::dontSendNotification);
		updateLCD();
		return;
	}
	juce::String currentPrompt = promptInput.getText().trim();
	if (currentPrompt.isEmpty())
	{
		statusLabel.setText("Error: Prompt cannot be empty", juce::dontSendNotification);
		updateLCD();
		statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
		return;
	}
	audioProcessor.getGenerationManager().syncSelectedTrackWithGlobalPrompt();
	audioProcessor.setIsGenerating(true);
	generatingTrackId = audioProcessor.getSelectedTrackId();
	audioProcessor.setGeneratingTrackId(generatingTrackId);
	TrackData *track = audioProcessor.getTrackManager().getTrack(generatingTrackId);

	if (!track)
	{
		statusLabel.setText("Error: No track selected", juce::dontSendNotification);
		updateLCD();
		return;
	}

	auto &currentPage = track->getCurrentPage();
	currentPage.selectedPrompt = promptInput.getText();
	currentPage.generationPrompt = promptInput.getText();
	currentPage.generationBpm = (float)audioProcessor.getHostBpm();
	currentPage.generationKey = keySelector.getText();
	currentPage.generationDuration = durationSelector.getSelectedId();
	if (currentPage.selectedModel.isEmpty())
		currentPage.selectedModel = "stable-audio-open-1.0";

	startGenerationUI(generatingTrackId);
	juce::String selectedTrackId = generatingTrackId;
	auto request = track->createLoopRequest();
	juce::Thread::launch(
	    [this, selectedTrackId, request]()
	    {
		    try
		    {
			    juce::MessageManager::callAsync(
			        [this]()
			        {
				        statusLabel.setText("Generating loop (this may take a few minutes)...",
				                            juce::dontSendNotification);
				        updateLCD();
			        });

			    audioProcessor.setServerUrl(audioProcessor.getServerUrl());
			    audioProcessor.setApiKey(audioProcessor.getApiKey());
			    juce::Thread::sleep(100);
			    audioProcessor.getGenerationManager().generateLoop(request, generatingTrackId);
		    }
		    catch (const std::exception &e)
		    {
			    juce::MessageManager::callAsync(
			        [this, selectedTrackId, error = juce::String(e.what())]()
			        {
				        stopGenerationUI(selectedTrackId, false, error);
				        audioProcessor.setIsGenerating(false);
				        audioProcessor.setGeneratingTrackId("");
			        });
		    }
	    });
}

void DjIaVstEditor::loadPromptPresets()
{
	promptPresetSelector.clear();
	juce::StringArray allPrompts = audioProcessor.promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();
	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);

	for (int i = 0; i < allPrompts.size(); ++i)
	{
		promptPresetSelector.addItem(allPrompts[i], i + 1);
	}
	int lastPresetIndex = audioProcessor.getLastPresetIndex();
	if (lastPresetIndex >= 1 && lastPresetIndex <= allPrompts.size())
	{
		promptPresetSelector.setSelectedId(lastPresetIndex + 1, juce::dontSendNotification);
	}
	else
	{
		promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
	}
	juce::String selectedPresetText = promptPresetSelector.getText();
	promptInput.setText(selectedPresetText, juce::dontSendNotification);
}

DjIaVstEditor::KeyboardLayout DjIaVstEditor::detectKeyboardLayout()
{
#if JUCE_WINDOWS
	HKL layout = GetKeyboardLayout(0);
	WORD primaryLang = PRIMARYLANGID(LOWORD(layout));

	if (primaryLang == LANG_FRENCH)
		return AZERTY;
	if (primaryLang == LANG_GERMAN)
		return QWERTZ;
#endif
	return QWERTY;
}

bool DjIaVstEditor::keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected)
{
	if (pressed == expected)
		return true;
	if (pressed.getKeyCode() == expected.getKeyCode())
		return true;
	if (expected.getKeyCode() >= '1' && expected.getKeyCode() <= '4')
	{
		int expectedNum = expected.getKeyCode() - '0';
		if (pressed.getKeyCode() >= '1' && pressed.getKeyCode() <= '4')
		{
			int pressedNum = pressed.getKeyCode() - '0';
			return pressedNum == expectedNum;
		}
	}

	return false;
}

void DjIaVstEditor::refreshAllPromptLists()
{
	loadPromptPresets();
	notifyTracksPromptUpdate();
}

bool DjIaVstEditor::keyPressed(const juce::KeyPress &key)
{
	KeyboardLayout layout = detectKeyboardLayout();

	std::vector<std::vector<juce::KeyPress>> layoutKeys(8);

	switch (layout)
	{
	case AZERTY:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('z'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('q'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('w'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress(','), juce::KeyPress(';')}};
		break;

	case QWERTY:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('z'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')}};
		break;

	case QWERTZ:
		layoutKeys = {{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
		              {juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
		              {juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
		              {juce::KeyPress('y'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
		              {juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
		              {juce::KeyPress('t'), juce::KeyPress('z'), juce::KeyPress('u'), juce::KeyPress('i')},
		              {juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
		              {juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')}};
		break;
	}

	for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
	{
		for (int page = 0; page < 4; ++page)
		{
			if (keyMatches(key, layoutKeys[slotIndex][page]))
			{
				for (auto &trackComp : trackComponents)
				{
					if (auto *track = trackComp->getTrack())
					{
						if (track->slotIndex == slotIndex)
						{
							if (audioProcessor.getIsGenerating() &&
							    audioProcessor.getGeneratingTrackId() == track->trackId)
							{
								setStatusWithTimeout("Cannot switch pages during generation...");
								return false;
							}
							else
							{
								trackComp->onPageSelected(page);
								return true;
							}
						}
					}
				}
			}
		}
	}

	return Component::keyPressed(key);
}

void DjIaVstEditor::onPresetSelected()
{
	int selectedId = promptPresetSelector.getSelectedId();
	audioProcessor.setLastPresetIndex(selectedId);
	juce::String selectedPrompt = promptPresetSelector.getText();
	if (!selectedPrompt.isEmpty())
	{
		promptInput.setText(selectedPrompt);
		statusLabel.setText("Preset loaded: " + selectedPrompt, juce::dontSendNotification);
		updateLCD();
	}
	else
	{
		promptInput.clear();
		statusLabel.setText("Custom prompt mode", juce::dontSendNotification);
		updateLCD();
	}
}

void DjIaVstEditor::onSavePreset()
{
	juce::String currentPrompt = promptInput.getText().trim();
	if (currentPrompt.isNotEmpty())
	{
		audioProcessor.addCustomPrompt(currentPrompt);
		loadPromptPresets();
		notifyTracksPromptUpdate();
		int totalItems = promptPresetSelector.getNumItems();
		for (int i = 0; i < totalItems; ++i)
		{
			if (promptPresetSelector.getItemText(i) == currentPrompt)
			{
				promptPresetSelector.setSelectedId(i + 1, juce::dontSendNotification);
				break;
			}
		}

		statusLabel.setText("Preset saved: " + currentPrompt, juce::dontSendNotification);
		updateLCD();
	}
	else
	{
		statusLabel.setText("Enter a prompt first!", juce::dontSendNotification);
		updateLCD();
	}
}

void DjIaVstEditor::onAutoLoadToggled()
{
	bool autoLoadOn = autoLoadButton.getToggleState();
	audioProcessor.setAutoLoadEnabled(autoLoadOn);

	if (autoLoadOn)
	{
		autoLoadButton.setButtonText("Auto-Load Mode");
		statusLabel.setText("Auto-load enabled - samples load automatically", juce::dontSendNotification);
		updateLCD();
		loadSampleButton.setButtonText("Load\nSample");
		loadSampleButton.setEnabled(false);
	}
	else
	{
		autoLoadButton.setButtonText("Manual\nMode");
		statusLabel.setText("Manual mode - click Load Sample when ready", juce::dontSendNotification);
		updateLCD();
		loadSampleButton.setEnabled(true);
		updateLoadButtonState();
	}
}

void DjIaVstEditor::onLoadSampleClicked()
{
	if (audioProcessor.hasSampleWaiting())
	{
		juce::String trackId = audioProcessor.getSelectedTrackId();

		audioProcessor.loadPendingSample();
		statusLabel.setText("Sample loaded manually!", juce::dontSendNotification);
		updateLCD();

		onSampleLoaded(trackId);
		updateLoadButtonState();
	}
	else
	{
		statusLabel.setText("Generate a loop first", juce::dontSendNotification);
		updateLCD();
	}
}

void DjIaVstEditor::updateLoadButtonState()
{
	if (!autoLoadButton.getToggleState())
	{
		if (audioProcessor.hasSampleWaiting())
		{
			loadSampleButton.setButtonText("Ready");
		}
		else
		{
			loadSampleButton.setButtonText("Load\nSample");
		}
	}
}

void DjIaVstEditor::visibilityChanged()
{
	if (isVisible())
	{
		juce::Timer::callAfterDelay(50,
		                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		                            {
			                            if (safeThis != nullptr)
				                            safeThis->refreshTrackComponents();
		                            });
	}
}

void DjIaVstEditor::refreshTrackComponents()
{
	if (isBeingDestroyed.load())
		return;
	if (audioProcessor.getTrackManager().isInitializing.load())
	{
		juce::Timer::callAfterDelay(50, [this]() { refreshTrackComponents(); });
		return;
	}
	auto trackIds = audioProcessor.getAllTrackIds();

	if (trackComponents.size() == trackIds.size())
	{
		bool allVisible = true;
		for (auto &comp : trackComponents)
		{
			if (!comp->isVisible() || comp->getParentComponent() == nullptr)
			{
				allVisible = false;
				break;
			}
		}
		if (allVisible)
		{
			for (int i = 0; i < (int)trackComponents.size() && i < (int)trackIds.size(); ++i)
			{
				trackComponents[i]->setTrackData(audioProcessor.getTrack(trackIds[i]));

				trackComponents[i]->updateFromTrackData();
				if (auto *sequencer = trackComponents[i]->getSequencer())
					sequencer->updateFromTrackData();
			}
			updateSelectedTrack();
			return;
		}
	}
	setEnabled(false);
	juce::String previousSelectedId = audioProcessor.getSelectedTrackId();
	juce::String generatingId = audioProcessor.getGeneratingTrackId();
	bool wasGeneratingLocal = audioProcessor.getIsGenerating();

	trackComponents.clear();
	tracksContainer.removeAllChildren();

	for (const auto &trackId : trackIds)
	{
		TrackData *trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;

		auto trackComp = std::make_unique<TrackComponent>(trackId, audioProcessor);
		trackComp->setTrackData(trackData);

		trackComp->onSelectTrack = [this](const juce::String &id)
		{
			audioProcessor.selectTrack(id);
			updateSelectedTrack();
		};

		trackComp->onGenerateWithImage =
		    [this](const juce::String &trackId, const juce::String &image, const juce::StringArray &keywords)
		{ audioProcessor.getGenerationManager().generateSampleWithImage(trackId, image, keywords); };

		trackComp->onTrackRenamed = [this](const juce::String &id, const juce::String &newName)
		{
			if (mixerPanel)
			{
				mixerPanel->updateTrackName(id, newName);
			}
		};

		trackComp->onModelChanged = [this](const juce::String &id)
		{
			if (mixerPanel)
			{
				mixerPanel->updateModelUI(id);
			}
		};

		trackComp->onGenerateForTrack = [this](const juce::String &id)
		{
			audioProcessor.selectTrack(id);
			generateFromTrackComponent(id);
		};

		trackComp->onReorderTrack = [this](const juce::String &fromId, const juce::String &toId)
		{
			audioProcessor.reorderTracks(fromId, toId);
			juce::Timer::callAfterDelay(10, [this]() { refreshTrackComponents(); });
		};

		trackComp->onPreviewTrack = [this](const juce::String &trackId) { audioProcessor.previewTrack(trackId); };

		trackComp->onTrackPromptChanged = [this](const juce::String /*&trackId*/, const juce::String &prompt)
		{ setStatusWithTimeout("Track prompt updated: " + prompt.substring(0, 20) + "...", 3000); };

		trackComp->onStatusMessage = [this](const juce::String &message) { setStatusWithTimeout(message, 3000); };

		trackComp->onStopPreview = [this](const juce::String &trackId)
		{ audioProcessor.getAudioManager().stopTrackPreview(trackId); };

		if (trackId == audioProcessor.getSelectedTrackId())
		{
			trackComp->setSelected(true);
		}
		if (wasGeneratingLocal && trackId == generatingId)
		{
			trackComp->startGeneratingAnimation();
		}

		tracksContainer.addAndMakeVisible(trackComp.get());
		trackComponents.push_back(std::move(trackComp));
	}

	layoutTracksGrid();

	if (mixerPanel)
	{
		mixerPanel->refreshMixerChannels();
	}
	setEnabled(true);
	juce::MessageManager::callAsync(
	    [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
	    {
		    if (safeThis != nullptr)
		    {
			    safeThis->resized();
			    safeThis->repaint();
		    }
	    });
	tracksContainer.repaint();
}

void DjIaVstEditor::reEnableCanvasForTrack()
{
	setAllGenerateButtonsEnabled(true);
}

void DjIaVstEditor::generateFromTrackComponent(const juce::String &trackId)
{
	audioProcessor.setIsGenerating(true);

	TrackData *track = audioProcessor.getTrack(trackId);
	if (!track)
	{
		statusLabel.setText("Error: Track not found", juce::dontSendNotification);
		updateLCD();
		audioProcessor.setIsGenerating(false);
		return;
	}

	if (track->getCurrentPage().selectedPrompt.isEmpty())
	{
		statusLabel.setText("Error: No prompt selected for this track", juce::dontSendNotification);
		updateLCD();
		audioProcessor.setIsGenerating(false);
		return;
	}

	juce::String currentGeneratingTrackId = trackId;
	audioProcessor.setGeneratingTrackId(currentGeneratingTrackId);

	auto &currentPage = track->getCurrentPage();

	currentPage.selectedPrompt = track->getCurrentPage().selectedPrompt;
	currentPage.generationPrompt = track->getCurrentPage().selectedPrompt;
	currentPage.generationBpm = audioProcessor.getGlobalBpm();
	currentPage.generationKey = audioProcessor.getGlobalKey();
	currentPage.generationDuration = audioProcessor.getGlobalDuration();
	if (currentPage.selectedModel.isEmpty())
		currentPage.selectedModel = "stable-audio-open-1.0";

	startGenerationUI(currentGeneratingTrackId);

	juce::Thread::launch(
	    [this, currentGeneratingTrackId, track]()
	    {
		    try
		    {
			    auto request = track->createLoopRequest();
			    audioProcessor.getGenerationManager().generateLoop(request, currentGeneratingTrackId);
		    }
		    catch (const std::exception &e)
		    {
			    juce::MessageManager::callAsync(
			        [this, currentGeneratingTrackId, error = juce::String(e.what())]()
			        {
				        stopGenerationUI(currentGeneratingTrackId, false, error);
				        audioProcessor.setIsGenerating(false);
				        audioProcessor.setGeneratingTrackId("");
			        });
		    }
	    });
}

juce::StringArray DjIaVstEditor::getAllPrompts() const
{
	juce::StringArray allPrompts = audioProcessor.promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();

	for (const auto &customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}

	return allPrompts;
}

void DjIaVstEditor::restoreUICallbacks()
{
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrack())
		{
			trackComp->setupMidiLearn();
		}
	}
}

void DjIaVstEditor::setStatusWithTimeout(const juce::String &message, int timeoutMs)
{
	statusLabel.setText(message, juce::dontSendNotification);
	updateLCD();
	juce::Timer::callAfterDelay(timeoutMs,
	                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
	                            {
		                            if (auto *editor = safeThis.getComponent())
		                            {
			                            editor->statusLabel.setText("Ready", juce::dontSendNotification);
			                            editor->updateLCD();
		                            }
	                            });
}

void DjIaVstEditor::updateSelectedTrack()
{
	for (auto &trackComp : trackComponents)
	{
		trackComp->setSelected(false);
	}

	juce::String selectedId = audioProcessor.getSelectedTrackId();

	bool found = false;
	for (auto &trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == selectedId)
		{
			trackComp->setSelected(true);
			found = true;
			break;
		}
	}

	if (mixerPanel)
	{
		mixerPanel->trackSelected(selectedId);
	}
}

void *DjIaVstEditor::getSequencerForTrack(const juce::String &trackId)
{
	if (isBeingDestroyed.load())
		return nullptr;
	if (trackComponents.empty())
		return nullptr;
	for (auto &trackComp : trackComponents)
	{
		if (trackComp == nullptr)
			continue;
		if (trackComp->getTrackId() == trackId)
			return (void *)trackComp->getSequencer();
	}
	return nullptr;
}

void DjIaVstEditor::refreshMixerChannels()
{
	if (mixerPanel)
	{
		mixerPanel->refreshAllChannels();
	}
}

void DjIaVstEditor::refreshCredits()
{
	refreshCreditsAsync();
}

void DjIaVstEditor::refreshCreditsAsync()
{
	juce::String currentApiKey = audioProcessor.getApiKey();
	juce::String currentServerUrl = audioProcessor.getServerUrl();
	int timeout = audioProcessor.getRequestTimeout();

	if (currentApiKey.isEmpty())
	{
		creditsLabel.setText("Credits: No API Key", juce::dontSendNotification);
		return;
	}

	if (currentServerUrl.isEmpty())
	{
		creditsLabel.setText("Credits: No Server", juce::dontSendNotification);
		return;
	}

	creditsLabel.setText("Credits: Loading...", juce::dontSendNotification);

	audioProcessor.getApiClient().setApiKey(currentApiKey);
	audioProcessor.getApiClient().setBaseUrl(currentServerUrl);

	juce::Thread::launch(
	    [this, timeout, safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
	    {
		    if (audioProcessor.isShuttingDown.load())
			    return;
		    auto creditsInfo = audioProcessor.getApiClient().checkCredits(timeout);
		    juce::MessageManager::callAsync(
		        [safeThis, creditsInfo]()
		        {
			        if (auto *editor = safeThis.getComponent())
			        {
				        if (creditsInfo.success)
				        {
					        juce::String creditsText;
					        if (creditsInfo.creditsRemaining == -1 || creditsInfo.creditsTotal == -1)
					        {
						        creditsText = "Credits: Unlimited";
					        }
					        else
					        {
						        creditsText = "Credits: " + juce::String(creditsInfo.creditsRemaining) + " / " +
						                      juce::String(creditsInfo.creditsTotal);
					        }

					        editor->creditsLabel.setText(creditsText, juce::dontSendNotification);
					        editor->audioProcessor.setCreditsRemaining(creditsInfo.creditsRemaining);
					        editor->audioProcessor.canGenerateStandard = creditsInfo.canGenerateStandard;
				        }
				        else
				        {
					        editor->creditsLabel.setText("Credits: Error", juce::dontSendNotification);
				        }
			        }
		        });
	    });
}

void DjIaVstEditor::checkForUpdates()
{
	juce::Thread::launch(
	    [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
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
				        [safeThis, tagName]()
				        {
					        if (auto *editor = safeThis.getComponent())
					        {
						        if (editor->isInitialized.load())
						        {
							        juce::Timer::callAfterDelay(2000,
							                                    [safeThis, tagName]()
							                                    {
								                                    if (auto *editor = safeThis.getComponent())
								                                    {
									                                    ObsidianAlertManager::showUpdateAvailable(
									                                        safeThis, tagName,
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

TrackComponent *DjIaVstEditor::getTrackComponent(const juce::String &trackId)
{
	for (auto &track : trackComponents)
	{
		if (track->getTrackId() == trackId)
		{
			return track.get();
		}
	}

	return nullptr;
}