#include "LeftPanelWrapper.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

LeftPanelWrapper::LeftPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor) : editor(editor)
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
		btn.setColour(juce::TextButton::buttonOnColourId, ColourPalette::lightGrey.withAlpha(0.3f));
		btn.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		btn.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
		btn.onClick = [this, tab]() { setActiveTab(tab); };
		addAndMakeVisible(btn);
	};

	setupTab(promptTabButton, Tab::Prompt);
	setupTab(sampleTabButton, Tab::Sample);

	promptTabButton.loadIcon(BinaryData::chattext_svg, BinaryData::chattext_svgSize);
	sampleTabButton.loadIcon(BinaryData::fileaudio_svg, BinaryData::fileaudio_svgSize);

	collapseButton.loadIcon(BinaryData::caretleft_svg, BinaryData::caretleft_svgSize);
	collapseButton.setColour(juce::TextButton::buttonColourId, ColourPalette::lightGrey.withAlpha(0.05f));
	collapseButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::lightGrey.withAlpha(0.05f));
	collapseButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	collapseButton.setColour(juce::TextButton::textColourOnId, ColourPalette::textPrimary);
	collapseButton.setClickingTogglesState(true);
	collapseButton.setToggleState(true, juce::dontSendNotification);
	collapseButton.onClick = [this]() { collapseExpand(collapseButton.getToggleState()); };
	addAndMakeVisible(collapseButton);

	promptTabButton.setCompactMode(true);
	sampleTabButton.setCompactMode(true);
	collapseButton.setCompactMode(true);

	setActiveTab(Tab::Prompt);
}

LeftPanelWrapper::~LeftPanelWrapper() = default;

void LeftPanelWrapper::paint(juce::Graphics &g)
{
	paintBaseBackgroundWithRightBorder(g);

	if (!isExpanded)
	{
		g.setColour(ColourPalette::textSecondary);
		g.setFont(juce::FontOptions(Obsidian::MICHROMA).withHeight(Obsidian::TEXT_TITLE));

		auto area = getLocalBounds();

		g.saveState();

		g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
		                                               static_cast<float>(area.getCentreX()),
		                                               static_cast<float>(area.getCentreY())));

		juce::Rectangle<int> textArea(0, 0, area.getHeight(), area.getWidth());

		textArea.setCentre(area.getCentreX(), area.getCentreY());

		juce::String label = activeTab == Tab::Prompt ? "PROMPT BANK" : "SAMPLE BANK";
		g.drawText(label, textArea, juce::Justification::centred);

		std::vector<juce::Colour> colourList = {ColourPalette::emerald, ColourPalette::amber, ColourPalette::coral};

		const float ellipseSize = 6.0f;
		float startX = static_cast<float>(textArea.getX()) + 12.0f;
		for (juce::Colour colour : colourList)
		{
			g.setColour(colour);
			g.fillEllipse(startX, static_cast<float>((area.getCentreY() - ellipseSize / 2) + 1.0f), ellipseSize,
			              ellipseSize);
			startX += ellipseSize + Obsidian::PADDING;
		}

		g.restoreState();
	}
}

void LeftPanelWrapper::collapseExpand(bool expanded)
{
	isExpanded = expanded;

	promptTabButton.setVisible(isExpanded);
	sampleTabButton.setVisible(isExpanded);

	if (isExpanded)
	{
		updateTabVisibility();
	}
	else
	{
		promptBank->setVisible(false);
		sampleBank->setVisible(false);
	}

	if (isExpanded)
	{
		collapseButton.loadIcon(BinaryData::caretleft_svg, BinaryData::caretleft_svgSize);
	}
	else
		collapseButton.loadIcon(BinaryData::caretright_svg, BinaryData::caretright_svgSize);

	repaint();
	editor.uiLayoutManager->resized();
}

void LeftPanelWrapper::resized()
{
	auto area = getLocalBounds().reduced(Obsidian::PADDING);

	auto tabBar = area.removeFromTop(Obsidian::TAB_BAR_HEIGHT);

	if (isExpanded)
	{

		const int collapseButtonWidth = 28;
		const int tabW = (tabBar.getWidth() - collapseButtonWidth - Obsidian::SPACER_MD * 2) / 2;
		promptTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		sampleTabButton.setBounds(tabBar.removeFromLeft(tabW));
		tabBar.removeFromLeft(Obsidian::SPACER_MD);
		collapseButton.setBounds(tabBar.removeFromLeft(collapseButtonWidth));
	}
	else
	{
		collapseButton.setBounds(tabBar.removeFromLeft(tabBar.getWidth()));
	}

	area.removeFromTop(4);

	if (promptBank)
	{
		if (isExpanded)
		{
			promptBank->setBounds(area);
		}
	}
	if (sampleBank)
	{
		if (isExpanded)
		{
			sampleBank->setBounds(area);
		}
	}
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
	o->setProperty("isExpanded", (bool)isExpanded);

	if (promptBank)
		o->setProperty("promptBankState", promptBank->saveUIState(promptBank->getSortType()));
	if (sampleBank)
		o->setProperty("sampleBankState", sampleBank->saveUIState(sampleBank->getSortType()));

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
	{
		promptBank->restoreUIState(
		    o->getProperty("promptBankState"), [this]() { promptBank->refreshList(); }, PromptBankPanel::firstSort,
		    PromptBankPanel::lastSort);
	}
	if (sampleBank)
		sampleBank->restoreUIState(
		    o->getProperty("sampleBankState"), [this]() { sampleBank->refreshSampleList(); },
		    SampleBankPanel::firstSort, SampleBankPanel::lastSort,
		    [this](ObsidianAccordion *acc, const juce::String &name)
		    { sampleBank->ensureAccordionItemsCreated(acc, name); });

	if (o->hasProperty("isExpanded"))
	{
		collapseExpand((bool)o->getProperty("isExpanded"));
		collapseButton.setToggleState(isExpanded, juce::dontSendNotification);
	}
	else
		isExpanded = true;
}