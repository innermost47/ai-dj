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
	{
		juce::Component::SafePointer<PromptBankPanel> safeThis(this);
		bank->onBankChanged = [safeThis]()
		{
			if (safeThis)
				juce::MessageManager::callAsync(
				    [safeThis]()
				    {
					    if (safeThis)
						    safeThis->refreshList();
				    });
		};
	}
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
		rebuildAccordions(true);
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
	area.removeFromBottom(Obsidian::GAP_XL);
	header.setBounds(area.removeFromTop(header.getPreferredHeight()));
	area.removeFromTop(Obsidian::GAP);
	accordionViewport.setBounds(area);

	int containerWidth = accordionViewport.getWidth() - 12;

	for (auto &acc : accordions)
		acc->setSize(containerWidth, acc->getHeight());

	int totalHeight = 0;
	for (auto &acc : accordions)
		totalHeight += acc->getPreferredHeight() + Obsidian::SPACER_XS;

	accordionContainer.setSize(containerWidth, juce::jmax(totalHeight, area.getHeight()));

	int y = 0;
	for (auto &acc : accordions)
	{
		int h = acc->getPreferredHeight();
		acc->setBounds(0, y, containerWidth, h);
		y += h + Obsidian::SPACER_XS;
	}
}

void PromptBankPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);
	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_08));
	g.fillRoundedRectangle(accordionViewport.getBounds().toFloat(), Obsidian::LIST_PANEL_CORNER_SIZE);

	g.setColour(ColourPalette::backgroundLight.withAlpha(Obsidian::LIGHT_BORDER));
	g.drawRoundedRectangle(accordionViewport.getBounds().toFloat(), Obsidian::LIST_PANEL_CORNER_SIZE,
	                       Obsidian::BORDER_WIDTH);

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
	juce::Component::SafePointer<PromptBankPanel> safeThis(this);
	juce::MessageManager::callAsync(
	    [safeThis]()
	    {
		    if (safeThis)
			    safeThis->scrollToSelected();
	    });
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

void PromptBankPanel::rebuildAccordions(bool autoExpandOnSort)
{
	if (filteredPrompts.empty())
	{
		header.setChildNum(0);
		header.setExpanded(false);
		return;
	}
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
	{
		return;
	}

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

	if (selectedId.isNotEmpty())
	{
		for (auto *entry : filteredPrompts)
		{
			if (entry->id == selectedId)
			{
				juce::String cat = entry->category.isEmpty() ? "Uncategorized" : entry->category;
				openCategories.insert(cat);
				break;
			}
		}
	}

	bool autoExpandOnSearch = currentSearch.isNotEmpty();
	bool shouldExpandIfAllAreOpened = true;
	for (const auto &catName : categoryOrder)
	{
		juce::Colour colour =
		    catName == "Uncategorized" ? ColourPalette::textSecondary : resolveCategoryColour(catName);

		auto accordion = std::make_unique<PromptCategoryAccordion>(catName, colour);

		std::vector<std::unique_ptr<PromptBankItem>> items;
		for (auto *entry : byCategory[catName])
		{
			auto item = std::make_unique<PromptBankItem>(entry);
			item->setCategoryColourResolver([this](const juce::String &n) { return resolveCategoryColour(n); });
			item->onItemClicked = [this, entry]() { onPromptClicked(entry); };
			item->onItemDoubleClicked = [this, entry]() { onPromptEditRequested(entry); };
			item->setSelected(entry->id == selectedId);
			item->onEditRequested = [this, entry]() { onPromptEditRequested(entry); };
			item->onDeleteRequested = [this, entry]() { onPromptDeleteRequested(entry); };
			items.push_back(std::move(item));
		}
		accordion->setItems(std::move(items));

		bool shouldExpand = autoExpandOnSort || autoExpandOnSearch || (openCategories.count(catName) > 0);
		if (!shouldExpand)
			shouldExpandIfAllAreOpened = false;
		accordion->setExpanded(shouldExpand, false);

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
	isExpanded = shouldExpandIfAllAreOpened;
	header.setExpanded(isExpanded);
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
	if (selectedEntry == entry)
		return;
	selectedEntry = entry;
	selectedId = entry->id;

	for (auto &acc : accordions)
		for (int i = 0; i < acc->getNumChildComponents(); ++i)
			if (auto *item = dynamic_cast<PromptBankItem *>(acc->getChildComponent(i)))
			{
				bool isSelected = item->getEntry() == entry;
				item->setSelected(isSelected);
			}
}

void PromptBankPanel::scrollToSelected()
{
	if (selectedId.isEmpty())
		return;

	PromptBankEntry *targetEntry = nullptr;
	for (auto *entry : filteredPrompts)
	{
		if (entry->id == selectedId)
		{
			targetEntry = entry;
			break;
		}
	}

	if (targetEntry == nullptr)
		return;

	juce::String categoryName = targetEntry->category.isEmpty() ? "Uncategorized" : targetEntry->category;

	ObsidianAccordion *targetAccordion = nullptr;
	for (auto &acc : accordions)
	{
		if (acc->getName() == categoryName)
		{
			targetAccordion = acc.get();
			break;
		}
	}

	if (targetAccordion == nullptr)
		return;

	if (!targetAccordion->isExpanded())
	{
		targetAccordion->setExpanded(true, false);
		openCategories.insert(categoryName);
		resized();
	}

	PromptBankItem *targetItem = nullptr;
	for (int i = 0; i < targetAccordion->getNumChildComponents(); ++i)
	{
		if (auto *item = dynamic_cast<PromptBankItem *>(targetAccordion->getChildComponent(i)))
		{
			if (item->getEntry() == targetEntry)
			{
				targetItem = item;
				break;
			}
		}
	}

	if (targetItem == nullptr)
		return;

	auto itemBoundsInContainer = accordionContainer.getLocalArea(targetItem, targetItem->getLocalBounds());

	int itemTop = itemBoundsInContainer.getY();
	int itemBottom = itemBoundsInContainer.getBottom();
	int viewportHeight = accordionViewport.getHeight();
	int currentScrollY = accordionViewport.getViewPositionY();

	if (itemTop < currentScrollY)
	{
		accordionViewport.setViewPosition(0, itemTop - Obsidian::GAP);
	}
	else if (itemBottom > currentScrollY + viewportHeight)
	{
		accordionViewport.setViewPosition(0, itemBottom - viewportHeight + Obsidian::GAP);
	}
}

void PromptBankPanel::addPromptDialog()
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return;

	juce::StringArray availCats;
	for (const auto &c : bank->getCategories())
		availCats.add(c.name);

	auto safeThis = juce::Component::SafePointer<PromptBankPanel>(this);

	juce::String modelName =
	    audioProcessor.getUseLocalModel() ? Obsidian::STABLE_AUDIO_OPEN_V3_MEDIUM : Obsidian::STABLE_AUDIO_OPEN_V1;

	ObsidianAlertManager::showPromptEditor(this, "", modelName, "", availCats,
	                                       [this, safeThis](const ObsidianAlertManager::PromptEditorResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;
		                                       if (auto *bank = audioProcessor.getPromptBank())
		                                       {
			                                       PromptBankEntry *newEntry =
			                                           bank->addPrompt(res.text, res.modelName, res.category);
			                                       selectedId = newEntry->id;
		                                       }
	                                       });
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

	auto safeThis = juce::Component::SafePointer<PromptBankPanel>(this);

	ObsidianAlertManager::showPromptEditor(
	    this, entry->text, entry->modelName, entry->category, availCats,
	    [this, entryId, safeThis, entry](const ObsidianAlertManager::PromptEditorResult &res)
	    {
		    if (!res.confirmed)
			    return;
		    if (auto *bank = audioProcessor.getPromptBank())
		    {
			    bank->updatePrompt(entryId, res.text, res.modelName, res.category);
			    selectedId = entryId;
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
