#include "OnboardingFlow.h"
#include "ObsidianModal.h"
#include "OnboardingStep.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "UIModalManager.h"

OnboardingFlow::OnboardingFlow(DjIaVstEditor &editorIn, UIModalManager &modalManagerIn, OnboardingVariant variantIn)
    : editor(editorIn), modalManager(modalManagerIn), variant(variantIn)
{
	buildSteps();
}

OnboardingFlow::~OnboardingFlow() = default;

void OnboardingFlow::buildSteps()
{
	const int total = 8;
	auto makeTitle = [total](int n, const juce::String &subtitle)
	{ return juce::String(n) + " of " + juce::String(total) + " - " + subtitle; };

	{
		OnboardingStepData s;
		s.title = makeTitle(1, "Welcome");
		s.headline = "Welcome to OBSIDIAN Neural";
		s.lead = "OBSIDIAN is an AI sound engine. Describe a sound, the engine generates it "
		         "as audio you can play, loop, and sequence.\n\n"
		         "Eight quick steps and you'll be making noise.";
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(2, "The Prompt Bank");
		s.headline = "The Prompt Bank";
		s.lead = "Your prompt library lives in the left panel.\nBuild a personal palette of "
		         "sound descriptions you can reuse and combine.";
		s.rows = {
		    {"chat", "Write your prompts",
		     "Describe sounds in plain language. \"Acid bass, 303, gritty\" works. So does \"warm pad, slow attack\"."},
		    {"folder", "Organize into categories",
		     "Group prompts by genre, mood, instrument. Categories are shared with the Sample Bank - rename one, the "
		     "other follows."},
		    {"pencil", "Edit anytime", "Right-click for rename, edit, or delete. Double-click for quick edit."},
		    {"dragndrop", "Assign to a track",
		     "Drag a prompt from the bank onto any track. The prompt and its AI model travel together."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(3, "Tracks & Pages");
		s.headline = "Tracks & Pages";
		s.lead = "Eight tracks, each with four pages (A/B/C/D) to keep variations of the same idea side by side.";
		s.rows = {{"dragndrop", "Assign a prompt",
		           "Drag from the bank or pick from the prompt dropdown. The model selector chooses your AI engine - "
		           "eight models available, each with its own color and personality."},
		          {"folder", "Switch between pages",
		           "A, B, C, D - one slot per page. Use them to try multiple takes without losing previous ones."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(4, "Generate");
		s.headline = "Generate";
		s.lead = "Each track has its own GEN button - the lightning bolt on the right side of the track header.";
		s.rows = {{"lightning", "Hit GEN",
		           "Click GEN to generate audio for the current page. The track pulses while the AI works. When done, "
		           "the waveform appears."},
		          {"dice", "Roll the dice",
		           "Not happy with the result? Hit GEN again - fresh roll. Switch pages (A/B/C/D) to keep the previous "
		           "take and try something new."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(5, "The Sample Bank");
		s.headline = "The Sample Bank";
		s.lead = "Every generation is automatically saved to the Sample Bank - the second tab in the left panel.";
		s.rows = {
		    {"disk", "Auto-saved", "All your generations are kept. Search by prompt, model, category, BPM, or key."},
		    {"dragndrop", "Reuse on any track",
		     "Drag a sample from the bank onto a track to use it without re-generating."},
		    {"export", "Export to your DAW", "Ctrl+Drag a sample directly into your DAW as an audio file."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(6, "Preview, Arm, Play");
		s.headline = "Preview, Arm, Play";
		s.lead = "Two ways to hear what you've made.";
		s.leadStandaloneOnly = "Preview and playback go directly to your audio output.";
		s.leadVstOnly = "Preview is routed to channel 9. Enable the plugin's multi-output "
		                "in your DAW to hear it.";
		s.rows = {
		    {"headphones", "Preview",
		     "Click the headphones icon on a track header or on a sample in the bank. Instant audition, no arming."},
		    {"play", "Mixer play (arm)",
		     "Click play on a mixer channel (bottom panel) to arm the track. The track doesn't start immediately - it "
		     "kicks in at the next bar and loops in sync. Same for stopping."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(7, "Sculpt Your Sound");
		s.headline = "Sculpt Your Sound";
		s.lead = "Each track gives you several ways to shape what comes out.";
		s.rows = {
		    {"sliders", "ADSR knobs",
		     "Attack, decay, sustain, release. Standard envelope applied to the sample playback."},
		    {"repeat", "Beat repeat",
		     "The button between preview and ADSR. Triggers stutter and repeat effects. RND randomizes the pattern."},
		    {"waveform", "Waveform editor",
		     "Drag on the waveform to set loop in/out points. Mouse wheel zooms, Ctrl+Wheel for finer zoom."},
		    {"grid", "Step sequencer",
		     "The grid below the waveform places retrigger steps. Each page (A/B/C/D) has its own sequence, and you "
		     "can extend it up to 4 bars using the 1/2/3/4 buttons."}};
		steps.push_back(s);
	}

	{
		OnboardingStepData s;
		s.title = makeTitle(8, "The Big Picture");
		s.headline = "The Big Picture";
		s.lead = "Quick map of the rest. Now go make noise.";
		s.leadStandaloneOnly = "Hit the transport play button (top right) to start the clock. "
		                       "Ableton Link is available if you want to sync with other apps.";
		s.leadVstOnly = "Start your DAW's transport - your armed tracks kick in at the next bar.";
		s.rows = {
		    {"sliders", "Mixer", "Volume, pitch, pan per track, plus EQ on the master. Bottom of the screen."},
		    {"waveform", "Delay & Reverb", "Global sends on the right side, with BPM-synced divisions for the delay."},
		    {"map", "Master Key & Duration",
		     "Applied to every generation. Set the musical key your session is in. Bottom right."}};
		steps.push_back(s);
	}
}

void OnboardingFlow::start()
{
	currentStepIndex = 0;
	showStep(currentStepIndex);
}

void OnboardingFlow::showStep(int stepIndex)
{
	if (stepIndex < 0 || stepIndex >= (int)steps.size())
		return;

	const auto &stepData = steps[stepIndex];
	const bool isLastStep = (stepIndex == (int)steps.size() - 1);

	auto stepContent = std::make_unique<OnboardingStep>(stepData, variant);
	const int contentHeight = stepContent->getPreferredHeight(Obsidian::BASE_MODAL_WIDTH) + 120;

	auto modal = std::make_unique<ObsidianModalWindow>(stepData.title, Obsidian::BASE_MODAL_WIDTH, contentHeight);
	modal->setContent(std::move(stepContent));

	auto overlayOwned = std::make_unique<ObsidianModalOverlay>(std::move(modal));
	auto *overlay = overlayOwned.get();
	modalManager.addModal(std::move(overlayOwned));

	juce::String arrowSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"></line><polyline points="12 5 19 12 12 19"></polyline></svg>)";
	juce::String skipSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><polyline points="13 17 18 12 13 7"></polyline><polyline points="6 17 11 12 6 7"></polyline></svg>)";

	overlay->modalWindow->addButton("Skip tour", skipSvg, ColourPalette::buttonInactive,
	                                [this, overlay]()
	                                {
		                                overlay->close();
		                                skip();
	                                });

	overlay->modalWindow->addButton(
	    isLastStep ? "Let's go !" : "Next", arrowSvg, ColourPalette::slate,
	    [this, overlay, stepIndex, isLastStep]()
	    {
		    overlay->close();
		    if (isLastStep)
		    {
			    finish();
		    }
		    else
		    {
			    juce::Timer::callAfterDelay(
			        400,
			        [safeEditor = juce::Component::SafePointer<DjIaVstEditor>(&editor), nextIndex = stepIndex + 1]()
			        {
				        if (auto *e = safeEditor.getComponent())
				        {
					        e->uiModalManager->advanceOnboardingTo(nextIndex);
				        }
			        });
		    }
	    });
}

void OnboardingFlow::finish()
{
	editor.audioProcessor.setOnboardingDone(true);
	editor.audioProcessor.saveGlobalConfig();

	editor.statusLabel.setText(juce::String::fromUTF8("Ready - pick a prompt, hit GEN and let's hear what comes out."),
	                           juce::dontSendNotification);
	editor.statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
	editor.uiStatusManager->updateLCD();
}

void OnboardingFlow::skip()
{
	editor.audioProcessor.setOnboardingDone(true);
	editor.audioProcessor.saveGlobalConfig();
}