#include "SampleBankPanel.h"
#include "DetailPanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SampleBank.h"
#include "SampleBankItem.h"

SampleBankPanel::SampleBankPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	setupUI();

	if (auto *bank = audioProcessor.getSampleBank())
		bank->onBankChanged = [this]() { juce::MessageManager::callAsync([this]() { refreshSampleList(); }); };
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
		rebuildAccordions();
	};

	header.onExpandAllRequested = [this]() { expandAll(); };
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

void SampleBankPanel::rebuildAccordions()
{
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

	std::sort(categoryOrder.begin(), categoryOrder.end(),
	          [](const juce::String &a, const juce::String &b) { return a.compareIgnoreCase(b) < 0; });

	if (samplesByCategory.count("Uncategorized") > 0)
		categoryOrder.push_back("Uncategorized");

	const bool autoExpandOnSearch = currentSearch.isNotEmpty();

	for (const auto &catName : categoryOrder)
	{
		juce::Colour colour =
		    (catName == "Uncategorized") ? ColourPalette::backgroundLight : resolveCategoryColour(catName);

		auto accordion = std::make_unique<ObsidianAccordion>(catName, colour);

		const bool shouldExpand = autoExpandOnSearch || (openCategories.count(catName) > 0);

		juce::String catNameCopy = catName;
		accordion->onRenameRequested = [this, catNameCopy](const juce::String &newName)
		{
			if (auto *pb = audioProcessor.getPromptBank())
			{
				pb->renameCategory(catNameCopy, newName, resolveCategoryColour(catNameCopy));
				if (auto *bank = audioProcessor.getSampleBank())
				{
					for (auto *s : bank->getAllSamples())
						if (s->category == catNameCopy)
							s->category = newName;
					bank->saveBankData();
				}
				if (openCategories.count(catNameCopy) > 0)
				{
					openCategories.erase(catNameCopy);
					openCategories.insert(newName);
				}
				refreshSampleList();
			}
		};

		accordion->onDeleteRequested = [this, catNameCopy]()
		{
			ObsidianAlertManager::showConfirm(this, "Delete Category",
			                                  "Delete '" + catNameCopy + "'? Samples in it will become Uncategorized.",
			                                  "Delete", "Cancel",
			                                  [this, catNameCopy](bool ok)
			                                  {
				                                  if (!ok)
					                                  return;
				                                  if (auto *pb = audioProcessor.getPromptBank())
					                                  pb->removeCategory(catNameCopy);
				                                  if (auto *bank = audioProcessor.getSampleBank())
				                                  {
					                                  for (auto *s : bank->getAllSamples())
						                                  if (s->category == catNameCopy)
							                                  s->category.clear();
					                                  bank->saveBankData();
				                                  }
				                                  openCategories.erase(catNameCopy);
				                                  refreshSampleList();
			                                  });
		};

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

	resized();
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

		item->setSelected(selectedEntry == entry);

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
		selectEntry(nullptr);
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
				                                         refreshSampleListSilent();
			                                         }
		                                         }
	                                         });
}

void SampleBankPanel::resized()
{
	auto area = getLocalBounds();

	auto detailPanelArea = area.removeFromBottom(ObsidianSizes::SAMPLE_DETAIL_HEIGHT);
	detailPanel.setBounds(detailPanelArea);

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
		y += h + ObsidianSizes::SPACER;
	}
}

void SampleBankPanel::selectEntry(SampleBankEntry *entry)
{
	if (currentPreviewEntry != nullptr)
		stopPreview();

	selectedEntry = entry;

	for (auto &acc : accordions)
	{
		for (const auto &item : acc->getItems())
		{
			if (auto *sampleItem = dynamic_cast<SampleBankItem *>(item.get()))
				sampleItem->setSelected(sampleItem->getSampleEntry() == entry);
		}
	}

	detailPanel.setEntry(entry);
}

void SampleBankPanel::expandAll()
{
	openCategories.clear();
	for (auto &acc : accordions)
	{
		acc->setExpanded(true, false);
		ensureAccordionItemsCreated(acc.get(), acc->getName());
		openCategories.insert(acc->getName());
	}
	resized();
}

void SampleBankPanel::collapseAll()
{
	openCategories.clear();
	for (auto &acc : accordions)
		acc->setExpanded(false, false);
	resized();
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
	listBg.addRoundedRectangle(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(),
	                           ObsidianSizes::LIST_PANEL_CORNER_SIZE);
	g.fillPath(listBg);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
	g.drawRoundedRectangle(lb, ObsidianSizes::CORNER, 1);

	if (filteredSamples.empty() && hasEverLoaded.load())
		drawEmptyState(g);
}

void SampleBankPanel::drawEmptyState(juce::Graphics &g)
{
	auto b = accordionViewport.getBounds();

	auto iconBounds = b.withSizeKeepingCentre(64, 64).translated(0, -20);
	auto iconSvg = juce::Drawable::createFromImageData(BinaryData::musicnotes_svg, BinaryData::musicnotes_svgSize);
	if (iconSvg != nullptr)
	{
		iconSvg->replaceColour(juce::Colours::black, ColourPalette::textSecondary);
		iconSvg->drawWithin(g, iconBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
	}

	g.setColour(ColourPalette::textSecondary);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SUBTITLE, juce::Font::bold));
	g.drawText("No samples yet", b.withSizeKeepingCentre(300, 28).translated(0, 35), juce::Justification::centred);

	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));
	g.drawText("Generate some loops to populate your bank!", b.withSizeKeepingCentre(300, 28).translated(0, 60),
	           juce::Justification::centred);
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
		selectEntry(filteredSamples[0]);

	hasEverLoaded.store(true);
	repaint();
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
			for (int p = 0; p < ObsidianDataConst::MAX_PAGES; ++p)
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

juce::Colour SampleBankPanel::resolveCategoryColour(const juce::String &name) const
{
	auto *promptBank = audioProcessor.getPromptBank();
	if (promptBank == nullptr)
		return ColourPalette::backgroundLight;

	for (const auto &c : promptBank->getCategories())
		if (c.name == name)
			return c.colour != juce::Colour(0) ? c.colour : ColourPalette::backgroundLight;

	return ColourPalette::backgroundLight;
}

juce::var SampleBankPanel::saveUIState() const
{
	juce::DynamicObject::Ptr o = new juce::DynamicObject();

	juce::Array<juce::var> openArr;
	for (const auto &cat : openCategories)
		openArr.add(juce::var(cat));
	o->setProperty("openCategories", juce::var(openArr));

	o->setProperty("sort", (int)currentSortType);
	o->setProperty("search", currentSearch);

	return juce::var(o.get());
}

void SampleBankPanel::restoreUIState(const juce::var &state)
{
	if (!state.isObject())
		return;

	auto *o = state.getDynamicObject();
	if (o == nullptr)
		return;

	openCategories.clear();
	auto arr = o->getProperty("openCategories");
	if (arr.isArray())
		for (int i = 0; i < arr.getArray()->size(); ++i)
			openCategories.insert(arr.getArray()->getUnchecked(i).toString());

	int s = (int)o->getProperty("sort");
	if (s >= Time && s <= Duration)
	{
		currentSortType = (SortType)s;
		header.setSelectedSortId(s, false);
	}

	juce::String savedSearch = o->getProperty("search").toString();
	currentSearch = savedSearch;
	header.setSearchText(savedSearch, false);

	refreshSampleList();

	juce::MessageManager::callAsync(
	    [safe = juce::Component::SafePointer(this)]()
	    {
		    if (safe)
			    safe->resized();
	    });
}

void SampleBankPanel::showEditPromptDialog(SampleBankEntry *entry)
{
	if (entry == nullptr)
		return;

	juce::String entryId = entry->id;
	juce::String oldPrompt = entry->originalPrompt;

	ObsidianAlertManager::showEditPrompt(this, entry->originalPrompt,
	                                     [this, entryId, oldPrompt](const juce::String &newPrompt)
	                                     {
		                                     if (newPrompt.isEmpty() || newPrompt == oldPrompt)
			                                     return;

		                                     auto *bank = audioProcessor.getSampleBank();
		                                     if (bank == nullptr)
			                                     return;

		                                     auto *e = bank->getSample(entryId);
		                                     if (e == nullptr)
			                                     return;

		                                     e->originalPrompt = newPrompt;
		                                     bank->saveBankData();

		                                     audioProcessor.addCustomPrompt(newPrompt);

		                                     if (auto *editor =
		                                             dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			                                     editor->uiPresetManager->notifyTracksPromptUpdate();

		                                     refreshSampleListSilent();

		                                     selectedEntry = e;
		                                     detailPanel.setEntry(e);
	                                     });
}
