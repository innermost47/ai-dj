#include "PromptBankPanel.h"
#include "BankCategoryOperations.h"
#include "BasePanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

PromptBankPanel::PromptBankPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : BasePanel(processor), editor(editor)
{
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
	addAndMakeVisible(header);

	header.setTitle("PROMPT BANK");
	header.setHelpText("Drag a prompt onto a track to assign it. Double-click or right-click to edit.\n"
	                   "Categories are shared with the Sample Bank - renaming or deleting affects both.");

	header.setSearchEnabled(true);
	header.setSearchPlaceholder("Search prompts...");

	header.setSortOptions({{Recent, "Sort: Recent"},
	                       {Alphabetical, "Sort: Alphabetical"},
	                       {MostUsed, "Sort: Most Used"},
	                       {Model, "Sort: Model"}},
	                      Recent);

	header.addPrimaryButton("New Prompt", ColourPalette::violet, [this]() { addPromptDialog(); });
	header.addPrimaryButton("New Category", ColourPalette::slate, [this]() { addCategoryDialog(); });

	header.setShowExpandCollapseButtons(true);

	header.onSearchChanged = [this](const juce::String &q)
	{
		currentSearch = q;
		applyFilterAndSort();
		rebuildAccordions();
	};

	header.onSortChanged = [this](int id)
	{
		currentSortType = static_cast<SortType>(id);
		applyFilterAndSort();
		rebuildAccordions();
	};

	header.onExpandAllRequested = [this]() { expandAll(); };
	header.onCollapseAllRequested = [this]() { collapseAll(); };

	addAndMakeVisible(accordionViewport);
	accordionViewport.setViewedComponent(&accordionContainer, false);
	accordionViewport.setScrollBarsShown(true, false);
	accordionViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
}

void PromptBankPanel::resized()
{
	auto area = getLocalBounds();

	area.removeFromBottom(ObsidianSizes::GAP_XL);

	header.setBounds(area.removeFromTop(header.getPreferredHeight()));

	area.removeFromTop(ObsidianSizes::GAP);
	accordionViewport.setBounds(area);

	int containerWidth = accordionViewport.getWidth() - 12;
	int totalHeight = 0;
	for (auto &acc : accordions)
		totalHeight += acc->getPreferredHeight() + ObsidianSizes::SPACER;

	accordionContainer.setSize(containerWidth, juce::jmax(totalHeight, area.getHeight()));

	int y = 0;
	for (auto &acc : accordions)
	{
		int h = acc->getPreferredHeight();
		acc->setBounds(0, y, containerWidth, h);
		y += h + ObsidianSizes::SPACER_XS;
	}
}

void PromptBankPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);
	g.setColour(ColourPalette::backgroundDeep.withAlpha(ObsidianShades::ALPHA_08));
	g.fillRoundedRectangle(accordionViewport.getBounds().toFloat(), ObsidianSizes::LIST_PANEL_CORNER_SIZE);

	g.setColour(ColourPalette::backgroundLight.withAlpha(ObsidianShades::LIGHT_BORDER));
	g.drawRoundedRectangle(accordionViewport.getBounds().toFloat(), ObsidianSizes::LIST_PANEL_CORNER_SIZE,
	                       ObsidianSizes::BORDER_WIDTH);

	if (accordions.empty() || filteredPrompts.empty())
	{
		auto iconSvg = juce::Drawable::createFromImageData(BinaryData::musicnotes_svg, BinaryData::musicnotes_svgSize);
		juce::String noItemYet = "No prompts yet";
		juce::String tip = "Add your first prompt to get started!";
		juce::String noMatch = "No prompts match ";
		drawEmptyState(g, *iconSvg, noItemYet, tip, noMatch);
	}
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

	switch (currentSortType)
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

	for (const auto &c : bank->getCategories())
	{
		if (byCategory.find(c.name) == byCategory.end())
			byCategory[c.name] = {};
	}

	std::vector<juce::String> categoryOrder;
	for (const auto &pair : byCategory)
		if (pair.first != "Uncategorized")
			categoryOrder.push_back(pair.first);

	if (currentSearch.isNotEmpty())
	{
		categoryOrder.erase(std::remove_if(categoryOrder.begin(), categoryOrder.end(),
		                                   [this, &byCategory](const juce::String &catName)
		                                   { return byCategory[catName].empty(); }),
		                    categoryOrder.end());
	}

	std::sort(categoryOrder.begin(), categoryOrder.end(),
	          [](const juce::String &a, const juce::String &b) { return a.compareIgnoreCase(b) < 0; });

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
			item->setCategoryColourResolver([this](const juce::String &n) { return resolveCategoryColour(n); });

			item->onItemClicked = [this, entry]() { onPromptClicked(entry); };
			item->onItemDoubleClicked = [this, entry]() { onPromptEditRequested(entry); };

			item->onEditRequested = [this, entry]() { onPromptEditRequested(entry); };
			item->onDeleteRequested = [this, entry]() { onPromptDeleteRequested(entry); };

			items.push_back(std::move(item));
		}
		accordion->setItems(std::move(items));

		bool shouldExpand = autoExpandOnSearch || (openCategories.count(catName) > 0);
		accordion->setExpanded(shouldExpand, false);
		header.setExpanded(shouldExpand);

		juce::String catNameCopy = catName;

		accordion->onRenameRequested = [this, catNameCopy](const juce::String &newName)
		{
			juce::Colour currentColour = resolveCategoryColour(catNameCopy);
			BankCategoryOperations::renameCategory(audioProcessor, catNameCopy, newName, currentColour);

			if (openCategories.count(catNameCopy) > 0)
			{
				openCategories.erase(catNameCopy);
				openCategories.insert(newName);
			}
			refreshList();
		};

		accordion->onEditRequested = [this, catNameCopy]() { editCategoryDialog(catNameCopy); };
		accordion->onDeleteRequested = [this, catNameCopy]() { deleteCategoryDialog(catNameCopy); };

		juce::String catCopy = catName;
		accordion->onExpansionChanged = [this, catCopy](bool expanded) { onAccordionExpanded(catCopy, expanded); };

		accordionContainer.addAndMakeVisible(*accordion);
		accordions.push_back(std::move(accordion));
	}
	header.setChildNum(accordionContainer.getNumChildComponents());
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
				item->setSelected(item->getEntry() == entry);
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

void PromptBankPanel::deleteCategoryDialog(const juce::String &categoryName)
{
	BankCategoryOperations::promptDeleteCategoryWithDialog(audioProcessor, this, categoryName,
	                                                       [this, categoryName]()
	                                                       {
		                                                       openCategories.erase(categoryName);
		                                                       refreshList();
	                                                       });
}

void PromptBankPanel::editCategoryDialog(const juce::String &categoryName)
{
	juce::Colour currentColour = resolveCategoryColour(categoryName);

	BankCategoryOperations::promptEditCategoryWithDialog(audioProcessor, this, categoryName, currentColour,
	                                                     [this](const BankCategoryOperations::EditResult &res)
	                                                     {
		                                                     if (res.wasRenamed)
			                                                     transferOpenCategoryState(res.oldName, res.newName);
		                                                     refreshList();
	                                                     });
}
