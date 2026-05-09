#include "LeftPanelWrapper.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

LeftPanelWrapper::LeftPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor)
{
	sampleBank = std::make_unique<SampleBankPanel>(processor);
	addAndMakeVisible(*sampleBank);

	promptBank = std::make_unique<PromptBankPanel>(processor, editor);
	addAndMakeVisible(*promptBank);

	auto setupTab = [this](juce::TextButton &btn, Tab tab)
	{
		btn.setClickingTogglesState(true);
		btn.setRadioGroupId(0xCAFE);
		btn.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep);
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::trackSelected.withAlpha(0.3f));
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
		btn.onClick = [this, tab]() { setActiveTab(tab); };
		addAndMakeVisible(btn);
	};

	setupTab(promptTabButton, Tab::Prompt);
	setupTab(sampleTabButton, Tab::Sample);

	setActiveTab(Tab::Prompt);
}

LeftPanelWrapper::~LeftPanelWrapper() = default;

void LeftPanelWrapper::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds, ObsidianSizes::CORNER);

	g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
	g.drawRoundedRectangle(bounds, ObsidianSizes::CORNER, 1);
}

void LeftPanelWrapper::resized()
{
	auto area = getLocalBounds().reduced(ObsidianSizes::MAIN_PADDING);

	auto tabBar = area.removeFromTop(tabBarHeight);
	const int tabW = tabBar.getWidth() / 2;
	promptTabButton.setBounds(tabBar.removeFromLeft(tabW));
	tabBar.removeFromLeft(ObsidianSizes::SPACER_MD);
	sampleTabButton.setBounds(tabBar);

	area.removeFromTop(4);

	if (promptBank)
		promptBank->setBounds(area);
	if (sampleBank)
		sampleBank->setBounds(area);
}

void LeftPanelWrapper::setActiveTab(Tab tab)
{
	activeTab = tab;
	promptTabButton.setToggleState(tab == Tab::Prompt, juce::dontSendNotification);
	sampleTabButton.setToggleState(tab == Tab::Sample, juce::dontSendNotification);
	updateTabVisibility();
}

void LeftPanelWrapper::updateTabVisibility()
{
	if (promptBank)
		promptBank->setVisible(activeTab == Tab::Prompt);
	if (sampleBank)
		sampleBank->setVisible(activeTab == Tab::Sample);
}

juce::var LeftPanelWrapper::saveUIState() const
{
	juce::DynamicObject::Ptr o = new juce::DynamicObject();
	o->setProperty("activeTab", (int)activeTab);

	if (promptBank)
		o->setProperty("promptBankState", promptBank->saveUIState());

	return juce::var(o.get());
}

void LeftPanelWrapper::restoreUIState(const juce::var &state)
{
	if (!state.isObject())
		return;
	auto *o = state.getDynamicObject();
	if (!o)
		return;

	int tab = (int)o->getProperty("activeTab");
	setActiveTab(static_cast<Tab>(tab));

	if (promptBank)
		promptBank->restoreUIState(o->getProperty("promptBankState"));
}