#include "SampleBankPanel.h"
#include "DetailPanel.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "SampleBank.h"
#include "SampleBankItem.h"

SampleBankPanel::SampleBankPanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
	categoryNames[SampleCategory::All] = "All Samples";
	categoryNames[SampleCategory::Drums] = "Drums";
	categoryNames[SampleCategory::Bass] = "Bass";
	categoryNames[SampleCategory::Melody] = "Melody";
	categoryNames[SampleCategory::Ambient] = "Ambient";
	categoryNames[SampleCategory::Percussion] = "Percussion";
	categoryNames[SampleCategory::Vocal] = "Vocal";
	categoryNames[SampleCategory::FX] = "FX";
	categoryNames[SampleCategory::Loop] = "Loops";
	categoryNames[SampleCategory::OneShot] = "One-shots";
	categoryNames[SampleCategory::House] = "House";
	categoryNames[SampleCategory::Techno] = "Techno";
	categoryNames[SampleCategory::HipHop] = "Hip-Hop";
	categoryNames[SampleCategory::Jazz] = "Jazz";
	categoryNames[SampleCategory::Rock] = "Rock";
	categoryNames[SampleCategory::Electronic] = "Electronic";
	categoryNames[SampleCategory::Piano] = "Piano";
	categoryNames[SampleCategory::Guitar] = "Guitar";
	categoryNames[SampleCategory::Synth] = "Synth";

	loadCategoriesConfig();
	setupUI();

	if (auto *bank = audioProcessor.getSampleBank())
	{
		bank->onBankChanged = [this]() { juce::MessageManager::callAsync([this]() { refreshSampleList(); }); };

		bank->onCheckCategoryExists = [this](const juce::String &name) -> bool
		{
			if (audioProcessor.getPromptBank() == nullptr)
				return false;
			for (const auto &c : audioProcessor.getPromptBank()->getCategories())
				if (c.name.compareIgnoreCase(name) == 0)
					return true;
			return false;
		};

		bank->onMigrateLegacyCategory = [this](const juce::String &name, juce::Colour colour)
		{
			if (audioProcessor.getPromptBank() == nullptr)
				return;
			audioProcessor.getPromptBank()->addCategory(name, colour);
		};

		bank->runLegacyCategoriesMigration();
	}
}

SampleBankPanel::~SampleBankPanel()
{
	stopTimer();
	stopPreview();
	sampleListBox.setVisible(false);
	sampleListBox.setModel(nullptr);
	if (auto *bank = audioProcessor.getSampleBank())
		bank->onBankChanged = nullptr;
}

void SampleBankPanel::setupUI()
{
	addAndMakeVisible(titleLabel);
	titleLabel.setText("SAMPLE BANK", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_TITLE));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);

	addAndMakeVisible(infoLabel);
	infoLabel.setText("Preview: ch.9 | Drag: drop on track | Ctrl+Drag: drop in DAW | Right-click: categories",
	                  juce::dontSendNotification);
	infoLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setJustificationType(juce::Justification::centredLeft);

	addAndMakeVisible(cleanupButton);
	cleanupButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	cleanupButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	cleanupButton.onClick = [this]() { cleanupUnusedSamples(); };

	addAndMakeVisible(sortMenu);
	sortMenu.addItem("Sort: Recent", SortType::Time);
	sortMenu.addItem("Sort: Prompt", SortType::Prompt);
	sortMenu.addItem("Sort: Usage", SortType::Usage);
	sortMenu.addItem("Sort: BPM", SortType::BPM);
	sortMenu.addItem("Sort: Duration", SortType::Duration);
	sortMenu.setSelectedId(SortType::Time);
	sortMenu.onChange = [this]()
	{
		currentSortType = static_cast<SortType>(sortMenu.getSelectedId());
		refreshSampleList();
	};

	addAndMakeVisible(sampleListBox);
	sampleListBox.setModel(this);
	sampleListBox.setRowHeight(ObsidianSizes::SAMPLE_ROW_HEIGHT);
	sampleListBox.setOutlineThickness(0);
	sampleListBox.getViewport()->setScrollBarsShown(true, false);
	sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

	addAndMakeVisible(categoryFilter);
	for (const auto &info : categoryInfos)
		categoryFilter.addItem(info.name, info.id + 1);
	categoryFilter.setSelectedId(1);
	categoryFilter.onChange = [this]()
	{
		int sel = categoryFilter.getSelectedId();
		currentCategoryId = sel > 0 ? sel - 1 : 0;
		editCategoryButton.setEnabled(isCategoryEditable(currentCategoryId));
		deleteCategoryButton.setEnabled(isCategoryEditable(currentCategoryId));
		refreshSampleList();
	};

	addAndMakeVisible(categoryInput);
	categoryInput.setTextToShowWhenEmpty("New category name...", ColourPalette::textSecondary);

	addAndMakeVisible(addCategoryButton);
	addCategoryButton.loadIcon(BinaryData::plus_svg, BinaryData::plus_svgSize);
	addCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::slate);
	addCategoryButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);

	addCategoryButton.onClick = [this]() { addCategory(); };

	addAndMakeVisible(editCategoryButton);
	editCategoryButton.loadIcon(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	editCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::indigo);
	editCategoryButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	editCategoryButton.onClick = [this]() { editCategory(); };
	editCategoryButton.setEnabled(false);

	addAndMakeVisible(deleteCategoryButton);
	deleteCategoryButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	deleteCategoryButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	deleteCategoryButton.onClick = [this]() { deleteCategory(); };
	deleteCategoryButton.setEnabled(false);

	deleteCategoryButton.setCompactMode(true);
	editCategoryButton.setCompactMode(true);
	addCategoryButton.setCompactMode(true);
	cleanupButton.setCompactMode(true);

	addAndMakeVisible(detailPanel);

	detailPanel.categoryColourResolver = [this](const juce::String &name) -> juce::Colour
	{ return resolveCategoryColour(name); };

	detailPanel.onPlayRequested = [this](SampleBankEntry *e) { playPreview(e); };
	detailPanel.onStopRequested = [this]() { stopPreview(); };
	detailPanel.onDeleteRequested = [this](const juce::String &id)
	{
		if (auto *e = audioProcessor.getSampleBank()->getSample(id))
			showDeleteConfirmation(id, e->originalPrompt);
	};
}

void SampleBankPanel::resized()
{
	auto area = getLocalBounds();

	auto detailPanelArea = area.removeFromBottom(ObsidianSizes::SAMPLE_DETAIL_HEIGHT);
	detailPanel.setBounds(detailPanelArea);

	auto hdr = area.removeFromTop(ObsidianSizes::TITLE_PANEL_HEIGHT);
	titleLabel.setBounds(hdr.removeFromLeft(160));
	infoLabel.setBounds(area.removeFromTop(ObsidianSizes::INFO_PANEL_HEIGHT));
	area.removeFromTop(ObsidianSizes::GAP_8);

	sortMenu.setBounds(area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT));
	area.removeFromTop(ObsidianSizes::GAP_4);
	categoryFilter.setBounds(area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT));
	area.removeFromTop(ObsidianSizes::GAP_4);

	auto btnRow = area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT);
	addCategoryButton.setBounds(btnRow.removeFromRight(28));
	btnRow.removeFromRight(2);
	deleteCategoryButton.setBounds(btnRow.removeFromRight(28));
	btnRow.removeFromRight(2);
	editCategoryButton.setBounds(btnRow.removeFromRight(28));
	btnRow.removeFromRight(4);
	categoryInput.setBounds(btnRow);
	area.removeFromTop(ObsidianSizes::GAP_4);

	cleanupButton.setBounds(area.removeFromTop(ObsidianSizes::COMBO_BOX_BASE_HEIGHT));
	area.removeFromTop(ObsidianSizes::GAP);

	sampleListBox.setBounds(area);
}

void SampleBankPanel::selectEntry(SampleBankEntry *entry)
{
	if (currentPreviewEntry != nullptr)
		stopPreview();
	selectedEntry = entry;
	sampleListBox.updateContent();
	detailPanel.setEntry(entry);
}

int SampleBankPanel::getNumRows()
{
	return (int)filteredSamples.size();
}

juce::Component *SampleBankPanel::refreshComponentForRow(int rowNumber, bool,
                                                         juce::Component *existingComponentToUpdate)
{
	auto *wrapper = dynamic_cast<SampleBankItemWrapper *>(existingComponentToUpdate);

	if (rowNumber < 0 || rowNumber >= (int)filteredSamples.size())
	{
		delete wrapper;
		return nullptr;
	}

	auto *entry = filteredSamples[rowNumber];
	SampleBankItem *item = nullptr;

	if (wrapper == nullptr || wrapper->getItem()->getSampleEntry() != entry)
	{
		delete wrapper;
		item = new SampleBankItem(entry, audioProcessor);
		wrapper = new SampleBankItemWrapper(item);
	}
	else
	{
		item = wrapper->getItem();
	}

	item->setSelected(selectedEntry == entry);

	item->onItemClicked = [this](SampleBankEntry *e)
	{
		if (selectedEntry == e)
			selectEntry(nullptr);
		else
			selectEntry(e);
	};

	item->onCategoryChanged = [this](SampleBankEntry *, const juce::String &)
	{
		if (auto *bank = audioProcessor.getSampleBank())
			bank->saveBankData();
		refreshSampleListSilent();
	};

	item->getCategoriesList = [this]() -> std::vector<juce::String>
	{
		std::vector<juce::String> cats;
		for (const auto &info : categoryInfos)
			if (info.id > 0)
				cats.push_back(info.name);
		return cats;
	};

	item->categoryColourResolver = [this](const juce::String &name) -> juce::Colour
	{ return resolveCategoryColour(name); };

	item->onPromptEditRequested = [this](SampleBankEntry *e) { showEditPromptDialog(e); };

	item->onDeleteRequested = [this](SampleBankEntry *e)
	{
		if (e)
			showDeleteConfirmation(e->id, e->originalPrompt);
	};

	return wrapper;
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
	if (isLoading.load())
	{
		loadingAngle += 0.15f;
		if (loadingAngle >= juce::MathConstants<float>::twoPi)
			loadingAngle -= juce::MathConstants<float>::twoPi;
		repaint();
	}

	if (currentPreviewEntry && !audioProcessor.getAudioManager().isSamplePreviewing())
		stopPreview();

	if (!isLoading.load() && !currentPreviewEntry)
		stopTimer();
}

void SampleBankPanel::paint(juce::Graphics &g)
{
	g.fillAll(ColourPalette::backgroundDark);

	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.8f));
	juce::Path listBg;
	auto lb = sampleListBox.getBounds().toFloat();
	listBg.addRoundedRectangle(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(),
	                           ObsidianSizes::LIST_PANEL_CORNER_SIZE);
	g.fillPath(listBg);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
	g.drawRoundedRectangle(lb, ObsidianSizes::CORNER, 1);

	if (isLoading.load())
		drawLoader(g);
	else if (filteredSamples.empty() && hasEverLoaded.load())
		drawEmptyState(g);
}

void SampleBankPanel::drawLoader(juce::Graphics &g)
{
	auto b = sampleListBox.getBounds().toFloat();
	float cx = b.getCentreX(), cy = b.getCentreY();
	const float r = 40.0f, t = 4.0f;

	juce::Path bg;
	bg.addCentredArc(cx, cy, r, r, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.strokePath(bg, juce::PathStrokeType(t));

	juce::Path arc;
	arc.addCentredArc(cx, cy, r, r, 0.0f, loadingAngle, loadingAngle + juce::MathConstants<float>::pi * 1.5f, true);
	g.setColour(ColourPalette::violet);
	g.strokePath(arc, juce::PathStrokeType(t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	g.setColour(ColourPalette::textSecondary);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SUBTITLE));
	g.drawText("Loading samples...", b.withSizeKeepingCentre(200, 30).translated(0, r + 30),
	           juce::Justification::centred);
}

void SampleBankPanel::drawEmptyState(juce::Graphics &g)
{
	auto b = sampleListBox.getBounds();
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.setFont(juce::FontOptions(48.0f));
	g.drawText(juce::String::fromUTF8("\xE2\x99\xAA"), b.withSizeKeepingCentre(60, 60), juce::Justification::centred);
	g.setColour(ColourPalette::textSecondary);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SUBTITLE, juce::Font::bold));
	g.drawText("No samples yet", b.withSizeKeepingCentre(300, 28).translated(0, 55), juce::Justification::centred);
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));
	g.drawText("Generate some loops to populate your bank!", b.withSizeKeepingCentre(300, 28).translated(0, 80),
	           juce::Justification::centred);
}

void SampleBankPanel::refreshSampleList()
{
	isLoading.store(true);
	startTimer(30);
	repaint();

	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
	{
		filteredSamples.clear();
		isLoading.store(false);
		hasEverLoaded.store(true);
		sampleListBox.updateContent();
		stopTimer();
		repaint();
		return;
	}

	applyFiltersAndSort();

	juce::Timer::callAfterDelay(600,
	                            [this, safe = juce::Component::SafePointer(this)]()
	                            {
		                            if (!safe)
			                            return;
		                            sampleListBox.updateContent();
		                            isLoading.store(false);
		                            hasEverLoaded.store(true);
		                            if (selectedEntry == nullptr && !filteredSamples.empty())
			                            selectEntry(filteredSamples[0]);
		                            if (!currentPreviewEntry)
			                            stopTimer();
		                            repaint();
	                            });
}

void SampleBankPanel::refreshSampleListSilent()
{
	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
	{
		filteredSamples.clear();
		sampleListBox.updateContent();
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

	sampleListBox.updateContent();
	hasEverLoaded.store(true);
	repaint();
}

void SampleBankPanel::applyFiltersAndSort()
{
	auto *bank = audioProcessor.getSampleBank();
	if (!bank)
	{
		filteredSamples.clear();
		return;
	}

	auto samples = bank->getAllSamples();

	if (currentCategoryId != 0)
	{
		juce::String catName;
		for (const auto &info : categoryInfos)
			if (info.id == currentCategoryId)
			{
				catName = info.name;
				break;
			}

		if (!catName.isEmpty())
			samples.erase(std::remove_if(samples.begin(), samples.end(),
			                             [&catName](const SampleBankEntry *e) { return e->category == catName; }),
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
		isLoading.store(true);
		startTimer(30);
		juce::Timer::callAfterDelay(100,
		                            [this, safe = juce::Component::SafePointer(this)]()
		                            {
			                            if (safe)
				                            refreshSampleList();
		                            });
	}
	else
	{
		stopPreview();
		isLoading.store(false);
		stopTimer();
		filteredSamples.clear();
		sampleListBox.updateContent();
	}
}

void SampleBankPanel::deleteSample(const juce::String &id)
{
	if (currentPreviewEntry && currentPreviewEntry->id == id)
		stopPreview();

	if (selectedEntry && selectedEntry->id == id)
	{
		selectedEntry = nullptr;
		detailPanel.setEntry(nullptr);
	}

	filteredSamples.clear();
	sampleListBox.updateContent();

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

void SampleBankPanel::addCategory()
{
	showAddCategoryDialog();
}

void SampleBankPanel::editCategory()
{
	if (!isCategoryEditable(currentCategoryId))
	{
		ObsidianAlertManager::showError(this, "Edit Category", "Cannot edit built-in categories.");
		return;
	}
	showEditCategoryDialog();
}

void SampleBankPanel::showAddCategoryDialog()
{
	ObsidianAlertManager::showAddCategoryDialog(this,
	                                            [this](const juce::String &name, juce::Colour colour)
	                                            {
		                                            for (const auto &info : categoryInfos)
			                                            if (info.name.compareIgnoreCase(name) == 0)
			                                            {
				                                            ObsidianAlertManager::showError(this, "Add Category",
				                                                                            "Already exists.");
				                                            return;
			                                            }
		                                            int id = std::max(20, getNextCategoryId());
		                                            categoryInfos.push_back({id, name, colour});
		                                            rebuildCategoryFilter();
		                                            categoryFilter.setSelectedId(id + 1);
		                                            currentCategoryId = id;
		                                            editCategoryButton.setEnabled(true);
		                                            deleteCategoryButton.setEnabled(true);
		                                            categoryInput.clear();
		                                            saveCategoriesConfig();
		                                            refreshSampleList();
	                                            });
}

void SampleBankPanel::showEditCategoryDialog()
{
	auto it = std::find_if(categoryInfos.begin(), categoryInfos.end(),
	                       [this](const CategoryInfo &i) { return i.id == currentCategoryId; });
	if (it == categoryInfos.end())
		return;

	juce::String currentName = it->name;
	juce::Colour currentColour = it->colour != juce::Colour(0) ? it->colour : resolveCategoryColour(currentName);
	int editId = currentCategoryId;

	ObsidianAlertManager::showEditCategoryDialog(
	    this, currentName, currentColour,
	    [this, editId](const juce::String &newName, juce::Colour newColour)
	    {
		    auto jt = std::find_if(categoryInfos.begin(), categoryInfos.end(),
		                           [editId](const CategoryInfo &i) { return i.id == editId; });
		    if (jt == categoryInfos.end())
			    return;

		    for (const auto &info : categoryInfos)
			    if (info.id != editId && info.name.compareIgnoreCase(newName) == 0)
			    {
				    ObsidianAlertManager::showError(this, "Edit Category", "Already exists.");
				    return;
			    }

		    juce::String oldName = jt->name;
		    jt->name = newName;
		    jt->colour = newColour;

		    rebuildCategoryFilter();
		    if (auto *bank = audioProcessor.getSampleBank())
			    for (auto *s : bank->getAllSamples())
				    if (s->category == oldName)
					    s->category = newName;

		    categoryInput.clear();
		    saveCategoriesConfig();
		    refreshSampleList();
	    });
}

void SampleBankPanel::deleteCategory()
{
	if (!isCategoryEditable(currentCategoryId))
	{
		ObsidianAlertManager::showError(this, "Delete Category", "Cannot delete built-in categories.");
		return;
	}
	auto it = std::find_if(categoryInfos.begin(), categoryInfos.end(),
	                       [this](const CategoryInfo &i) { return i.id == currentCategoryId; });
	if (it == categoryInfos.end())
		return;
	juce::String catName = it->name;
	int catId = currentCategoryId;
	ObsidianAlertManager::showConfirm(
	    this, "Delete Category", "Delete '" + catName + "'? Samples won't be deleted.", "Delete", "Cancel",
	    [this, catName, catId](bool ok)
	    {
		    if (!ok)
			    return;
		    if (auto *bank = audioProcessor.getSampleBank())
		    {
			    for (auto *s : bank->getAllSamples())
				    s->category = "";
			    bank->saveBankData();
		    }
		    categoryInfos.erase(std::remove_if(categoryInfos.begin(), categoryInfos.end(),
		                                       [catId](const CategoryInfo &i) { return i.id == catId; }),
		                        categoryInfos.end());
		    rebuildCategoryFilter();
		    categoryFilter.setSelectedId(1);
		    currentCategoryId = 0;
		    editCategoryButton.setEnabled(false);
		    deleteCategoryButton.setEnabled(false);
		    saveCategoriesConfig();
		    refreshSampleList();
	    });
}

bool SampleBankPanel::isCategoryEditable(int id) const
{
	return id >= 20;
}

juce::Colour SampleBankPanel::resolveCategoryColour(const juce::String &name) const
{
	for (const auto &info : categoryInfos)
	{
		if (info.name == name && info.colour != juce::Colour(0))
			return info.colour;
	}

	static const std::map<juce::String, juce::Colour> defaults = {{"Drums", ColourPalette::indigo},
	                                                              {"Bass", ColourPalette::teal},
	                                                              {"Melody", ColourPalette::coral},
	                                                              {"Ambient", ColourPalette::emerald},
	                                                              {"Percussion", ColourPalette::slate},
	                                                              {"Vocal", ColourPalette::amber},
	                                                              {"FX", ColourPalette::backgroundLight},
	                                                              {"Loops", ColourPalette::buttonSuccess},
	                                                              {"One-shots", ColourPalette::buttonSecondary},
	                                                              {"House", ColourPalette::buttonDangerDark},
	                                                              {"Techno", ColourPalette::lime},
	                                                              {"Hip-Hop", ColourPalette::violet},
	                                                              {"Jazz", ColourPalette::amber},
	                                                              {"Rock", ColourPalette::buttonDanger},
	                                                              {"Electronic", ColourPalette::cyan},
	                                                              {"Piano", ColourPalette::textSecondary},
	                                                              {"Guitar", ColourPalette::textWarning},
	                                                              {"Synth", ColourPalette::textSecondary}};
	auto it = defaults.find(name);
	return it != defaults.end() ? it->second : ColourPalette::backgroundLight;
}

void SampleBankPanel::showEditPromptDialog(SampleBankEntry *entry)
{
	if (!entry)
		return;

	juce::String entryId = entry->id;
	juce::String oldPrompt = entry->originalPrompt;

	ObsidianAlertManager::showEditPrompt(this, entry->originalPrompt,
	                                     [this, entryId, oldPrompt](const juce::String &newPrompt)
	                                     {
		                                     if (newPrompt.isEmpty() || newPrompt == oldPrompt)
			                                     return;

		                                     auto *bank = audioProcessor.getSampleBank();
		                                     if (!bank)
			                                     return;

		                                     auto *e = bank->getSample(entryId);
		                                     if (!e)
			                                     return;

		                                     e->originalPrompt = newPrompt;
		                                     bank->saveBankData();

		                                     audioProcessor.addCustomPrompt(newPrompt);

		                                     if (auto *editor =
		                                             dynamic_cast<DjIaVstEditor *>(audioProcessor.getActiveEditor()))
			                                     editor->uiPresetManager->notifyTracksPromptUpdate();

		                                     refreshSampleListSilent();

		                                     selectedEntry = e;
		                                     sampleListBox.updateContent();

		                                     auto it = std::find(filteredSamples.begin(), filteredSamples.end(), e);
		                                     if (it != filteredSamples.end())
		                                     {
			                                     int newRow = (int)std::distance(filteredSamples.begin(), it);
			                                     sampleListBox.selectRow(newRow, true, true);
			                                     const int rowHeight = sampleListBox.getRowHeight();
			                                     sampleListBox.getViewport()->setViewPosition(0, newRow * rowHeight);
		                                     }

		                                     detailPanel.setEntry(e);
	                                     });
}

int SampleBankPanel::getNextCategoryId()
{
	int mx = 19;
	for (const auto &i : categoryInfos)
		mx = std::max(mx, i.id);
	return mx + 1;
}

void SampleBankPanel::saveCategoriesConfig()
{
	juce::File f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                   .getChildFile("OBSIDIAN-Neural")
	                   .getChildFile("categories.json");
	juce::DynamicObject::Ptr cfg = new juce::DynamicObject();
	juce::Array<juce::var> arr;
	for (const auto &info : categoryInfos)
		if (info.id > 0)
		{
			juce::DynamicObject::Ptr d = new juce::DynamicObject();
			d->setProperty("id", info.id);
			d->setProperty("name", info.name);
			if (info.colour != juce::Colour(0))
				d->setProperty("colour", (int)info.colour.getARGB());
			arr.add(d.get());
		}
	cfg->setProperty("categories", arr);
	f.getParentDirectory().createDirectory();
	f.replaceWithText(juce::JSON::toString(juce::var(cfg.get())));
}

void SampleBankPanel::loadCategoriesConfig()
{
	juce::File f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	                   .getChildFile("OBSIDIAN-Neural")
	                   .getChildFile("categories.json");
	if (!f.exists())
		return;
	auto cfg = juce::JSON::parse(f);
	if (!cfg.isObject())
		return;
	auto *obj = cfg.getDynamicObject();
	if (!obj)
		return;
	auto av = obj->getProperty("categories");
	if (!av.isArray())
		return;
	categoryInfos.erase(
	    std::remove_if(categoryInfos.begin(), categoryInfos.end(), [](const CategoryInfo &i) { return i.id >= 20; }),
	    categoryInfos.end());
	for (int i = 0; i < av.getArray()->size(); ++i)
	{
		auto v = av.getArray()->getUnchecked(i);
		if (!v.isObject())
			continue;
		auto *o = v.getDynamicObject();
		if (!o)
			continue;
		int id = o->getProperty("id");
		juce::String name = o->getProperty("name").toString();
		if (id >= 20)
		{
			CategoryInfo info;
			info.id = id;
			info.name = name;
			auto colourVar = o->getProperty("colour");
			if (!colourVar.isVoid())
				info.colour = juce::Colour((juce::uint32)(int)colourVar);
			categoryInfos.push_back(info);
		}
	}
}

void SampleBankPanel::rebuildCategoryFilter()
{
	int cur = categoryFilter.getSelectedId();
	categoryFilter.clear();
	for (const auto &info : categoryInfos)
		categoryFilter.addItem(info.name, info.id + 1);
	bool found = std::any_of(categoryInfos.begin(), categoryInfos.end(),
	                         [cur](const CategoryInfo &i) { return i.id == cur - 1; });
	categoryFilter.setSelectedId(found ? cur : 1);
}