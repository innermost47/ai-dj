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
	setSize(Obsidian::BASE_PLUGIN_WIDTH, Obsidian::BASE_PLUGIN_HEIGHT);
	setResizable(true, true);
	setResizeLimits(Obsidian::MIN_PLUGIN_WIDTH, Obsidian::MIN_PLUGIN_HEIGHT, Obsidian::MAX_PLUGIN_WIDTH,
	                Obsidian::MAX_PLUGIN_HEIGHT);
	getConstrainer()->setFixedAspectRatio(Obsidian::ASPECT_RATIO);

	juce::LookAndFeel::setDefaultLookAndFeel(&CustomLookAndFeel::getInstance());
	ObsidianAlertManager::initialize();
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(false);
	setFocusContainerType(FocusContainerType::focusContainer);
	setInterceptsMouseClicks(true, true);
	tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
	uiStatusManager = std::make_unique<UIStatusManager>(*this);
	uiModalManager = std::make_unique<UIModalManager>(*this);
	uiGenerationManager = std::make_unique<UIGenerationManager>(*this, audioProcessor);
	uiTrackManager = std::make_unique<UITrackManager>(*this);
	uiPresetManager = std::make_unique<UIPresetManager>(*this);
	uiMidiManager = std::make_unique<UIMidiManager>(*this);
	mixerPanel = std::make_unique<MixerPanel>(audioProcessor, *this);
	lcdScreen = std::make_unique<LCDScreen>();
	masterWaveformDisplay = std::make_unique<MasterWaveformDisplay>();

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
		waitingForState = true;

	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });

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
	vBlankAttachment.reset();
	if (uiLayoutManager)
	{
		juce::DynamicObject::Ptr root = new juce::DynamicObject();

		if (uiLayoutManager->getLeftPanelWrapper())
			root->setProperty("leftPanel", uiLayoutManager->getLeftPanelWrapper()->saveUIState());

		if (uiLayoutManager->getRightPanelWrapper())
			root->setProperty("rightPanel", uiLayoutManager->getRightPanelWrapper()->saveUIState());

		audioProcessor.setPanelStateJson(juce::JSON::toString(juce::var(root.get())));
	}
	audioProcessor.onMasterOutput = nullptr;
	audioProcessor.setMidiIndicatorCallback(nullptr);
	audioProcessor.onUIUpdateNeeded = nullptr;
	audioProcessor.setGenerationListener(nullptr);
	uiModalManager->clearAll();
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

	setupScreen();
	canPersistSize.store(true);
	uiTrackManager->refreshTracks();
	creditsLabel.setText("Local Edition", juce::dontSendNotification);
	if (uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel())
		uiLayoutManager->getLeftPanelWrapper()->getPromptBankPanel()->refreshList();

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

	if (!uiLayoutManager)
		uiLayoutManager = std::make_unique<UILayoutManager>(audioProcessor, *this, *mixerPanel);
	lcdScreen->setTwoLineMode(true);
	setupUI();
	uiTrackManager->refreshTrackComponents();
	uiTrackManager->refreshUIForMode();
	juce::WeakReference<DjIaVstEditor> weakThis(this);
	juce::Timer::callAfterDelay(500,
	                            [weakThis]()
	                            {
		                            if (weakThis == nullptr)
			                            return;

		                            weakThis->uiModalManager->showOnboardingTour();
	                            });
	isInitialized.store(true);

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

void DjIaVstEditor::handleVBlank()
{
	if (audioProcessor.stateJustLoaded.exchange(false))
		if (isInitialized.load())
			updateUIFromProcessor();

	if (audioProcessor.needsUIUpdate.exchange(false))
		if (audioProcessor.onUIUpdateNeeded)
			audioProcessor.onUIUpdateNeeded();

	if (waitingForState)
	{
		if (!audioProcessor.isStateReady())
			return;
		waitingForState = false;
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

	if (!isInitialized.load())
		return;

	bool anyTrackPlaying = false;
	for (auto &trackComp : uiTrackManager->getTrackComponents())
	{
		if (trackComp->isShowing())
		{
			TrackData *track = trackComp->getTrack();
			if (track && track->isPlaying.load())
			{
				trackComp->updateFromTrackData();
				anyTrackPlaying = true;
			}
		}
	}
	if (!anyTrackPlaying)
	{
		skipFrames++;
		if (skipFrames < 10)
			return;
		skipFrames = 0;
	}

	double currentHostBpm = audioProcessor.getHostBpm();
	if (std::abs(currentHostBpm - lastHostBpm) > 0.1)
	{
		lastHostBpm = currentHostBpm;
		for (auto &trackComp : uiTrackManager->getTrackComponents())
		{
			TrackData *track = trackComp->getTrack();
			if (track)
				trackComp->updateWaveformWithTimeStretch();
		}
	}
}

static int maxWidthForScreen()
{
	int maxW = Obsidian::MAX_PLUGIN_WIDTH;
	if (auto *display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
	{
		auto area = display->userArea;
		float s = juce::jmin(area.getWidth() / (float)Obsidian::BASE_PLUGIN_WIDTH,
		                     area.getHeight() / (float)Obsidian::BASE_PLUGIN_HEIGHT);
		maxW = juce::jlimit(Obsidian::MIN_PLUGIN_WIDTH, Obsidian::MAX_PLUGIN_WIDTH,
		                    (int)(Obsidian::BASE_PLUGIN_WIDTH * s));
	}
	return maxW;
}

void DjIaVstEditor::setupScreen()
{
	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		const int w = maxWidthForScreen();
		setSize(w, Obsidian::heightForWidth(w));
	}
	else
	{
		const int screenMaxW = juce::jmax(Obsidian::MIN_PLUGIN_WIDTH, (int)(maxWidthForScreen() * 0.92f));
		int w = audioProcessor.getSavedWindowWidth();
		if (w < Obsidian::MIN_PLUGIN_WIDTH || w > Obsidian::MAX_PLUGIN_WIDTH)
			w = Obsidian::BASE_PLUGIN_WIDTH;
		w = juce::jmin(w, screenMaxW);
		setSize(w, Obsidian::heightForWidth(w));
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

	uiTrackManager->refreshTrackComponents();

	juce::String json = audioProcessor.getPanelStateJson();
	if (json.isNotEmpty())
	{
		auto state = juce::JSON::parse(json);

		if (auto *o = state.getDynamicObject())
		{
			if (o->hasProperty("leftPanel"))
				uiLayoutManager->getLeftPanelWrapper()->restoreUIState(o->getProperty("leftPanel"));
			else
				uiLayoutManager->getLeftPanelWrapper()->restoreUIState(state);

			if (o->hasProperty("rightPanel"))
				uiLayoutManager->getRightPanelWrapper()->restoreUIState(o->getProperty("rightPanel"));
		}
	}

	addEventListeners();
	setupScreen();
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
	if (!isInitialized.load() || !uiLayoutManager)
		return;
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
		juce::Timer::callAfterDelay(50,
		                            [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		                            {
			                            if (safeThis != nullptr)
				                            if (safeThis->uiLayoutManager)
					                            safeThis->uiTrackManager->refreshTrackComponents();
		                            });
}

void DjIaVstEditor::resized()
{
	if (!uiLayoutManager)
		return;
	const float scale = getWidth() / (float)Obsidian::BASE_PLUGIN_WIDTH;
	auto &root = uiLayoutManager->getContentRoot();
	root.setTransform(juce::AffineTransform::scale(scale));
	root.setBounds(0, 0, Obsidian::BASE_PLUGIN_WIDTH, Obsidian::BASE_PLUGIN_HEIGHT);
	if (uiModalManager)
		uiModalManager->applyScale(scale);
	if (canPersistSize.load())
		audioProcessor.setWindowSize(getWidth(), getHeight());
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
		mixerPanel->refreshAllChannels();
}
