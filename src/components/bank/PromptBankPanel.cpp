#include "PromptBankPanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

ScaleAndDurationPanel::ScaleAndDurationPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
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
	keySelector.setTooltip("Select musical key and mode for generation");

	addAndMakeVisible(titleLabel);
	titleLabel.setText("Choose your key and duration", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_REGULAR));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);
	titleLabel.setJustificationType(juce::Justification::centredLeft);

	addAndMakeVisible(helpLabel);
	helpLabel.setText("These settings apply to every generation.\nThey will be saved with the project.",
	                  juce::dontSendNotification);
	helpLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO));
	helpLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	helpLabel.setJustificationType(juce::Justification::topLeft);

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
}

void ScaleAndDurationPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);
}

void ScaleAndDurationPanel::resized()
{
	auto area = getLocalBounds();
	area.removeFromBottom(6);
	titleLabel.setBounds(area.removeFromTop(22));
	area.removeFromTop(2);
	helpLabel.setBounds(area.removeFromTop(32));
	area.removeFromTop(8);

	int selectorWidth = (int)(area.getWidth() * 0.65);
	keySelector.setBounds(area.removeFromLeft(selectorWidth));
	area.removeFromLeft(4);
	durationSelector.setBounds(area);
}

void ScaleAndDurationPanel::update()
{
	int currentDur = juce::roundToInt(audioProcessor.getGlobalDuration());
	durationSelector.setSelectedId(currentDur, juce::dontSendNotification);
	if (durationSelector.getSelectedId() == 0)
		durationSelector.setSelectedId(6, juce::dontSendNotification);

	keySelector.setText(audioProcessor.getGlobalKey(), juce::dontSendNotification);
}

PromptBankPanel::PromptBankPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	scaleAndDurationPanel = std::make_unique<ScaleAndDurationPanel>(processor);

	setupUI();

	if (auto *bank = audioProcessor.getPromptBank())
		bank->onBankChanged = [this]() { juce::MessageManager::callAsync([this]() { refreshList(); }); };

	refreshList();
}

PromptBankPanel::~PromptBankPanel()
{
	if (auto *bank = audioProcessor.getPromptBank())
		bank->onBankChanged = nullptr;
}

void PromptBankPanel::setupUI()
{
	addAndMakeVisible(titleLabel);
	titleLabel.setText("PROMPT BANK", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_TITLE));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);

	addAndMakeVisible(helpLabel);
	helpLabel.setText("Drag a prompt onto a track to assign both the prompt and its model.",
	                  juce::dontSendNotification);
	helpLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));
	helpLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	helpLabel.setJustificationType(juce::Justification::topLeft);

	addAndMakeVisible(searchInput);
	searchInput.setTextToShowWhenEmpty("Search prompts...", ColourPalette::textSecondary);
	searchInput.onTextChange = [this]()
	{
		currentSearch = searchInput.getText();
		applyFilterAndSort();
		rebuildAccordions();
	};

	addAndMakeVisible(sortMenu);
	sortMenu.addItem("Sort: Recent", Recent);
	sortMenu.addItem("Sort: Alphabetical", Alphabetical);
	sortMenu.addItem("Sort: Most Used", MostUsed);
	sortMenu.addItem("Sort: Model", Model);
	sortMenu.setSelectedId(Recent);
	sortMenu.onChange = [this]()
	{
		currentSort = static_cast<SortType>(sortMenu.getSelectedId());
		applyFilterAndSort();
		rebuildAccordions();
	};

	addAndMakeVisible(addCategoryButton);
	addCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
	addCategoryButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addCategoryButton.onClick = [this]() { addCategoryDialog(); };

	addAndMakeVisible(addPromptButton);
	addPromptButton.setColour(juce::TextButton::buttonColourId, ColourPalette::violet);
	addPromptButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addPromptButton.onClick = [this]() { addPromptDialog(); };

	addAndMakeVisible(expandAllButton);
	expandAllButton.loadIcon(BinaryData::expand_svg, BinaryData::expand_svgSize);
	expandAllButton.setCompactMode(true);
	expandAllButton.onClick = [this]() { expandAll(); };

	addAndMakeVisible(collapseAllButton);
	collapseAllButton.loadIcon(BinaryData::collapse_svg, BinaryData::collapse_svgSize);
	collapseAllButton.setCompactMode(true);
	collapseAllButton.onClick = [this]() { collapseAll(); };

	addAndMakeVisible(accordionViewport);
	accordionViewport.setViewedComponent(&accordionContainer, false);
	accordionViewport.setScrollBarsShown(true, false);
	accordionViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

	addAndMakeVisible(scaleAndDurationPanel.get());
}

void PromptBankPanel::resized()
{
	auto area = getLocalBounds();

	scaleAndDurationPanel->setBounds(area.removeFromBottom(ObsidianSizes::SCALE_AND_DURATION_HEIGHT));

	titleLabel.setBounds(area.removeFromTop(28));
	area.removeFromTop(4);
	helpLabel.setBounds(area.removeFromTop(32));
	area.removeFromTop(8);

	searchInput.setBounds(area.removeFromBottom(24).reduced(0, 2));
	area.removeFromBottom(4);
	sortMenu.setBounds(area.removeFromBottom(24).reduced(0, 2));
	area.removeFromBottom(4);

	auto createRow = area.removeFromBottom(28).reduced(0, 2);
	const int halfW = (createRow.getWidth() - 4) / 2;
	addPromptButton.setBounds(createRow.removeFromLeft(halfW));
	createRow.removeFromLeft(4);
	addCategoryButton.setBounds(createRow);

	area.removeFromBottom(6);

	auto expandRow = area.removeFromBottom(22);
	const int eW = expandRow.getWidth() / 2 - 2;
	expandAllButton.setBounds(expandRow.removeFromLeft(eW));
	expandRow.removeFromLeft(4);
	collapseAllButton.setBounds(expandRow);

	area.removeFromBottom(6);

	accordionViewport.setBounds(area);

	int containerWidth = accordionViewport.getWidth() - 12;
	int totalHeight = 0;
	for (auto &acc : accordions)
		totalHeight += acc->getPreferredHeight() + 4;

	accordionContainer.setSize(containerWidth, juce::jmax(totalHeight, area.getHeight()));

	int y = 0;
	for (auto &acc : accordions)
	{
		int h = acc->getPreferredHeight();
		acc->setBounds(0, y, containerWidth, h);
		y += h + 4;
	}
}

void PromptBankPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);
	g.setColour(ColourPalette::backgroundDeep.withAlpha(ObsidianShades::ALPHA_08));
	g.fillRoundedRectangle(accordionViewport.getBounds().toFloat(), ObsidianSizes::LIST_PANEL_CORNER_SIZE);

	g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
	g.drawRoundedRectangle(accordionViewport.getBounds().toFloat(), ObsidianSizes::LIST_PANEL_CORNER_SIZE, 1);
}

void PromptBankPanel::updateFromProcessor()
{
	scaleAndDurationPanel->update();
}

void PromptBankPanel::refreshList()
{
	applyFilterAndSort();
	rebuildAccordions();
	editor.uiPresetManager->notifyTracksPromptUpdate();
}

void PromptBankPanel::applyFilterAndSort()
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
	{
		filteredPrompts.clear();
		return;
	}

	auto all = bank->getAllPrompts();

	if (currentSearch.isNotEmpty())
	{
		auto needle = currentSearch.toLowerCase();
		all.erase(std::remove_if(all.begin(), all.end(),
		                         [&needle](PromptBankEntry *e)
		                         {
			                         return !e->text.toLowerCase().contains(needle) &&
			                                !e->modelName.toLowerCase().contains(needle) &&
			                                !e->category.toLowerCase().contains(needle);
		                         }),
		          all.end());
	}

	switch (currentSort)
	{
	case Recent:
		std::sort(all.begin(), all.end(), [](auto *a, auto *b) { return a->creationTime > b->creationTime; });
		break;
	case Alphabetical:
		std::sort(all.begin(), all.end(), [](auto *a, auto *b) { return a->text.compareIgnoreCase(b->text) < 0; });
		break;
	case MostUsed:
		std::sort(all.begin(), all.end(), [](auto *a, auto *b) { return a->usageCount > b->usageCount; });
		break;
	case Model:
		std::sort(all.begin(), all.end(),
		          [](auto *a, auto *b) { return a->modelName.compareIgnoreCase(b->modelName) < 0; });
		break;
	}

	filteredPrompts = all;
}

void PromptBankPanel::addPromptDialog()
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	juce::StringArray availCats;
	for (const auto &c : bank->getCategories())
		availCats.add(c.name);

	ObsidianAlertManager::showPromptEditor(this, "", "stable-audio-open-1.0", "", availCats,
	                                       [this](const ObsidianAlertManager::PromptEditorResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;
		                                       if (auto *bank = audioProcessor.getPromptBank())
		                                       {
			                                       bank->addPrompt(res.text, res.modelName, res.category);
			                                       refreshList();
		                                       }
	                                       });
}

void PromptBankPanel::rebuildAccordions()
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	accordionContainer.removeAllChildren();
	accordions.clear();

	std::map<juce::String, std::vector<PromptBankEntry *>> byCategory;
	for (auto *p : filteredPrompts)
	{
		juce::String cat = p->category.isEmpty() ? "Uncategorized" : p->category;
		byCategory[cat].push_back(p);
	}

	std::vector<juce::String> categoryOrder;
	for (const auto &c : bank->getCategories())
	{
		categoryOrder.push_back(c.name);
		if (byCategory.find(c.name) == byCategory.end())
			byCategory[c.name] = {};
	}
	if (byCategory.count("Uncategorized") > 0)
		categoryOrder.push_back("Uncategorized");

	bool autoExpandOnSearch = currentSearch.isNotEmpty();

	for (const auto &catName : categoryOrder)
	{
		juce::Colour colour =
		    catName == "Uncategorized" ? ColourPalette::backgroundLight : resolveCategoryColour(catName);

		auto accordion = std::make_unique<PromptCategoryAccordion>(catName, colour);

		std::vector<std::unique_ptr<PromptBankItem>> items;
		for (auto *entry : byCategory[catName])
		{
			auto item = std::make_unique<PromptBankItem>(entry);
			item->categoryColourResolver = [this](const juce::String &n) { return resolveCategoryColour(n); };
			item->onItemClicked = [this](PromptBankEntry *e) { onPromptClicked(e); };
			item->onEditRequested = [this](PromptBankEntry *e) { onPromptEditRequested(e); };
			item->onDeleteRequested = [this](PromptBankEntry *e) { onPromptDeleteRequested(e); };
			items.push_back(std::move(item));
		}
		accordion->setItems(std::move(items));

		bool shouldExpand = autoExpandOnSearch || (openCategories.count(catName) > 0);
		accordion->setExpanded(shouldExpand, false);

		bool editable = false;
		if (catName != "Uncategorized")
		{
			for (const auto &c : bank->getCategories())
				if (c.name == catName && !c.isBuiltIn)
				{
					editable = true;
					break;
				}
		}
		accordion->setEditable(editable);

		if (editable)
		{
			juce::String catNameCopy = catName;
			accordion->onEditRequested = [this, catNameCopy]() { editCategoryDialog(catNameCopy); };
			accordion->onDeleteRequested = [this, catNameCopy]() { deleteCategoryDialog(catNameCopy); };
		}

		juce::String catCopy = catName;
		accordion->onExpansionChanged = [this, catCopy](bool expanded) { onAccordionExpanded(catCopy, expanded); };

		accordionContainer.addAndMakeVisible(*accordion);
		accordions.push_back(std::move(accordion));
	}

	resized();
}

void PromptBankPanel::onAccordionExpanded(const juce::String &categoryName, bool expanded)
{
	if (expanded)
		openCategories.insert(categoryName);
	else
		openCategories.erase(categoryName);
	resized();
}

void PromptBankPanel::onPromptClicked(PromptBankEntry *entry)
{
	selectedEntry = entry;

	for (auto &acc : accordions)
		for (int i = 0; i < acc->getNumChildComponents(); ++i)
			if (auto *item = dynamic_cast<PromptBankItem *>(acc->getChildComponent(i)))
				item->setSelected(item->getPromptEntry() == entry);
}

void PromptBankPanel::onPromptEditRequested(PromptBankEntry *entry)
{
	if (!entry)
		return;

	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	juce::StringArray availCats;
	for (const auto &c : bank->getCategories())
		availCats.add(c.name);

	juce::String entryId = entry->id;

	ObsidianAlertManager::showPromptEditor(this, entry->text, entry->modelName, entry->category, availCats,
	                                       [this, entryId](const ObsidianAlertManager::PromptEditorResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;
		                                       if (auto *bank = audioProcessor.getPromptBank())
		                                       {
			                                       bank->updatePrompt(entryId, res.text, res.modelName, res.category);
			                                       refreshList();
		                                       }
	                                       });
}

void PromptBankPanel::onPromptDeleteRequested(PromptBankEntry *entry)
{
	if (!entry)
		return;

	juce::String id = entry->id;
	juce::String text = entry->text;

	ObsidianAlertManager::showConfirm(this, "Delete Prompt", "Delete this prompt?\n\n\"" + text + "\"", "Delete",
	                                  "Cancel",
	                                  [this, id](bool ok)
	                                  {
		                                  if (!ok)
			                                  return;
		                                  if (auto *bank = audioProcessor.getPromptBank())
			                                  bank->removePrompt(id);
	                                  });
}

void PromptBankPanel::expandAll()
{
	openCategories.clear();
	for (auto &acc : accordions)
	{
		acc->setExpanded(true, false);
		openCategories.insert(acc->getCategoryName());
	}
	resized();
}

void PromptBankPanel::collapseAll()
{
	openCategories.clear();
	for (auto &acc : accordions)
		acc->setExpanded(false, false);
	resized();
}

juce::Colour PromptBankPanel::resolveCategoryColour(const juce::String &name) const
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return ColourPalette::backgroundLight;

	for (const auto &c : bank->getCategories())
		if (c.name == name)
			return c.colour != juce::Colour(0) ? c.colour : ColourPalette::backgroundLight;

	return ColourPalette::backgroundLight;
}

void PromptBankPanel::addCategoryDialog()
{
	ObsidianAlertManager::showAddCategoryDialog(this,
	                                            [this](const juce::String &name, juce::Colour colour)
	                                            {
		                                            if (name.isEmpty())
			                                            return;
		                                            if (auto *bank = audioProcessor.getPromptBank())
		                                            {
			                                            bank->addCategory(name, colour);
			                                            refreshList();
		                                            }
	                                            });
}

void PromptBankPanel::editCategoryDialog(const juce::String &categoryName)
{
	juce::Colour oldColour = resolveCategoryColour(categoryName);

	ObsidianAlertManager::showEditCategoryDialog(
	    this, categoryName, oldColour,
	    [this, categoryName](const juce::String &newName, juce::Colour newColour)
	    {
		    if (auto *bank = audioProcessor.getPromptBank())
		    {
			    bank->renameCategory(categoryName, newName, newColour);
			    refreshList();
		    }
	    });
}

void PromptBankPanel::deleteCategoryDialog(const juce::String &categoryName)
{
	ObsidianAlertManager::showConfirm(this, "Delete Category",
	                                  "Delete '" + categoryName + "'? Prompts in it will become Uncategorized.",
	                                  "Delete", "Cancel",
	                                  [this, categoryName](bool ok)
	                                  {
		                                  if (!ok)
			                                  return;
		                                  if (auto *bank = audioProcessor.getPromptBank())
		                                  {
			                                  bank->removeCategory(categoryName);
			                                  refreshList();
		                                  }
	                                  });
}
juce::var PromptBankPanel::saveUIState() const
{
	juce::DynamicObject::Ptr o = new juce::DynamicObject();
	juce::Array<juce::var> openArr;
	for (const auto &cat : openCategories)
		openArr.add(juce::var(cat));
	o->setProperty("openCategories", juce::var(openArr));
	o->setProperty("sort", (int)currentSort);
	return juce::var(o.get());
}

void PromptBankPanel::restoreUIState(const juce::var &state)
{
	if (!state.isObject())
		return;
	auto *o = state.getDynamicObject();
	if (!o)
		return;

	openCategories.clear();
	auto arr = o->getProperty("openCategories");
	if (arr.isArray())
		for (int i = 0; i < arr.getArray()->size(); ++i)
			openCategories.insert(arr.getArray()->getUnchecked(i).toString());

	int s = (int)o->getProperty("sort");
	if (s >= Recent && s <= Model)
	{
		currentSort = (SortType)s;
		sortMenu.setSelectedId(s, juce::dontSendNotification);
	}

	refreshList();

	juce::MessageManager::callAsync(
	    [safe = juce::Component::SafePointer(this)]()
	    {
		    if (safe)
			    safe->resized();
	    });
}