#include "./PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include "SequencerComponent.h"
#include "version.h"
#include "ColourPalette.h"
#if JUCE_WINDOWS
#include <windows.h>
#include <winuser.h>
#endif

DjIaVstEditor::DjIaVstEditor(DjIaVstProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	setResizable(false, false);
	setSize(1300, 800);
	setScaleFactor(1.0f);
	setLookAndFeel(&customLookAndFeel);
	setWantsKeyboardFocus(true);
	setMouseClickGrabsKeyboardFocus(false);
	setFocusContainerType(FocusContainerType::focusContainer);
	setInterceptsMouseClicks(true, true);
	tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
	logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_png,
		BinaryData::logo_pngSize);
	audioProcessor.setGenerationListener(this);
	if (audioProcessor.isStateReady())
	{
		initUI();
	}
	else
		startTimer(50);

	juce::Timer::callAfterDelay(300, [this]()
		{
			loadPromptPresets();
			refreshTracks();
			refreshWavevormsAndSequencers();
			refreshCreditsAsync();
			if (audioProcessor.getIsGenerating())
			{
				generateButton.setEnabled(false);
				setAllGenerateButtonsEnabled(false);
				statusLabel.setText("Generation in progress...", juce::dontSendNotification);
				juce::String generatingId = audioProcessor.getGeneratingTrackId();
				for (auto& trackComp : trackComponents)
				{
					if (trackComp->getTrackId() == generatingId)
					{
						trackComp->startGeneratingAnimation();
						break;
					}
				}
			} });
			juce::Timer::callAfterDelay(4000, [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
				{
					if (auto* editor = safeThis.getComponent())
					{
						if (!editor->audioProcessor.updateCheckDone)
						{
							editor->audioProcessor.updateCheckDone = true;
							editor->checkForUpdates();
						}
					}
				});
}

DjIaVstEditor::~DjIaVstEditor()
{
	audioProcessor.onUIUpdateNeeded = nullptr;
	audioProcessor.setGenerationListener(nullptr);
	audioProcessor.getMidiLearnManager().registerUICallback("promptPresetSelector", nullptr);
	setLookAndFeel(nullptr);
	if (midiEditorWindow != nullptr)
	{
		midiEditorWindow->setVisible(false);
		delete midiEditorWindow;
		midiEditorWindow = nullptr;
	}
}

void DjIaVstEditor::refreshWavevormsAndSequencers()
{
	for (auto& trackComp : trackComponents)
	{
		if (trackComp->getTrack() && trackComp->getTrack()->showWaveform)
		{
			trackComp->toggleWaveformDisplay();
		}
		if (trackComp->getTrack() && trackComp->getTrack()->showSequencer)
		{
			trackComp->toggleSequencerDisplay();
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

void DjIaVstEditor::updateMidiIndicator(const juce::String& noteInfo)
{
	lastMidiNote = noteInfo;
	juce::MessageManager::callAsync([this, noteInfo]()
		{
			if (midiIndicator.isShowing())
			{
				midiIndicator.setText(noteInfo, juce::dontSendNotification);
				midiIndicator.setColour(juce::Label::textColourId, juce::Colours::black);
				auto redWithOpacity = ColourPalette::buttonPrimary.withAlpha(0.3f);
				juce::Timer::callAfterDelay(200, [this]()
					{
						if (midiIndicator.isShowing())
						{
							midiIndicator.setColour(juce::Label::backgroundColourId, juce::Colours::white);
						}
					});
			} });
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
	for (auto& trackComp : trackComponents)
	{
		if (trackComp->isShowing())
		{
			TrackData* track = audioProcessor.getTrack(trackComp->getTrackId());
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
			midiBlinkCounter = 0;
		}
	}

	if (!autoLoadButton.getToggleState())
	{
		updateLoadButtonState();
	}

	for (auto& trackComp : trackComponents)
	{
		TrackData* track = audioProcessor.getTrack(trackComp->getTrackId());
		if (track && track->isPlaying.load() && track->numSamples > 0)
		{
			double startSample = track->loopStart * track->sampleRate;
			double currentTimeInSection = (startSample + track->readPosition.load()) / track->sampleRate;

			trackComp->updatePlaybackPosition(currentTimeInSection);
		}
	}

	static bool currentWasGenerating = false;
	bool isCurrentlyGenerating = generateButton.isEnabled() == false;
	if (currentWasGenerating && !isCurrentlyGenerating)
	{
		for (auto& trackComp : trackComponents)
		{
			trackComp->refreshWaveformIfNeeded();
		}
	}
	currentWasGenerating = isCurrentlyGenerating;
}

void DjIaVstEditor::onGenerationComplete(const juce::String& trackId, const juce::String& message)
{
	bool isError = message.startsWith("ERROR:");
	stopGenerationUI(trackId, !isError, isError ? message : "");

	if (isShowing())
	{
		statusLabel.setText(message, juce::dontSendNotification);

		if (isError)
		{
			statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
			juce::Timer::callAfterDelay(5000, [this]()
				{
					if (isShowing())
					{
						statusLabel.setText("Ready", juce::dontSendNotification);
						statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
					}
				});
		}
		else
		{
			statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
			juce::Timer::callAfterDelay(3000, [this]()
				{
					if (isShowing())
					{
						statusLabel.setText("Ready", juce::dontSendNotification);
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
	updateSelectedTrack();
	repaint();
}

void DjIaVstEditor::initUI()
{
	if (isInitialized.load())
	{
		return;
	}
	setupUI();
	refreshUIForMode();
	serverUrlInput.setText(audioProcessor.getServerUrl(), juce::dontSendNotification);
	apiKeyInput.setText(audioProcessor.getApiKey(), juce::dontSendNotification);
	if (audioProcessor.getServerUrl().isEmpty())
	{
		juce::Timer::callAfterDelay(500, [this]()
			{ showFirstTimeSetup(); });
	}
	isInitialized.store(true);
	juce::WeakReference<DjIaVstEditor> weakThis(this);
	audioProcessor.setMidiIndicatorCallback([weakThis](const juce::String& noteInfo)
		{
			if (weakThis != nullptr) {
				weakThis->updateMidiIndicator(noteInfo);
			} });
			loadPromptPresets();
			refreshTracks();
			audioProcessor.onUIUpdateNeeded = [this]()
				{
					juce::MessageManager::callAsync([this]()
						{ updateUIComponents(); });
				};
}

void DjIaVstEditor::showFirstTimeSetup()
{
	auto alertWindow = std::make_unique<juce::AlertWindow>(
		"OBSIDIAN-Neural Configuration " + Version::FULL,
		"Choose your generation method.\n\n"
		"No API key yet? Get your free account at:\n"
		"obsidian-neural.com\n\n"
		"7-day free trial - 100 credits included - no credit card required.",
		juce::MessageBoxIconType::InfoIcon);

	juce::StringArray modes;
	modes.add("Server/API (Full features + stems separation)");
	modes.add("Local Model (Basic - requires manual setup)");
	alertWindow->addComboBox("generationMode", modes, "Generation Mode:");
	if (auto* modeCombo = alertWindow->getComboBoxComponent("generationMode"))
	{
		modeCombo->setSelectedItemIndex(audioProcessor.getUseLocalModel() ? 1 : 0);
	}

	alertWindow->addTextEditor("serverUrl",
		audioProcessor.getServerUrl().isEmpty() ? "http://localhost:8000" : audioProcessor.getServerUrl(),
		"Server URL:");
	alertWindow->addTextEditor("apiKey", "", "API Key:");
	if (auto* apiKeyEditor = alertWindow->getTextEditor("apiKey"))
	{
		apiKeyEditor->setPasswordCharacter('*');
	}

	juce::StringArray timeouts = { "1 minute", "2 minutes", "5 minutes", "10 minutes", "15 minutes", "20 minutes", "30 minutes", "45 minutes" };
	alertWindow->addComboBox("requestTimeout", timeouts, "Request Timeout:");
	if (auto* timeoutCombo = alertWindow->getComboBoxComponent("requestTimeout"))
	{
		timeoutCombo->setSelectedItemIndex(2);
	}

	alertWindow->addButton("Save & Continue", 1);
	alertWindow->addButton("Skip for now", 0);

	auto* windowPtr = alertWindow.get();
	alertWindow.release()->enterModalState(true, juce::ModalCallbackFunction::create([this, windowPtr](int result)
		{
			if (result == 1) {
				auto* modeCombo = windowPtr->getComboBoxComponent("generationMode");
				auto* urlEditor = windowPtr->getTextEditor("serverUrl");
				auto* keyEditor = windowPtr->getTextEditor("apiKey");
				auto* timeoutCombo = windowPtr->getComboBoxComponent("requestTimeout");

				if (modeCombo && urlEditor && keyEditor && timeoutCombo) {
					bool useLocal = (modeCombo->getSelectedItemIndex() == 1);
					audioProcessor.setUseLocalModel(useLocal);

					if (useLocal) {
						checkLocalModelsAndNotify();
					}
					else {
						audioProcessor.setServerUrl(urlEditor->getText());
						audioProcessor.setApiKey(keyEditor->getText());
					}

					juce::Array<int> timeoutMinutes = { 1, 2, 5, 10, 15, 20, 30, 45 };
					int selectedTimeoutMs = timeoutMinutes[timeoutCombo->getSelectedItemIndex()] * 60 * 1000;
					audioProcessor.setRequestTimeout(selectedTimeoutMs);
					audioProcessor.saveGlobalConfig();

					if (!useLocal) {
						statusLabel.setText("Configuration saved!", juce::dontSendNotification);
					}
					refreshUIForMode();
					juce::Timer::callAfterDelay(400, [this]() { showOnboardingTour(); });
				}
			}
			windowPtr->exitModalState(result);
			delete windowPtr; }));
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
	};

	std::vector<StepInfo> steps = {
		{
			"OBSIDIAN Neural - Step 1 / 3",
			"Welcome! Let's generate your first sound in 3 steps.\n\n"
			"STEP 1: Choose a prompt\n\n"
			"At the top of the interface, you'll see a dropdown\n"
			"and a text field. This is where you describe the sound\n"
			"you want to generate.\n\n"
			"Example: \"Dark techno bassline\", \"Ambient guitar loop\"\n\n"
			"Select a preset from the dropdown, or type your own.",
			"Next ->",
			"Skip tour"
		},
		{
			"OBSIDIAN Neural - Step 2 / 3",
			"STEP 2: Hit Generate\n\n"
			"Once your prompt is ready, click the [>] button\n"
			"to the right of the text field.\n\n"
			"The AI will generate a sample for the selected track.\n"
			"This takes about 10-30 seconds - perfectly normal!\n\n"
			"You'll see a generation animation on the track\n"
			"while it's working.",
			"Next ->",
			"Skip tour"
		},
		{
			"OBSIDIAN Neural - Step 3 / 3",
			"STEP 3: Play it back\n\n"
			"Once generated, a waveform appears on your track.\n\n"
			"To preview instantly:\n"
			"  - Click the Play button on the track (left side)\n\n"
			"To play in sync with your DAW:\n"
			"  - Enable a step in the sequencer on the track\n"
			"  - Click Play on the matching Mixer channel (right panel)\n"
			"  - Press Play in your DAW\n\n"
			"Tip: hover any button to see a tooltip.\n\n"
			"Full setup guide: obsidian-neural.com -> Documentation\n\n"
			"That's it - you're ready to create. Enjoy!",
			"Let's go!",
			"Skip"
		}
	};

	if (step < 1 || step >(int)steps.size())
		return;

	const auto& info = steps[step - 1];
	bool isLastStep = (step == steps.size());

	auto* alertWindow = new juce::AlertWindow(
		info.title,
		info.message,
		juce::MessageBoxIconType::InfoIcon);

	alertWindow->setColour(juce::AlertWindow::backgroundColourId,
		ColourPalette::backgroundDeep);
	alertWindow->setColour(juce::AlertWindow::textColourId,
		ColourPalette::textPrimary);
	alertWindow->setColour(juce::AlertWindow::outlineColourId,
		ColourPalette::textAccent.withAlpha(0.4f));

	alertWindow->addButton(info.buttonNext, 1);
	alertWindow->addButton(info.buttonSkip, 0);

	alertWindow->enterModalState(true,
		juce::ModalCallbackFunction::create([this, alertWindow, step, isLastStep](int result)
			{
				alertWindow->exitModalState(result);
				delete alertWindow;

				if (result == 1 && !isLastStep)
				{
					juce::Timer::callAfterDelay(50, [this, step]()
						{
							showOnboardingStep(step + 1);
						});
				}
				else
				{
					audioProcessor.setOnboardingDone(true);
					audioProcessor.saveGlobalConfig();


					if (isLastStep && result == 1)
					{
						statusLabel.setText(
							juce::String::fromUTF8("Ready - select a prompt and hit [>] to generate your first loop!"),
							juce::dontSendNotification);
						statusLabel.setColour(juce::Label::textColourId,
							ColourPalette::violet);
					}
				}
			}),
		true);
	juce::Timer::callAfterDelay(100, [alertWindow]()
		{
			if (alertWindow != nullptr)
			{
				alertWindow->toFront(true);
				alertWindow->grabKeyboardFocus();
			}
		});
}

void DjIaVstEditor::refreshUIForMode()
{
	bool isLocalMode = audioProcessor.getUseLocalModel();

	durationSlider.setEnabled(!isLocalMode);
	durationLabel.setEnabled(!isLocalMode);

	resized();
}

void DjIaVstEditor::showConfigDialog()
{
	auto alertWindow = std::make_unique<juce::AlertWindow>(
		"OBSIDIAN-Neural Configuration " + Version::FULL,
		"Update your settings:",
		juce::MessageBoxIconType::QuestionIcon);

	juce::StringArray modes;
	modes.add("Server/API (Full features + stems separation)");
	modes.add("Local Model (Basic - requires manual setup)");
	alertWindow->addComboBox("generationMode", modes, "Generation Mode:");
	if (auto* modeCombo = alertWindow->getComboBoxComponent("generationMode"))
	{
		modeCombo->setSelectedItemIndex(audioProcessor.getUseLocalModel() ? 1 : 0);
	}

	alertWindow->addTextEditor("serverUrl", audioProcessor.getServerUrl(), "Server URL:");
	alertWindow->addTextEditor("apiKey", "", "API Key (leave blank to keep current):");
	if (auto* apiKeyEditor = alertWindow->getTextEditor("apiKey"))
	{
		apiKeyEditor->setPasswordCharacter('*');
	}

	juce::StringArray timeouts = { "1 minute", "2 minutes", "5 minutes", "10 minutes", "15 minutes", "20 minutes", "30 minutes", "45 minutes" };
	alertWindow->addComboBox("requestTimeout", timeouts, "Request Timeout:");
	if (auto* timeoutCombo = alertWindow->getComboBoxComponent("requestTimeout"))
	{
		int currentTimeoutMs = audioProcessor.getRequestTimeout();
		int currentTimeoutMinutes = currentTimeoutMs / (60 * 1000);
		juce::Array<int> timeoutValues = { 1, 2, 5, 10, 15, 20, 30, 45 };
		int selectedIndex = 2;
		for (int i = 0; i < timeoutValues.size(); ++i)
		{
			if (timeoutValues[i] == currentTimeoutMinutes)
			{
				selectedIndex = i;
				break;
			}
		}
		timeoutCombo->setSelectedItemIndex(selectedIndex);
	}

	alertWindow->addButton("Update", 1);
	alertWindow->addButton("Cancel", 0);

	auto* windowPtr = alertWindow.get();
	alertWindow.release()->enterModalState(true, juce::ModalCallbackFunction::create([this, windowPtr](int result)
		{
			if (result == 1) {
				auto* modeCombo = windowPtr->getComboBoxComponent("generationMode");
				auto* urlEditor = windowPtr->getTextEditor("serverUrl");
				auto* keyEditor = windowPtr->getTextEditor("apiKey");
				auto* timeoutCombo = windowPtr->getComboBoxComponent("requestTimeout");

				if (modeCombo && urlEditor && keyEditor && timeoutCombo) {
					bool useLocal = (modeCombo->getSelectedItemIndex() == 1);
					bool modeChanged = (useLocal != audioProcessor.getUseLocalModel());

					audioProcessor.setUseLocalModel(useLocal);

					if (useLocal) {
						checkLocalModelsAndNotify();
					}
					else {
						audioProcessor.setServerUrl(urlEditor->getText());
						juce::String newKey = keyEditor->getText();
						if (newKey.isNotEmpty()) {
							audioProcessor.setApiKey(newKey);
						}
					}

					juce::Array<int> timeoutMinutes = { 1, 2, 5, 10, 15, 20, 30, 45 };
					int selectedTimeoutMs = timeoutMinutes[timeoutCombo->getSelectedItemIndex()] * 60 * 1000;
					audioProcessor.setRequestTimeout(selectedTimeoutMs);

					audioProcessor.saveGlobalConfig();

					if (modeChanged) {
						refreshUIForMode();
						statusLabel.setText("Mode changed! Configuration updated.", juce::dontSendNotification);
					}
					else {
						statusLabel.setText("Configuration updated!", juce::dontSendNotification);
					}

					statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);

					juce::Timer::callAfterDelay(3000, [this]() {
						statusLabel.setText("Ready", juce::dontSendNotification);
						statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
						});
				}
			}
			windowPtr->setLookAndFeel(nullptr);
			windowPtr->exitModalState(result);
			delete windowPtr; }));
}

void DjIaVstEditor::checkLocalModelsAndNotify()
{
	auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural");
	auto stableAudioDir = appDataDir.getChildFile("stable-audio");

	StableAudioEngine tempEngine;
	bool modelsPresent = tempEngine.initialize(stableAudioDir.getFullPathName());

	if (modelsPresent)
	{
		statusLabel.setText("Local models found! Configuration saved.", juce::dontSendNotification);
		statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
	}
	else
	{
		juce::AlertWindow::showAsync(
			juce::MessageBoxOptions()
			.withIconType(juce::MessageBoxIconType::InfoIcon)
			.withTitle("Local Models Required")
			.withMessage("Local models not found!\n\n"
				"You need to download and setup the required model files.\n"
				"Please follow the setup instructions on GitHub.\n\n"
				"Expected location: " +
				stableAudioDir.getFullPathName())
			.withButton("Open GitHub Instructions")
			.withButton("OK"),
			[this](int result)
			{
				if (result == 1)
				{
					juce::URL githubUrl("https://github.com/innermost47/ai-dj/blob/main/README.md");
					githubUrl.launchInDefaultBrowser();
				}
			});

		statusLabel.setText("Local mode selected - Models setup required", juce::dontSendNotification);
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
		}
		bool anyTrackPlaying = false;

		for (auto& trackComp : trackComponents)
		{
			if (trackComp->isShowing())
			{
				TrackData* track = audioProcessor.getTrack(trackComp->getTrackId());
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
			for (auto& trackComp : trackComponents)
			{
				TrackData* track = audioProcessor.getTrack(trackComp->getTrackId());
				if (track && (track->timeStretchMode == 3 || track->timeStretchMode == 4))
				{
					trackComp->updateWaveformWithTimeStretch();
				}
			}
		}
	}
	else
	{
		if (isButtonBlinking)
		{
			blinkCounter++;
			if (blinkCounter % 3 == 0)
			{
				auto currentColor = generateButton.findColour(juce::TextButton::buttonColourId);
				bool isWarning = (currentColor == ColourPalette::buttonPrimary);

				generateButton.setColour(juce::TextButton::buttonColourId,
					isWarning ? ColourPalette::buttonSuccess : ColourPalette::buttonPrimary);
			}
		}
	}
}

void DjIaVstEditor::startGenerationButtonAnimation()
{
	if (!isButtonBlinking)
	{
		generateButton.setEnabled(false);
		generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonWarning);
		isButtonBlinking = true;
		blinkCounter = 0;
	}
}

void DjIaVstEditor::stopGenerationButtonAnimation()
{
	if (isButtonBlinking)
	{
		generateButton.setEnabled(true);
		generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
		isButtonBlinking = false;
		generatingTrackId.clear();
	}
}

void DjIaVstEditor::setupUI()
{
	addAndMakeVisible(nextTrackButton);
	nextTrackButton.setButtonText("Next Track");
	nextTrackButton.setTooltip("Select next track (Right-click for MIDI learn)");
	nextTrackButton.setDescription("Next Track");

	addAndMakeVisible(prevTrackButton);
	prevTrackButton.setButtonText("Prev Track");
	prevTrackButton.setTooltip("Select previous track (Right-click for MIDI learn)");
	prevTrackButton.setDescription("Previous Track");

	addAndMakeVisible(pluginNameLabel);
	pluginNameLabel.setText("NEURAL SOUND ENGINE", juce::dontSendNotification);
	pluginNameLabel.setFont(juce::FontOptions("Courier New", 22.0f, juce::Font::bold));
	pluginNameLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	pluginNameLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(developerLabel);
	developerLabel.setText("Developed by InnerMost47", juce::dontSendNotification);
	developerLabel.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::italic));
	developerLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	developerLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(stabilityLabel);
	stabilityLabel.setText("Powered by Stability AI", juce::dontSendNotification);
	stabilityLabel.setFont(juce::FontOptions("Consolas", 11.0f, juce::Font::plain));
	stabilityLabel.setColour(juce::Label::textColourId, ColourPalette::credits);
	stabilityLabel.setJustificationType(juce::Justification::left);

	addAndMakeVisible(promptPresetSelector);

	addAndMakeVisible(savePresetButton);
	savePresetButton.setButtonText(juce::String::fromUTF8("\xE2\x9C\x93"));

	addAndMakeVisible(promptInput);
	promptInput.setMultiLine(false);
	promptInput.setReturnKeyStartsNewLine(false);
	promptInput.setTextToShowWhenEmpty("Enter custom prompt or select preset...", ColourPalette::textSecondary);
	promptInput.setText(audioProcessor.getGlobalPrompt(), juce::dontSendNotification);

	addAndMakeVisible(resetUIButton);
	resetUIButton.setButtonText("Reset UI");
	resetUIButton.setTooltip("Reset UI state if stuck in generation mode");

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

	addAndMakeVisible(durationSlider);
	durationSlider.setRange(2.0, 30.0, 1.0);
	durationSlider.setValue(audioProcessor.getGlobalDuration(), juce::dontSendNotification);
	durationSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::black);
	durationSlider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
	durationSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	durationSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
	durationSlider.setColour(juce::Slider::textBoxBackgroundColourId, ColourPalette::backgroundDark);
	durationSlider.setColour(juce::Slider::textBoxOutlineColourId, ColourPalette::backgroundDark.darker(0.3f).withAlpha(0.3f));
	durationSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
	durationSlider.setTextValueSuffix(" s");
	durationSlider.setDoubleClickReturnValue(true, 6.0);

	addAndMakeVisible(durationLabel);
	durationLabel.setText("Duration", juce::dontSendNotification);

	addAndMakeVisible(generateButton);
	generateButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));

	addAndMakeVisible(configButton);
	configButton.setButtonText(juce::String::fromUTF8("\xE2\x9B\xAD"));
	configButton.setTooltip("Configure settings globally");
	configButton.onClick = [this]()
		{ showConfigDialog(); };

	addAndMakeVisible(creditsLabel);
	creditsLabel.setText("Credits: --", juce::dontSendNotification);
	creditsLabel.setFont(juce::FontOptions(14.0f));
	creditsLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	creditsLabel.setTooltip(
		"Available generation credits\n"
		"\n"
		"Cost per generation:\n"
		"  - 1 credit)"
	);

	addAndMakeVisible(statusLabel);
	statusLabel.setText("Ready", juce::dontSendNotification);
	statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);

	addAndMakeVisible(autoLoadButton);
	autoLoadButton.setButtonText("Auto-Load Mode");
	autoLoadButton.setClickingTogglesState(true);
	autoLoadButton.setToggleState(audioProcessor.getAutoLoadEnabled(), juce::dontSendNotification);
	autoLoadButton.setTooltip("Toggle between automatic and manual sample loading");
	autoLoadButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonWarning.darker(0.3f));

	addAndMakeVisible(loadSampleButton);
	loadSampleButton.setButtonText("Load Sample");
	loadSampleButton.setEnabled(!audioProcessor.getAutoLoadEnabled());

	addAndMakeVisible(midiIndicator);
	midiIndicator.setText("MIDI: Waiting...", juce::dontSendNotification);
	midiIndicator.setColour(juce::Label::backgroundColourId, juce::Colours::white);
	midiIndicator.setColour(juce::Label::textColourId, juce::Colours::black);
	midiIndicator.setJustificationType(juce::Justification::left);
	midiIndicator.setFont(juce::FontOptions(12.0f, juce::Font::bold));

	addAndMakeVisible(tracksLabel);
	tracksLabel.setText("Tracks:", juce::dontSendNotification);
	tracksLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

	addAndMakeVisible(addTrackButton);
	addTrackButton.setButtonText("+ Add Track");

	addAndMakeVisible(tracksViewport);
	tracksViewport.setViewedComponent(&tracksContainer, false);
	tracksViewport.setScrollBarsShown(true, false);
	tracksContainer.setWantsKeyboardFocus(false);
	tracksViewport.setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(true);

	if (!mixerPanel)
	{
		mixerPanel = std::make_unique<MixerPanel>(audioProcessor);
		addAndMakeVisible(*mixerPanel);
	}

	addAndMakeVisible(showSampleBankButton);
	showSampleBankButton.setButtonText("Bank");
	showSampleBankButton.setTooltip("Show/hide sample bank");

	statusLabel.setColour(juce::Label::backgroundColourId, ColourPalette::backgroundDeep);
	statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);

	addAndMakeVisible(bypassSequencerButton);
	bypassSequencerButton.setButtonText("Sequencer Mode");
	bypassSequencerButton.setClickingTogglesState(true);
	bypassSequencerButton.setToggleState(audioProcessor.getBypassSequencer(), juce::dontSendNotification);
	bypassSequencerButton.setTooltip("Global bypass - direct MIDI playback for composition mode");
	bypassSequencerButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);

	promptPresetSelector.setTooltip("Select a preset prompt (Right-click for MIDI learn, Ctrl+Right-click to edit custom prompts)");
	promptInput.setTooltip("Enter your custom prompt for audio generation");
	savePresetButton.setTooltip("Save current prompt as custom preset");
	keySelector.setTooltip("Select musical key and mode for generation");
	durationSlider.setTooltip("Generation duration in seconds (2-30s) - (unavailable with local TFLite models)");
	generateButton.setTooltip("Generate audio loop for selected track (Right-click for MIDI learn)");
	configButton.setTooltip("Configure API settings and generation mode");
	autoLoadButton.setTooltip("Automatically load generated samples (disable for manual control)");
	loadSampleButton.setTooltip("Manually load pending generated sample");
	addTrackButton.setTooltip("Add a new track to the session");
	resetUIButton.setTooltip("Reset UI if stuck in generation mode");

	if (!sampleBankPanel) {
		sampleBankPanel = std::make_unique<SampleBankPanel>(audioProcessor);
		addChildComponent(*sampleBankPanel);
		sampleBankPanel->setVisible(false);
	}

	openMidiEditorButton.setButtonText("Edit Mappings");
	openMidiEditorButton.setTooltip("Open MIDI mappings editor");
	addAndMakeVisible(openMidiEditorButton);

	refreshTrackComponents();
	addEventListeners();
}

void DjIaVstEditor::addEventListeners()
{
	addTrackButton.onClick = [this]()
		{ onAddTrack(); };
	autoLoadButton.onClick = [this]
		{ onAutoLoadToggled(); };
	loadSampleButton.onClick = [this]
		{ onLoadSampleClicked(); };
	savePresetButton.onClick = [this]
		{ onSavePreset(); };
	promptPresetSelector.onChange = [this]
		{ onPresetSelected(); };
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

	durationSlider.onValueChange = [this]()
		{
			audioProcessor.setLastDuration(durationSlider.getValue());
			audioProcessor.setGlobalDuration((int)durationSlider.getValue());
		};

	promptPresetSelector.onChange = [this]()
		{
			onPresetSelected();
			audioProcessor.setLastPresetIndex(promptPresetSelector.getSelectedId() - 1);
		};

	resetUIButton.onClick = [this]()
		{
			audioProcessor.setIsGenerating(false);
			audioProcessor.setGeneratingTrackId("");
			generateButton.setEnabled(true);
			setAllGenerateButtonsEnabled(true);
			toggleWaveFormButtonOnTrack();
			toggleSEQButtonOnTrack();
			statusLabel.setText("UI Reset - Ready", juce::dontSendNotification);
			for (auto& trackComp : trackComponents)
			{
				trackComp->stopGeneratingAnimation();
			}
			refreshTracks();
		};

	promptPresetSelector.onMidiLearn = [this]()
		{
			statusLabel.setText("Learning MIDI for prompt selector...", juce::dontSendNotification);
			audioProcessor.getMidiLearnManager().startLearning(
				"promptPresetSelector",
				&audioProcessor,
				[this](float value)
				{
					juce::MessageManager::callAsync([this, value]()
						{
							int numItems = promptPresetSelector.getNumItems();
							if (numItems > 0) {
								int selectedIndex = (int)(value * (numItems - 1));
								promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
							} });
				},
				"Prompt Preset Selector", &promptPresetSelector);
		};

	promptPresetSelector.onMidiRemove = [this]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter("promptPresetSelector");
		};

	audioProcessor.getMidiLearnManager().registerUICallback("promptPresetSelector",
		[this](float value)
		{
			juce::MessageManager::callAsync([this, value]()
				{
					int numItems = promptPresetSelector.getNumItems();
					if (numItems > 0) {
						int selectedIndex = (int)(value * (numItems - 1));
						promptPresetSelector.setSelectedItemIndex(selectedIndex, juce::sendNotification);
					} });
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
				statusLabel.setText("Composition mode - Direct MIDI playback", juce::dontSendNotification);
				bypassSequencerButton.setColour(juce::TextButton::buttonColourId,
					ColourPalette::buttonWarning.darker(0.3f));
			}
			else
			{
				bypassSequencerButton.setButtonText("Sequencer Mode");
				statusLabel.setText("Sequencer mode - Armed playback", juce::dontSendNotification);
				bypassSequencerButton.setColour(juce::TextButton::buttonColourId,
					ColourPalette::buttonPrimary);
			}
		};

	nextTrackButton.onMidiLearn = [this]()
		{
			statusLabel.setText("Learning MIDI for next track button...", juce::dontSendNotification);
			audioProcessor.getMidiLearnManager().startLearning(
				"nextTrack",
				&audioProcessor,
				nullptr,
				"Next Track", &nextTrackButton);
		};

	nextTrackButton.onMidiRemove = [this]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter("nextTrack");
		};

	nextTrackButton.onClick = [this]()
		{
			audioProcessor.selectNextTrack();
		};

	prevTrackButton.onMidiLearn = [this]()
		{
			statusLabel.setText("Learning MIDI for previous track button...", juce::dontSendNotification);
			audioProcessor.getMidiLearnManager().startLearning(
				"prevTrack",
				&audioProcessor,
				nullptr,
				"Previous Track", &prevTrackButton);
		};

	prevTrackButton.onMidiRemove = [this]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter("prevTrack");
		};

	prevTrackButton.onClick = [this]()
		{
			audioProcessor.selectPreviousTrack();
		};

	generateButton.onMidiLearn = [this]()
		{
			statusLabel.setText("Learning MIDI for generate button...", juce::dontSendNotification);
			audioProcessor.getMidiLearnManager().startLearning(
				"generate",
				&audioProcessor,
				nullptr,
				"Generate Loop", &generateButton);
		};

	generateButton.onMidiRemove = [this]()
		{
			audioProcessor.getMidiLearnManager().removeMappingForParameter("generate");
		};

	generateButton.onClick = [this]()
		{
			onGenerateButtonClicked();
		};

	showSampleBankButton.onClick = [this]()
		{ toggleSampleBank(); };

	sampleBankPanel->onSampleDroppedToTrack = [this](const juce::String& sampleId, const juce::String& trackId)
		{
			audioProcessor.loadSampleFromBank(sampleId, trackId);
			setStatusWithTimeout("Sample loaded from bank: " + sampleId.substring(0, 8) + "...", 3000);
		};

	openMidiEditorButton.onClick = [this] { openMidiMappingEditor(); };
}

void DjIaVstEditor::notifyTracksPromptUpdate()
{
	juce::StringArray allPrompts = promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();

	for (const auto& customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);
	for (auto& trackComp : trackComponents)
	{
		trackComp->updatePromptPresets(allPrompts);
	}
}

void DjIaVstEditor::mouseDown(const juce::MouseEvent& event)
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

			menu.showMenuAsync(juce::PopupMenu::Options(), [this, selectedPrompt](int result)
				{
					if (result == 1) {
						editCustomPromptDialog(selectedPrompt);
					}
					else if (result == 2) {
						juce::MessageManager::callAsync([this, selectedPrompt]()
							{
								juce::AlertWindow::showAsync(
									juce::MessageBoxOptions()
									.withIconType(juce::MessageBoxIconType::WarningIcon)
									.withTitle("Delete Custom Prompt")
									.withMessage("Are you sure you want to delete this prompt?\n\n'" + selectedPrompt + "'")
									.withButton("Delete")
									.withButton("Cancel"),
									[this, selectedPrompt](int result)
									{
										if (result == 1)
										{
											audioProcessor.removeCustomPrompt(selectedPrompt);
											promptPresets.removeString(selectedPrompt);
											audioProcessor.setLastPresetIndex(audioProcessor.getLastPresetIndex() - 1);
											loadPromptPresets();
											notifyTracksPromptUpdate();
										}
									});
							});

					} });
		}
	}
}

void DjIaVstEditor::editCustomPromptDialog(const juce::String& selectedPrompt)
{
	auto alertWindow = std::make_unique<juce::AlertWindow>(
		"Edit Custom Prompt",
		"Edit your prompt:",
		juce::MessageBoxIconType::InfoIcon);

	alertWindow->addTextEditor("promptText", selectedPrompt, "Prompt text:");
	alertWindow->addButton("Save", 1);
	alertWindow->addButton("Cancel", 0);

	auto* windowPtr = alertWindow.get();
	alertWindow.release()->enterModalState(true,
		juce::ModalCallbackFunction::create([this, windowPtr, selectedPrompt](int result)
			{
				if (result == 1) {
					auto* promptEditor = windowPtr->getTextEditor("promptText");
					if (promptEditor) {
						juce::String newPrompt = promptEditor->getText();
						if (!newPrompt.isEmpty()) {
							audioProcessor.editCustomPrompt(selectedPrompt, newPrompt);
							int index = promptPresets.indexOf(selectedPrompt);
							if (index >= 0) {
								promptPresets.set(index, newPrompt);
							}
							loadPromptPresets();
						}
					}
				}
				windowPtr->exitModalState(result);
				delete windowPtr; }));
}

void DjIaVstEditor::updateUIFromProcessor()
{
	serverUrlInput.setText(audioProcessor.getServerUrl(), juce::dontSendNotification);
	apiKeyInput.setText(audioProcessor.getApiKey(), juce::dontSendNotification);

	promptInput.setText(audioProcessor.getGlobalPrompt(), juce::dontSendNotification);
	durationSlider.setValue(audioProcessor.getGlobalDuration(), juce::dontSendNotification);

	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);

	bool autoLoadOn = audioProcessor.getAutoLoadEnabled();
	autoLoadButton.setToggleState(autoLoadOn, juce::dontSendNotification);
	loadSampleButton.setEnabled(!autoLoadOn);

	if (autoLoadOn)
	{
		autoLoadButton.setButtonText("Auto-Load Mode");
		autoLoadButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonWarning.darker(0.3f));
	}
	else
	{
		autoLoadButton.setButtonText("Manual Mode");
		autoLoadButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonPrimary);
	}

	bool bypassOn = audioProcessor.getBypassSequencer();
	bypassSequencerButton.setToggleState(bypassOn, juce::dontSendNotification);

	if (bypassOn)
	{
		bypassSequencerButton.setButtonText("Composition Mode");
		bypassSequencerButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonWarning.darker(0.3f));
	}
	else
	{
		bypassSequencerButton.setButtonText("Sequencer Mode");
		bypassSequencerButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonPrimary);
	}

	int presetIndex = audioProcessor.getLastPresetIndex();
	if (presetIndex >= 0 && presetIndex < promptPresets.size())
	{
		promptPresetSelector.setSelectedId(presetIndex + 1, juce::dontSendNotification);
	}
	else
	{
		promptPresetSelector.setSelectedId(1, juce::dontSendNotification);
	}

	refreshTrackComponents();
}

void DjIaVstEditor::paint(juce::Graphics& g)
{
	g.fillAll(ColourPalette::backgroundDeep);

	if (logoImage.isValid())
	{
		auto logoArea = juce::Rectangle<int>(0, 12, 100, 60);
		g.drawImage(logoImage, logoArea.toFloat(),
			juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
	}
}

void DjIaVstEditor::layoutPromptSection(juce::Rectangle<int> area, int spacing)
{
	auto row1 = area.removeFromTop(35);
	int saveButtonWidth = 50;
	promptPresetSelector.setBounds(row1.removeFromLeft(area.getWidth() - saveButtonWidth - spacing));
	row1.removeFromLeft(spacing);
	savePresetButton.setBounds(row1.removeFromLeft(saveButtonWidth));

	area.removeFromTop(spacing);

	auto row2 = area.removeFromTop(35);
	int generateButtonWidth = 50;
	promptInput.setBounds(row2.removeFromLeft(row2.getWidth() - generateButtonWidth - spacing));
	row2.removeFromLeft(spacing);
	generateButton.setBounds(row2);
}

void DjIaVstEditor::layoutConfigSection(juce::Rectangle<int> area, int reducing)
{
	auto creditsRow = area.removeFromTop(35);
	creditsLabel.setBounds(creditsRow.reduced(reducing));
	auto controlRow = area.removeFromTop(40);
	auto controlWidth = controlRow.getWidth() / 2;
	keySelector.setBounds(controlRow.removeFromLeft(controlWidth).reduced(reducing));
	durationSlider.setBounds(controlRow.removeFromLeft(controlWidth).reduced(reducing));
}

void DjIaVstEditor::resized()
{
	static bool resizing = false;
	const int spacing = 5;
	const int padding = 10;
	const int reducing = 2;
	if (resizing)
		return;
	resizing = true;
	auto bottomArea = getLocalBounds().removeFromBottom(45);
	auto area = getLocalBounds();

	area = area.reduced(padding);

	auto topArea = area.removeFromTop(80);
	this->bannerArea = topArea;

	auto column1 = topArea.removeFromLeft(330);

	auto logoSpace = column1.removeFromLeft(80);

	auto nameArea = column1.removeFromLeft(250);
	auto titleArea = nameArea.removeFromTop(30);
	auto devArea = nameArea.removeFromTop(10);
	auto partnerArea = nameArea.removeFromTop(25);
	pluginNameLabel.setBounds(titleArea);
	developerLabel.setBounds(devArea);
	stabilityLabel.setBounds(partnerArea);

	topArea.removeFromLeft(spacing * 2);

	auto column2 = topArea.removeFromLeft(490);
	layoutPromptSection(column2, spacing);

	topArea.removeFromLeft(spacing * 2);

	auto column3 = topArea.removeFromLeft(370);
	layoutConfigSection(column3, reducing);

	topArea.removeFromLeft(spacing * 2);

	auto column4 = topArea;

	auto configButtonArea = column4.removeFromTop(35);
	configButton.setBounds(configButtonArea.reduced(2));

	area.removeFromTop(spacing);

	auto tracksAndMixerArea = area.removeFromTop(area.getHeight() - 70);
	int tracksWidth = static_cast<int>(tracksAndMixerArea.getWidth() * 0.6f);
	auto tracksMainArea = tracksAndMixerArea.removeFromLeft(tracksWidth);
	tracksViewport.setBounds(tracksMainArea);
	tracksAndMixerArea.removeFromLeft(5);

	if (mixerPanel)
	{
		mixerPanel->setBounds(tracksAndMixerArea);
		mixerPanel->setVisible(true);
	}

	auto buttonsRow = area.removeFromTop(40);
	auto buttonWidth = buttonsRow.getWidth() / 9;
	bypassSequencerButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	autoLoadButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	addTrackButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	loadSampleButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	showSampleBankButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	openMidiEditorButton.setBounds(buttonsRow.removeFromLeft(buttonWidth).reduced(5));
	auto trackNavArea = buttonsRow.removeFromRight(buttonWidth * 2);
	prevTrackButton.setBounds(trackNavArea.removeFromLeft(buttonWidth).reduced(5));
	nextTrackButton.setBounds(trackNavArea.reduced(5));
	resetUIButton.setBounds(buttonsRow.reduced(5));

	bottomArea.removeFromTop(spacing);

	auto statusRow = bottomArea.removeFromTop(20);
	statusRow.removeFromLeft(10);
	statusLabel.setBounds(statusRow);

	auto midiRow = bottomArea.removeFromTop(20);
	midiRow.removeFromLeft(10);
	midiRow.removeFromRight(10);
	midiIndicator.setBounds(midiRow);

	if (sampleBankPanel && sampleBankVisible)
	{
		auto bankArea = getLocalBounds().removeFromRight(400);
		sampleBankPanel->setBounds(bankArea);
	}
	resizing = false;
}

void DjIaVstEditor::openMidiMappingEditor()
{
	if (midiEditorWindow != nullptr)
	{
		midiEditorWindow->toFront(true);
		return;
	}

	midiEditorWindow = new MidiMappingEditorWindow(&audioProcessor.getMidiLearnManager());

	midiEditorWindow->onWindowClosed = [this]()
		{
			midiEditorWindow = nullptr;
		};

	midiEditorWindow->centreAroundComponent(this,
		midiEditorWindow->getWidth(),
		midiEditorWindow->getHeight());
}


void DjIaVstEditor::setAllGenerateButtonsEnabled(bool enabled)
{
	for (auto& trackComp : trackComponents)
	{
		trackComp->setGenerateButtonEnabled(enabled);
		trackComp->setCanvasGenerating(!enabled);
	}
}

void DjIaVstEditor::toggleSampleBank()
{
	sampleBankVisible = !sampleBankVisible;
	sampleBankPanel->setVisible(sampleBankVisible);

	if (sampleBankVisible)
	{
		showSampleBankButton.setButtonText("Hide Bank");
		setStatusWithTimeout("Sample bank opened", 2000);
	}
	else
	{
		showSampleBankButton.setButtonText("Bank");
		setStatusWithTimeout("Sample bank closed", 2000);
	}

	resized();
}

void DjIaVstEditor::startGenerationUI(const juce::String& trackId)
{
	generateButton.setEnabled(false);
	setAllGenerateButtonsEnabled(false);
	statusLabel.setText("Connecting to server...", juce::dontSendNotification);

	for (auto& trackComp : trackComponents)
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

	juce::Timer::callAfterDelay(100, [this, trackId]()
		{
			if (audioProcessor.getIsGenerating() &&
				audioProcessor.getGeneratingTrackId() == trackId)
			{
				statusLabel.setText("Generating loop (this may take a few minutes)...",
					juce::dontSendNotification);
			}
		});
}

void DjIaVstEditor::stopGenerationUI(const juce::String& trackId, bool success, const juce::String& errorMessage)
{
	generateButton.setEnabled(true);
	setAllGenerateButtonsEnabled(true);

	for (auto& trackComp : trackComponents)
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
				}
				else
				{
					statusLabel.setText("Sample ready - Click 'Load Sample' to use it", juce::dontSendNotification);
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
	}
}

void DjIaVstEditor::onSampleLoaded(const juce::String& trackId)
{
	for (auto& trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			trackComp->setSamplePending(false);
			trackComp->updateFromTrackData();
			trackComp->repaint();
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
		return;
	}
	bool isLocalServer = serverUrl.contains("localhost") ||
		serverUrl.contains("127.0.0.1");
	if (apiKey.isEmpty() && !isLocalServer)
	{
		statusLabel.setText("Error: API Key is required", juce::dontSendNotification);
		return;
	}
	juce::String currentPrompt = promptInput.getText().trim();
	if (currentPrompt.isEmpty())
	{
		statusLabel.setText("Error: Prompt cannot be empty", juce::dontSendNotification);
		statusLabel.setColour(juce::Label::textColourId, ColourPalette::textDanger);
		return;
	}
	audioProcessor.syncSelectedTrackWithGlobalPrompt();
	audioProcessor.setIsGenerating(true);
	generatingTrackId = audioProcessor.getSelectedTrackId();
	audioProcessor.setGeneratingTrackId(generatingTrackId);
	TrackData* track = audioProcessor.trackManager.getTrack(generatingTrackId);

	if (!track)
	{
		statusLabel.setText("Error: No track selected", juce::dontSendNotification);
		return;
	}

	if (track->usePages.load())
	{
		auto& currentPage = track->getCurrentPage();
		currentPage.selectedPrompt = promptInput.getText();
		currentPage.generationPrompt = promptInput.getText();
		currentPage.generationBpm = (float)audioProcessor.getHostBpm();
		currentPage.generationKey = keySelector.getText();
		currentPage.generationDuration = (int)durationSlider.getValue();
		track->syncLegacyProperties();
	}
	else
	{
		track->generationPrompt = promptInput.getText();
		track->generationBpm = (float)audioProcessor.getHostBpm();
		track->generationKey = keySelector.getText();
		track->generationDuration = (int)durationSlider.getValue();
		track->selectedPrompt.clear();
	}

	startGenerationUI(generatingTrackId);
	juce::String selectedTrackId = generatingTrackId;
	auto request = track->createLoopRequest();
	juce::Thread::launch([this, selectedTrackId, request]()
		{
			try
			{
				juce::MessageManager::callAsync([this]() {
					statusLabel.setText("Generating loop (this may take a few minutes)...",
						juce::dontSendNotification);
					});

				audioProcessor.setServerUrl(audioProcessor.getServerUrl());
				audioProcessor.setApiKey(audioProcessor.getApiKey());
				juce::Thread::sleep(100);
				audioProcessor.generateLoop(request, generatingTrackId);
			}
			catch (const std::exception& e)
			{
				juce::MessageManager::callAsync([this, selectedTrackId, error = juce::String(e.what())]() {
					stopGenerationUI(selectedTrackId, false, error);
					audioProcessor.setIsGenerating(false);
					audioProcessor.setGeneratingTrackId("");
					});
			} });
}

void DjIaVstEditor::loadPromptPresets()
{
	promptPresetSelector.clear();
	juce::StringArray allPrompts = promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();
	for (const auto& customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}
	allPrompts.sort(true);
	promptPresets = allPrompts;
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

bool DjIaVstEditor::keyMatches(const juce::KeyPress& pressed, const juce::KeyPress& expected)
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

bool DjIaVstEditor::keyPressed(const juce::KeyPress& key)
{
	KeyboardLayout layout = detectKeyboardLayout();

	std::vector<std::vector<juce::KeyPress>> layoutKeys(8);

	switch (layout)
	{
	case AZERTY:
		layoutKeys = {
			{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
			{juce::KeyPress('a'), juce::KeyPress('z'), juce::KeyPress('e'), juce::KeyPress('r')},
			{juce::KeyPress('q'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
			{juce::KeyPress('w'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
			{juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
			{juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
			{juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
			{juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress(','), juce::KeyPress(';')} };
		break;

	case QWERTY:
		layoutKeys = {
			{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
			{juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
			{juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
			{juce::KeyPress('z'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
			{juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
			{juce::KeyPress('t'), juce::KeyPress('y'), juce::KeyPress('u'), juce::KeyPress('i')},
			{juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
			{juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')} };
		break;

	case QWERTZ:
		layoutKeys = {
			{juce::KeyPress('1'), juce::KeyPress('2'), juce::KeyPress('3'), juce::KeyPress('4')},
			{juce::KeyPress('a'), juce::KeyPress('s'), juce::KeyPress('d'), juce::KeyPress('f')},
			{juce::KeyPress('q'), juce::KeyPress('w'), juce::KeyPress('e'), juce::KeyPress('r')},
			{juce::KeyPress('y'), juce::KeyPress('x'), juce::KeyPress('c'), juce::KeyPress('v')},
			{juce::KeyPress('8'), juce::KeyPress('9'), juce::KeyPress('0'), juce::KeyPress('-')},
			{juce::KeyPress('t'), juce::KeyPress('z'), juce::KeyPress('u'), juce::KeyPress('i')},
			{juce::KeyPress('g'), juce::KeyPress('h'), juce::KeyPress('j'), juce::KeyPress('k')},
			{juce::KeyPress('b'), juce::KeyPress('n'), juce::KeyPress('m'), juce::KeyPress(',')} };
		break;
	}

	for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
	{
		for (int page = 0; page < 4; ++page)
		{
			if (keyMatches(key, layoutKeys[slotIndex][page]))
			{
				for (auto& trackComp : trackComponents)
				{
					if (auto* track = trackComp->getTrack())
					{
						if (track->slotIndex == slotIndex && track->usePages.load())
						{
							if (audioProcessor.getIsGenerating() && audioProcessor.getGeneratingTrackId() == track->trackId)
							{
								setStatusWithTimeout("Cannot switch pages during generation...");
								return false;
							}
							else
							{
								DBG("Global shortcut: slot " << (slotIndex + 1) << " page " << (char)('A' + page) << " [" << (layout == AZERTY ? "AZERTY" : layout == QWERTZ ? "QWERTZ"
									: "QWERTY")
									<< "]");
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
	}
	else
	{
		promptInput.clear();
		statusLabel.setText("Custom prompt mode", juce::dontSendNotification);
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
	}
	else
	{
		statusLabel.setText("Enter a prompt first!", juce::dontSendNotification);
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
		loadSampleButton.setButtonText("Load Sample");
		loadSampleButton.setEnabled(false);
		autoLoadButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonWarning.darker(0.3f));
	}
	else
	{
		autoLoadButton.setButtonText("Manual Mode");
		statusLabel.setText("Manual mode - click Load Sample when ready", juce::dontSendNotification);
		loadSampleButton.setEnabled(true);
		updateLoadButtonState();
		autoLoadButton.setColour(juce::TextButton::buttonColourId,
			ColourPalette::buttonPrimary);
	}
}

void DjIaVstEditor::onLoadSampleClicked()
{
	if (audioProcessor.hasSampleWaiting())
	{
		juce::String trackId = audioProcessor.getSelectedTrackId();

		audioProcessor.loadPendingSample();
		statusLabel.setText("Sample loaded manually!", juce::dontSendNotification);

		onSampleLoaded(trackId);
		updateLoadButtonState();
	}
	else
	{
		statusLabel.setText("Generate a loop first", juce::dontSendNotification);
	}
}

void DjIaVstEditor::updateLoadButtonState()
{
	if (!autoLoadButton.getToggleState())
	{
		if (audioProcessor.hasSampleWaiting())
		{
			loadSampleButton.setButtonText("Load Sample (Ready!)");
		}
		else
		{
			loadSampleButton.setButtonText("Load Sample");
		}
	}
}

void DjIaVstEditor::refreshTrackComponents()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	std::sort(trackIds.begin(), trackIds.end(),
		[this](const juce::String& a, const juce::String& b)
		{
			TrackData* trackA = audioProcessor.getTrack(a);
			TrackData* trackB = audioProcessor.getTrack(b);
			if (!trackA || !trackB)
				return false;

			return trackA->slotIndex < trackB->slotIndex;
		});
	if (trackComponents.size() == trackIds.size())
	{
		bool allVisible = true;
		for (auto& comp : trackComponents)
		{
			if (!comp->isVisible() || comp->getParentComponent() == nullptr)
			{
				allVisible = false;
				break;
			}
		}
		if (allVisible)
		{
			for (int i = 0; i < trackComponents.size() && i < trackIds.size(); ++i)
			{
				trackComponents[i]->setTrackData(audioProcessor.getTrack(trackIds[i]));

				juce::Timer::callAfterDelay(100, [this, i]()
					{ trackComponents[i]->updatePromptPresets(getAllPrompts()); });
				trackComponents[i]->updateFromTrackData();
				if (auto* sequencer = trackComponents[i]->getSequencer())
				{
					sequencer->updateFromTrackData();
				}
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
	int yPos = 5;

	for (const auto& trackId : trackIds)
	{
		TrackData* trackData = audioProcessor.getTrack(trackId);
		if (!trackData)
			continue;

		auto trackComp = std::make_unique<TrackComponent>(trackId, audioProcessor);
		trackComp->setTrackData(trackData);
		TrackComponent* trackCompPtr = trackComp.get();
		juce::Timer::callAfterDelay(100, [this, trackCompPtr, trackId]()
			{
				auto it = std::find_if(trackComponents.begin(), trackComponents.end(),
					[trackCompPtr](const auto& tc) { return tc.get() == trackCompPtr; });

				if (it != trackComponents.end() && trackCompPtr->getTrack() && !trackCompPtr->getTrack()->selectedPrompt.isEmpty())
				{
					trackCompPtr->updatePromptPresets(getAllPrompts());
				} });
				trackComp->onSelectTrack = [this](const juce::String& id)
					{
						audioProcessor.selectTrack(id);
						updateSelectedTrack();
					};

				trackComp->onDeleteTrack = [this](const juce::String& id)
					{
						if (audioProcessor.getAllTrackIds().size() > 1)
						{
							audioProcessor.deleteTrack(id);
							juce::Timer::callAfterDelay(10, [this]()
								{
									refreshTrackComponents();
									refreshWavevormsAndSequencers();
								});
						}
					};

				trackComp->onGenerateWithImage = [this](const juce::String& trackId, const juce::String& image, const juce::StringArray& keywords)
					{
						audioProcessor.generateSampleWithImage(trackId, image, keywords);
					};

				trackComp->onTrackRenamed = [this](const juce::String& id, const juce::String& newName)
					{
						if (mixerPanel)
						{
							mixerPanel->updateTrackName(id, newName);
						}
					};

				trackComp->onGenerateForTrack = [this](const juce::String& id)
					{
						audioProcessor.selectTrack(id);
						generateFromTrackComponent(id);
					};

				trackComp->onReorderTrack = [this](const juce::String& fromId, const juce::String& toId)
					{
						audioProcessor.reorderTracks(fromId, toId);
						juce::Timer::callAfterDelay(10, [this]()
							{ refreshTrackComponents(); });
					};

				trackComp->onPreviewTrack = [this](const juce::String& trackId)
					{
						audioProcessor.previewTrack(trackId);
					};

				trackComp->onTrackPromptChanged = [this](const juce::String /*&trackId*/, const juce::String& prompt)
					{
						setStatusWithTimeout("Track prompt updated: " + prompt.substring(0, 20) + "...", 3000);
					};

				trackComp->onStatusMessage = [this](const juce::String& message)
					{
						setStatusWithTimeout(message, 3000);
					};

				trackComp->onStopPreview = [this](const juce::String& trackId)
					{
						audioProcessor.stopTrackPreview(trackId);
					};

				int fullWidth = tracksContainer.getWidth() - 4;
				trackComp->setBounds(2, yPos, fullWidth, 60);

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

				yPos += 85;
	}

	tracksContainer.setSize(tracksViewport.getWidth() - 20, yPos + 5);
	if (mixerPanel)
	{
		mixerPanel->refreshMixerChannels();
	}
	setEnabled(true);
	juce::MessageManager::callAsync([this]()
		{
			resized();
			repaint(); });
	tracksContainer.repaint();
}

void DjIaVstEditor::reEnableCanvasForTrack()
{
	setAllGenerateButtonsEnabled(true);
}

void DjIaVstEditor::generateFromTrackComponent(const juce::String& trackId)
{
	audioProcessor.setIsGenerating(true);

	TrackData* track = audioProcessor.getTrack(trackId);
	if (!track)
	{
		statusLabel.setText("Error: Track not found", juce::dontSendNotification);
		audioProcessor.setIsGenerating(false);
		return;
	}

	if (track->selectedPrompt.isEmpty())
	{
		statusLabel.setText("Error: No prompt selected for this track", juce::dontSendNotification);
		audioProcessor.setIsGenerating(false);
		return;
	}

	juce::String currentGeneratingTrackId = trackId;
	audioProcessor.setGeneratingTrackId(currentGeneratingTrackId);

	if (track->usePages.load())
	{
		auto& currentPage = track->getCurrentPage();

		currentPage.selectedPrompt = track->selectedPrompt;
		currentPage.generationPrompt = track->selectedPrompt;
		currentPage.generationBpm = audioProcessor.getGlobalBpm();
		currentPage.generationKey = audioProcessor.getGlobalKey();
		currentPage.generationDuration = audioProcessor.getGlobalDuration();

		track->syncLegacyProperties();

		DBG("Track generation for page " << (char)('A' + track->currentPageIndex) << " - Prompt: " << currentPage.selectedPrompt);
	}
	else
	{
		track->generationBpm = audioProcessor.getGlobalBpm();
		track->generationKey = audioProcessor.getGlobalKey();
		track->generationDuration = audioProcessor.getGlobalDuration();
	}

	startGenerationUI(currentGeneratingTrackId);

	juce::Thread::launch([this, currentGeneratingTrackId, track]()
		{
			try {
				auto request = track->createLoopRequest();
				audioProcessor.generateLoop(request, currentGeneratingTrackId);
			}
			catch (const std::exception& e) {
				juce::MessageManager::callAsync([this, currentGeneratingTrackId, error = juce::String(e.what())]() {
					stopGenerationUI(currentGeneratingTrackId, false, error);
					audioProcessor.setIsGenerating(false);
					audioProcessor.setGeneratingTrackId("");
					});
			} });
}

juce::StringArray DjIaVstEditor::getAllPrompts() const
{
	juce::StringArray allPrompts = promptPresets;
	auto customPrompts = audioProcessor.getCustomPrompts();

	for (const auto& customPrompt : customPrompts)
	{
		if (!allPrompts.contains(customPrompt))
		{
			allPrompts.add(customPrompt);
		}
	}

	return allPrompts;
}

void DjIaVstEditor::toggleWaveFormButtonOnTrack()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = audioProcessor.getTrack(trackId);
		if (track)
		{
			track->showWaveform = false;
		}
	}
	for (auto& trackComponent : trackComponents)
	{
		trackComponent->showWaveformButton.setToggleState(false, juce::dontSendNotification);
	}
}

void DjIaVstEditor::restoreUICallbacks()
{
	for (auto& trackComp : trackComponents)
	{
		if (trackComp->getTrack())
		{
			trackComp->setupMidiLearn();
		}
	}
}

void DjIaVstEditor::toggleSEQButtonOnTrack()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto& trackId : trackIds)
	{
		TrackData* track = audioProcessor.getTrack(trackId);
		if (track)
		{
			track->showSequencer = false;
		}
	}
	for (auto& trackComponent : trackComponents)
	{
		trackComponent->sequencerToggleButton.setToggleState(false, juce::dontSendNotification);
	}
}

void DjIaVstEditor::setStatusWithTimeout(const juce::String& message, int timeoutMs)
{
	statusLabel.setText(message, juce::dontSendNotification);
	juce::Timer::callAfterDelay(timeoutMs, [safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		{
			if (auto* editor = safeThis.getComponent())
				editor->statusLabel.setText("Ready", juce::dontSendNotification); });
}

void DjIaVstEditor::onAddTrack()
{
	try
	{
		juce::String currentSelectedId = audioProcessor.getSelectedTrackId();
		juce::String newTrackId = audioProcessor.createNewTrack();

		refreshTrackComponents();
		refreshWavevormsAndSequencers();

		if (audioProcessor.getIsGenerating())
		{
			for (auto& trackComp : trackComponents)
			{
				if (trackComp->trackId == newTrackId)
				{
					trackComp->setGenerateButtonEnabled(false);
					trackComp->setCanvasGenerating(true);
					break;
				}
			}
		}

		if (mixerPanel)
		{
			mixerPanel->trackAdded(newTrackId);
			if (!currentSelectedId.isEmpty())
			{
				mixerPanel->trackSelected(currentSelectedId);
			}
		}
		setStatusWithTimeout("New track created");
	}
	catch (const std::exception& e)
	{
		setStatusWithTimeout("Error: " + juce::String(e.what()));
	}
}

void DjIaVstEditor::updateSelectedTrack()
{
	for (auto& trackComp : trackComponents)
	{
		trackComp->setSelected(false);
	}

	juce::String selectedId = audioProcessor.getSelectedTrackId();

	bool found = false;
	for (auto& trackComp : trackComponents)
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

void* DjIaVstEditor::getSequencerForTrack(const juce::String& trackId)
{
	for (auto& trackComp : trackComponents)
	{
		if (trackComp->getTrackId() == trackId)
		{
			return (void*)trackComp->getSequencer();
		}
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

	juce::Thread::launch([this, timeout, safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]() {
		auto creditsInfo = audioProcessor.getApiClient().checkCredits(timeout);
		juce::MessageManager::callAsync([safeThis, creditsInfo]() {
			if (auto* editor = safeThis.getComponent())
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
						creditsText = "Credits: " + juce::String(creditsInfo.creditsRemaining) +
							" / " + juce::String(creditsInfo.creditsTotal);
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
	juce::Thread::launch([safeThis = juce::Component::SafePointer<DjIaVstEditor>(this)]()
		{
			juce::URL url("https://api.github.com/repos/innermost47/ai-dj/releases/latest");
			auto stream = url.createInputStream(
				juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
				.withExtraHeaders("User-Agent: OBSIDIAN-Neural-Plugin")
				.withConnectionTimeoutMs(5000));

			if (stream == nullptr) return;

			auto json = juce::JSON::parse(stream->readEntireStreamAsString());
			if (auto* obj = json.getDynamicObject())
			{
				auto tagName = obj->getProperty("tag_name").toString();
				int latestNum = tagName.trimCharactersAtStart("v").getIntValue();
				int currentNum = juce::String(BUILD_NUMBER).getIntValue();

				if (latestNum > currentNum)
				{
					juce::MessageManager::callAsync([safeThis, tagName]()
						{
							if (auto* editor = safeThis.getComponent())
							{
								if (editor->isInitialized.load())
								{
									juce::Timer::callAfterDelay(2000, [safeThis, tagName]()
										{
											if (auto* editor = safeThis.getComponent())
											{
												juce::AlertWindow::showAsync(
													juce::MessageBoxOptions()
													.withIconType(juce::MessageBoxIconType::InfoIcon)
													.withTitle("Update Available!")
													.withMessage(
														"A new version of OBSIDIAN Neural is available: " + tagName + "\n\n"
														"Your current build: v" + juce::String(BUILD_NUMBER) + "\n\n"
														"Download the latest version at:\n"
														"github.com/innermost47/ai-dj/releases/latest")
													.withButton("Download Now")
													.withButton("Later"),
													[](int result)
													{
														if (result == 1)
														{
															juce::URL("https://github.com/innermost47/ai-dj/releases/latest")
																.launchInDefaultBrowser();
														}
													});
											}
										});
								}
							}
						});
				}
			}
		});
}

TrackComponent* DjIaVstEditor::getTrackComponent(const juce::String& trackId)
{
	for (auto& track : trackComponents)
	{
		if (track->getTrackId() == trackId)
		{
			return track.get();
		}
	}

	return nullptr;
}