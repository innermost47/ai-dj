#include "SampleBankPanel.h"
#include "SampleBank.h"
#include "PluginProcessor.h"
#include "components/shared/ObsidianAlertManager.h"
#include "BinaryData.h"
#include "PluginEditor.h"

SampleBankItem::SampleBankItem(SampleBankEntry* entry, DjIaVstProcessor& processor)
	: sampleEntry(entry), audioProcessor(processor)
{
	addAndMakeVisible(nameLabel);
	nameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	nameLabel.setFont(juce::FontOptions(13.0f));
	nameLabel.setText(entry->originalPrompt, juce::dontSendNotification);
	nameLabel.setInterceptsMouseClicks(false, false);
	setSize(400, 52);
}

SampleBankItem::~SampleBankItem() {}

void SampleBankItem::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();

	if (selected)
	{
		g.setColour(juce::Colours::white.withAlpha(0.06f));
		g.fillRect(bounds);
	}

	if (!sampleEntry)
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f,
			(float)bounds.getWidth() - 4.0f, (float)bounds.getBottom() - 1.0f, 0.5f);
		return;
	}

	if (sampleEntry && !sampleEntry->categories.empty())
	{
		const float thickness = selected ? 4.0f : 1.0f;
		g.setColour(getCategoryColor(sampleEntry->categories[0]));
		g.fillRect(0.0f, 0.0f, thickness, (float)bounds.getHeight());
	}
	else if (selected)
	{
		g.setColour(ColourPalette::trackSelected);
		g.fillRect(0.0f, 0.0f, 4.0f, (float)bounds.getHeight());
	}

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f,
		(float)bounds.getWidth() - 4.0f, (float)bounds.getBottom() - 1.0f, 0.5f);

	auto metaArea = bounds.removeFromBottom(22).withTrimmedLeft(12).withTrimmedRight(4);

	juce::StringArray parts;

	if (!sampleEntry->categories.empty())
		parts.add("[" + sampleEntry->categories[0] + "]");

	if (sampleEntry->duration > 0.0f)
	{
		int sc = (int)sampleEntry->duration % 60;
		int ms = (int)((sampleEntry->duration - (int)sampleEntry->duration) * 10);
		parts.add(juce::String::formatted("%d.%ds", sc, ms));
	}

	if (sampleEntry->bpm > 0.0f)
		parts.add(juce::String(sampleEntry->bpm, 1) + " BPM");

	if (sampleEntry->key.isNotEmpty())
		parts.add(sampleEntry->key);

	const int usageCount = (int)sampleEntry->usedInProjects.size();
	if (usageCount == 0)
		parts.add("Unused");
	else
		parts.add(juce::String(usageCount) + " project(s)");

	if (sampleEntry->modelName.isNotEmpty())
		parts.add("[" + sampleEntry->modelName + "]");

	g.setColour(ColourPalette::textSecondary.withAlpha(0.75f));
	g.setFont(juce::FontOptions(10.5f));
	g.drawText(parts.joinIntoString(" - "), metaArea, juce::Justification::centredLeft, true);

	if (sampleEntry->description.isNotEmpty())
	{
		auto descArea = bounds.removeFromBottom(14).withTrimmedLeft(12).withTrimmedRight(4);
		g.setColour(ColourPalette::textSecondary.withAlpha(0.5f));
		g.setFont(juce::FontOptions(10.0f));
		g.drawText(sampleEntry->description, descArea, juce::Justification::centredLeft, true);
	}
}

void SampleBankItem::resized()
{
	auto b = getLocalBounds().withTrimmedLeft(8).withTrimmedRight(4);
	nameLabel.setBounds(b.removeFromTop(28));
}

juce::Colour SampleBankItem::getCategoryColor(const juce::String& category)
{
	if (categoryColourResolver)
		return categoryColourResolver(category);

	static const std::map<juce::String, juce::Colour> colors = {
		{"Drums", ColourPalette::indigo},
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
		{"Synth", ColourPalette::textSecondary} };
	auto it = colors.find(category);
	return it != colors.end() ? it->second : ColourPalette::backgroundLight;
}

void SampleBankItem::mouseEnter(const juce::MouseEvent&)
{
	setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SampleBankItem::mouseExit(const juce::MouseEvent&)
{
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SampleBankItem::mouseDown(const juce::MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
	{
		showCategoryMenu();
		return;
	}

	if (event.getNumberOfClicks() == 2)
	{
		if (!selected && onItemClicked)
			onItemClicked(sampleEntry);
		if (onPromptEditRequested)
			onPromptEditRequested(sampleEntry);
		return;
	}

	if (onItemClicked)
		onItemClicked(sampleEntry);
}


void SampleBankItem::mouseDrag(const juce::MouseEvent& event)
{
	if (event.getDistanceFromDragStart() < 6 || isDragging)
		return;
	isDragging = true;

	if (event.mods.isCtrlDown() && sampleEntry)
	{
		juce::File f(sampleEntry->filePath);
		if (f.exists())
		{
			juce::StringArray files;
			files.add(f.getFullPathName());
			performExternalDragDropOfFiles(files, false);
			return;
		}
	}
	if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor(this))
		dc->startDragging(sampleEntry->id, this);
}

void SampleBankItem::mouseUp(const juce::MouseEvent&)
{
	isDragging = false;
}

void SampleBankItem::showCategoryMenu()
{
	if (!sampleEntry)
		return;

	juce::String sampleId = sampleEntry->id;

	std::vector<juce::String> avail;
	if (getCategoriesList)
		avail = getCategoriesList();
	else
		avail = { "Drums", "Bass", "Melody", "Ambient", "Percussion", "Vocal",
				 "FX", "Loops", "One-shots", "House", "Techno", "Hip-Hop",
				 "Jazz", "Rock", "Electronic", "Piano", "Guitar", "Synth" };

	ObsidianAlertManager::showCategoryEditor(
		this,
		sampleEntry->originalPrompt,
		sampleEntry->categories,
		avail,
		[sampleId, &ap = audioProcessor, cb = onCategoriesChanged](const std::vector<juce::String>& cats)
		{
			if (auto* bank = ap.getSampleBank())
			{
				if (auto* s = bank->getSample(sampleId))
				{
					s->categories = cats;
					if (cb)
						cb(s, cats);
				}
			}
		});
}

DetailPanel::DetailPanel()
{
	addAndMakeVisible(nameLabel);
	nameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	nameLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));

	addAndMakeVisible(metaLabel);
	metaLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	metaLabel.setFont(juce::FontOptions(11.0f));

	addAndMakeVisible(playButton);
	playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
	playButton.loadIconToggled(BinaryData::square_svg, BinaryData::square_svgSize);
	playButton.setHasToggledIcon(true);
	playButton.setClickingTogglesState(false);
	playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
	playButton.onClick = [this]()
		{
			if (!entry)
				return;
			if (isPlaying)
			{
				if (onStopRequested)
					onStopRequested();
			}
			else
			{
				if (onPlayRequested)
					onPlayRequested(entry);
			}
		};

	addAndMakeVisible(deleteButton);
	deleteButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.onClick = [this]()
		{
			if (entry && onDeleteRequested)
				onDeleteRequested(entry->id);
		};
	playButton.setCompactMode(true);
	deleteButton.setCompactMode(true);
	setVisible(false);
}

DetailPanel::~DetailPanel()
{
	destroyed.store(true);
	validity->store(false);
	stopTimer();
	removeAllChildren();
}

void DetailPanel::setEntry(SampleBankEntry* e)
{
	if (entry != e)
	{
		validity->store(false);
		validity = std::make_shared<std::atomic<bool>>(true);
		audioBuf.setSize(0, 0);
		thumbL.clear();
		thumbR.clear();
	}
	entry = e;
	isPlaying = false;
	playbackPos = 0.0f;
	updatePlayButton();
	if (!entry)
	{
		nameLabel.setText("", juce::dontSendNotification);
		metaLabel.setText("", juce::dontSendNotification);
		return;
	}
	nameLabel.setText(entry->originalPrompt, juce::dontSendNotification);

	juce::StringArray parts;
	parts.add(formatDuration(entry->duration));
	parts.add(juce::String(entry->bpm, 1) + " BPM");

	if (entry->usedInProjects.empty())
		parts.add("Unused");
	else
		parts.add(juce::String((int)entry->usedInProjects.size()) + " project(s)");

	if (!entry->categories.empty())
		parts.add("[" + entry->categories[0] + "]");

	metaLabel.setText(parts.joinIntoString(" - "), juce::dontSendNotification);

	setVisible(true);
	resized();
	loadAudio();
}

void DetailPanel::loadAudio()
{
	if (!entry)
		return;
	juce::File f(entry->filePath);
	if (!f.exists())
		return;

	auto v = validity;
	juce::Thread::launch([this, f, v]()
		{
			if (!v->load()) return;

			juce::AudioFormatManager fm;
			fm.registerBasicFormats();
			auto reader = std::unique_ptr<juce::AudioFormatReader>(fm.createReaderFor(f));
			if (!reader || !v->load()) return;

			const int total = (int)reader->lengthInSamples;
			const int target = 4096;
			const int ratio = std::max(1, total / target);
			const int num = total / ratio;

			juce::AudioBuffer<float> full(reader->numChannels, total);
			reader->read(&full, 0, total, 0, true, true);
			if (!v->load()) return;

			auto buf = std::make_shared<juce::AudioBuffer<float>>(reader->numChannels, num);
			for (int i = 0; i < num; ++i)
				for (int ch = 0; ch < (int)reader->numChannels; ++ch)
					buf->setSample(ch, i, full.getSample(ch, i * ratio));

			juce::MessageManager::callAsync([this, buf, v]()
				{
					if (!v->load() || destroyed.load()) return;
					audioBuf = *buf;
					generateThumbnail();
					repaint();
				}); });
}

void DetailPanel::generateThumbnail()
{
	thumbL.clear();
	thumbR.clear();
	if (audioBuf.getNumSamples() == 0 || waveformBounds.isEmpty())
		return;

	int n = audioBuf.getNumSamples();
	int ch = audioBuf.getNumChannels();
	bool mono = (ch == 1);
	int w = waveformBounds.getWidth() - 4;
	if (w <= 0)
		return;
	int spp = std::max(1, n / w);

	for (int p = 0; p < w; ++p)
	{
		int s0 = p * spp, s1 = std::min(s0 + spp, n);
		float rL = 0, rR = 0, pL = 0, pR = 0;
		int cnt = 0;
		for (int s = s0; s < s1; ++s)
		{
			float vL = audioBuf.getSample(0, s);
			float vR = mono ? vL : audioBuf.getSample(1, s);
			rL += vL * vL;
			rR += vR * vR;
			pL = std::max(pL, std::abs(vL));
			pR = std::max(pR, std::abs(vR));
			++cnt;
		}
		float fL = cnt > 0 ? std::sqrt(rL / cnt) * 0.7f + pL * 0.3f : 0.0f;
		float fR = cnt > 0 ? std::sqrt(rR / cnt) * 0.7f + pR * 0.3f : 0.0f;
		thumbL.push_back(fL);
		thumbR.push_back(fR);
	}
}

void DetailPanel::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();
	g.setColour(ColourPalette::backgroundMid);
	g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.4f));
	g.drawLine(0.0f, 0.5f, (float)bounds.getWidth(), 0.5f, 1.0f);
	if (entry && !entry->categories.empty())
	{
		auto nb = nameLabel.getBounds();
		float cy = nb.toFloat().getCentreY();
		juce::Colour col = this->categoryColourResolver ? this->categoryColourResolver(entry->categories[0])
			: getCategoryColor(entry->categories[0]);
		g.setColour(col);
		g.fillEllipse(8.0f, cy - 4.0f, 8.0f, 8.0f);
	}
	drawWaveform(g);
}

void DetailPanel::drawWaveform(juce::Graphics& g)
{
	if (thumbL.empty() || waveformBounds.isEmpty())
		return;

	g.saveState();
	juce::Path clip;
	clip.addRoundedRectangle(waveformBounds.toFloat(), 3.0f);
	g.reduceClipRegion(clip);

	g.setColour(ColourPalette::backgroundDark);
	g.fillRect(waveformBounds);

	auto innerBounds = waveformBounds.reduced(6, 4);

	size_t sz = std::min(thumbL.size(), thumbR.size());
	float ppx = (float)innerBounds.getWidth() / (float)sz;
	bool stereo = (thumbR.size() == thumbL.size() && audioBuf.getNumChannels() > 1);

	auto drawChannel = [&](const std::vector<float>& thumb, float centerY, float halfH)
		{
			juce::Path top, bot;
			bool ts = false, bs = false;
			for (size_t i = 0; i < sz; ++i)
			{
				float x = innerBounds.getX() + i * ppx;
				float h = thumb[i] * halfH;
				if (!ts)
				{
					top.startNewSubPath(x, centerY);
					ts = true;
				}
				if (!bs)
				{
					bot.startNewSubPath(x, centerY);
					bs = true;
				}
				top.lineTo(x, centerY - h);
				bot.lineTo(x, centerY + h);
			}
			g.setColour(ColourPalette::textPrimary.withAlpha(0.8f));
			g.strokePath(top, juce::PathStrokeType(1.0f));
			g.strokePath(bot, juce::PathStrokeType(1.0f));
		};

	if (stereo)
	{
		float qY = innerBounds.getY() + innerBounds.getHeight() * 0.25f;
		float tqY = innerBounds.getY() + innerBounds.getHeight() * 0.75f;
		float hH = innerBounds.getHeight() * 0.22f;
		drawChannel(thumbL, qY, hH);
		drawChannel(thumbR, tqY, hH);
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.15f));
		g.drawLine((float)innerBounds.getX(), (float)innerBounds.getCentreY(),
			(float)innerBounds.getRight(), (float)innerBounds.getCentreY(), 0.5f);
	}
	else
	{
		float cY = (float)innerBounds.getCentreY();
		float hH = innerBounds.getHeight() * 0.42f;
		drawChannel(thumbL, cY, hH);
	}

	if (isPlaying && entry && entry->duration > 0.0f)
	{
		float prog = playbackPos / entry->duration;
		float hx = innerBounds.getX() + prog * innerBounds.getWidth();
		g.setColour(ColourPalette::playArmed);
		g.drawLine(hx, (float)innerBounds.getY(),
			hx, (float)innerBounds.getBottom(), 2.0f);
	}

	g.restoreState();

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawRoundedRectangle(waveformBounds.toFloat(), 3.0f, 0.5f);
}

void DetailPanel::resized()
{
	auto area = getLocalBounds().reduced(6, 4);

	auto topRow = area.removeFromTop(18);
	nameLabel.setBounds(topRow.withTrimmedLeft(20));

	area.removeFromTop(2);

	auto metaRow = area.removeFromTop(14);
	metaLabel.setBounds(metaRow.withTrimmedLeft(4));

	area.removeFromTop(3);

	auto bottomRow = area;

	auto btnCol = bottomRow.removeFromRight(36);
	bottomRow.removeFromRight(4);
	playButton.setBounds(btnCol.removeFromTop(bottomRow.getHeight() / 2).reduced(2));
	deleteButton.setBounds(btnCol.reduced(2));

	waveformBounds = bottomRow.reduced(2);

	generateThumbnail();
	repaint();
}

void DetailPanel::setIsPlaying(bool playing)
{
	isPlaying = playing;
	updatePlayButton();
	if (playing)
	{
		playbackPos = 0.0f;
		lastTimerCall = juce::Time::getMillisecondCounterHiRes() / 1000.0;
		startTimer(30);
	}
	else
	{
		stopTimer();
		playbackPos = 0.0f;
	}
	repaint();
}

void DetailPanel::updatePlaybackPosition(float pos)
{
	playbackPos = pos;
	repaint();
}

void DetailPanel::timerCallback()
{
	if (!isPlaying || !entry)
	{
		stopTimer();
		return;
	}
	double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	playbackPos += (float)(now - lastTimerCall);
	lastTimerCall = now;
	if (playbackPos >= entry->duration)
	{
		playbackPos = entry->duration;
		setIsPlaying(false);
		if (onStopRequested)
			onStopRequested();
	}
	repaint();
}

void DetailPanel::updatePlayButton()
{
	if (isPlaying)
	{
		playButton.setToggleState(true, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	}
	else
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
	}
	playButton.repaint();
}

juce::String DetailPanel::formatDuration(float s)
{
	int m = (int)(s / 60);
	int sc = (int)s % 60;
	int ms = (int)((s - (int)s) * 100);
	return m > 0 ? juce::String::formatted("%d:%02d.%02d", m, sc, ms)
		: juce::String::formatted("%d.%02ds", sc, ms);
}

juce::Colour DetailPanel::getCategoryColor(const juce::String& category)
{
	static const std::map<juce::String, juce::Colour> colors = {
		{"Drums", ColourPalette::indigo},
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
		{"Synth", ColourPalette::textSecondary} };
	auto it = colors.find(category);
	return it != colors.end() ? it->second : ColourPalette::backgroundLight;
}

SampleBankPanel::SampleBankPanel(DjIaVstProcessor& processor)
	: audioProcessor(processor)
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

	if (auto* bank = audioProcessor.getSampleBank())
		bank->onBankChanged = [this]()
		{ juce::MessageManager::callAsync([this]()
			{ refreshSampleList(); }); };
}

SampleBankPanel::~SampleBankPanel()
{
	stopTimer();
	stopPreview();
	sampleListBox.setVisible(false);
	sampleListBox.setModel(nullptr);
	if (auto* bank = audioProcessor.getSampleBank())
		bank->onBankChanged = nullptr;
}

void SampleBankPanel::setupUI()
{
	addAndMakeVisible(titleLabel);
	titleLabel.setText("Sample Bank", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);

	addAndMakeVisible(infoLabel);
	infoLabel.setText("Preview: ch.9 | Drag: drop on track | Ctrl+Drag: drop in DAW | Right-click: categories",
		juce::dontSendNotification);
	infoLabel.setFont(juce::FontOptions(11.0f));
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setJustificationType(juce::Justification::centredLeft);

	addAndMakeVisible(cleanupButton);
	cleanupButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	cleanupButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	cleanupButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	cleanupButton.onClick = [this]()
		{ cleanupUnusedSamples(); };

	addAndMakeVisible(sortMenu);
	sortMenu.addItem("Sort: Recent", SortType::Time);
	sortMenu.addItem("Sort: Prompt", SortType::Prompt);
	sortMenu.addItem("Sort: Usage", SortType::Usage);
	sortMenu.addItem("Sort: BPM", SortType::BPM);
	sortMenu.addItem("Sort: Duration", SortType::Duration);
	sortMenu.setSelectedId(SortType::Prompt);
	sortMenu.onChange = [this]()
		{
			currentSortType = static_cast<SortType>(sortMenu.getSelectedId());
			refreshSampleList();
		};

	addAndMakeVisible(sampleListBox);
	sampleListBox.setModel(this);
	sampleListBox.setRowHeight(ROW_HEIGHT);
	sampleListBox.setOutlineThickness(0);
	sampleListBox.getViewport()->setScrollBarsShown(true, false);
	sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

	addAndMakeVisible(categoryFilter);
	for (const auto& info : categoryInfos)
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
	addCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
	addCategoryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

	addCategoryButton.onClick = [this]()
		{ addCategory(); };

	addAndMakeVisible(editCategoryButton);
	editCategoryButton.loadIcon(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	editCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::amber);
	editCategoryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	editCategoryButton.onClick = [this]()
		{ editCategory(); };
	editCategoryButton.setEnabled(false);

	addAndMakeVisible(deleteCategoryButton);
	deleteCategoryButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteCategoryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	deleteCategoryButton.onClick = [this]()
		{ deleteCategory(); };
	deleteCategoryButton.setEnabled(false);

	deleteCategoryButton.setCompactMode(true);
	editCategoryButton.setCompactMode(true);
	addCategoryButton.setCompactMode(true);
	cleanupButton.setCompactMode(true);

	addAndMakeVisible(detailPanel);

	detailPanel.categoryColourResolver = [this](const juce::String& name) -> juce::Colour
		{
			return resolveCategoryColour(name);
		};

	detailPanel.onPlayRequested = [this](SampleBankEntry* e)
		{ playPreview(e); };
	detailPanel.onStopRequested = [this]()
		{ stopPreview(); };
	detailPanel.onDeleteRequested = [this](const juce::String& id)
		{
			if (auto* e = audioProcessor.getSampleBank()->getSample(id))
				showDeleteConfirmation(id, e->originalPrompt);
		};
}

void SampleBankPanel::resized()
{
	auto area = getLocalBounds();

	detailPanel.setBounds(area.removeFromBottom(DETAIL_HEIGHT));

	area.removeFromLeft(10);
	area.removeFromRight(10);
	area.removeFromTop(6);

	auto hdr = area.removeFromTop(28);
	titleLabel.setBounds(hdr.removeFromLeft(160));
	cleanupButton.setBounds(hdr.removeFromRight(34).reduced(2));

	area.removeFromTop(4);

	sortMenu.setBounds(area.removeFromTop(24).reduced(0, 2));

	area.removeFromTop(4);

	infoLabel.setBounds(area.removeFromTop(30));

	area.removeFromTop(4);

	categoryFilter.setBounds(area.removeFromTop(24).reduced(0, 2));

	area.removeFromTop(4);

	auto btnRow = area.removeFromTop(24);
	addCategoryButton.setBounds(btnRow.removeFromRight(28).reduced(2));
	btnRow.removeFromRight(2);
	deleteCategoryButton.setBounds(btnRow.removeFromRight(28).reduced(2));
	btnRow.removeFromRight(2);
	editCategoryButton.setBounds(btnRow.removeFromRight(28).reduced(2));
	btnRow.removeFromRight(4);
	categoryInput.setBounds(btnRow.reduced(0, 2));

	area.removeFromTop(4);

	sampleListBox.setBounds(area);
}

void SampleBankPanel::selectEntry(SampleBankEntry* entry)
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

juce::Component* SampleBankPanel::refreshComponentForRow(
	int rowNumber, bool, juce::Component* existingComponentToUpdate)
{
	auto* wrapper = dynamic_cast<SampleBankItemWrapper*>(existingComponentToUpdate);

	if (rowNumber < 0 || rowNumber >= (int)filteredSamples.size())
	{
		delete wrapper;
		return nullptr;
	}

	auto* entry = filteredSamples[rowNumber];
	SampleBankItem* item = nullptr;

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

	item->onItemClicked = [this](SampleBankEntry* e)
		{
			if (selectedEntry == e)
				selectEntry(nullptr);
			else
				selectEntry(e);
		};

	item->onCategoriesChanged = [this](SampleBankEntry*, const std::vector<juce::String>&)
		{
			if (auto* bank = audioProcessor.getSampleBank())
				bank->saveBankData();
			refreshSampleListSilent();
		};

	item->getCategoriesList = [this]() -> std::vector<juce::String>
		{
			std::vector<juce::String> cats;
			for (const auto& info : categoryInfos)
				if (info.id > 0)
					cats.push_back(info.name);
			return cats;
		};

	item->categoryColourResolver = [this](const juce::String& name) -> juce::Colour
		{
			return resolveCategoryColour(name);
		};

	item->onPromptEditRequested = [this](SampleBankEntry* e)
		{
			showEditPromptDialog(e);
		};

	item->onDeleteRequested = [this](SampleBankEntry* e)
		{
			if (e)
				showDeleteConfirmation(e->id, e->originalPrompt);
		};

	return wrapper;
}

void SampleBankPanel::playPreview(SampleBankEntry* entry)
{
	if (!entry)
		return;
	stopPreview();

	if (!audioProcessor.previewSampleFromBank(entry->id))
		return;

	currentPreviewEntry = entry;
	detailPanel.setIsPlaying(true);
	startTimer(100);
}

void SampleBankPanel::stopPreview()
{
	audioProcessor.stopSamplePreview();
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

	if (currentPreviewEntry && !audioProcessor.isSamplePreviewing())
		stopPreview();

	if (!isLoading.load() && !currentPreviewEntry)
		stopTimer();
}

void SampleBankPanel::paint(juce::Graphics& g)
{
	g.fillAll(ColourPalette::backgroundDark);

	g.setColour(ColourPalette::backgroundDeep.withAlpha(0.8f));
	juce::Path listBg;
	auto lb = sampleListBox.getBounds().toFloat();
	listBg.addRoundedRectangle(lb.getX(), lb.getY(), lb.getWidth(), lb.getHeight(),
		4.0f, 4.0f, true, true, false, false);
	g.fillPath(listBg);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
	g.drawRect(getLocalBounds(), 1);

	if (isLoading.load())
		drawLoader(g);
	else if (filteredSamples.empty() && hasEverLoaded.load())
		drawEmptyState(g);
}

void SampleBankPanel::drawLoader(juce::Graphics& g)
{
	auto b = sampleListBox.getBounds().toFloat();
	float cx = b.getCentreX(), cy = b.getCentreY();
	const float r = 40.0f, t = 4.0f;

	juce::Path bg;
	bg.addCentredArc(cx, cy, r, r, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.strokePath(bg, juce::PathStrokeType(t));

	juce::Path arc;
	arc.addCentredArc(cx, cy, r, r, 0.0f,
		loadingAngle, loadingAngle + juce::MathConstants<float>::pi * 1.5f, true);
	g.setColour(ColourPalette::violet);
	g.strokePath(arc, juce::PathStrokeType(t, juce::PathStrokeType::curved,
		juce::PathStrokeType::rounded));

	g.setColour(ColourPalette::textSecondary);
	g.setFont(juce::FontOptions(14.0f));
	g.drawText("Loading samples...",
		b.withSizeKeepingCentre(200, 30).translated(0, r + 30),
		juce::Justification::centred);
}

void SampleBankPanel::drawEmptyState(juce::Graphics& g)
{
	auto b = sampleListBox.getBounds();
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.setFont(juce::FontOptions(48.0f));
	g.drawText(juce::String::fromUTF8("\xE2\x99\xAA"),
		b.withSizeKeepingCentre(60, 60), juce::Justification::centred);
	g.setColour(ColourPalette::textSecondary);
	g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
	g.drawText("No samples yet",
		b.withSizeKeepingCentre(300, 28).translated(0, 55), juce::Justification::centred);
	g.setFont(juce::FontOptions(12.0f));
	g.drawText("Generate some loops to populate your bank!",
		b.withSizeKeepingCentre(300, 28).translated(0, 80), juce::Justification::centred);
}

void SampleBankPanel::refreshSampleList()
{
	isLoading.store(true);
	startTimer(30);
	repaint();

	auto* bank = audioProcessor.getSampleBank();
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

	juce::Timer::callAfterDelay(600, [this, safe = juce::Component::SafePointer(this)]()
		{
			if (!safe) return;
			sampleListBox.updateContent();
			isLoading.store(false);
			hasEverLoaded.store(true);
			if (selectedEntry == nullptr && !filteredSamples.empty())
				selectEntry(filteredSamples[0]);
			if (!currentPreviewEntry) stopTimer();
			repaint(); });
}

void SampleBankPanel::refreshSampleListSilent()
{
	auto* bank = audioProcessor.getSampleBank();
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
	auto* bank = audioProcessor.getSampleBank();
	if (!bank)
	{
		filteredSamples.clear();
		return;
	}

	auto samples = bank->getAllSamples();

	if (currentCategoryId != 0)
	{
		juce::String catName;
		for (const auto& info : categoryInfos)
			if (info.id == currentCategoryId)
			{
				catName = info.name;
				break;
			}

		if (!catName.isEmpty())
			samples.erase(std::remove_if(samples.begin(), samples.end(),
				[&catName](const SampleBankEntry* e)
				{
					return std::find(e->categories.begin(), e->categories.end(), catName) == e->categories.end();
				}),
				samples.end());
	}

	switch (currentSortType)
	{
	case Time:
		std::sort(samples.begin(), samples.end(),
			[](auto* a, auto* b)
			{ return a->creationTime > b->creationTime; });
		break;
	case Prompt:
		std::sort(samples.begin(), samples.end(),
			[](auto* a, auto* b)
			{ return a->originalPrompt.compareIgnoreCase(b->originalPrompt) < 0; });
		break;
	case Usage:
		std::sort(samples.begin(), samples.end(),
			[](auto* a, auto* b)
			{ return a->usedInProjects.size() > b->usedInProjects.size(); });
		break;
	case BPM:
		std::sort(samples.begin(), samples.end(),
			[](auto* a, auto* b)
			{ return a->bpm > b->bpm; });
		break;
	case Duration:
		std::sort(samples.begin(), samples.end(),
			[](auto* a, auto* b)
			{ return a->duration > b->duration; });
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
		juce::Timer::callAfterDelay(100, [this, safe = juce::Component::SafePointer(this)]()
			{ if (safe) refreshSampleList(); });
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

void SampleBankPanel::deleteSample(const juce::String& id)
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

	if (auto* bank = audioProcessor.getSampleBank())
		if (bank->removeSample(id))
			refreshSampleList();
}

void SampleBankPanel::cleanupUnusedSamples()
{
	auto* bank = audioProcessor.getSampleBank();
	if (!bank)
		return;
	auto unused = bank->getUnusedSamples();
	if (unused.empty())
	{
		ObsidianAlertManager::showInfo(this, "Clean Unused", "No unused samples.");
		return;
	}
	ObsidianAlertManager::showConfirm(this, "Clean Unused",
		"Found " + juce::String((int)unused.size()) + " unused samples. Delete all?",
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

void SampleBankPanel::showDeleteConfirmation(const juce::String& id, const juce::String& name)
{
	auto* e = audioProcessor.getSampleBank()->getSample(id);
	if (!e)
		return;

	juce::StringArray loadedOn;
	const juce::String samplePath = e->filePath;
	const juce::String sampleId = e->id;

	for (const auto& trackId : audioProcessor.getAllTrackIds())
	{
		auto* track = audioProcessor.getTrack(trackId);
		if (!track) continue;

		bool used = false;
		juce::String pageHit;

		if (!sampleId.isEmpty() && track->currentSampleId == sampleId)
			used = true;

		if (!used && !samplePath.isEmpty())
		{
			static const char pageLetters[] = { 'A', 'B', 'C', 'D' };
			for (int p = 0; p < 4; ++p)
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
			"'" + name + "' is currently loaded on:\n\n"
			+ loadedOn.joinIntoString("\n")
			+ "\n\nUnload it from the track(s) before deleting.");
		return;
	}

	juce::String msg = "Delete '" + name + "'?";
	if (!e->usedInProjects.empty())
		msg += "\n\nUsed in " + juce::String((int)e->usedInProjects.size()) + " project(s).";
	ObsidianAlertManager::showConfirm(this, "Delete Sample", msg, "Delete", "Cancel",
		[this, id](bool ok)
		{ if (ok) deleteSample(id); });
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
		[this](const juce::String& name, juce::Colour colour)
		{
			for (const auto& info : categoryInfos)
				if (info.name.compareIgnoreCase(name) == 0)
				{
					ObsidianAlertManager::showError(this, "Add Category", "Already exists.");
					return;
				}
			int id = std::max(20, getNextCategoryId());
			categoryInfos.push_back({ id, name, colour });
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
		[this](const CategoryInfo& i) { return i.id == currentCategoryId; });
	if (it == categoryInfos.end()) return;

	juce::String currentName = it->name;
	juce::Colour currentColour = it->colour != juce::Colour(0) ? it->colour : resolveCategoryColour(currentName);
	int editId = currentCategoryId;

	ObsidianAlertManager::showEditCategoryDialog(this, currentName, currentColour,
		[this, editId](const juce::String& newName, juce::Colour newColour)
		{
			auto jt = std::find_if(categoryInfos.begin(), categoryInfos.end(),
				[editId](const CategoryInfo& i) { return i.id == editId; });
			if (jt == categoryInfos.end()) return;

			for (const auto& info : categoryInfos)
				if (info.id != editId && info.name.compareIgnoreCase(newName) == 0)
				{
					ObsidianAlertManager::showError(this, "Edit Category", "Already exists.");
					return;
				}

			juce::String oldName = jt->name;
			jt->name = newName;
			jt->colour = newColour;

			rebuildCategoryFilter();
			if (auto* bank = audioProcessor.getSampleBank())
				for (auto* s : bank->getAllSamples())
					for (auto& c : s->categories)
						if (c == oldName) c = newName;

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
		[this](const CategoryInfo& i)
		{ return i.id == currentCategoryId; });
	if (it == categoryInfos.end())
		return;
	juce::String catName = it->name;
	int catId = currentCategoryId;
	ObsidianAlertManager::showConfirm(this, "Delete Category",
		"Delete '" + catName + "'? Samples won't be deleted.",
		"Delete", "Cancel",
		[this, catName, catId](bool ok)
		{
			if (!ok)
				return;
			if (auto* bank = audioProcessor.getSampleBank())
			{
				for (auto* s : bank->getAllSamples())
					s->categories.erase(std::remove(s->categories.begin(),
						s->categories.end(), catName),
						s->categories.end());
				bank->saveBankData();
			}
			categoryInfos.erase(std::remove_if(categoryInfos.begin(), categoryInfos.end(),
				[catId](const CategoryInfo& i)
				{ return i.id == catId; }),
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

bool SampleBankPanel::isCategoryEditable(int id) const { return id >= 20; }

juce::Colour SampleBankPanel::resolveCategoryColour(const juce::String& name) const
{
	for (const auto& info : categoryInfos)
	{
		if (info.name == name && info.colour != juce::Colour(0))
			return info.colour;
	}

	static const std::map<juce::String, juce::Colour> defaults = {
		{"Drums", ColourPalette::indigo},
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
		{"Synth", ColourPalette::textSecondary} };
	auto it = defaults.find(name);
	return it != defaults.end() ? it->second : ColourPalette::backgroundLight;
}

void SampleBankPanel::showEditPromptDialog(SampleBankEntry* entry)
{
	if (!entry) return;

	juce::String entryId = entry->id;
	juce::String oldPrompt = entry->originalPrompt;

	ObsidianAlertManager::showEditPrompt(this, entry->originalPrompt,
		[this, entryId, oldPrompt](const juce::String& newPrompt)
		{
			if (newPrompt.isEmpty() || newPrompt == oldPrompt) return;

			auto* bank = audioProcessor.getSampleBank();
			if (!bank) return;

			auto* e = bank->getSample(entryId);
			if (!e) return;

			e->originalPrompt = newPrompt;
			bank->saveBankData();

			audioProcessor.addCustomPrompt(newPrompt);

			if (auto* editor = dynamic_cast<DjIaVstEditor*>(audioProcessor.getActiveEditor()))
				editor->refreshAllPromptLists();

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
	for (const auto& i : categoryInfos)
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
	for (const auto& info : categoryInfos)
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
	auto* obj = cfg.getDynamicObject();
	if (!obj)
		return;
	auto av = obj->getProperty("categories");
	if (!av.isArray())
		return;
	categoryInfos.erase(std::remove_if(categoryInfos.begin(), categoryInfos.end(),
		[](const CategoryInfo& i)
		{ return i.id >= 20; }),
		categoryInfos.end());
	for (int i = 0; i < av.getArray()->size(); ++i)
	{
		auto v = av.getArray()->getUnchecked(i);
		if (!v.isObject())
			continue;
		auto* o = v.getDynamicObject();
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
	for (const auto& info : categoryInfos)
		categoryFilter.addItem(info.name, info.id + 1);
	bool found = std::any_of(categoryInfos.begin(), categoryInfos.end(),
		[cur](const CategoryInfo& i)
		{ return i.id == cur - 1; });
	categoryFilter.setSelectedId(found ? cur : 1);
}