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
	uiLayoutManager = std::make_unique<UILayoutManager>(*this);
	uiStatusManager = std::make_unique<UIStatusManager>(*this);
	uiModalManager = std::make_unique<UIModalManager>(*this);
	uiGenerationManager = std::make_unique<UIGenerationManager>(*this);
	uiTrackManager = std::make_unique<UITrackManager>(*this);
	uiPresetManager = std::make_unique<UIPresetManager>(*this);
	uiMidiManager = std::make_unique<UIMidiManager>(*this);
	leftPanelWrapper = std::make_unique<LeftPanelWrapper>(audioProcessor, *this);
	mixerPanel = std::make_unique<MixerPanel>(audioProcessor);
	lcdScreen = std::make_unique<LCDScreen>();
	masterWaveformDisplay = std::make_unique<MasterWaveformDisplay>();
	rightPanelWrapper = std::make_unique<RightPanelWrapper>(audioProcessor);

#if JucePlugin_Build_Standalone
	lcdScreen->setTwoLineMode(true);
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
	if (leftPanelWrapper)
	{
		auto state = leftPanelWrapper->saveUIState();
		audioProcessor.setPanelStateJson(juce::JSON::toString(state));
	}
	audioProcessor.onMasterOutput = nullptr;
	audioProcessor.setMidiIndicatorCallback(nullptr);
	audioProcessor.onUIUpdateNeeded = nullptr;
	audioProcessor.setGenerationListener(nullptr);

	uiLayoutManager = nullptr;
	uiStatusManager = nullptr;
	uiModalManager = nullptr;
	uiGenerationManager = nullptr;
	uiTrackManager = nullptr;
	uiPresetManager = nullptr;
	uiMidiManager = nullptr;
	leftPanelWrapper = nullptr;
	mixerPanel = nullptr;
	lcdScreen = nullptr;
	masterWaveformDisplay = nullptr;
	rightPanelWrapper = nullptr;

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

	uiTrackManager->refreshTracks();
	uiStatusManager->refreshCreditsAsync();

	if (audioProcessor.getIsGenerating())
	{
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
	addAndMakeVisible(configButton);
	configButton.loadIcon(BinaryData::gear_svg, BinaryData::gear_svgSize);
	configButton.setTooltip("Configure settings globally");
	configButton.onClick = [this]() { uiModalManager->showConfigDialog(); };

	addAndMakeVisible(tracksViewport);
	tracksViewport.setViewedComponent(&tracksContainer, false);
	tracksViewport.setScrollBarsShown(true, true);
	tracksContainer.setWantsKeyboardFocus(false);
	tracksViewport.setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(true);

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
	configButton.setTooltip("Configure API settings and generation mode");

	addAndMakeVisible(leftPanelWrapper.get());

	addAndMakeVisible(openMidiEditorButton);
	openMidiEditorButton.loadIcon(BinaryData::piano_svg, BinaryData::piano_svgSize);
	openMidiEditorButton.setTooltip("Open MIDI mappings editor");

	addAndMakeVisible(helpButton);
	helpButton.loadIcon(BinaryData::info_svg, BinaryData::info_svgSize);
	helpButton.setTooltip("Open the Quick Start tour");

	addAndMakeVisible(lcdScreen.get());

	addAndMakeVisible(masterWaveformDisplay.get());
	mixerPanel->setMasterWaveform(masterWaveformDisplay.get());
	mixerPanel->setLCDScreen(lcdScreen.get());
	audioProcessor.onMasterOutput = [this](const float *l, const float *r, int n, double ppq)
	{
		if (masterWaveformDisplay)
		{
			masterWaveformDisplay->pushSamples(l, r, n);
			masterWaveformDisplay->setPositionInBeats(ppq);
		}
	};

	addAndMakeVisible(rightPanelWrapper.get());

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
	setupControlBtn(openMidiEditorButton, false);
	setupControlBtn(configButton, false);
	setupControlBtn(helpButton, false);
	setupControlBtn(bypassLLMButton);

	setSize(audioProcessor.getSavedWindowWidth(), audioProcessor.getSavedWindowHeight());

	uiTrackManager->refreshTrackComponents();

	juce::String json = audioProcessor.getPanelStateJson();
	if (json.isNotEmpty())
	{
		auto state = juce::JSON::parse(json);
		leftPanelWrapper->restoreUIState(state);
	}

	addEventListeners();
}

void DjIaVstEditor::addEventListeners()
{

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

	leftPanelWrapper->getSampleBankPanel()->onSampleDroppedToTrack =
	    [this](const juce::String &sampleId, const juce::String &trackId)
	{
		audioProcessor.getAudioManager().loadSampleFromBank(sampleId, trackId);
		uiStatusManager->setStatusWithTimeout("Sample loaded from bank: " + sampleId.substring(0, 8) + "...", 3000);
	};

	openMidiEditorButton.onClick = [this] { uiModalManager->openMidiMappingEditor(); };

	helpButton.onClick = [this]() { uiModalManager->showOnboardingStep(1); };
}

void DjIaVstEditor::updateUIFromProcessor()
{

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
