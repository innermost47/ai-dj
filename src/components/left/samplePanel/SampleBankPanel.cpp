#include "SampleBankPanel.h"
#include "BankCategoryOperations.h"
#include "BasePanel.h"
#include "DetailPanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SampleBank.h"
#include "SampleBankItem.h"

SampleBankPanel::SampleBankPanel(DjIaVstProcessor &processor) : BasePanel(processor)
{
	setupUI();

	if (auto *bank = audioProcessor.getSampleBank())
	{
		juce::Component::SafePointer<SampleBankPanel> safeThis(this);
		bank->onBankChanged = [safeThis]()
		{
			if (safeThis)
				juce::MessageManager::callAsync(
				    [safeThis]()
				    {
					    if (safeThis)
						    safeThis->refreshSampleList();
				    });
		};
	}
}

SampleBankPanel::~SampleBankPanel()
{
	stopTimer();
	stopPreview();
	if (auto *bank = audioProcessor.getSampleBank())
		bank->onBankChanged = nullptr;
}

void SampleBankPanel::setupUI()
{
	addAndMakeVisible(header);

	header.setTitle("SAMPLE BANK");
	header.setHelpText("Drag onto a track to assign. Ctrl+Drag to export to DAW.\n"
	                   "Double-click or right-click to edit, move category, or delete.");

	header.setSearchEnabled(true);
	header.setSearchPlaceholder("Search samples...");

	header.setSortOptions({{SortType::Time, "Sort: Recent"},
	                       {SortType::Prompt, "Sort: Prompt"},
	                       {SortType::Usage, "Sort: Usage"},
	                       {SortType::BPM, "Sort: BPM"},
	                       {SortType::Duration, "Sort: Duration"}},
	                      SortType::Time);

	header.addPrimaryButton("Clean unused", ColourPalette::buttonDangerDark, [this]() { cleanupUnusedSamples(); });
	header.addPrimaryButton("New Category", ColourPalette::slate, [this]() { addCategoryDialog(); });

	header.setShowExpandCollapseButtons(true);

	header.onSearchChanged = [this](const juce::String &q)
	{
		currentSearch = q;
		applyFiltersAndSort();
		rebuildAccordions();
	};

	header.onSortChanged = [this](int id)
	{
		currentSortType = static_cast<SortType>(id);
		applyFiltersAndSort();
		rebuildAccordions(true);
	};

	header.onExpandAllRequested = [this]()
	{
		expandAll([this](ObsidianAccordion *acc, const juce::String &name) { ensureAccordionItemsCreated(acc, name); });
	};
	header.onCollapseAllRequested = [this]() { collapseAll(); };

	addAndMakeVisible(accordionViewport);
	accordionViewport.setViewedComponent(&accordionContainer, false);
	accordionViewport.setScrollBarsShown(true, false);
	accordionViewport.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

	addAndMakeVisible(detailPanel);

	detailPanel.categoryColourResolver = [this](const juce::String &name) -> juce::Colour
	{ return resolveCategoryColour(name); };

	detailPanel.onPlayRequested = [this](SampleBankEntry *e) { playPreview(e); };
	detailPanel.onStopRequested = [this]() { stopPreview(); };
}

void SampleBankPanel::addCategoryDialog()
{
	ObsidianAlertManager::showAddCategoryDialog(this,
	                                            [this](const juce::String &name, juce::Colour colour)
	                                            {
		                                            if (name.isEmpty())
			                                            return;
		                                            if (auto *bank = audioProcessor.getPromptBank())
		                                            {
			                                            bank->addCategory(name, colour);
			                                            refreshSampleListSilent();
		                                            }
	                                            });
}

void SampleBankPanel::rebuildAccordions(bool autoExpandOnSort)
{
	if (filteredSamples.empty())
	{
		isExpanded = false;
		header.setChildNum(0);
		header.setExpanded(false);
		return;
	}
	auto *bank = audioProcessor.getSampleBank();
	auto *promptBank = audioProcessor.getPromptBank();
	if (bank == nullptr || promptBank == nullptr)
		return;

	accordionContainer.removeAllChildren();
	accordions.clear();
	samplesByCategory.clear();

	for (auto *s : filteredSamples)
	{
		const juce::String cat = s->category.isEmpty() ? "Uncategorized" : s->category;
		samplesByCategory[cat].push_back(s);
	}

	for (const auto &c : promptBank->getCategories())
	{
		if (samplesByCategory.find(c.name) == samplesByCategory.end())
			samplesByCategory[c.name] = {};
	}

	std::vector<juce::String> categoryOrder;
	for (const auto &pair : samplesByCategory)
		if (pair.first != "Uncategorized")
			categoryOrder.push_back(pair.first);

	if (currentSearch.isNotEmpty())
	{
		categoryOrder.erase(std::remove_if(categoryOrder.begin(), categoryOrder.end(),
		                                   [this](const juce::String &catName)
		                                   { return samplesByCategory[catName].empty(); }),
		                    categoryOrder.end());
	}

	std::sort(categoryOrder.begin(), categoryOrder.end(),
	          [](const juce::String &a, const juce::String &b) { return a.compareIgnoreCase(b) < 0; });

	if (samplesByCategory.count("Uncategorized") > 0)
		categoryOrder.push_back("Uncategorized");

	if (selectedId.isNotEmpty())
	{
		for (auto *entry : filteredSamples)
		{
			if (entry->id == selectedId)
			{
				juce::String cat = entry->category.isEmpty() ? "Uncategorized" : entry->category;
				openCategories.insert(cat);
				break;
			}
		}
	}

	const bool autoExpandOnSearch = currentSearch.isNotEmpty();
	bool shouldExpandIfAllAreOpened = true;
	for (const auto &catName : categoryOrder)
	{
		juce::Colour colour =
		    (catName == "Uncategorized") ? ColourPalette::textSecondary : resolveCategoryColour(catName);

		auto accordion = std::make_unique<ObsidianAccordion>(catName, colour);

		const bool shouldExpand = autoExpandOnSort || autoExpandOnSearch || (openCategories.count(catName) > 0);
		if (!shouldExpand)
			shouldExpandIfAllAreOpened = false;
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
			refreshSampleList();
		};

		accordion->onEditRequested = [this, catNameCopy]() { editCategoryDialog(catNameCopy); };
		accordion->onDeleteRequested = [this, catNameCopy]() { deleteCategoryDialog(catNameCopy); };

		juce::String catCopy = catName;
		ObsidianAccordion *accPtr = accordion.get();
		accordion->onExpansionChanged = [this, catCopy, accPtr](bool expanded)
		{
			onAccordionExpanded(catCopy, expanded);
			if (expanded)
			{
				ensureAccordionItemsCreated(accPtr, catCopy);
			}
		};
		accordion->setExpanded(shouldExpand, false);

		if (shouldExpand)
			ensureAccordionItemsCreated(accordion.get(), catName);

		accordionContainer.addAndMakeVisible(*accordion);
		accordions.push_back(std::move(accordion));
	}
	isExpanded = shouldExpandIfAllAreOpened;
	header.setExpanded(isExpanded);
	header.setChildNum(accordionContainer.getNumChildComponents());

	resized();
}

void SampleBankPanel::deleteCategoryDialog(const juce::String &categoryName)
{
	BankCategoryOperations::promptDeleteCategoryWithDialog(audioProcessor, this, categoryName,
	                                                       [this, categoryName]()
	                                                       {
		                                                       openCategories.erase(categoryName);
		                                                       refreshSampleList();
	                                                       });
}

void SampleBankPanel::editCategoryDialog(const juce::String &categoryName)
{
	juce::Colour currentColour = resolveCategoryColour(categoryName);

	BankCategoryOperations::promptEditCategoryWithDialog(audioProcessor, this, categoryName, currentColour,
	                                                     [this](const BankCategoryOperations::EditResult &res)
	                                                     {
		                                                     if (res.wasRenamed)
			                                                     transferOpenCategoryState(res.oldName, res.newName);
		                                                     refreshSampleList();
	                                                     });
}

void SampleBankPanel::ensureAccordionItemsCreated(ObsidianAccordion *accordion, const juce::String &categoryName)
{
	if (accordion == nullptr)
		return;

	if (!accordion->getItems().empty())
		return;

	auto it = samplesByCategory.find(categoryName);
	if (it == samplesByCategory.end())
		return;

	std::vector<std::unique_ptr<AccordionItem>> items;
	items.reserve(it->second.size());

	for (auto *entry : it->second)
	{
		auto item = std::make_unique<SampleBankItem>(entry, audioProcessor);

		item->setCategoryColourResolver([this](const juce::String &n) { return resolveCategoryColour(n); });

		item->setSelected(entry->id == selectedId);

		item->onItemClicked = [this, entry]() { onSampleClicked(entry); };
		item->onItemDoubleClicked = [this, entry]() { showEditPromptDialog(entry); };

		item->onPromptEditRequested = [this](SampleBankEntry *e) { showEditPromptDialog(e); };
		item->onChangeCategoryRequested = [this](SampleBankEntry *e) { showChangeCategoryDialog(e); };
		item->onSampleDeleteRequested = [this](SampleBankEntry *e) { onSampleDeleteRequested(e); };

		items.emplace_back(std::unique_ptr<AccordionItem>(item.release()));
	}

	accordion->setItems(std::move(items));

	resized();
}

void SampleBankPanel::onSampleDeleteRequested(SampleBankEntry *entry)
{
	if (entry != nullptr)
		showDeleteConfirmation(entry->id, entry->originalPrompt);
}

void SampleBankPanel::onSampleClicked(SampleBankEntry *entry)
{
	if (selectedEntry == entry)
		return;
	else
		selectEntry(entry);
}

void SampleBankPanel::showChangeCategoryDialog(SampleBankEntry *entry)
{
	if (entry == nullptr)
		return;

	auto *promptBank = audioProcessor.getPromptBank();
	if (promptBank == nullptr)
		return;

	std::vector<juce::String> available;
	for (const auto &c : promptBank->getCategories())
		available.push_back(c.name);

	juce::String sampleId = entry->id;

	ObsidianAlertManager::showCategoryEditor(this, entry->originalPrompt, entry->category, available,
	                                         [this, sampleId](const juce::String &chosenCategory)
	                                         {
		                                         if (auto *bank = audioProcessor.getSampleBank())
		                                         {
			                                         if (auto *s = bank->getSample(sampleId))
			                                         {
				                                         s->category = chosenCategory;
				                                         bank->saveBankData();
				                                         selectedId = sampleId;
				                                         refreshSampleListSilent();
			                                         }
		                                         }
	                                         });
}

void SampleBankPanel::resized()
{
	auto area = getLocalBounds();
	int removeBot = Obsidian::SAMPLE_DETAIL_HEIGHT_VST;
	if (juce::JUCEApplicationBase::isStandaloneApp())
	{
		removeBot = Obsidian::SAMPLE_DETAIL_HEIGHT_STANDALONE;
	}

	auto detailPanelArea = area.removeFromBottom(removeBot);
	detailPanel.setBounds(detailPanelArea);

	header.setBounds(area.removeFromTop(header.getPreferredHeight()));
	area.removeFromTop(Obsidian::GAP);

	accordionViewport.setBounds(area);

	int containerWidth = accordionViewport.getWidth() - 12;
	int totalHeight = 0;
	for (auto &acc : accordions)
		totalHeight += acc->getPreferredHeight() + Obsidian::SPACER;

	accordionContainer.setSize(containerWidth, juce::jmax(totalHeight, area.getHeight()));

	int y = 0;
	for (auto &acc : accordions)
	{
		int h = acc->getPreferredHeight();
		acc->setBounds(0, y, containerWidth, h);
		y += h + Obsidian::SPACER_XS;
	}
}

void SampleBankPanel::selectEntry(SampleBankEntry *entry)
{
	if (currentPreviewEntry != nullptr)
		stopPreview();

	selectedEntry = entry;
	selectedId = entry->id;

	for (auto &acc : accordions)
	{
		for (const auto &item : acc->getItems())
		{
			if (auto *sampleItem = dynamic_cast<SampleBankItem *>(item.get()))
				sampleItem->setSelected(sampleItem->getSampleEntry() == entry);
		}
	}

	detailPanel.setEntry(entry);
	juce::Component::SafePointer<SampleBankPanel> safeThis(this);
	juce::MessageManager::callAsync(
	    [safeThis]()
	    {
		    if (safeThis)
			    safeThis->scrollToSelected();
	    });
}

void SampleBankPanel::onAccordionExpanded(const juce::String &categoryName, bool expanded)
{
	if (expanded)
		openCategories.insert(categoryName);
	else
		openCategories.erase(categoryName);
	resized();
}

void SampleBankPanel::playPreview(SampleBankEntry *entry)
{
	if (!entry)
		return;
	stopPreview();

	if (!audioProcessor.getAudioManager().previewSampleFromBank(entry->id))
		return;

	currentPreviewEntry = entry;
	detailPanel.setIsPlaying(true);
	startTimer(100);
}

void SampleBankPanel::stopPreview()
{
	audioProcessor.getAudioManager().stopSamplePreview();
	currentPreviewEntry = nullptr;
	detailPanel.setIsPlaying(false);
	stopTimer();
}

void SampleBankPanel::timerCallback()
{
	if (currentPreviewEntry != nullptr && !audioProcessor.getAudioManager().isSamplePreviewing())
		stopPreview();

	if (currentPreviewEntry == nullptr)
		stopTimer();
}

void SampleBankPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);

	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.8f));
	juce::Path listBg;
	auto lb = accordionViewport.getBounds().toFloat();
	listBg.addRoundedRectangle(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(), Obsidian::LIST_PANEL_CORNER_SIZE);
	g.fillPath(listBg);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
	g.drawRoundedRectangle(lb, Obsidian::CORNER, 1);

	if (filteredSamples.empty() && hasEverLoaded.load())
	{
		auto iconSvg = juce::Drawable::createFromImageData(BinaryData::musicnotes_svg, BinaryData::musicnotes_svgSize);
		juce::String noItemYet = "No samples yet";
		juce::String tip = "Generate some loops to populate your bank!";
		juce::String noMatch = "No samples match ";
		drawEmptyState(g, *iconSvg, noItemYet, tip, noMatch);
	}
}

void SampleBankPanel::refreshSampleList()
{
	auto *bank = audioProcessor.getSampleBank();
	if (bank == nullptr)
	{
		filteredSamples.clear();
		samplesByCategory.clear();
		rebuildAccordions();
		hasEverLoaded.store(true);
		repaint();
		return;
	}

	applyFiltersAndSort();
	rebuildAccordions();

	if (selectedEntry == nullptr && !filteredSamples.empty())
	{
		bool exists = false;
		if (selectedId.isNotEmpty())
		{
			for (const auto &item : filteredSamples)
			{
				if (item->id == selectedId)
				{
					selectEntry(item);
					exists = true;
					break;
				}
			}
		}
		if (!exists)
			selectEntry(filteredSamples[0]);
	}

	hasEverLoaded.store(true);
	repaint();
	juce::Timer::callAfterDelay(50,
	                            [safeThis = juce::Component::SafePointer<SampleBankPanel>(this)]()
	                            {
		                            if (safeThis)
			                            safeThis->scrollToSelected();
	                            });
}

void SampleBankPanel::refreshSampleListSilent()
{
	auto *bank = audioProcessor.getSampleBank();
	if (bank == nullptr)
	{
		filteredSamples.clear();
		samplesByCategory.clear();
		rebuildAccordions();
		repaint();
		return;
	}

	applyFiltersAndSort();

	if (selectedEntry != nullptr)
	{
		auto it = std::find(filteredSamples.begin(), filteredSamples.end(), selectedEntry);
		if (it == filteredSamples.end())
			selectEntry(filteredSamples.empty() ? nullptr : filteredSamples[0]);
		else
		{
			selectedId = (*it)->id;
			selectEntry(*it);
		}
	}

	rebuildAccordions();
	hasEverLoaded.store(true);
	repaint();
}

void SampleBankPanel::applyFiltersAndSort()
{
	auto *bank = audioProcessor.getSampleBank();
	if (bank == nullptr)
	{
		filteredSamples.clear();
		return;
	}

	auto samples = bank->getAllSamples();

	if (currentSearch.isNotEmpty())
	{
		auto needle = currentSearch.toLowerCase();
		samples.erase(std::remove_if(samples.begin(), samples.end(),
		                             [&needle](SampleBankEntry *e)
		                             {
			                             return !e->originalPrompt.toLowerCase().contains(needle) &&
			                                    !e->modelName.toLowerCase().contains(needle) &&
			                                    !e->category.toLowerCase().contains(needle) &&
			                                    !e->description.toLowerCase().contains(needle) &&
			                                    !e->key.toLowerCase().contains(needle);
		                             }),
		              samples.end());
	}

	switch (currentSortType)
	{
	case Time:
		std::sort(samples.begin(), samples.end(), [](auto *a, auto *b) { return a->creationTime > b->creationTime; });
		break;
	case Prompt:
		std::sort(samples.begin(), samples.end(),
		          [](auto *a, auto *b) { return a->originalPrompt.compareIgnoreCase(b->originalPrompt) < 0; });
		break;
	case Usage:
		std::sort(samples.begin(), samples.end(),
		          [](auto *a, auto *b) { return a->usedInProjects.size() > b->usedInProjects.size(); });
		break;
	case BPM:
		std::sort(samples.begin(), samples.end(), [](auto *a, auto *b) { return a->bpm > b->bpm; });
		break;
	case Duration:
		std::sort(samples.begin(), samples.end(), [](auto *a, auto *b) { return a->duration > b->duration; });
		break;
	}

	filteredSamples = samples;
}

void SampleBankPanel::setVisible(bool v)
{
	Component::setVisible(v);
	if (v)
	{
		hasEverLoaded.store(false);
		refreshSampleList();

		juce::Timer::callAfterDelay(50,
		                            [safeThis = juce::Component::SafePointer<SampleBankPanel>(this)]()
		                            {
			                            if (safeThis)
				                            safeThis->scrollToSelected();
		                            });
	}
	else
	{
		stopPreview();
		filteredSamples.clear();
		samplesByCategory.clear();
	}
}

void SampleBankPanel::deleteSample(const juce::String &id)
{
	if (currentPreviewEntry != nullptr && currentPreviewEntry->id == id)
		stopPreview();

	if (selectedEntry != nullptr && selectedEntry->id == id)
	{
		selectedEntry = nullptr;
		detailPanel.setEntry(nullptr);
	}

	if (auto *bank = audioProcessor.getSampleBank())
		if (bank->removeSample(id))
			refreshSampleList();
}

void SampleBankPanel::scrollToSelected()
{
	if (selectedId.isEmpty())
		return;

	SampleBankEntry *targetEntry = nullptr;
	for (auto *entry : filteredSamples)
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
		ensureAccordionItemsCreated(targetAccordion, categoryName);
		openCategories.insert(categoryName);
		resized();
	}

	SampleBankItem *targetItem = nullptr;
	for (const auto &item : targetAccordion->getItems())
	{
		if (auto *sampleItem = dynamic_cast<SampleBankItem *>(item.get()))
		{
			if (sampleItem->getSampleEntry() == targetEntry)
			{
				targetItem = sampleItem;
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

void SampleBankPanel::cleanupUnusedSamples()
{
	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
		return;
	auto unused = bank->getUnusedSamples();
	if (unused.empty())
	{
		ObsidianAlertManager::showInfo(this, "Clean Unused", "No unused samples.");
		return;
	}
	ObsidianAlertManager::showConfirm(
	    this, "Clean Unused", "Found " + juce::String((int)unused.size()) + " unused samples. Delete all?",
	    "Delete All", "Cancel",
	    [this, bank](bool ok)
	    {
		    if (!ok)
			    return;
		    int n = bank->removeUnusedSamples();
		    refreshSampleList();
		    ObsidianAlertManager::showInfo(this, "Done", "Removed " + juce::String(n) + " samples.");
	    });
}

void SampleBankPanel::showDeleteConfirmation(const juce::String &id, const juce::String &name)
{
	auto *e = audioProcessor.getSampleBank()->getSample(id);
	if (!e)
		return;

	juce::StringArray loadedOn;
	const juce::String samplePath = e->filePath;
	const juce::String sampleId = e->id;

	for (const auto &trackId : audioProcessor.getAllTrackIds())
	{
		auto *track = audioProcessor.getTrack(trackId);
		if (!track)
			continue;

		bool used = false;
		juce::String pageHit;

		if (!sampleId.isEmpty() && track->currentSampleId == sampleId)
			used = true;

		if (!used && !samplePath.isEmpty())
		{
			static const char pageLetters[] = {'A', 'B', 'C', 'D'};
			for (int p = 0; p < Obsidian::MAX_PAGES; ++p)
			{
				if (track->pages[p].audioFilePath == samplePath)
				{
					used = true;
					pageHit = juce::String(" (page ") + pageLetters[p] + ")";
					break;
				}
			}
		}

		if (used)
		{
			juce::String label = "Track " + juce::String(track->slotIndex + 1) + pageHit;
			loadedOn.add(label);
		}
	}

	if (!loadedOn.isEmpty())
	{
		ObsidianAlertManager::showError(this, "Cannot Delete",
		                                "'" + name + "' is currently loaded on:\n\n" + loadedOn.joinIntoString("\n") +
		                                    "\n\nUnload it from the track(s) before deleting.");
		return;
	}

	juce::String msg = "Delete '" + name + "'?";
	if (!e->usedInProjects.empty())
		msg += "\n\nUsed in " + juce::String((int)e->usedInProjects.size()) + " project(s).";
	ObsidianAlertManager::showConfirm(this, "Delete Sample", msg, "Delete", "Cancel",
	                                  [this, id](bool ok)
	                                  {
		                                  if (ok)
			                                  deleteSample(id);
	                                  });
}

void SampleBankPanel::showEditPromptDialog(SampleBankEntry *entry)
{
	if (entry == nullptr)
		return;

	auto *promptBank = audioProcessor.getPromptBank();
	juce::StringArray availCats;
	if (promptBank)
		for (const auto &c : promptBank->getCategories())
			availCats.add(c.name);

	juce::String sampleId = entry->id;

	ObsidianAlertManager::showPromptEditor(this, entry->originalPrompt, entry->modelName, entry->category, availCats,
	                                       [this, sampleId](const ObsidianAlertManager::PromptEditorResult &res)
	                                       {
		                                       if (!res.confirmed)
			                                       return;
		                                       auto *sampleBank = audioProcessor.getSampleBank();
		                                       if (!sampleBank)
			                                       return;
		                                       auto *entry = sampleBank->getSample(sampleId);
		                                       if (!entry)
			                                       return;
		                                       entry->originalPrompt = res.text;
		                                       entry->modelName = res.modelName;
		                                       entry->category = res.category;
		                                       sampleBank->saveBankData();
		                                       selectedId = sampleId;
		                                       selectedEntry = nullptr;
		                                       refreshSampleList();
	                                       });
}
