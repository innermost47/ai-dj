#include "ConfigComponent.h"
#include "OnboardingStepData.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ScaleAndDuration.h"
#include "config/version.h"

ConfigComponent::ConfigComponent(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	auto imagePtr = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

	if (imagePtr.isValid())
		logoImage = imagePtr;

	scaleAndDurationPanel = std::make_unique<ScaleAndDurationPanel>(processor);

	setupUI();
}

ConfigComponent::~ConfigComponent() = default;

void ConfigComponent::setupUI()
{
	addAndMakeVisible(configLabel);
	configLabel.setText("Settings", juce::dontSendNotification);
	configLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	configLabel.setJustificationType(juce::Justification::left);
	configLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_REGULAR));

	addAndMakeVisible(versionLabel);
	versionLabel.setText("OBSIDIAN Neural - " + Version::VERSION, juce::dontSendNotification);
	versionLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	versionLabel.setJustificationType(juce::Justification::right);
	versionLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_XXS));

	addAndMakeVisible(buildLabel);
	buildLabel.setText(Version::BUILD, juce::dontSendNotification);
	buildLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	buildLabel.setJustificationType(juce::Justification::right);
	buildLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_XXS));

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

	bypassLLMButton.setTooltip("Disables prompt enhancement for faster, raw generation - Disabled by default");
	configButton.setTooltip("Configure API settings and generation mode");

	addAndMakeVisible(openMidiEditorButton);
	openMidiEditorButton.loadIcon(BinaryData::piano_svg, BinaryData::piano_svgSize);
	openMidiEditorButton.setTooltip("Open MIDI mappings editor");

	addAndMakeVisible(helpButton);
	helpButton.loadIcon(BinaryData::infofill_svg, BinaryData::infofill_svgSize);
	helpButton.setTooltip("Open the Quick Start tour");

	addAndMakeVisible(configButton);
	configButton.loadIcon(BinaryData::gear_svg, BinaryData::gear_svgSize);
	configButton.setTooltip("Configure settings globally");

	auto setupControlBtn = [this](IconButtonSimple &btn, bool hasAccentBar = true)
	{
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundMid);
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
		btn.setHasAccentBar(hasAccentBar);
		btn.setShowBackground(false);
		btn.setShowBorder(false);
		addAndMakeVisible(btn);
	};

	setupControlBtn(bypassSequencerButton);
	setupControlBtn(openMidiEditorButton, false);
	setupControlBtn(configButton, false);
	setupControlBtn(helpButton, false);
	setupControlBtn(bypassLLMButton);

	addAndMakeVisible(scaleAndDurationPanel.get());

	addEventListeners();
}

void ConfigComponent::addEventListeners()
{
	configButton.onClick = [this]() { editor.uiModalManager->showConfigDialog(); };

	bypassSequencerButton.onClick = [this]()
	{
		bool isBypassed = bypassSequencerButton.getToggleState();
		audioProcessor.setBypassSequencer(isBypassed);

		if (isBypassed)
		{
			bypassSequencerButton.setButtonText("Composition Mode");
			bypassSequencerButton.loadIcon(BinaryData::cpuregular_svg, BinaryData::cpuregular_svgSize);
			editor.statusLabel.setText("Composition mode - Direct MIDI playback", juce::dontSendNotification);
			editor.uiStatusManager->updateLCD();
		}
		else
		{
			bypassSequencerButton.setButtonText("Sequencer Mode");
			bypassSequencerButton.loadIcon(BinaryData::cpu_svg, BinaryData::cpu_svgSize);
			editor.statusLabel.setText("Sequencer mode - Armed playback", juce::dontSendNotification);
			editor.uiStatusManager->updateLCD();
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
			editor.statusLabel.setText("Direct Mode: LLM Bypassed", juce::dontSendNotification);
			editor.uiStatusManager->updateLCD();
		}
		else
		{
			bypassLLMButton.setButtonText("Enhanced Mode");
			bypassLLMButton.loadIcon(BinaryData::robotfill_svg, BinaryData::robotfill_svgSize);
			editor.statusLabel.setText("AI-optimized prompt activated", juce::dontSendNotification);
			editor.uiStatusManager->updateLCD();
		}
	};

	openMidiEditorButton.onClick = [this] { editor.uiModalManager->openMidiMappingEditor(); };

	helpButton.onClick = [this]()
	{
		const auto variant = audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone
		                         ? OnboardingVariant::Standalone
		                         : OnboardingVariant::VST;
		editor.uiModalManager->showOnboarding(variant);
	};
}

void ConfigComponent::paint(juce::Graphics & /*g*/)
{
}

void ConfigComponent::resized()
{
	auto area = getLocalBounds().reduced(ObsidianSizes::PADDING);

	buildLabel.setBounds(area.removeFromBottom((int)ObsidianSizes::TEXT_XS));
	versionLabel.setBounds(area.removeFromBottom((int)ObsidianSizes::TEXT_XS));

	auto configArea = area.removeFromTop(ObsidianSizes::CONFIG_AREA_HEIGHT);

	juce::FlexBox column;
	column.flexDirection = juce::FlexBox::Direction::column;

	juce::FlexBox btnBox;
	btnBox.flexDirection = juce::FlexBox::Direction::row;
	btnBox.flexWrap = juce::FlexBox::Wrap::wrap;
	btnBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
	btnBox.alignItems = juce::FlexBox::AlignItems::center;

	if (!juce::JUCEApplicationBase::isStandaloneApp())
	{
		btnBox.items.add(juce::FlexItem(bypassSequencerButton)
		                     .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
		                     .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
		                     .withFlex(1));
	}

	btnBox.items.add(juce::FlexItem(bypassLLMButton)
	                     .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                     .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                     .withFlex(1));
	btnBox.items.add(juce::FlexItem(configButton)
	                     .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                     .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                     .withFlex(1));
	btnBox.items.add(juce::FlexItem(helpButton)
	                     .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                     .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                     .withFlex(1));
	btnBox.items.add(juce::FlexItem(openMidiEditorButton)
	                     .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                     .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                     .withFlex(1));

	column.items.add(juce::FlexItem(*scaleAndDurationPanel).withMinHeight(ObsidianSizes::SCALE_AND_DURATION_HEIGHT));
	column.items.add(juce::FlexItem(configLabel).withMinHeight(26));
	column.items.add(juce::FlexItem(btnBox).withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT));

	column.performLayout(configArea);
}

void ConfigComponent::updateFromProcessor()
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

	scaleAndDurationPanel->update();
}