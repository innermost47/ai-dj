#include "DrawingCanvas.h"

KeywordBadge::KeywordBadge(const juce::String &text) : juce::TextButton(text)
{
}

void KeywordBadge::mouseDown(const juce::MouseEvent &e)
{
	if (e.mods.isRightButtonDown() && onRightClick)
	{
		onRightClick(e);
	}
	else
	{
		juce::TextButton::mouseDown(e);
	}
}

ColorSwatch::ColorSwatch(juce::Colour color) : buttonColor(color)
{
	setColour(juce::TextButton::buttonColourId, color);
}

void ColorSwatch::paintButton(juce::Graphics &g, bool /*shouldDrawButtonAsHighlighted*/,
                              bool /*shouldDrawButtonAsDown*/)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(buttonColor);
	g.fillRoundedRectangle(bounds.reduced(3), ObsidianSizes::CORNER);

	if (getToggleState())
	{
		g.setColour(ColourPalette::buttonPrimary);
		g.drawRoundedRectangle(bounds.reduced(1), ObsidianSizes::CORNER, 3.0f);
	}
	else
	{
		g.setColour(ColourPalette::backgroundLight);
		g.drawRoundedRectangle(bounds.reduced(3), ObsidianSizes::CORNER, 1.0f);
	}
}

DrawingCanvas::CanvasState DrawingCanvas::getState() const
{
	CanvasState state;
	state.imageBase64 = const_cast<DrawingCanvas *>(this)->getBase64Image();

	switch (currentBrushType)
	{
	case BrushType::Pencil:
		state.brushType = 0;
		break;
	case BrushType::Brush:
		state.brushType = 1;
		break;
	case BrushType::Airbrush:
		state.brushType = 2;
		break;
	case BrushType::Fill:
		state.brushType = 3;
		break;
	case BrushType::Eraser:
		state.brushType = 4;
		break;
	}

	state.brushSize = currentBrushSize;
	state.brushColor = currentColor;
	state.selectedKeywords = selectedKeywords;

	return state;
}

DrawingCanvas::DrawingCanvas(DjIaVstProcessor &proc) : audioProcessor(proc)
{
	canvas = juce::Image(juce::Image::RGB, 400, 400, true);
	clearCanvas();
	resetHistory();
	setLookAndFeel(&CustomLookAndFeel::getInstance());
	setupUI();
	setupKeywordsUI();
	setWantsKeyboardFocus(true);
	startTimerHz(60);
}

DrawingCanvas::~DrawingCanvas()
{
	setLookAndFeel(nullptr);
}

void DrawingCanvas::setGenerating(bool generating)
{
	isGenerating = generating;
	generateButton.setEnabled(!generating);
	generateButton.setButtonText(generating ? "Generating..." : "Generate");
}

void DrawingCanvas::paint(juce::Graphics &g)
{
	const float corner = ObsidianSizes::CORNER;

	auto canvasFrame = canvasAreaBounds.expanded(6).toFloat();

	g.setColour(juce::Colours::black.withAlpha(0.4f));
	g.fillRoundedRectangle(canvasFrame.translated(0, 2), corner);

	juce::ColourGradient frameGradient(ColourPalette::backgroundDeep.brighter(0.03f), canvasFrame.getX(),
	                                   canvasFrame.getY(), ColourPalette::backgroundDeep.darker(0.03f),
	                                   canvasFrame.getX(), canvasFrame.getBottom(), false);
	g.setGradientFill(frameGradient);
	g.fillRoundedRectangle(canvasFrame, corner);

	{
		juce::Graphics::ScopedSaveState saveState(g);
		g.reduceClipRegion(canvasAreaBounds);
		g.drawImageAt(canvas, canvasAreaBounds.getX(), canvasAreaBounds.getY());
	}

	g.setColour(ColourPalette::lightGrey.withAlpha(0.4f));
	g.drawRect(canvasAreaBounds.toFloat(), 1.0f);

	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.3f));
	g.drawRoundedRectangle(canvasFrame, corner, 0.8f);

	if (!keywordsPanelBounds.isEmpty())
	{
		auto kwFrame = keywordsPanelBounds.toFloat();

		juce::ColourGradient kwGradient(ColourPalette::backgroundDeep.brighter(0.02f), kwFrame.getX(), kwFrame.getY(),
		                                ColourPalette::backgroundDeep.darker(0.04f), kwFrame.getX(),
		                                kwFrame.getBottom(), false);
		g.setGradientFill(kwGradient);
		g.fillRoundedRectangle(kwFrame, corner);

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawRoundedRectangle(kwFrame, corner, 0.8f);

		auto accentBar = juce::Rectangle<float>(kwFrame.getX() + 8.0f, kwFrame.getY() + 14.0f, 3.0f, 18.0f);
		g.setColour(ColourPalette::lightGrey);
		g.fillRoundedRectangle(accentBar, 1.5f);
	}

	if (!toolsPanelBounds.isEmpty())
	{
		auto toolsFrame = toolsPanelBounds.toFloat();

		juce::ColourGradient toolsGradient(ColourPalette::backgroundDeep.brighter(0.02f), toolsFrame.getX(),
		                                   toolsFrame.getY(), ColourPalette::backgroundDeep.darker(0.04f),
		                                   toolsFrame.getX(), toolsFrame.getBottom(), false);
		g.setGradientFill(toolsGradient);
		g.fillRoundedRectangle(toolsFrame, corner);

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawRoundedRectangle(toolsFrame, corner, 0.8f);

		g.setColour(juce::Colours::white.withAlpha(0.025f));
		auto topHighlight = toolsFrame.withHeight(toolsFrame.getHeight() * 0.4f);
		g.fillRoundedRectangle(topHighlight, corner);
	}
}

void DrawingCanvas::resized()
{
	auto bounds = getLocalBounds().reduced(6);

	auto mainArea = bounds.removeFromTop(416);

	auto canvasContainer = mainArea.removeFromLeft(416);
	canvasAreaBounds = canvasContainer.withSizeKeepingCentre(400, 400);

	mainArea.removeFromLeft(12);

	keywordsPanelBounds = mainArea;
	auto keywordsArea = mainArea.reduced(14, 12);

	auto keywordsHeaderRow = keywordsArea.removeFromTop(28);
	keywordsHeaderRow.removeFromLeft(8);
	keywordsLabel.setBounds(keywordsHeaderRow);

	keywordsArea.removeFromTop(8);

	auto inputRow = keywordsArea.removeFromTop(34);
	addKeywordButton.setBounds(inputRow.removeFromRight(38));
	inputRow.removeFromRight(6);
	keywordInput.setBounds(inputRow);

	keywordsArea.removeFromTop(10);

	keywordsViewport.setBounds(keywordsArea);

	int badgeWidth = 105;
	int badgeHeight = 28;
	int spacingX = 6;
	int spacingY = 6;

	int totalBadges = keywordBadges.size();
	if (totalBadges > 0)
	{
		int availableHeight = keywordsArea.getHeight() - keywordsViewport.getScrollBarThickness();
		int maxRows = juce::jmax(1, availableHeight / (badgeHeight + spacingY));
		int numColumns = (totalBadges + maxRows - 1) / maxRows;
		int totalWidth = numColumns * (badgeWidth + spacingX) + spacingX;

		keywordsBadgesContainer.setSize(juce::jmax(totalWidth, keywordsArea.getWidth()), availableHeight);

		int badgeIndex = 0;
		for (auto *badge : keywordBadges)
		{
			int col = badgeIndex / maxRows;
			int row = badgeIndex % maxRows;
			int x = col * (badgeWidth + spacingX) + spacingX;
			int y = row * (badgeHeight + spacingY) + spacingY;
			badge->setBounds(x, y, badgeWidth, badgeHeight);
			badgeIndex++;
		}
	}

	bounds.removeFromTop(12);

	toolsPanelBounds = bounds;
	auto toolsArea = bounds.reduced(14, 10);

	auto brushRow = toolsArea.removeFromTop(42);

	const int actionBtnW = 60;
	clearButton.setBounds(brushRow.removeFromRight(actionBtnW));
	brushRow.removeFromRight(6);
	redoButton.setBounds(brushRow.removeFromRight(actionBtnW));
	brushRow.removeFromRight(6);
	undoButton.setBounds(brushRow.removeFromRight(actionBtnW));
	brushRow.removeFromRight(16);

	int numBrushes = 5;
	int btnW = (brushRow.getWidth() - (numBrushes - 1) * 6) / numBrushes;

	pencilButton.setBounds(brushRow.removeFromLeft(btnW));
	brushRow.removeFromLeft(6);
	brushButton.setBounds(brushRow.removeFromLeft(btnW));
	brushRow.removeFromLeft(6);
	airbrushButton.setBounds(brushRow.removeFromLeft(btnW));
	brushRow.removeFromLeft(6);
	fillButton.setBounds(brushRow.removeFromLeft(btnW));
	brushRow.removeFromLeft(6);
	eraserButton.setBounds(brushRow.removeFromLeft(btnW));

	toolsArea.removeFromTop(10);

	auto sizeRow = toolsArea.removeFromTop(36);
	brushSizeLabel.setBounds(sizeRow.removeFromLeft(56));
	sizeRow.removeFromLeft(8);
	brushSizeSlider.setBounds(sizeRow);

	toolsArea.removeFromTop(10);

	auto colorRow = toolsArea.removeFromTop(38);
	colorLabel.setBounds(colorRow.removeFromLeft(56));
	colorRow.removeFromLeft(8);

	const int generateWidth = 140;
	generateButton.setBounds(colorRow.removeFromRight(generateWidth));
	colorRow.removeFromRight(12);

	int numSwatches = colorSwatches.size();
	if (numSwatches > 0)
	{
		int totalSpacing = (numSwatches - 1) * 5;
		int availableWidth = colorRow.getWidth() - totalSpacing;
		int swatchWidth = availableWidth / numSwatches;

		for (int i = 0; i < numSwatches; ++i)
		{
			auto *swatch = colorSwatches[i];
			swatch->setBounds(colorRow.removeFromLeft(swatchWidth));
			if (i < numSwatches - 1)
				colorRow.removeFromLeft(5);
		}
	}
}

void DrawingCanvas::mouseMove(const juce::MouseEvent &e)
{
	if (isPointInCanvas(e.getPosition()))
	{
		updateMouseCursor();
	}
	else
	{
		setMouseCursor(juce::MouseCursor::NormalCursor);
	}
}

void DrawingCanvas::mouseDown(const juce::MouseEvent &e)
{
	if (isPointInCanvas(e.getPosition()))
	{
		updateMouseCursor();

		if (currentBrushType == BrushType::Fill)
		{
			if (undoHistory.empty() || historyIndex == -1)
			{
				saveToHistory();
			}

			auto point = getCanvasPoint(e.getPosition());
			juce::Colour targetColor = canvas.getPixelAt(point.x, point.y);
			floodFill(point.x, point.y, targetColor, currentColor);
			needsRepaint = true;
			saveToHistory();
		}
		else
		{
			if (undoHistory.empty() || historyIndex == -1)
			{
				saveToHistory();
			}

			isDrawing = true;
			lastPoint = getCanvasPoint(e.getPosition());

			juce::Graphics g(canvas);
			drawAtPoint(g, lastPoint);
			needsRepaint = true;
		}
	}
}

void DrawingCanvas::mouseDrag(const juce::MouseEvent &e)
{
	if (isDrawing && isPointInCanvas(e.getPosition()))
	{
		updateMouseCursor();
		auto currentPoint = getCanvasPoint(e.getPosition());
		if (lastPoint != currentPoint)
		{
			juce::Graphics g(canvas);
			drawLine(g, lastPoint, currentPoint);
			lastPoint = currentPoint;
			needsRepaint = true;
		}
	}
}

void DrawingCanvas::mouseUp(const juce::MouseEvent &)
{
	if (isDrawing)
	{
		saveToHistory();
	}
	isDrawing = false;
}

void DrawingCanvas::drawAtPoint(juce::Graphics &g, juce::Point<int> point)
{
	switch (currentBrushType)
	{
	case BrushType::Pencil:
		g.setColour(currentColor);
		g.fillRect(point.x, point.y, 2, 2);
		break;

	case BrushType::Brush:
		g.setColour(currentColor);
		g.fillEllipse(point.x - currentBrushSize / 2.0f, point.y - currentBrushSize / 2.0f, currentBrushSize,
		              currentBrushSize);
		break;

	case BrushType::Airbrush:
	{
		int sprayRadius = std::max(5, (int)(currentBrushSize * 1.5f));
		int numParticles = std::max(10, (int)(currentBrushSize * 3));

		for (int i = 0; i < numParticles; ++i)
		{
			float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
			float dist = random.nextFloat() * sprayRadius;
			int px = point.x + (int)(std::cos(angle) * dist);
			int py = point.y + (int)(std::sin(angle) * dist);

			if (canvas.getBounds().contains(px, py))
			{
				g.setColour(currentColor.withAlpha(0.15f));
				g.fillEllipse(px - 1.0f, py - 1.0f, 2.0f, 2.0f);
			}
		}
	}
	break;

	case BrushType::Eraser:
		g.setColour(juce::Colours::white);
		g.fillEllipse(point.x - currentBrushSize / 2.0f, point.y - currentBrushSize / 2.0f, currentBrushSize,
		              currentBrushSize);
		break;
	}
}

void DrawingCanvas::drawLine(juce::Graphics &g, juce::Point<int> from, juce::Point<int> to)
{
	float dx = (float)(to.x - from.x);
	float dy = (float)(to.y - from.y);
	float distance = std::sqrt(dx * dx + dy * dy);

	if (distance < 1.0f)
	{
		drawAtPoint(g, to);
		return;
	}

	float stepSize = 1.0f;

	switch (currentBrushType)
	{
	case BrushType::Pencil:
		stepSize = 1.0f;
		break;
	case BrushType::Brush:
		stepSize = currentBrushSize * 0.25f;
		break;
	case BrushType::Airbrush:
		stepSize = currentBrushSize * 0.5f;
		break;
	case BrushType::Eraser:
		stepSize = currentBrushSize * 0.25f;
		break;
	}

	stepSize = std::max(1.0f, stepSize);
	int steps = std::max(1, (int)(distance / stepSize));

	for (int i = 0; i <= steps; ++i)
	{
		float t = (float)i / steps;
		int x = (int)(from.x + t * dx);
		int y = (int)(from.y + t * dy);
		drawAtPoint(g, {x, y});
	}
}

void DrawingCanvas::clearCanvas()
{
	juce::Graphics g(canvas);
	g.fillAll(juce::Colour(0xff1a1a1a));
	repaint();
}

void DrawingCanvas::clearCanvasWithConfirmation()
{
	if (!isShowing() || !isVisible())
		return;

	ObsidianAlertManager::showConfirm(
	    this, "Clear Canvas", "Are you sure you want to clear the canvas? This will erase the undo/redo history.",
	    "Clear", "Cancel",
	    [this](bool confirmed)
	    {
		    if (confirmed && isShowing())
		    {
			    juce::Graphics g(canvas);
			    g.fillAll(juce::Colours::white);
			    repaint();
			    undoHistory.clear();
			    historyIndex = -1;
			    updateUndoRedoButtons();
		    }
	    });
}

juce::String DrawingCanvas::getBase64Image()
{
	juce::MemoryOutputStream memStream;
	juce::PNGImageFormat pngFormat;

	if (pngFormat.writeImageToStream(canvas, memStream))
	{
		juce::MemoryBlock block = memStream.getMemoryBlock();
		juce::String base64 = juce::Base64::toBase64(block.getData(), block.getSize());

		int padding = 0;
		if (base64.endsWith("=="))
			padding = 2;
		else if (base64.endsWith("="))
			padding = 1;

		return base64;
	}

	return {};
}

void DrawingCanvas::loadFromBase64(const juce::String &base64Data)
{
	if (base64Data.isEmpty())
	{
		return;
	}

	juce::MemoryOutputStream tempStream;

	bool success = juce::Base64::convertFromBase64(tempStream, base64Data);

	if (!success || tempStream.getDataSize() == 0)
	{
		return;
	}
	auto decodedData = tempStream.getMemoryBlock();
	juce::MemoryInputStream imageStream(decodedData, false);

	juce::PNGImageFormat pngFormat;
	auto loadedImage = pngFormat.decodeImage(imageStream);

	if (loadedImage.isValid())
	{

		canvas = loadedImage;

		repaint();
		needsRepaint = true;
	}
}

void DrawingCanvas::setState(const CanvasState &state)
{
	if (!state.imageBase64.isEmpty())
	{
		loadFromBase64(state.imageBase64);
	}
	resetHistory();

	switch (state.brushType)
	{
	case 0:
		currentBrushType = BrushType::Pencil;
		pencilButton.setToggleState(true, juce::dontSendNotification);
		break;
	case 1:
		currentBrushType = BrushType::Brush;
		brushButton.setToggleState(true, juce::dontSendNotification);
		break;
	case 2:
		currentBrushType = BrushType::Airbrush;
		airbrushButton.setToggleState(true, juce::dontSendNotification);
		break;
	case 3:
		currentBrushType = BrushType::Fill;
		fillButton.setToggleState(true, juce::dontSendNotification);
		break;
	case 4:
		currentBrushType = BrushType::Eraser;
		eraserButton.setToggleState(true, juce::dontSendNotification);
		break;
	}

	currentBrushSize = state.brushSize;
	brushSizeSlider.setValue(state.brushSize, juce::dontSendNotification);

	currentColor = state.brushColor;
	updateColorSwatchSelection();

	selectedKeywords = state.selectedKeywords;

	for (auto *badge : keywordBadges)
	{
		juce::String keyword = badge->getButtonText();
		badge->setToggleState(selectedKeywords.contains(keyword), juce::dontSendNotification);
	}

	repaint();
}

juce::StringArray DrawingCanvas::getDefaultKeywords()
{
	return {"drums", "bass", "techno", "ambient", "glitch", "synth",      "melody", "percussion", "kick",   "snare",
	        "hihat", "808",  "acid",   "reverb",  "delay",  "distortion", "filter", "groove",     "rhythm", "texture"};
}

void DrawingCanvas::updateMouseCursor()
{
	switch (currentBrushType)
	{
	case BrushType::Brush:
	case BrushType::Airbrush:
	case BrushType::Pencil:
	case BrushType::Fill:
		setMouseCursor(juce::MouseCursor::CrosshairCursor);
		break;

	case BrushType::Eraser:
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
		break;
	}
}

void DrawingCanvas::setupKeywordsUI()
{
	availableKeywords = getDefaultKeywords();

	auto customKeywords = audioProcessor.getCustomKeywords();
	for (const auto &keyword : customKeywords)
	{
		if (!availableKeywords.contains(keyword))
		{
			availableKeywords.add(keyword);
		}
	}

	addAndMakeVisible(keywordsLabel);
	keywordsLabel.setText("Keywords", juce::dontSendNotification);
	keywordsLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	keywordsLabel.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::bold));

	addAndMakeVisible(keywordInput);
	keywordInput.setFont(juce::FontOptions(13.0f));
	keywordInput.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundLight);
	keywordInput.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
	keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep);
	keywordInput.setTextToShowWhenEmpty("Add keyword...", ColourPalette::textSecondary);
	keywordInput.onReturnKey = [this]() { addCustomKeyword(); };

	addAndMakeVisible(addKeywordButton);
	addKeywordButton.loadIcon(BinaryData::plus_svg, BinaryData::plus_svgSize);
	addKeywordButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	addKeywordButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	addKeywordButton.onClick = [this]() { addCustomKeyword(); };

	addAndMakeVisible(keywordsViewport);
	keywordsViewport.setViewedComponent(&keywordsBadgesContainer, false);
	keywordsViewport.setScrollBarsShown(false, true);

	addKeywordButton.setIconSize(18.0f);

	updateKeywordBadges();
}

void DrawingCanvas::updateKeywordBadges()
{
	keywordBadges.clear();

	juce::StringArray sortedKeywords = availableKeywords;
	sortedKeywords.sort(true);

	for (const auto &keyword : sortedKeywords)
	{
		auto *badge = new KeywordBadge(keyword);
		badge->setClickingTogglesState(true);
		badge->setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		badge->setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		badge->setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		badge->setColour(juce::TextButton::textColourOnId, juce::Colours::white);

		if (selectedKeywords.contains(keyword))
		{
			badge->setToggleState(true, juce::dontSendNotification);
		}

		badge->onClick = [this, keyword]() { toggleKeyword(keyword); };

		badge->onRightClick = [this, badge, keyword](const juce::MouseEvent &)
		{ showKeywordContextMenu(badge, keyword); };

		keywordsBadgesContainer.addAndMakeVisible(badge);
		keywordBadges.add(badge);
	}

	resized();
}

void DrawingCanvas::showKeywordContextMenu(KeywordBadge *badge, const juce::String &keyword)
{
	juce::PopupMenu menu;

	bool isDefaultKeyword = getDefaultKeywords().contains(keyword);

	if (!isDefaultKeyword)
	{
		menu.addItem(1, "Edit");
		menu.addItem(2, "Delete");
	}
	else
	{
		menu.addItem(1, "Edit", false);
		menu.addItem(2, "Delete", false);
		menu.addSeparator();
		menu.addItem(3, "Cannot edit default keywords", false);
	}

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(badge),
	                   [this, keyword, isDefaultKeyword](int result)
	                   {
		                   if (result == 1 && !isDefaultKeyword)
		                   {
			                   editKeyword(keyword);
		                   }
		                   else if (result == 2 && !isDefaultKeyword)
		                   {
			                   deleteKeyword(keyword);
		                   }
	                   });
}

void DrawingCanvas::editKeyword(const juce::String &oldKeyword)
{
	ObsidianAlertManager::showEditPrompt(
	    this, oldKeyword,
	    [this, oldKeyword](const juce::String &newKeyword)
	    {
		    juce::String kw = newKeyword.trim().toLowerCase();

		    if (!isKeywordValid(kw))
		    {
			    ObsidianAlertManager::showError(
			        this, "Invalid Keyword",
			        "Keyword must be 1-15 characters and contain only letters, numbers, spaces or hyphens.");
			    return;
		    }
		    if (kw != oldKeyword && availableKeywords.contains(kw))
		    {
			    ObsidianAlertManager::showError(this, "Duplicate Keyword", "This keyword already exists.");
			    return;
		    }

		    int index = availableKeywords.indexOf(oldKeyword);
		    if (index >= 0)
			    availableKeywords.set(index, kw);

		    if (selectedKeywords.contains(oldKeyword))
		    {
			    selectedKeywords.removeString(oldKeyword);
			    selectedKeywords.add(kw);
		    }

		    auto customKeywords = audioProcessor.getCustomKeywords();
		    if (customKeywords.contains(oldKeyword))
		    {
			    juce::StringArray newCustomKeywords;
			    for (const auto &k : customKeywords)
				    newCustomKeywords.add(k == oldKeyword ? kw : k);
			    audioProcessor.setCustomKeywords(newCustomKeywords);
		    }

		    updateKeywordBadges();
	    });
}

void DrawingCanvas::deleteKeyword(const juce::String &keyword)
{
	ObsidianAlertManager::showConfirm(this, "Delete Keyword", "Are you sure you want to delete \"" + keyword + "\"?",
	                                  "Delete", "Cancel",
	                                  [this, keyword](bool confirmed)
	                                  {
		                                  if (confirmed)
		                                  {
			                                  availableKeywords.removeString(keyword);
			                                  selectedKeywords.removeString(keyword);
			                                  auto customKeywords = audioProcessor.getCustomKeywords();
			                                  customKeywords.removeString(keyword);
			                                  audioProcessor.setCustomKeywords(customKeywords);
			                                  updateKeywordBadges();
		                                  }
	                                  });
}

void DrawingCanvas::toggleKeyword(const juce::String &keyword)
{
	if (selectedKeywords.contains(keyword))
	{
		selectedKeywords.removeString(keyword);
	}
	else
	{
		selectedKeywords.add(keyword);
	}
}

bool DrawingCanvas::isKeywordValid(const juce::String &keyword) const
{
	if (keyword.trim().isEmpty())
		return false;

	if (keyword.trim().length() > 15)
		return false;

	juce::String trimmed = keyword.trim();
	for (int i = 0; i < trimmed.length(); ++i)
	{
		juce::juce_wchar c = trimmed[i];
		if (!juce::CharacterFunctions::isLetterOrDigit(c) && c != ' ' && c != '-')
			return false;
	}

	return true;
}

void DrawingCanvas::addCustomKeyword()
{
	juce::String newKeyword = keywordInput.getText().trim().toLowerCase();

	if (!isKeywordValid(newKeyword))
	{
		keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonDanger);
		juce::Timer::callAfterDelay(
		    500,
		    [this]() { keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep); });
		return;
	}

	if (availableKeywords.contains(newKeyword))
	{
		keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonWarning);
		juce::Timer::callAfterDelay(
		    500,
		    [this]() { keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep); });
		keywordInput.clear();
		return;
	}

	availableKeywords.add(newKeyword);
	audioProcessor.addCustomKeyword(newKeyword);

	updateKeywordBadges();

	keywordInput.clear();

	keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonSuccess);
	juce::Timer::callAfterDelay(
	    500, [this]() { keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep); });
}

void DrawingCanvas::updateColorSwatchSelection()
{
	for (auto *swatch : colorSwatches)
	{
		swatch->setToggleState(false, juce::dontSendNotification);
	}

	bool colorFound = false;
	for (auto *swatch : colorSwatches)
	{
		auto swatchColor = swatch->findColour(juce::TextButton::buttonColourId);
		if (swatchColor == currentColor)
		{
			swatch->setToggleState(true, juce::dontSendNotification);
			selectedColorSwatch = swatch;
			colorFound = true;
			break;
		}
	}

	for (auto *swatch : colorSwatches)
	{
		swatch->repaint();
	}
}

void DrawingCanvas::timerCallback()
{
	if (needsRepaint)
	{
		repaint(canvasAreaBounds);
		needsRepaint = false;
	}
}

bool DrawingCanvas::isPointInCanvas(juce::Point<int> p)
{
	return canvasAreaBounds.contains(p);
}

juce::Point<int> DrawingCanvas::getCanvasPoint(juce::Point<int> screenPoint)
{
	return screenPoint - canvasAreaBounds.getPosition();
}

void DrawingCanvas::setupUI()
{
	addAndMakeVisible(pencilButton);
	pencilButton.loadIcon(BinaryData::pencil_svg, BinaryData::pencil_svgSize);
	pencilButton.setRadioGroupId(1);
	pencilButton.setClickingTogglesState(true);
	pencilButton.setToggleState(true, juce::dontSendNotification);
	pencilButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	pencilButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
	pencilButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	pencilButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
	pencilButton.onClick = [this] { currentBrushType = BrushType::Pencil; };

	addAndMakeVisible(brushButton);
	brushButton.loadIcon(BinaryData::brush_svg, BinaryData::brush_svgSize);
	brushButton.setRadioGroupId(1);
	brushButton.setClickingTogglesState(true);
	brushButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	brushButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
	brushButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	brushButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
	brushButton.onClick = [this] { currentBrushType = BrushType::Brush; };

	addAndMakeVisible(airbrushButton);
	airbrushButton.loadIcon(BinaryData::wind_svg, BinaryData::wind_svgSize);
	airbrushButton.setRadioGroupId(1);
	airbrushButton.setClickingTogglesState(true);
	airbrushButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	airbrushButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
	airbrushButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	airbrushButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
	airbrushButton.onClick = [this] { currentBrushType = BrushType::Airbrush; };

	addAndMakeVisible(eraserButton);
	eraserButton.loadIcon(BinaryData::eraser_svg, BinaryData::eraser_svgSize);
	eraserButton.setRadioGroupId(1);
	eraserButton.setClickingTogglesState(true);
	eraserButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	eraserButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
	eraserButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	eraserButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
	eraserButton.onClick = [this] { currentBrushType = BrushType::Eraser; };

	addAndMakeVisible(fillButton);
	fillButton.loadIcon(BinaryData::bucket_svg, BinaryData::bucket_svgSize);
	fillButton.setRadioGroupId(1);
	fillButton.setClickingTogglesState(true);
	fillButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	fillButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
	fillButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	fillButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
	fillButton.onClick = [this] { currentBrushType = BrushType::Fill; };

	addAndMakeVisible(brushSizeLabel);
	brushSizeLabel.setText("Size:", juce::dontSendNotification);
	brushSizeLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	brushSizeLabel.setJustificationType(juce::Justification::centredRight);

	addAndMakeVisible(brushSizeSlider);
	brushSizeSlider.setRange(1, 50, 1);
	brushSizeSlider.setValue(5, juce::dontSendNotification);
	brushSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
	brushSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
	brushSizeSlider.setColour(juce::Slider::thumbColourId, ColourPalette::sliderThumb);
	brushSizeSlider.setColour(juce::Slider::trackColourId, ColourPalette::sliderTrack);
	brushSizeSlider.setColour(juce::Slider::textBoxTextColourId, ColourPalette::textPrimary);
	brushSizeSlider.onValueChange = [this] { currentBrushSize = (float)brushSizeSlider.getValue(); };

	addAndMakeVisible(colorLabel);
	colorLabel.setText("Color:", juce::dontSendNotification);
	colorLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	colorLabel.setJustificationType(juce::Justification::centredRight);

	juce::Array<juce::Colour> colors = {
	    juce::Colours::black,  juce::Colours::red,    juce::Colours::blue,  juce::Colours::green, juce::Colours::yellow,
	    juce::Colours::orange, juce::Colours::purple, juce::Colours::brown, juce::Colours::grey,  juce::Colours::white};

	for (auto c : colors)
	{
		auto *b = new ColorSwatch(c);
		addAndMakeVisible(b);
		b->setClickingTogglesState(true);
		b->setRadioGroupId(2);

		if (c == juce::Colours::black)
		{
			b->setToggleState(true, juce::dontSendNotification);
		}

		b->onClick = [this, c]() { currentColor = c; };

		colorSwatches.add(b);
	}

	addAndMakeVisible(clearButton);
	clearButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	clearButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
	clearButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
	clearButton.onClick = [this] { clearCanvasWithConfirmation(); };

	addAndMakeVisible(undoButton);
	undoButton.loadIcon(BinaryData::undo_svg, BinaryData::undo_svgSize);
	undoButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	undoButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	undoButton.onClick = [this] { undo(); };
	undoButton.setEnabled(false);

	addAndMakeVisible(redoButton);
	redoButton.loadIcon(BinaryData::redo_svg, BinaryData::redo_svgSize);
	redoButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
	redoButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
	redoButton.onClick = [this] { redo(); };
	redoButton.setEnabled(false);

	addAndMakeVisible(generateButton);
	generateButton.loadIcon(BinaryData::zap_svg, BinaryData::zap_svgSize);
	generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
	generateButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
	generateButton.onClick = [this]
	{
		if (onGenerate && !isGenerating)
		{
			onGenerate(getBase64Image());
		}
	};
	generateButton.setIconSize(18.0f);
	redoButton.setIconSize(18.0f);
	undoButton.setIconSize(18.0f);
	clearButton.setIconSize(18.0f);
	fillButton.setIconSize(18.0f);
	eraserButton.setIconSize(18.0f);
	airbrushButton.setIconSize(18.0f);
	brushButton.setIconSize(18.0f);
	pencilButton.setIconSize(18.0f);

	for (auto *swatch : colorSwatches)
	{
		swatch->setSize(28, 28);
	}
}

void DrawingCanvas::resetHistory()
{
	undoHistory.clear();
	historyIndex = -1;
	saveToHistory();
	updateUndoRedoButtons();
}

void DrawingCanvas::saveToHistory(bool forceAdd)
{
	if (!forceAdd && !undoHistory.empty() && historyIndex >= 0)
	{
		auto &lastImage = undoHistory[historyIndex];
		if (imagesAreEqual(canvas, lastImage))
		{
			return;
		}
	}

	if (historyIndex < (int)undoHistory.size() - 1)
	{
		undoHistory.erase(undoHistory.begin() + historyIndex + 1, undoHistory.end());
	}

	undoHistory.push_back(canvas.createCopy());

	if (undoHistory.size() > maxHistorySize)
	{
		undoHistory.erase(undoHistory.begin());
	}
	else
	{
		historyIndex++;
	}

	updateUndoRedoButtons();
}

bool DrawingCanvas::imagesAreEqual(const juce::Image &img1, const juce::Image &img2)
{
	if (img1.getWidth() != img2.getWidth() || img1.getHeight() != img2.getHeight())
		return false;

	juce::Image::BitmapData data1(img1, juce::Image::BitmapData::readOnly);
	juce::Image::BitmapData data2(img2, juce::Image::BitmapData::readOnly);

	for (int y = 0; y < img1.getHeight(); y += 10)
	{
		for (int x = 0; x < img1.getWidth(); x += 10)
		{
			if (data1.getPixelColour(x, y) != data2.getPixelColour(x, y))
				return false;
		}
	}

	return true;
}

void DrawingCanvas::undo()
{
	if (historyIndex > 0)
	{
		historyIndex--;
		canvas = undoHistory[historyIndex].createCopy();
		needsRepaint = true;
		updateUndoRedoButtons();
	}
}

void DrawingCanvas::redo()
{
	if (historyIndex < (int)undoHistory.size() - 1)
	{
		historyIndex++;
		canvas = undoHistory[historyIndex].createCopy();
		needsRepaint = true;
		updateUndoRedoButtons();
	}
}

void DrawingCanvas::updateUndoRedoButtons()
{
	undoButton.setEnabled(historyIndex > 0);
	redoButton.setEnabled(historyIndex < (int)undoHistory.size() - 1);
}

void DrawingCanvas::floodFill(int x, int y, juce::Colour targetColor, juce::Colour replacementColor)
{
	if (targetColor == replacementColor)
		return;
	if (!canvas.getBounds().contains(x, y))
		return;

	std::vector<juce::Point<int>> stack;
	stack.push_back({x, y});

	while (!stack.empty())
	{
		auto p = stack.back();
		stack.pop_back();

		if (!canvas.getBounds().contains(p.x, p.y))
			continue;
		if (canvas.getPixelAt(p.x, p.y) != targetColor)
			continue;

		canvas.setPixelAt(p.x, p.y, replacementColor);

		stack.push_back({p.x + 1, p.y});
		stack.push_back({p.x - 1, p.y});
		stack.push_back({p.x, p.y + 1});
		stack.push_back({p.x, p.y - 1});
	}
	repaint();
}

bool DrawingCanvas::keyPressed(const juce::KeyPress &key)
{
	if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
	{
		undo();
		return true;
	}
	if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
	    key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
	{
		redo();
		return true;
	}
	return false;
}
