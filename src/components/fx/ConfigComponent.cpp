#include "ConfigComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "config/version.h"

ConfigComponent::ConfigComponent(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	auto imagePtr = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

	if (imagePtr.isValid())
		logoImage = imagePtr;

	setupUI();
}

void ConfigComponent::setupUI()
{
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
	helpButton.loadIcon(BinaryData::info_svg, BinaryData::info_svgSize);
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

	helpButton.onClick = [this]() { editor.uiModalManager->showOnboardingStep(1); };
}

void ConfigComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat().reduced(ObsidianSizes::PADDING);

	auto imageArea = bounds.removeFromTop(bounds.getHeight() * 0.65f);

	g.drawImageWithin(logoImage, (int)imageArea.getX(), (int)imageArea.getY(), (int)imageArea.getWidth(),
	                  (int)imageArea.getHeight(),
	                  juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, false);

	float valSize = 14.0f;
	g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), valSize, juce::Font::bold));

	g.setColour(ColourPalette::textPrimary);
	g.drawText("OBSIDIAN Neural", bounds, juce::Justification::centredTop);

	bounds.removeFromTop(14.0f);

	g.setFont(juce::FontOptions(juce::Font::getSystemUIFontName(), valSize, juce::Font::italic));
	g.setColour(ColourPalette::textAccent);
	g.drawText("Sound Engine - " + Version::VERSION, bounds, juce::Justification::centredTop);
}

void ConfigComponent::resized()
{
	auto area = getLocalBounds().reduced(ObsidianSizes::PADDING);

	juce::FlexBox firstBox;
	firstBox.flexDirection = juce::FlexBox::Direction::row;
	firstBox.flexWrap = juce::FlexBox::Wrap::wrap;
	firstBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
	firstBox.alignItems = juce::FlexBox::AlignItems::center;

	if (!juce::JUCEApplicationBase::isStandaloneApp())
	{
		firstBox.items.add(juce::FlexItem(bypassSequencerButton)
		                       .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
		                       .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
		                       .withFlex(1));
	}

	firstBox.items.add(juce::FlexItem(bypassLLMButton)
	                       .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                       .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                       .withFlex(1));
	firstBox.items.add(juce::FlexItem(configButton)
	                       .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                       .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                       .withFlex(1));
	firstBox.items.add(juce::FlexItem(helpButton)
	                       .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                       .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                       .withFlex(1));
	firstBox.items.add(juce::FlexItem(openMidiEditorButton)
	                       .withMinWidth(ObsidianSizes::MIN_SMALL_BTN_WIDTH)
	                       .withMinHeight(ObsidianSizes::MIN_SMALL_BTN_HEIGHT)
	                       .withFlex(1));

	firstBox.performLayout(area.removeFromBottom(ObsidianSizes::MIN_SMALL_BTN_HEIGHT));
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
}