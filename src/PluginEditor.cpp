#include "PluginEditor.h"
#include "BinaryData.h"
#include "ColourPalette.h"
#include "ConfigComponent.h"
#include "LeftPanelWrapper.h"
#include "ObsidianAlertManager.h"
#include "PluginProcessor.h"
#include "RightPanelWrapper.h"
#include "SequencerComponent.h"
#include "StandaloneTransport.h"
#include "StandaloneTransportComponent.h"

DjIaVstEditor::DjIaVstEditor(DjIaVstProcessor &p) : AudioProcessorEditor(&p), audioProcessor(p)
{
	setResizable(true, true);
	setResizeLimits(1100, 820, 2400, 1600);
	setSize(1620, 840);
	setScaleFactor(1.0f);
	juce::LookAndFeel::setDefaultLookAndFeel(&CustomLookAndFeel::getInstance());
	ObsidianAlertManager::initialize();
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(false);
	setFocusContainerType(FocusContainerType::focusContainer);
	setInterceptsMouseClicks(true, true);
	tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
	uiStatusManager = std::make_unique<UIStatusManager>(*this);
	uiModalManager = std::make_unique<UIModalManager>(*this);
	uiGenerationManager = std::make_unique<UIGenerationManager>(*this);
	uiTrackManager = std::make_unique<UITrackManager>(*this);
	uiPresetManager = std::make_unique<UIPresetManager>(*this);
	uiMidiManager = std::make_unique<UIMidiManager>(*this);
	mixerPanel = std::make_unique<MixerPanel>(audioProcessor);
	lcdScreen = std::make_unique<LCDScreen>();
	masterWaveformDisplay = std::make_unique<MasterWaveformDisplay>();
	uiLayoutManager = std::make_unique<UILayoutManager>(audioProcessor, *this, *mixerPanel);
	lcdScreen->setTwoLineMode(true);

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
	auto state = uiLayoutManager->getLeftPanelWrapper()->saveUIState();
	audioProcessor.setPanelStateJson(juce::JSON::toString(state));
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
	mixerPanel = nullptr;
	lcdScreen = nullptr;
	masterWaveformDisplay = nullptr;

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
		window->setTitleBarButtonsRequired(juce::DocumentWindow::allButtons, false);

		static bool isFullscreen = false;
		if (!isFullscreen && juce::JUCEApplication::isStandaloneApp())
		{
			window->setFullScreen(true);
			isFullscreen = true;
		}
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
			if (track)
				trackComp->updateWaveformWithTimeStretch();
		}
	}
}

void DjIaVstEditor::setupUI()
{

	if (juce::JUCEApplicationBase::isStandaloneApp())
		if (audioProcessor.getStandaloneTransport())
			uiLayoutManager->getRightPanelWrapper()->setStandaloneTransport(audioProcessor.getStandaloneTransport());

	statusLabel.setColour(juce::Label::backgroundColourId, ColourPalette::backgroundDeep);
	statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);

	creditsLabel.setText("Loading...", juce::dontSendNotification);

	addAndMakeVisible(lcdScreen.get());

	addAndMakeVisible(masterWaveformDisplay.get());
	uiLayoutManager->getRightPanelWrapper()->setMasterWaveform(masterWaveformDisplay.get());
	uiLayoutManager->getRightPanelWrapper()->setLCDScreen(lcdScreen.get());
	audioProcessor.onMasterOutput = [this](const float *l, const float *r, int n, double ppq)
	{
		if (masterWaveformDisplay)
		{
			masterWaveformDisplay->pushSamples(l, r, n);
			masterWaveformDisplay->setPositionInBeats(ppq);
		}
	};

	setSize(audioProcessor.getSavedWindowWidth(), audioProcessor.getSavedWindowHeight());

	uiTrackManager->refreshTrackComponents();

	juce::String json = audioProcessor.getPanelStateJson();
	if (json.isNotEmpty())
	{
		auto state = juce::JSON::parse(json);
		uiLayoutManager->getLeftPanelWrapper()->restoreUIState(state);
	}

	addEventListeners();
}

void DjIaVstEditor::addEventListeners()
{

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

	uiLayoutManager->getLeftPanelWrapper()->getSampleBankPanel()->onSampleDroppedToTrack =
	    [this](const juce::String &sampleId, const juce::String &trackId)
	{
		audioProcessor.getAudioManager().loadSampleFromBank(sampleId, trackId);
		uiStatusManager->setStatusWithTimeout("Sample loaded from bank: " + sampleId.substring(0, 8) + "...", 3000);
	};

	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (audioProcessor.getStandaloneTransport())
		{
			uiLayoutManager->getRightPanelWrapper()->getStandaloneTransportComponent()->onTimeSignatureChanged =
			    [this]() { uiTrackManager->refreshTrackComponents(); };
			uiLayoutManager->getRightPanelWrapper()->getStandaloneTransportComponent()->onBpmChanged =
			    [this](double /*value*/) { uiTrackManager->refreshTrackComponents(); };
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

void DjIaVstEditor::updateUIFromProcessor()
{
	uiLayoutManager->getRightPanelWrapper()->getConfigComponent()->updateFromProcessor();
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
