#include "SampleBankPanel.h"
#include "PluginProcessor.h"
#include "CategoryWindow.h"

SampleBankItem::SampleBankItem(SampleBankEntry* entry, DjIaVstProcessor& processor)
	: sampleEntry(entry), audioProcessor(processor), validityFlag(std::make_shared<std::atomic<bool>>(true))
{
	addAndMakeVisible(nameLabel);
	addAndMakeVisible(durationLabel);
	addAndMakeVisible(bpmLabel);
	addAndMakeVisible(usageLabel);
	addAndMakeVisible(playButton);
	addAndMakeVisible(deleteButton);

	nameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	nameLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

	durationLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	durationLabel.setFont(juce::FontOptions(12.0f));

	bpmLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	bpmLabel.setFont(juce::FontOptions(12.0f));

	usageLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	usageLabel.setFont(juce::FontOptions(12.0f));

	playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
	playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	playButton.setTooltip("Preview sample");
	playButton.onClick = [this]()
		{
			if (isPlaying)
			{
				if (onStopRequested)
					onStopRequested();
			}
			else
			{
				if (onPreviewRequested)
					onPreviewRequested(sampleEntry);
			}
		};

	updatePlayButton();

	deleteButton.setButtonText(juce::String::fromUTF8("\xE2\x9C\x95"));
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteButton.setTooltip("Delete sample");
	deleteButton.onClick = [this]()
		{
			if (onDeleteRequested)
				onDeleteRequested(sampleEntry->id);
		};
	updateLabels();
	int requiredHeight = getRequiredHeight();
	setSize(400, requiredHeight);
}

SampleBankItem::~SampleBankItem()
{
	isDestroyed.store(true);
	validityFlag->store(false);
	stopTimer();
	sampleEntry = nullptr;
}

int SampleBankItem::getRequiredHeight()
{
	const int labelsHeight = 16 + 16 + 4;
	const int waveformHeight = 30;
	const int margins = 16;
	const int baseHeight = labelsHeight + waveformHeight + margins;

	if (sampleEntry && !sampleEntry->categories.empty())
	{
		return baseHeight + 25;
	}

	return baseHeight;
}

void SampleBankItem::loadAudioDataIfNeeded()
{
	if (audioBuffer.getNumSamples() == 0)
	{
		loadAudioData();
		if (!waveformBounds.isEmpty())
		{
			generateThumbnail();
			repaint();
		}
	}
}

void SampleBankItem::setIsPlaying(bool playing)
{
	isPlaying = playing;
	updatePlayButton();

	if (isPlaying)
	{
		playbackPosition = 0.0f;
		lastTimerCall = juce::Time::getMillisecondCounterHiRes() / 1000.0;
		startTimer(30);
	}
	else
	{
		stopTimer();
		playbackPosition = 0.0f;
	}

	repaint();
}

void SampleBankItem::timerCallback()
{
	if (!isPlaying || !sampleEntry)
	{
		stopTimer();
		return;
	}
	double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	double deltaTime = currentTime - lastTimerCall;
	lastTimerCall = currentTime;

	playbackPosition += static_cast<float>(deltaTime);

	if (playbackPosition >= sampleEntry->duration)
	{
		playbackPosition = sampleEntry->duration;
		setIsPlaying(false);
		if (onStopRequested)
			onStopRequested();
	}

	repaint();
}

void SampleBankItem::updatePlayButton()
{
	if (isPlaying)
	{
		playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xA0"));
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
		playButton.setTooltip("Stop preview");
	}
	else
	{
		playButton.setButtonText(juce::String::fromUTF8("\xE2\x96\xB6"));
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
		playButton.setTooltip("Preview sample");
	}
}

void SampleBankItem::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();

	juce::Colour bgColour;

	if (isDragging)
	{
		bgColour = ColourPalette::buttonWarning.withAlpha(0.5f);
	}
	else if (isSelected)
	{
		bgColour = ColourPalette::trackSelected.withAlpha(0.2f);
	}
	else
	{
		bgColour = ColourPalette::backgroundDark.withAlpha(0.8f);
	}

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

	g.setColour(isDragging ? ColourPalette::buttonWarning : ColourPalette::backgroundLight);
	g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f,
		isDragging ? 2.0f : 1.0f);
	drawMiniWaveform(g);
	if (!sampleEntry->categories.empty())
	{
		drawCategoryBadges(g);
	}
}

void SampleBankItem::drawCategoryBadges(juce::Graphics& g)
{
	const int badgeHeight = 18;
	const int badgeMargin = 3;
	const int badgePadding = 6;
	const int startY = waveformBounds.getBottom() + 5;

	juce::FontOptions badgeFont(12.0f);
	g.setFont(badgeFont);

	int currentX = 10;

	for (int i = 0; i < std::min(maxVisibleBadges, (int)sampleEntry->categories.size()); ++i)
	{
		const auto& category = sampleEntry->categories[i];

		juce::GlyphArrangement glyphs;
		glyphs.addLineOfText(badgeFont, category, 0.0f, 0.0f);
		int textWidth = (int)glyphs.getBoundingBox(0, -1, true).getWidth();
		int badgeWidth = textWidth + (badgePadding * 2);
		if (currentX + badgeWidth > getWidth() - 5)
			break;

		juce::Colour badgeColor = getCategoryColor(category);

		g.setColour(badgeColor);
		g.fillRoundedRectangle(static_cast<float>(currentX), static_cast<float>(startY),
			static_cast<float>(badgeWidth), static_cast<float>(badgeHeight), 9.0f);

		g.setColour(ColourPalette::textPrimary.withAlpha(0.3f));
		g.drawRoundedRectangle(static_cast<float>(currentX), static_cast<float>(startY),
			static_cast<float>(badgeWidth), static_cast<float>(badgeHeight),
			9.0f, 0.35f);

		float brightness = badgeColor.getBrightness();
		juce::Colour textColor = brightness > 0.5f ? ColourPalette::textPrimary : juce::Colours::white;

		g.setColour(textColor);
		g.drawText(category, currentX, startY, badgeWidth, badgeHeight, juce::Justification::centred);

		currentX += badgeWidth + badgeMargin;
	}
}

juce::Colour SampleBankItem::getCategoryColor(const juce::String& category)
{
	static const std::map<juce::String, juce::Colour> categoryColors = {
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
		{"Techno", ColourPalette::backgroundMid},
		{"Hip-Hop", ColourPalette::violet},
		{"Jazz", ColourPalette::amber},
		{"Rock", ColourPalette::buttonDanger},
		{"Electronic", ColourPalette::backgroundDeep},
		{"Piano", ColourPalette::textSecondary},
		{"Guitar", ColourPalette::textWarning},
		{"Synth", ColourPalette::textSecondary} };

	auto it = categoryColors.find(category);
	if (it != categoryColors.end())
		return it->second;

	return ColourPalette::backgroundLight;
}

void SampleBankItem::setPlaybackPosition(float positionInSeconds)
{
	playbackPosition = positionInSeconds;
	repaint();
}

void SampleBankItem::resized()
{
	auto area = getLocalBounds().reduced(8);
	auto buttonArea = area.removeFromRight(65);

	int buttonSize = 28;
	int buttonY = 8;

	auto playButtonBounds = juce::Rectangle<int>(buttonArea.getX(), buttonY, buttonSize, buttonSize);
	auto deleteButtonBounds = juce::Rectangle<int>(buttonArea.getX() + buttonSize + 5, buttonY, buttonSize, buttonSize);

	playButton.setBounds(playButtonBounds);
	deleteButton.setBounds(deleteButtonBounds);

	area.removeFromRight(10);

	auto topRow = area.removeFromTop(16);
	nameLabel.setBounds(topRow.removeFromLeft(200));

	auto bottomRow = area.removeFromTop(16);
	durationLabel.setBounds(bottomRow.removeFromLeft(60));
	bottomRow.removeFromLeft(10);
	bpmLabel.setBounds(bottomRow.removeFromLeft(60));
	bottomRow.removeFromLeft(10);
	usageLabel.setBounds(bottomRow);

	area.removeFromTop(4);

	const int waveformHeight = 30;
	waveformBounds = area.removeFromTop(waveformHeight);

	generateThumbnail();

	int requiredHeight = getRequiredHeight();
	if (getHeight() != requiredHeight)
	{
		setSize(getWidth(), requiredHeight);
		if (auto* parent = getParentComponent())
			parent->resized();
		return;
	}

	updateBadgeLayout();
	repaint();
}

void SampleBankItem::updateBadgeLayout()
{
	const int badgeMargin = 3;
	const int badgePadding = 6;
	const int availableWidth = getWidth() - 10;

	juce::FontOptions badgeFont(12.0f);

	maxVisibleBadges = 0;
	int currentX = 0;

	for (const auto& category : sampleEntry->categories)
	{
		juce::GlyphArrangement glyphs;
		glyphs.addLineOfText(badgeFont, category, 0.0f, 0.0f);
		int textWidth = (int)glyphs.getBoundingBox(0, -1, true).getWidth();
		int badgeWidth = textWidth + (badgePadding * 2);

		if (currentX + badgeWidth > availableWidth)
		{
			if (maxVisibleBadges < sampleEntry->categories.size())
			{
				if (currentX + 25 <= availableWidth)
				{
				}
				else if (maxVisibleBadges > 0)
				{
					maxVisibleBadges--;
				}
			}
			break;
		}

		maxVisibleBadges++;
		currentX += badgeWidth + badgeMargin;
	}
}

void SampleBankItem::loadAudioData()
{
	if (!sampleEntry)
		return;

	juce::File audioFile(sampleEntry->filePath);
	if (!audioFile.exists())
		return;

	double currentSampleRate = audioProcessor.getSampleRate();
	auto validity = validityFlag;

	juce::Thread::launch([this, audioFile, currentSampleRate, validity]()
		{
			if (!validity->load()) return;

			juce::AudioFormatManager formatManager;
			formatManager.registerBasicFormats();

			auto reader = std::unique_ptr<juce::AudioFormatReader>(
				formatManager.createReaderFor(audioFile));

			if (reader && validity->load())
			{
				const int downsampleRatio = std::max(1, (int)(reader->lengthInSamples / 4096));
				const int numSamples = (int)(reader->lengthInSamples / downsampleRatio);

				auto tempBuffer = std::make_shared<juce::AudioBuffer<float>>(reader->numChannels, numSamples);

				for (int i = 0; i < numSamples; ++i)
				{
					if (!validity->load()) return;
					reader->read(tempBuffer.get(), i, 1, i * downsampleRatio, true, true);
				}

				if (validity->load())
				{
					juce::MessageManager::callAsync([this, tempBuffer, currentSampleRate, validity]()
						{
							if (validity->load() && !isDestroyed.load() && sampleEntry)
							{
								audioBuffer = *tempBuffer;
								sampleRate = currentSampleRate;

								if (!waveformBounds.isEmpty())
								{
									generateThumbnail();
									repaint();
								}
							}
						});
				}
			} });
}

void SampleBankItem::drawMiniWaveform(juce::Graphics& g)
{
	if (thumbnail.empty() || waveformBounds.isEmpty())
		return;

	g.saveState();

	juce::Path clipPath;
	clipPath.addRoundedRectangle(waveformBounds.toFloat(), 4.0f);
	g.reduceClipRegion(clipPath);

	g.setColour(ColourPalette::backgroundMid);
	g.fillRoundedRectangle(waveformBounds.toFloat(), 4.0f);

	g.setColour(ColourPalette::buttonDanger);

	juce::Path waveformPath;
	juce::Path bottomPath;
	bool topPathStarted = false;
	bool bottomPathStarted = false;

	float centerY = static_cast<float>(waveformBounds.getCentreY());
	float pixelsPerPoint = static_cast<float>(waveformBounds.getWidth()) / static_cast<float>(thumbnail.size());

	for (size_t i = 0; i < thumbnail.size(); ++i)
	{
		float x = waveformBounds.getX() + (i * pixelsPerPoint);
		float amplitude = thumbnail[i];
		float waveHeight = amplitude * (waveformBounds.getHeight() * 0.4f);

		float topY = centerY - waveHeight;
		float bottomY = centerY + waveHeight;

		if (!topPathStarted)
		{
			waveformPath.startNewSubPath(x, centerY);
			topPathStarted = true;
		}
		waveformPath.lineTo(x, topY);

		if (!bottomPathStarted)
		{
			bottomPath.startNewSubPath(x, centerY);
			bottomPathStarted = true;
		}
		bottomPath.lineTo(x, bottomY);
	}

	g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
	g.strokePath(bottomPath, juce::PathStrokeType(1.0f));

	g.setColour(juce::Colours::lightblue.withAlpha(0.3f));
	g.drawLine(static_cast<float>(waveformBounds.getX()), centerY,
		static_cast<float>(waveformBounds.getRight()), centerY, 0.5f);

	if (isPlaying && sampleEntry)
	{
		float duration = sampleEntry->duration;
		if (duration > 0.0f && playbackPosition >= 0.0f && playbackPosition <= duration)
		{
			float progress = playbackPosition / duration;
			float headX = waveformBounds.getX() + (progress * waveformBounds.getWidth());

			g.setColour(ColourPalette::playArmed);
			g.drawLine(headX, static_cast<float>(waveformBounds.getY()),
				headX, static_cast<float>(waveformBounds.getBottom()), 2.0f);
		}
	}

	g.restoreState();

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.5f));
	g.drawRoundedRectangle(waveformBounds.toFloat(), 4.0f, 1.0f);
}

void SampleBankItem::generateThumbnail()
{
	thumbnail.clear();

	if (audioBuffer.getNumSamples() == 0)
		return;

	int targetPoints = waveformBounds.getWidth();
	if (targetPoints <= 0)
		targetPoints = 100;

	int samplesPerPoint = juce::jmax(1, audioBuffer.getNumSamples() / targetPoints);

	for (int point = 0; point < targetPoints; ++point)
	{
		int sampleStart = point * samplesPerPoint;
		int sampleEnd = std::min(sampleStart + samplesPerPoint, audioBuffer.getNumSamples());

		if (sampleStart >= audioBuffer.getNumSamples())
			break;

		float rmsSum = 0.0f;
		float peak = 0.0f;
		int count = 0;

		for (int sample = sampleStart; sample < sampleEnd; ++sample)
		{
			for (int ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
			{
				float val = audioBuffer.getSample(ch, sample);
				rmsSum += val * val;
				peak = std::max(peak, std::abs(val));
				count++;
			}
		}

		float rms = count > 0 ? std::sqrt(rmsSum / count) : 0.0f;
		float finalValue = (rms * 0.7f) + (peak * 0.3f);

		thumbnail.push_back(finalValue);
	}
}

void SampleBankItem::mouseEnter(const juce::MouseEvent& event)
{
	if (!playButton.getBounds().contains(event.getPosition()) &&
		!deleteButton.getBounds().contains(event.getPosition()))
	{
		setMouseCursor(juce::MouseCursor::DraggingHandCursor);
	}
	else
	{
		setMouseCursor(juce::MouseCursor::NormalCursor);
	}
}

void SampleBankItem::mouseExit(const juce::MouseEvent& /*event*/)
{
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SampleBankItem::mouseDown(const juce::MouseEvent& event)
{
	if (playButton.getBounds().contains(event.getPosition()) ||
		deleteButton.getBounds().contains(event.getPosition()))
	{
		return;
	}

	if (event.mods.isLeftButtonDown())
	{
		isSelected = true;
		repaint();
	}
	else if (event.mods.isRightButtonDown())
	{
		showCategoryMenu();
	}
}

void SampleBankItem::showCategoryMenu()
{
	if (!sampleEntry)
		return;

	juce::String sampleId = sampleEntry->id;

	std::vector<juce::String> availableCategories;
	if (getCategoriesList)
	{
		availableCategories = getCategoriesList();
	}
	else
	{
		availableCategories = {
			"Drums", "Bass", "Melody", "Ambient", "Percussion",
			"Vocal", "FX", "Loops", "One-shots", "House",
			"Techno", "Hip-Hop", "Jazz", "Rock", "Electronic",
			"Piano", "Guitar", "Synth" };
	}

	auto* window = new CategoryWindow(sampleEntry->originalPrompt,
		sampleEntry->categories,
		availableCategories);

	window->onCategoriesChanged = [sampleId, &audioProcessor = this->audioProcessor, onCategoriesChanged = this->onCategoriesChanged](const std::vector<juce::String>& newCategories)
		{
			auto* bank = audioProcessor.getSampleBank();
			if (!bank)
				return;

			auto* sample = bank->getSample(sampleId);
			if (!sample)
				return;

			sample->categories = newCategories;

			if (onCategoriesChanged)
				onCategoriesChanged(sample, newCategories);
		};
}

void SampleBankItem::mouseDrag(const juce::MouseEvent& event)
{
	if (playButton.getBounds().contains(event.getMouseDownPosition()) ||
		deleteButton.getBounds().contains(event.getMouseDownPosition()))
	{
		return;
	}

	if (event.getDistanceFromDragStart() > 5 && !isDragging)
	{
		isDragging = true;
		repaint();

		if (event.mods.isCtrlDown() && sampleEntry)
		{
			juce::File sampleFile(sampleEntry->filePath);
			if (sampleFile.exists())
			{
				juce::File exportedFile = audioProcessor.exportSampleForDragDrop(sampleFile);

				if (exportedFile.existsAsFile())
				{
					juce::StringArray files;
					files.add(exportedFile.getFullPathName());
					DBG("Starting external drag with exported copy: " + exportedFile.getFullPathName());
					performExternalDragDropOfFiles(files, false);
				}
				else
				{
					juce::StringArray files;
					files.add(sampleFile.getFullPathName());
					DBG("Export failed, dragging original file: " + sampleFile.getFullPathName());
					performExternalDragDropOfFiles(files, false);
				}
				return;
			}
		}
		juce::DragAndDropContainer* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
		if (dragContainer)
		{
			juce::var dragData;
			dragData = sampleEntry->id;
			dragContainer->startDragging(dragData, this);
		}
	}
}

void SampleBankItem::mouseUp(const juce::MouseEvent& /*event*/)
{
	isDragging = false;
	isSelected = false;
	repaint();
}

void SampleBankItem::updateLabels()
{
	if (!sampleEntry)
		return;

	nameLabel.setText(sampleEntry->originalPrompt, juce::dontSendNotification);
	durationLabel.setText(formatDuration(sampleEntry->duration), juce::dontSendNotification);
	bpmLabel.setText(juce::String(sampleEntry->bpm, 1) + " BPM", juce::dontSendNotification);
	usageLabel.setText(formatUsage(), juce::dontSendNotification);
}

juce::String SampleBankItem::formatDuration(float seconds)
{
	int mins = (int)(seconds / 60);
	int secs = (int)(seconds) % 60;
	int ms = (int)((seconds - (int)seconds) * 100);

	if (mins > 0)
		return juce::String::formatted("%d:%02d.%02d", mins, secs, ms);
	else
		return juce::String::formatted("%d.%02ds", secs, ms);
}

juce::String SampleBankItem::formatUsage()
{
	if (sampleEntry->usedInProjects.empty())
		return "Unused";
	else if (sampleEntry->usedInProjects.size() == 1)
		return "1 project";
	else
		return juce::String(sampleEntry->usedInProjects.size()) + " projects";
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
	refreshSampleList();

	juce::Timer::callAfterDelay(500, [this]()
		{
			rebuildCategoryFilter();
			refreshSampleList(); });

	if (auto* bank = audioProcessor.getSampleBank())
	{
		bank->onBankChanged = [this]()
			{
				juce::MessageManager::callAsync([this]()
					{ refreshSampleList(); });
			};
	}
}

SampleBankPanel::~SampleBankPanel()
{
	stopPreview();
	if (auto* bank = audioProcessor.getSampleBank())
	{
		bank->onBankChanged = nullptr;
	}
}

void SampleBankPanel::playPreview(SampleBankEntry* entry)
{
	if (!entry)
		return;

	if (currentPreviewEntry && currentPreviewItem)
	{
		currentPreviewItem->setIsPlaying(false);
	}

	stopPreview();

	bool previewStarted = audioProcessor.previewSampleFromBank(entry->id);
	if (!previewStarted)
	{
		DBG("Failed to start preview for: " + entry->originalPrompt);
		return;
	}

	currentPreviewEntry = entry;

	for (auto& item : sampleItems)
	{
		if (item->getSampleEntry() == entry)
		{
			item->setIsPlaying(true);
			currentPreviewItem = item.get();
			break;
		}
	}

	startTimer(100);
}

void SampleBankPanel::stopPreview()
{
	audioProcessor.stopSamplePreview();

	if (currentPreviewItem)
	{
		currentPreviewItem->setIsPlaying(false);
		currentPreviewItem = nullptr;
	}

	currentPreviewEntry = nullptr;
	stopTimer();
}

void SampleBankPanel::timerCallback()
{
	if (currentPreviewEntry && !audioProcessor.isSamplePreviewing())
	{
		stopPreview();
	}
}

void SampleBankPanel::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::white);
	g.setColour(ColourPalette::backgroundDeep);
	g.drawRect(getLocalBounds(), 1);
}

void SampleBankPanel::resized()
{
	auto area = getLocalBounds().reduced(10);

	auto headerArea = area.removeFromTop(40);
	titleLabel.setBounds(headerArea.removeFromLeft(150));
	cleanupButton.setBounds(headerArea.removeFromRight(100).reduced(5));
	headerArea.removeFromRight(5);
	sortMenu.setBounds(headerArea.removeFromRight(150).reduced(5));

	auto infoArea = area.removeFromTop(50);
	infoLabel.setBounds(infoArea);
	area.removeFromTop(5);
	auto categoryArea = area.removeFromTop(35);
	categoryFilter.setBounds(categoryArea.removeFromLeft(150));
	categoryArea.removeFromLeft(10);
	categoryInput.setBounds(categoryArea);

	area.removeFromTop(5);
	auto buttonArea = area.removeFromTop(30);
	addCategoryButton.setBounds(buttonArea.removeFromLeft(60).reduced(5));
	buttonArea.removeFromLeft(5);
	editCategoryButton.setBounds(buttonArea.removeFromLeft(60).reduced(5));
	buttonArea.removeFromLeft(5);
	deleteCategoryButton.setBounds(buttonArea.removeFromLeft(60).reduced(5));

	area.removeFromTop(5);
	samplesViewport.setBounds(area);

	int totalHeight = 5;
	for (auto& item : sampleItems)
	{
		totalHeight += item->getRequiredHeight() + 5;
	}
	samplesContainer.setSize(area.getWidth() - 20, totalHeight);
}

void SampleBankPanel::refreshSampleList()
{
	sampleItems.clear();
	samplesContainer.removeAllChildren();
	auto* bank = audioProcessor.getSampleBank();
	if (!bank)
		return;

	auto samples = bank->getAllSamples();

	DBG("Before filtering: " + juce::String(samples.size()) + " samples");
	DBG("Current category ID: " + juce::String(currentCategoryId));

	if (currentCategoryId != 0)
	{
		juce::String selectedCategoryName;
		for (const auto& info : categoryInfos)
		{
			if (info.id == currentCategoryId)
			{
				selectedCategoryName = info.name;
				break;
			}
		}

		DBG("Filtering by category: " + selectedCategoryName);

		if (!selectedCategoryName.isEmpty())
		{
			samples.erase(
				std::remove_if(samples.begin(), samples.end(),
					[&selectedCategoryName](const SampleBankEntry* entry)
					{
						return std::find(entry->categories.begin(), entry->categories.end(),
							selectedCategoryName) == entry->categories.end();
					}),
				samples.end());
		}
	}

	switch (currentSortType)
	{
	case SortType::Time:
		std::sort(samples.begin(), samples.end(),
			[](const SampleBankEntry* a, const SampleBankEntry* b)
			{
				return a->creationTime > b->creationTime;
			});
		break;

	case SortType::Prompt:
		std::sort(samples.begin(), samples.end(),
			[](const SampleBankEntry* a, const SampleBankEntry* b)
			{
				return a->originalPrompt.compareIgnoreCase(b->originalPrompt) < 0;
			});
		break;

	case SortType::Usage:
		std::sort(samples.begin(), samples.end(),
			[](const SampleBankEntry* a, const SampleBankEntry* b)
			{
				return a->usedInProjects.size() > b->usedInProjects.size();
			});
		break;

	case SortType::BPM:
		std::sort(samples.begin(), samples.end(),
			[](const SampleBankEntry* a, const SampleBankEntry* b)
			{
				return a->bpm > b->bpm;
			});
		break;

	case SortType::Duration:
		std::sort(samples.begin(), samples.end(),
			[](const SampleBankEntry* a, const SampleBankEntry* b)
			{
				return a->duration > b->duration;
			});
		break;
	}

	createSampleItems(samples);
}

void SampleBankPanel::setVisible(bool shouldBeVisible)
{
	Component::setVisible(shouldBeVisible);
	if (shouldBeVisible)
	{
		for (auto& item : sampleItems)
		{
			item->loadAudioDataIfNeeded();
		}
		juce::Timer::callAfterDelay(100, [this]()
			{ refreshSampleList(); });
	}
	else
	{
		stopPreview();
		sampleItems.clear();
		samplesContainer.removeAllChildren();
	}
}

void SampleBankPanel::setupUI()
{
	addAndMakeVisible(titleLabel);
	titleLabel.setText("Sample Bank", juce::dontSendNotification);
	titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
	titleLabel.setColour(juce::Label::textColourId, ColourPalette::textAccent);

	addAndMakeVisible(infoLabel);
	infoLabel.setText("Preview plays on channel 9 (Preview). Enable multioutput in DAW to hear it.\nDrag: Drop on track | Ctrl+Drag: Drop in DAW | Right-click: Categories", juce::dontSendNotification);
	infoLabel.setFont(juce::FontOptions(12.0f));
	infoLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	infoLabel.setJustificationType(juce::Justification::centredLeft);

	addAndMakeVisible(cleanupButton);
	cleanupButton.setButtonText("Clean Unused");
	cleanupButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	cleanupButton.onClick = [this]()
		{ cleanupUnusedSamples(); };

	addAndMakeVisible(sortMenu);
	sortMenu.addItem("Sort by: Recent", SortType::Time);
	sortMenu.addItem("Sort by: Prompt", SortType::Prompt);
	sortMenu.addItem("Sort by: Usage", SortType::Usage);
	sortMenu.addItem("Sort by: BPM", SortType::BPM);
	sortMenu.addItem("Sort by: Duration", SortType::Duration);
	sortMenu.setSelectedId(SortType::Prompt);
	sortMenu.onChange = [this]()
		{
			currentSortType = static_cast<SortType>(sortMenu.getSelectedId());
			refreshSampleList();
		};

	addAndMakeVisible(samplesViewport);
	samplesViewport.setViewedComponent(&samplesContainer, false);
	samplesViewport.setScrollBarsShown(true, false);

	addAndMakeVisible(categoryFilter);
	for (const auto& info : categoryInfos)
	{
		categoryFilter.addItem(info.name, info.id + 1);
	}
	categoryFilter.setSelectedId(1);
	categoryFilter.onChange = [this]()
		{
			int selectedJuceId = categoryFilter.getSelectedId();
			if (selectedJuceId > 0)
			{
				currentCategoryId = selectedJuceId - 1;
			}
			else
			{
				currentCategoryId = 0;
			}

			bool editable = isCategoryEditable(currentCategoryId);
			editCategoryButton.setEnabled(editable);
			deleteCategoryButton.setEnabled(editable);

			refreshSampleList();
		};

	addAndMakeVisible(categoryInput);
	categoryInput.setTextToShowWhenEmpty("New category name...", ColourPalette::textSecondary);

	addAndMakeVisible(addCategoryButton);
	addCategoryButton.setButtonText("Add");
	addCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::emerald);
	addCategoryButton.onClick = [this]()
		{ addCategory(); };

	addAndMakeVisible(editCategoryButton);
	editCategoryButton.setButtonText("Edit");
	editCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::amber);
	editCategoryButton.onClick = [this]()
		{ editCategory(); };

	addAndMakeVisible(deleteCategoryButton);
	deleteCategoryButton.setButtonText("Delete");
	deleteCategoryButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	deleteCategoryButton.onClick = [this]()
		{ deleteCategory(); };

	editCategoryButton.setEnabled(false);
	deleteCategoryButton.setEnabled(false);
}

void SampleBankPanel::createSampleItems(const std::vector<SampleBankEntry*>& samples)
{
	int yPos = 5;

	for (auto* sampleEntry : samples)
	{
		auto item = std::make_unique<SampleBankItem>(sampleEntry, audioProcessor);

		item->onPreviewRequested = [this](SampleBankEntry* entry)
			{
				playPreview(entry);
			};
		item->onStopRequested = [this]()
			{
				stopPreview();
			};
		item->onDeleteRequested = [this](const juce::String& sampleId)
			{
				auto* entry = audioProcessor.getSampleBank()->getSample(sampleId);
				if (entry)
				{
					showDeleteConfirmation(sampleId, entry->originalPrompt);
				}
			};
		item->onCategoriesChanged = [this](SampleBankEntry* /*entry*/, const std::vector<juce::String>& /*newCategories*/)
			{
				auto* bank = audioProcessor.getSampleBank();
				if (bank)
				{
					bank->saveBankData();
				}
				refreshSampleList();
			};
		item->getCategoriesList = [this]() -> std::vector<juce::String>
			{
				std::vector<juce::String> categories;
				for (const auto& info : categoryInfos)
				{
					if (info.id > 0)
					{
						categories.push_back(info.name);
					}
				}
				return categories;
			};
		int itemHeight = item->getRequiredHeight();
		item->setBounds(5, yPos, samplesContainer.getWidth() - 10, itemHeight);

		samplesContainer.addAndMakeVisible(item.get());

		if (isVisible())
		{
			item->loadAudioDataIfNeeded();
		}

		sampleItems.push_back(std::move(item));
		yPos += itemHeight + 5;
	}

	samplesContainer.setSize(samplesViewport.getWidth() - 20, yPos + 5);
	resized();
}

void SampleBankPanel::deleteSample(const juce::String& sampleId)
{
	auto* bank = audioProcessor.getSampleBank();
	if (bank && bank->removeSample(sampleId))
	{
		refreshSampleList();
	}
}

void SampleBankPanel::cleanupUnusedSamples()
{
	auto* bank = audioProcessor.getSampleBank();
	if (!bank)
		return;

	auto unusedSamples = bank->getUnusedSamples();
	if (unusedSamples.empty())
	{
		juce::AlertWindow::showMessageBoxAsync(
			juce::MessageBoxIconType::InfoIcon,
			"Clean Unused Samples",
			"No unused samples found.",
			"OK");
		return;
	}

	juce::String message = "Found " + juce::String(unusedSamples.size()) +
		" unused samples.\n\nDelete them all?";

	juce::AlertWindow::showAsync(
		juce::MessageBoxOptions()
		.withIconType(juce::MessageBoxIconType::WarningIcon)
		.withTitle("Clean Unused Samples")
		.withMessage(message)
		.withButton("Delete All")
		.withButton("Cancel"),
		[this, bank](int result)
		{
			if (result == 1)
			{
				int removed = bank->removeUnusedSamples();
				refreshSampleList();

				juce::AlertWindow::showMessageBoxAsync(
					juce::MessageBoxIconType::InfoIcon,
					"Cleanup Complete",
					"Removed " + juce::String(removed) + " unused samples.",
					"OK");
			}
		});
}

void SampleBankPanel::showDeleteConfirmation(const juce::String& sampleId, const juce::String& sampleName)
{
	auto* entry = audioProcessor.getSampleBank()->getSample(sampleId);
	if (!entry)
		return;

	juce::String message = "Delete sample:\n'" + sampleName + "'";

	if (!entry->usedInProjects.empty())
	{
		message += "\n\nWarning: This sample is used in " +
			juce::String(entry->usedInProjects.size()) + " project(s).";
	}

	juce::AlertWindow::showAsync(
		juce::MessageBoxOptions()
		.withIconType(juce::MessageBoxIconType::WarningIcon)
		.withTitle("Delete Sample")
		.withMessage(message)
		.withButton("Delete")
		.withButton("Cancel"),
		[this, sampleId](int result)
		{
			if (result == 1)
			{
				deleteSample(sampleId);
			}
		});
}

void SampleBankPanel::addCategory()
{
	juce::String newCategoryName = categoryInput.getText().trim();

	if (newCategoryName.isEmpty())
	{
		juce::AlertWindow::showMessageBoxAsync(
			juce::MessageBoxIconType::WarningIcon,
			"Add Category",
			"Please enter a category name.",
			"OK");
		return;
	}

	for (const auto& info : categoryInfos)
	{
		if (info.name.compareIgnoreCase(newCategoryName) == 0)
		{
			juce::AlertWindow::showMessageBoxAsync(
				juce::MessageBoxIconType::WarningIcon,
				"Add Category",
				"Category '" + newCategoryName + "' already exists.",
				"OK");
			return;
		}
	}

	int newId = std::max(20, getNextCategoryId());

	DBG("Adding new category: " + newCategoryName + " with ID: " + juce::String(newId));

	categoryInfos.push_back({ newId, newCategoryName });
	rebuildCategoryFilter();

	categoryFilter.setSelectedId(newId + 1);
	currentCategoryId = newId;

	editCategoryButton.setEnabled(true);
	deleteCategoryButton.setEnabled(true);

	categoryInput.clear();
	saveCategoriesConfig();
	refreshSampleList();
}

void SampleBankPanel::editCategory()
{

	if (!isCategoryEditable(currentCategoryId))
	{
		juce::AlertWindow::showMessageBoxAsync(
			juce::MessageBoxIconType::WarningIcon,
			"Edit Category",
			"Cannot edit built-in categories.",
			"OK");
		return;
	}

	auto it = std::find_if(categoryInfos.begin(), categoryInfos.end(),
		[this](const CategoryInfo& info)
		{ return info.id == currentCategoryId; });

	if (it == categoryInfos.end())
		return;

	juce::String newName = categoryInput.getText().trim();
	if (newName.isEmpty())
	{
		categoryInput.setText(it->name, juce::dontSendNotification);
		return;
	}

	for (const auto& info : categoryInfos)
	{
		if (info.id != currentCategoryId && info.name.compareIgnoreCase(newName) == 0)
		{
			juce::AlertWindow::showMessageBoxAsync(
				juce::MessageBoxIconType::WarningIcon,
				"Edit Category",
				"Category '" + newName + "' already exists.",
				"OK");
			return;
		}
	}

	juce::String oldName = it->name;
	it->name = newName;

	int currentId = categoryFilter.getSelectedId();
	categoryFilter.clear();
	for (const auto& info : categoryInfos)
	{
		categoryFilter.addItem(info.name, info.id + 1);
	}
	categoryFilter.setSelectedId(currentId);

	auto* bank = audioProcessor.getSampleBank();
	if (bank)
	{
		auto samples = bank->getAllSamples();
		for (auto* sample : samples)
		{
			auto& categories = sample->categories;
			auto catIt = std::find(categories.begin(), categories.end(), oldName);
			if (catIt != categories.end())
			{
				*catIt = newName;
			}
		}
	}

	categoryInput.clear();
	saveCategoriesConfig();
	refreshSampleList();
}

void SampleBankPanel::deleteCategory()
{
	if (!isCategoryEditable(currentCategoryId))
	{
		juce::AlertWindow::showMessageBoxAsync(
			juce::MessageBoxIconType::WarningIcon,
			"Delete Category",
			"Cannot delete built-in categories.",
			"OK");
		return;
	}

	auto it = std::find_if(categoryInfos.begin(), categoryInfos.end(),
		[this](const CategoryInfo& info)
		{ return info.id == currentCategoryId; });

	if (it == categoryInfos.end())
		return;

	juce::String categoryName = it->name;
	int categoryIdToDelete = currentCategoryId;

	juce::AlertWindow::showAsync(
		juce::MessageBoxOptions()
		.withIconType(juce::MessageBoxIconType::QuestionIcon)
		.withTitle("Delete Category")
		.withMessage("Delete category '" + categoryName + "'?\n\nSamples will not be deleted, only the category assignment.")
		.withButton("Delete")
		.withButton("Cancel"),
		[this, categoryName, categoryIdToDelete](int result)
		{
			if (result == 1)
			{
				auto* bank = audioProcessor.getSampleBank();
				if (bank)
				{
					auto samples = bank->getAllSamples();
					for (auto* sample : samples)
					{
						auto& categories = sample->categories;
						categories.erase(
							std::remove(categories.begin(), categories.end(), categoryName),
							categories.end());
					}
					bank->saveBankData();
				}
				categoryInfos.erase(
					std::remove_if(categoryInfos.begin(), categoryInfos.end(),
						[categoryIdToDelete](const CategoryInfo& info)
						{
							return info.id == categoryIdToDelete;
						}),
					categoryInfos.end());

				rebuildCategoryFilter();
				categoryFilter.setSelectedId(1);
				currentCategoryId = 0;
				editCategoryButton.setEnabled(false);
				deleteCategoryButton.setEnabled(false);

				saveCategoriesConfig();
				refreshSampleList();

				DBG("Category deleted: " + categoryName);
			}
		});
}

bool SampleBankPanel::isCategoryEditable(int categoryId) const
{
	bool editable = categoryId >= 20;
	return editable;
}

int SampleBankPanel::getNextCategoryId()
{
	int maxId = 19;
	for (const auto& info : categoryInfos)
	{
		maxId = std::max(maxId, info.id);
	}
	return maxId + 1;
}

void SampleBankPanel::saveCategoriesConfig()
{
	juce::File configFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("categories.json");

	juce::DynamicObject::Ptr config = new juce::DynamicObject();
	juce::Array<juce::var> categoriesArray;

	for (const auto& info : categoryInfos)
	{
		if (info.id > 0)
		{
			juce::DynamicObject::Ptr categoryData = new juce::DynamicObject();
			categoryData->setProperty("id", info.id);
			categoryData->setProperty("name", info.name);
			categoriesArray.add(categoryData.get());
		}
	}

	config->setProperty("categories", categoriesArray);
	juce::String jsonString = juce::JSON::toString(juce::var(config.get()));

	configFile.getParentDirectory().createDirectory();
	configFile.replaceWithText(jsonString);
}

void SampleBankPanel::loadCategoriesConfig()
{
	juce::File configFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		.getChildFile("OBSIDIAN-Neural")
		.getChildFile("categories.json");

	if (!configFile.exists())
		return;

	juce::var config = juce::JSON::parse(configFile);
	if (!config.isObject())
		return;

	auto* configObj = config.getDynamicObject();
	if (!configObj)
		return;

	auto categoriesVar = configObj->getProperty("categories");
	if (!categoriesVar.isArray())
		return;

	auto* categoriesArray = categoriesVar.getArray();

	categoryInfos.erase(
		std::remove_if(categoryInfos.begin(), categoryInfos.end(),
			[](const CategoryInfo& info)
			{ return info.id >= 20; }),
		categoryInfos.end());

	for (int i = 0; i < categoriesArray->size(); ++i)
	{
		auto categoryVar = categoriesArray->getUnchecked(i);
		if (!categoryVar.isObject())
			continue;

		auto* categoryObj = categoryVar.getDynamicObject();
		if (!categoryObj)
			continue;

		int id = categoryObj->getProperty("id");
		juce::String name = categoryObj->getProperty("name").toString();

		if (id >= 20)
		{
			categoryInfos.push_back({ id, name });
		}
	}
}

void SampleBankPanel::rebuildCategoryFilter()
{
	int currentId = categoryFilter.getSelectedId();
	categoryFilter.clear();

	for (const auto& info : categoryInfos)
	{
		categoryFilter.addItem(info.name, info.id + 1);
	}
	if (std::any_of(categoryInfos.begin(), categoryInfos.end(),
		[currentId](const CategoryInfo& info)
		{ return info.id == currentId - 1; }))
	{
		categoryFilter.setSelectedId(currentId);
	}
	else
	{
		categoryFilter.setSelectedId(1);
	}
}