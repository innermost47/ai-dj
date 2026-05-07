#include "PluginEditor.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "ObsidianAlertManager.h"
#include "PluginProcessor.h"
#include "SequencerComponent.h"
#if JucePlugin_Build_Standalone
#include "StandaloneTransport.h"
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
	uiLayoutManager = std::make_unique<UILayoutManager>(*this);
	uiStatusManager = std::make_unique<UIStatusManager>(*this);
	uiModalManager = std::make_unique<UIModalManager>(*this);
	uiGenerationManager = std::make_unique<UIGenerationManager>(*this);
	uiTrackManager = std::make_unique<UITrackManager>(*this);
	uiPresetManager = std::make_unique<UIPresetManager>(*this);
	uiMidiManager = std::make_unique<UIMidiManager>(*this);
#if JucePlugin_Build_Standalone
	lcdScreen.setTwoLineMode(true);
#endif
	audioProcessor.setGenerationListener(uiGenerationManager.get());
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
			    weakThis->uiMidiManager->updateMidiIndicator(noteInfo);
	    });

	audioProcessor.onUIUpdateNeeded = [weakThis]()
	{
		juce::MessageManager::callAsync(
		    [weakThis]()
		    {
			    if (weakThis != nullptr)
				    weakThis->uiTrackManager->updateUIComponents();
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
			                            safeThis->uiModalManager->checkForUpdates();
		                            }
	                            });
}

DjIaVstEditor::~DjIaVstEditor()
{
	isBeingDestroyed.store(true);
	stopTimer();

	audioProcessor.onMasterOutput = nullptr;
	audioProcessor.setMidiIndicatorCallback(nullptr);
	audioProcessor.onUIUpdateNeeded = nullptr;
	audioProcessor.setGenerationListener(nullptr);

	uiTrackManager = nullptr;
	uiModalManager = nullptr;
	uiStatusManager = nullptr;
	uiLayoutManager = nullptr;
	uiGenerationManager = nullptr;
	uiPresetManager = nullptr;
	uiMidiManager = nullptr;

	if (mixerPanel)
	{
		mixerViewport.setViewedComponent(nullptr, false);
		mixerViewport.setVisible(false);
		mixerPanel.reset();
	}

	setLookAndFeel(nullptr);
	ObsidianAlertManager::shutdown();
}

bool DjIaVstEditor::keyPressed(const juce::KeyPress &key)
{
	return uiMidiManager->keyPressed(key);
}

bool DjIaVstEditor::keyStateChanged(bool isKeyDown)
{
	return uiMidiManager->keyStateChanged(isKeyDown);
}

#if JucePlugin_Build_Standalone
void DjIaVstEditor::parentHierarchyChanged()
{
	if (auto *window = findParentComponentOfClass<juce::DocumentWindow>())
	{
		window->setTitleBarButtonsRequired(juce::DocumentWindow::minimiseButton | juce::DocumentWindow::maximiseButton |
		                                       juce::DocumentWindow::closeButton,
		                                   false);

		window->setResizable(true, false);
		window->setFullScreen(true);
	}
}
#endif

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

	uiPresetManager->loadPromptPresets();
	uiTrackManager->refreshTracks();
	uiStatusManager->refreshCreditsAsync();

	if (audioProcessor.getIsGenerating())
	{
		generateButton.setEnabled(false);
		uiGenerationManager->setAllGenerateButtonsEnabled(false);
		statusLabel.setText("Generation in progress...", juce::dontSendNotification);
		uiStatusManager->updateLCD();
		juce::String generatingId = audioProcessor.getGeneratingTrackId();
		for (auto &trackComp : uiTrackManager->getTrackComponents())
		{
			if (trackComp && trackComp->getTrackId() == generatingId)
			{
				trackComp->startGeneratingAnimation();
				break;
			}
		}
	}
}

void DjIaVstEditor::initUI()
{
	if (isInitialized.load())
		return;

	setupUI();
	uiTrackManager->refreshUIForMode();
	serverUrlInput.setText(audioProcessor.getServerUrl(), juce::dontSendNotification);
	apiKeyInput.setText(audioProcessor.getApiKey(), juce::dontSendNotification);
	if (audioProcessor.getServerUrl().isEmpty())
	{
		juce::Timer::callAfterDelay(500, [this]() { uiModalManager->showFirstTimeSetup(); });
	}
	isInitialized.store(true);
	juce::WeakReference<DjIaVstEditor> weakThis(this);

	audioProcessor.setMidiIndicatorCallback(
	    [weakThis](const juce::String &noteInfo)
	    {
		    if (weakThis != nullptr)
			    weakThis->uiMidiManager->updateMidiIndicator(noteInfo);
	    });

	audioProcessor.onUIUpdateNeeded = [weakThis]()
	{
		juce::MessageManager::callAsync(
		    [weakThis]()
		    {
			    if (weakThis != nullptr)
				    weakThis->uiTrackManager->updateUIComponents();
		    });
	};
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
	for (auto &trackComp : uiTrackManager->getTrackComponents())
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
		for (auto &trackComp : uiTrackManager->getTrackComponents())
		{
			TrackData *track = audioProcessor.getTrack(trackComp->getTrackId());
			if (track && (track->timeStretchMode == 3 || track->timeStretchMode == 4))
				trackComp->updateWaveformWithTimeStretch();
		}
	}
}

void DjIaVstEditor::addModal(std::unique_ptr<ObsidianModalOverlay> overlay)
{
	uiModalManager->addModal(std::move(overlay));
}

void DjIaVstEditor::removeModal(ObsidianModalOverlay *overlay)
{
	uiModalManager->removeModal(overlay);
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
	configButton.onClick = [this]() { uiModalManager->showConfigDialog(); };

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
#if JucePlugin_Build_Standalone
		if (audioProcessor.getStandaloneTransport())
			mixerPanel->setStandaloneTransport(audioProcessor.getStandaloneTransport());
#endif
		mixerPanel->onTrackRenamedFromMixer = [this](const juce::String &trackId, const juce::String &newName)
		{
			for (auto &trackComp : uiTrackManager->getTrackComponents())
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

	auto setupControlBtn = [](IconButtonSimple &btn, bool hasAccentBar = true)
	{
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
		btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		btn.setHasAccentBar(hasAccentBar);
		btn.setShowBorder(true);
	};

	setupControlBtn(bypassSequencerButton);
	setupControlBtn(autoLoadButton);
	setupControlBtn(loadSampleButton, false);
	setupControlBtn(openMidiEditorButton, false);
	setupControlBtn(configButton, false);
	setupControlBtn(helpButton, false);
	setupControlBtn(toggleBankButton);
	setupControlBtn(bypassLLMButton);

	savePresetButton.setShowBorder(true);
	generateButton.setShowBorder(true);

	setSize(audioProcessor.getSavedWindowWidth(), audioProcessor.getSavedWindowHeight());

	bool bankVisible = audioProcessor.getSavedBankVisible();
	toggleBankButton.setToggleState(bankVisible, juce::dontSendNotification);
	if (sampleBankPanel)
		sampleBankPanel->setVisible(bankVisible);

	uiTrackManager->refreshTrackComponents();

	addEventListeners();
}

void DjIaVstEditor::addEventListeners()
{
	autoLoadButton.onClick = [this] { onAutoLoadToggled(); };
	loadSampleButton.onClick = [this] { onLoadSampleClicked(); };
	savePresetButton.onClick = [this] { uiPresetManager->onSavePreset(); };
	promptPresetSelector.onChange = [this] { uiPresetManager->onPresetSelected(); };
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
		uiPresetManager->onPresetSelected();
		audioProcessor.setLastPresetIndex(promptPresetSelector.getSelectedId() - 1);
	};

	promptPresetSelector.onMidiLearn = [this]()
	{
		statusLabel.setText("Learning MIDI for prompt selector...", juce::dontSendNotification);
		uiStatusManager->updateLCD();
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
			uiPresetManager->loadPromptPresets();
			uiPresetManager->notifyTracksPromptUpdate();
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
			uiStatusManager->updateLCD();
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
			uiStatusManager->updateLCD();
		}
		else
		{
			bypassSequencerButton.setButtonText("Sequencer Mode");
			bypassSequencerButton.loadIcon(BinaryData::cpu_svg, BinaryData::cpu_svgSize);
			statusLabel.setText("Sequencer mode - Armed playback", juce::dontSendNotification);
			uiStatusManager->updateLCD();
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
			uiStatusManager->updateLCD();
		}
		else
		{
			bypassLLMButton.setButtonText("Enhanced Mode");
			bypassLLMButton.loadIcon(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
			statusLabel.setText("Enhanced Mode: AI-optimized prompt routing", juce::dontSendNotification);
			uiStatusManager->updateLCD();
		}
	};

	generateButton.onMidiLearn = [this]()
	{
		statusLabel.setText("Learning MIDI for generate button...", juce::dontSendNotification);
		uiStatusManager->updateLCD();
		audioProcessor.getMidiLearnManager().startLearning("generate", &audioProcessor, nullptr, "Generate Loop",
		                                                   &generateButton);
	};

	generateButton.onMidiRemove = [this]()
	{ audioProcessor.getMidiLearnManager().removeMappingForParameter("generate"); };

	generateButton.onClick = [this]() { uiGenerationManager->onGenerateButtonClicked(); };

	sampleBankPanel->onSampleDroppedToTrack = [this](const juce::String &sampleId, const juce::String &trackId)
	{
		audioProcessor.getAudioManager().loadSampleFromBank(sampleId, trackId);
		uiStatusManager->setStatusWithTimeout("Sample loaded from bank: " + sampleId.substring(0, 8) + "...", 3000);
	};

	openMidiEditorButton.onClick = [this] { uiModalManager->openMidiMappingEditor(); };

	helpButton.onClick = [this]() { uiModalManager->showOnboardingStep(1); };

	toggleBankButton.onClick = [this]()
	{
		bool visible = toggleBankButton.getToggleState();
		if (sampleBankPanel)
			sampleBankPanel->setVisible(visible);
		audioProcessor.setBankVisible(visible);
		resized();
	};
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
					                   uiModalManager->editCustomPromptDialog(selectedPrompt);
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
							                       uiPresetManager->loadPromptPresets();
							                       uiPresetManager->notifyTracksPromptUpdate();
						                       }
					                       });
				                   }
			                   });
		}
	}
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

	uiTrackManager->refreshTrackComponents();
}

void DjIaVstEditor::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDeep);
}

bool DjIaVstEditor::keyMatches(const juce::KeyPress &pressed, const juce::KeyPress &expected)
{
	return uiMidiManager->keyMatches(pressed, expected);
}

void DjIaVstEditor::onAutoLoadToggled()
{
	bool autoLoadOn = autoLoadButton.getToggleState();
	audioProcessor.setAutoLoadEnabled(autoLoadOn);

	if (autoLoadOn)
	{
		autoLoadButton.setButtonText("Auto-Load Mode");
		statusLabel.setText("Auto-load enabled - samples load automatically", juce::dontSendNotification);
		uiStatusManager->updateLCD();
		loadSampleButton.setButtonText("Load\nSample");
		loadSampleButton.setEnabled(false);
	}
	else
	{
		autoLoadButton.setButtonText("Manual\nMode");
		statusLabel.setText("Manual mode - click Load Sample when ready", juce::dontSendNotification);
		uiStatusManager->updateLCD();
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
		uiStatusManager->updateLCD();

		uiTrackManager->onSampleLoaded(trackId);
		updateLoadButtonState();
	}
	else
	{
		statusLabel.setText("Generate a loop first", juce::dontSendNotification);
		uiStatusManager->updateLCD();
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
				                            safeThis->uiTrackManager->refreshTrackComponents();
		                            });
	}
}

void DjIaVstEditor::resized()
{
	if (uiLayoutManager)
		uiLayoutManager->resized();
}

void DjIaVstEditor::restoreUICallbacks()
{
	for (auto &trackComp : uiTrackManager->getTrackComponents())
	{
		if (trackComp->getTrack())
		{
			trackComp->setupMidiLearn();
		}
	}
}

void *DjIaVstEditor::getSequencerForTrack(const juce::String &trackId)
{
	if (isBeingDestroyed.load())
		return nullptr;
	if (uiTrackManager->getTrackComponents().empty())
		return nullptr;
	for (auto &trackComp : uiTrackManager->getTrackComponents())
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
