#pragma once
#include "JuceHeader.h"
#include "ColourPalette.h"
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class KeywordBadge : public juce::TextButton
{
public:
	KeywordBadge(const juce::String& text) : juce::TextButton(text) {}

	std::function<void(const juce::MouseEvent&)> onRightClick;

	void mouseDown(const juce::MouseEvent& e) override
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
};

class ColorSwatch : public juce::TextButton
{
public:
	ColorSwatch(juce::Colour color) : buttonColor(color)
	{
		setColour(juce::TextButton::buttonColourId, color);
	}

	void paintButton(juce::Graphics& g, bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
	{
		auto bounds = getLocalBounds().toFloat();

		g.setColour(buttonColor);
		g.fillRoundedRectangle(bounds.reduced(3), 2.0f);

		if (getToggleState())
		{
			g.setColour(ColourPalette::buttonPrimary);
			g.drawRoundedRectangle(bounds.reduced(1), 2.0f, 3.0f);
		}
		else
		{
			g.setColour(ColourPalette::backgroundLight);
			g.drawRoundedRectangle(bounds.reduced(3), 2.0f, 1.0f);
		}
	}

private:
	juce::Colour buttonColor;
};

class DrawingCanvas : public juce::Component, private juce::Timer
{
public:
	struct CanvasState
	{
		juce::String imageBase64;
		int brushType = 0;
		float brushSize = 5.0f;
		juce::Colour brushColor = juce::Colours::black;
		juce::StringArray selectedKeywords;

		juce::String toXml() const
		{
			juce::XmlElement xml("CanvasState");
			xml.setAttribute("brushType", brushType);
			xml.setAttribute("brushSize", brushSize);
			xml.setAttribute("brushColor", brushColor.toString());

			auto* imageElement = xml.createNewChildElement("Image");
			imageElement->setAttribute("data", imageBase64);

			auto* keywordsElement = xml.createNewChildElement("Keywords");
			keywordsElement->setAttribute("data", selectedKeywords.joinIntoString("|"));

			return xml.toString();
		}

		static CanvasState fromXml(const juce::String& xmlString)
		{
			CanvasState state;

			if (auto xml = juce::parseXML(xmlString))
			{
				state.brushType = xml->getIntAttribute("brushType", 0);
				state.brushSize = (float)xml->getDoubleAttribute("brushSize", 5.0);
				state.brushColor = juce::Colour::fromString(xml->getStringAttribute("brushColor", "ff000000"));

				if (auto* imageElement = xml->getChildByName("Image"))
				{
					state.imageBase64 = imageElement->getStringAttribute("data");
				}

				if (auto* keywordsElement = xml->getChildByName("Keywords"))
				{
					juce::String keywordsData = keywordsElement->getStringAttribute("data");
					if (keywordsData.isNotEmpty())
					{
						state.selectedKeywords.addTokens(keywordsData, "|", "");
					}
				}
			}

			return state;
		}
	};

	CanvasState getState() const
	{
		CanvasState state;
		state.imageBase64 = const_cast<DrawingCanvas*>(this)->getBase64Image();

		switch (currentBrushType)
		{
		case BrushType::Pencil: state.brushType = 0; break;
		case BrushType::Brush: state.brushType = 1; break;
		case BrushType::Airbrush: state.brushType = 2; break;
		case BrushType::Fill: state.brushType = 3; break;
		case BrushType::Eraser: state.brushType = 4; break;
		}

		state.brushSize = currentBrushSize;
		state.brushColor = currentColor;
		state.selectedKeywords = selectedKeywords;

		return state;
	}

	enum class BrushType
	{
		Pencil,
		Brush,
		Airbrush,
		Eraser,
		Fill
	};

	DrawingCanvas(DjIaVstProcessor& proc)
		: audioProcessor(proc)
	{
		canvas = juce::Image(juce::Image::RGB, 512, 512, true);
		clearCanvas();
		resetHistory();
		setLookAndFeel(&CustomLookAndFeel::getInstance());
		setupUI();
		setupKeywordsUI();
		setWantsKeyboardFocus(true);
		setSize(900, 750);
		startTimerHz(60);
	}

	~DrawingCanvas()
	{
		setLookAndFeel(nullptr);
	}

	void setGenerating(bool generating)
	{
		isGenerating = generating;
		generateButton.setEnabled(!generating);
		generateButton.setButtonText(generating ? "Generating..." : "Generate");
	}

	void paint(juce::Graphics& g) override
	{
		g.fillAll(ColourPalette::backgroundDeep);

		g.setColour(juce::Colours::white);
		g.fillRect(canvasAreaBounds);

		g.drawImageAt(canvas, canvasAreaBounds.getX(), canvasAreaBounds.getY());

		g.setColour(ColourPalette::backgroundLight);
		g.drawRect(canvasAreaBounds, 2);
	}

	void resized() override
	{
		auto bounds = getLocalBounds().reduced(10);
		bounds.removeFromTop(10);

		auto mainArea = bounds.removeFromTop(512);

		auto canvasArea = mainArea.removeFromLeft(512);
		canvasAreaBounds = juce::Rectangle<int>(canvasArea.getX(), canvasArea.getY(), 512, 512);

		mainArea.removeFromLeft(15);

		auto keywordsArea = mainArea;

		auto keywordsHeaderRow = keywordsArea.removeFromTop(35);
		keywordsLabel.setBounds(keywordsHeaderRow);

		keywordsArea.removeFromTop(5);

		auto inputRow = keywordsArea.removeFromTop(35);
		addKeywordButton.setBounds(inputRow.removeFromRight(40));
		inputRow.removeFromRight(5);
		keywordInput.setBounds(inputRow);

		keywordsArea.removeFromTop(10);

		auto viewportArea = keywordsArea.removeFromTop(425);
		keywordsViewport.setBounds(viewportArea);

		int badgeWidth = 105;
		int badgeHeight = 28;
		int spacingX = 5;
		int spacingY = 5;

		int totalBadges = keywordBadges.size();
		if (totalBadges > 0)
		{
			int availableHeight = viewportArea.getHeight() - keywordsViewport.getScrollBarThickness();
			int maxRows = juce::jmax(1, availableHeight / (badgeHeight + spacingY));

			int numColumns = (totalBadges + maxRows - 1) / maxRows;

			int totalWidth = numColumns * (badgeWidth + spacingX) + spacingX;

			keywordsBadgesContainer.setSize(juce::jmax(totalWidth, viewportArea.getWidth()),
				availableHeight);

			int badgeIndex = 0;
			for (auto* badge : keywordBadges)
			{
				int col = badgeIndex / maxRows;
				int row = badgeIndex % maxRows;

				int x = col * (badgeWidth + spacingX) + spacingX;
				int y = row * (badgeHeight + spacingY) + spacingY;

				badge->setBounds(x, y, badgeWidth, badgeHeight);

				badgeIndex++;
			}
		}

		bounds.removeFromTop(15);
		auto toolsArea = bounds;

		auto brushRow = toolsArea.removeFromTop(45);
		int btnW = (brushRow.getWidth() - 20) / 5;

		pencilButton.setBounds(brushRow.removeFromLeft(btnW));
		brushRow.removeFromLeft(5);
		brushButton.setBounds(brushRow.removeFromLeft(btnW));
		brushRow.removeFromLeft(5);
		airbrushButton.setBounds(brushRow.removeFromLeft(btnW));
		brushRow.removeFromLeft(5);
		fillButton.setBounds(brushRow.removeFromLeft(btnW));
		brushRow.removeFromLeft(5);
		eraserButton.setBounds(brushRow.removeFromLeft(btnW));

		toolsArea.removeFromTop(12);

		auto sizeRow = toolsArea.removeFromTop(45);
		brushSizeLabel.setBounds(sizeRow.removeFromLeft(60));
		sizeRow.removeFromLeft(5);
		brushSizeSlider.setBounds(sizeRow);

		toolsArea.removeFromTop(12);

		auto colorRow = toolsArea.removeFromTop(35);
		colorLabel.setBounds(colorRow.removeFromLeft(60));
		colorRow.removeFromLeft(5);

		int numSwatches = colorSwatches.size();
		if (numSwatches > 0)
		{
			int totalSpacing = (numSwatches - 1) * 5;
			int availableWidth = colorRow.getWidth() - totalSpacing;
			int swatchWidth = availableWidth / numSwatches;

			for (int i = 0; i < numSwatches; ++i)
			{
				auto* swatch = colorSwatches[i];
				swatch->setBounds(colorRow.removeFromLeft(swatchWidth));
				if (i < numSwatches - 1)
					colorRow.removeFromLeft(5);
			}
		}

		toolsArea.removeFromTop(15);

		auto actionRow = toolsArea.removeFromTop(45);
		int thirdWidth = (actionRow.getWidth() - 20) / 4;
		undoButton.setBounds(actionRow.removeFromLeft(thirdWidth));
		actionRow.removeFromLeft(10);
		redoButton.setBounds(actionRow.removeFromLeft(thirdWidth));
		actionRow.removeFromLeft(10);
		clearButton.setBounds(actionRow.removeFromLeft(thirdWidth));
		actionRow.removeFromLeft(10);
		generateButton.setBounds(actionRow);
	}

	void mouseMove(const juce::MouseEvent& e) override
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

	void mouseDown(const juce::MouseEvent& e) override
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

	void mouseDrag(const juce::MouseEvent& e) override
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

	void mouseUp(const juce::MouseEvent&) override
	{
		if (isDrawing)
		{
			saveToHistory();
		}
		isDrawing = false;
	}

	void drawAtPoint(juce::Graphics& g, juce::Point<int> point)
	{
		switch (currentBrushType)
		{
		case BrushType::Pencil:
			g.setColour(currentColor);
			g.fillRect(point.x, point.y, 2, 2);
			break;

		case BrushType::Brush:
			g.setColour(currentColor);
			g.fillEllipse(
				point.x - currentBrushSize / 2.0f,
				point.y - currentBrushSize / 2.0f,
				currentBrushSize,
				currentBrushSize
			);
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
			g.fillEllipse(
				point.x - currentBrushSize / 2.0f,
				point.y - currentBrushSize / 2.0f,
				currentBrushSize,
				currentBrushSize
			);
			break;
		}
	}

	void drawLine(juce::Graphics& g, juce::Point<int> from, juce::Point<int> to)
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
			drawAtPoint(g, { x, y });
		}
	}

	void clearCanvas()
	{
		juce::Graphics g(canvas);
		g.fillAll(juce::Colours::white);
		repaint();
	}

	void clearCanvasWithConfirmation()
	{
		if (!isShowing() || !isVisible())
		{
			return;
		}

		juce::AlertWindow::showOkCancelBox(
			juce::MessageBoxIconType::QuestionIcon,
			"Clear Canvas",
			"Are you sure you want to clear the canvas? This will erase the undo/redo history.",
			"Clear",
			"Cancel",
			nullptr,
			juce::ModalCallbackFunction::create([this](int result)
				{
					if (result == 1 && isShowing())
					{
						juce::Graphics g(canvas);
						g.fillAll(juce::Colours::white);
						repaint();
						undoHistory.clear();
						historyIndex = -1;
						updateUndoRedoButtons();
					}
				})
		);
	}

	juce::String getBase64Image()
	{
		juce::MemoryOutputStream memStream;
		juce::PNGImageFormat pngFormat;

		if (pngFormat.writeImageToStream(canvas, memStream))
		{
			juce::MemoryBlock block = memStream.getMemoryBlock();
			juce::String base64 = juce::Base64::toBase64(block.getData(), block.getSize());


			int padding = 0;
			if (base64.endsWith("==")) padding = 2;
			else if (base64.endsWith("=")) padding = 1;

			return base64;
		}

		return {};
	}

	void loadFromBase64(const juce::String& base64Data)
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

	void setState(const CanvasState& state)
	{
		DBG("setState called");

		if (!state.imageBase64.isEmpty())
		{
			DBG("Restoring image from state");
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

		for (auto* badge : keywordBadges)
		{
			juce::String keyword = badge->getButtonText();
			badge->setToggleState(selectedKeywords.contains(keyword), juce::dontSendNotification);
		}

		repaint();

		DBG("State restored - brush type: " << state.brushType
			<< ", size: " << state.brushSize
			<< ", color: " << state.brushColor.toString()
			<< ", keywords: " << selectedKeywords.joinIntoString(", "));
	}

	std::function<void(const juce::String&)> onGenerate;
	std::function<void()> onClose;

private:
	DjIaVstProcessor& audioProcessor;

	juce::StringArray selectedKeywords;
	juce::StringArray availableKeywords;

	juce::TextEditor keywordInput;
	juce::TextButton addKeywordButton;
	juce::Label keywordsLabel;

	juce::OwnedArray<KeywordBadge> keywordBadges;

	juce::Viewport keywordsViewport;
	juce::Component keywordsBadgesContainer;

	static juce::StringArray getDefaultKeywords()
	{
		return {
			"drums", "bass", "techno", "ambient", "glitch",
			"synth", "melody", "percussion", "kick", "snare",
			"hihat", "808", "acid", "reverb", "delay",
			"distortion", "filter", "groove", "rhythm", "texture"
		};
	}

	void updateMouseCursor()
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

	void setupKeywordsUI()
	{
		availableKeywords = getDefaultKeywords();

		auto customKeywords = audioProcessor.getCustomKeywords();
		for (const auto& keyword : customKeywords)
		{
			if (!availableKeywords.contains(keyword))
			{
				availableKeywords.add(keyword);
			}
		}

		addAndMakeVisible(keywordsLabel);
		keywordsLabel.setText("Keywords:", juce::dontSendNotification);
		keywordsLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		keywordsLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

		addAndMakeVisible(keywordInput);
		keywordInput.setFont(juce::FontOptions(13.0f));
		keywordInput.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundLight);
		keywordInput.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
		keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep);
		keywordInput.setTextToShowWhenEmpty("Add keyword...", ColourPalette::textSecondary);
		keywordInput.onReturnKey = [this]() { addCustomKeyword(); };

		addAndMakeVisible(addKeywordButton);
		addKeywordButton.setButtonText("+");
		addKeywordButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
		addKeywordButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		addKeywordButton.onClick = [this]() { addCustomKeyword(); };

		addAndMakeVisible(keywordsViewport);
		keywordsViewport.setViewedComponent(&keywordsBadgesContainer, false);
		keywordsViewport.setScrollBarsShown(false, true);

		updateKeywordBadges();
	}

	void updateKeywordBadges()
	{
		keywordBadges.clear();

		juce::StringArray sortedKeywords = availableKeywords;
		sortedKeywords.sort(true);

		for (const auto& keyword : sortedKeywords)
		{
			auto* badge = new KeywordBadge(keyword);
			badge->setClickingTogglesState(true);
			badge->setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
			badge->setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
			badge->setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
			badge->setColour(juce::TextButton::textColourOnId, juce::Colours::white);

			if (selectedKeywords.contains(keyword))
			{
				badge->setToggleState(true, juce::dontSendNotification);
			}

			badge->onClick = [this, keyword]()
				{
					toggleKeyword(keyword);
				};

			badge->onRightClick = [this, badge, keyword](const juce::MouseEvent&)
				{
					showKeywordContextMenu(badge, keyword);
				};

			keywordsBadgesContainer.addAndMakeVisible(badge);
			keywordBadges.add(badge);
		}

		resized();
	}

	void showKeywordContextMenu(KeywordBadge* badge, const juce::String& keyword)
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

	void editKeyword(const juce::String& oldKeyword)
	{
		auto* alertWindow = new juce::AlertWindow(
			"Edit Keyword",
			"Enter new keyword name:",
			juce::MessageBoxIconType::NoIcon
		);

		alertWindow->addTextEditor("keyword", oldKeyword, "Keyword:");
		alertWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
		alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

		alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
			[this, oldKeyword, alertWindow](int result)
			{
				if (result == 1)
				{
					juce::String newKeyword = alertWindow->getTextEditorContents("keyword").trim().toLowerCase();

					if (!isKeywordValid(newKeyword))
					{
						juce::AlertWindow::showMessageBoxAsync(
							juce::MessageBoxIconType::WarningIcon,
							"Invalid Keyword",
							"Keyword must be 1-15 characters and contain only letters, numbers, spaces or hyphens.");
						return;
					}

					if (newKeyword != oldKeyword && availableKeywords.contains(newKeyword))
					{
						juce::AlertWindow::showMessageBoxAsync(
							juce::MessageBoxIconType::WarningIcon,
							"Duplicate Keyword",
							"This keyword already exists.");
						return;
					}

					int index = availableKeywords.indexOf(oldKeyword);
					if (index >= 0)
					{
						availableKeywords.set(index, newKeyword);
					}

					if (selectedKeywords.contains(oldKeyword))
					{
						selectedKeywords.removeString(oldKeyword);
						selectedKeywords.add(newKeyword);
					}

					auto customKeywords = audioProcessor.getCustomKeywords();
					if (customKeywords.contains(oldKeyword))
					{
						juce::StringArray newCustomKeywords;
						for (const auto& kw : customKeywords)
						{
							if (kw == oldKeyword)
								newCustomKeywords.add(newKeyword);
							else
								newCustomKeywords.add(kw);
						}
						audioProcessor.setCustomKeywords(newCustomKeywords);
					}

					updateKeywordBadges();

					DBG("Keyword renamed: " << oldKeyword << " -> " << newKeyword);
				}

				delete alertWindow;
			}), true);
	}

	void deleteKeyword(const juce::String& keyword)
	{
		juce::AlertWindow::showOkCancelBox(
			juce::MessageBoxIconType::QuestionIcon,
			"Delete Keyword",
			"Are you sure you want to delete \"" + keyword + "\"?",
			"Delete",
			"Cancel",
			nullptr,
			juce::ModalCallbackFunction::create([this, keyword](int result)
				{
					if (result == 1)
					{
						availableKeywords.removeString(keyword);

						selectedKeywords.removeString(keyword);

						auto customKeywords = audioProcessor.getCustomKeywords();
						customKeywords.removeString(keyword);
						audioProcessor.setCustomKeywords(customKeywords);

						updateKeywordBadges();

						DBG("Keyword deleted: " << keyword);
					}
				})
		);
	}

	void toggleKeyword(const juce::String& keyword)
	{
		if (selectedKeywords.contains(keyword))
		{
			selectedKeywords.removeString(keyword);
			DBG("Keyword deselected: " + keyword);
		}
		else
		{
			selectedKeywords.add(keyword);
			DBG("Keyword selected: " + keyword);
		}

		DBG("Selected keywords: " + selectedKeywords.joinIntoString(", "));
	}

	bool isKeywordValid(const juce::String& keyword) const
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

	void addCustomKeyword()
	{
		juce::String newKeyword = keywordInput.getText().trim().toLowerCase();

		if (!isKeywordValid(newKeyword))
		{
			keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonDanger);
			juce::Timer::callAfterDelay(500, [this]()
				{
					keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep);
				});
			return;
		}

		if (availableKeywords.contains(newKeyword))
		{
			keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonWarning);
			juce::Timer::callAfterDelay(500, [this]()
				{
					keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep);
				});
			keywordInput.clear();
			return;
		}

		availableKeywords.add(newKeyword);
		audioProcessor.addCustomKeyword(newKeyword);

		updateKeywordBadges();

		keywordInput.clear();

		keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonSuccess);
		juce::Timer::callAfterDelay(500, [this]()
			{
				keywordInput.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundDeep);
			});

		DBG("Custom keyword added: " + newKeyword);
	}

	bool isGenerating = false;

	juce::TextButton* selectedColorSwatch = nullptr;

	void updateColorSwatchSelection()
	{
		for (auto* swatch : colorSwatches)
		{
			swatch->setToggleState(false, juce::dontSendNotification);
		}

		bool colorFound = false;
		for (auto* swatch : colorSwatches)
		{
			auto swatchColor = swatch->findColour(juce::TextButton::buttonColourId);
			if (swatchColor == currentColor)
			{
				swatch->setToggleState(true, juce::dontSendNotification);
				selectedColorSwatch = swatch;
				colorFound = true;
				DBG("Color swatch selected: " << currentColor.toString());
				break;
			}
		}

		if (!colorFound)
		{
			DBG("Warning: No matching color swatch found for " << currentColor.toString());
		}

		for (auto* swatch : colorSwatches)
		{
			swatch->repaint();
		}
	}


	void timerCallback() override
	{
		if (needsRepaint)
		{
			repaint(canvasAreaBounds);
			needsRepaint = false;
		}
	}

	bool isPointInCanvas(juce::Point<int> p)
	{
		return canvasAreaBounds.contains(p);
	}

	juce::Point<int> getCanvasPoint(juce::Point<int> screenPoint)
	{
		return screenPoint - canvasAreaBounds.getPosition();
	}

	void setupUI()
	{
		addAndMakeVisible(pencilButton);
		pencilButton.setButtonText("Pencil");
		pencilButton.setRadioGroupId(1);
		pencilButton.setClickingTogglesState(true);
		pencilButton.setToggleState(true, juce::dontSendNotification);
		pencilButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		pencilButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		pencilButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		pencilButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		pencilButton.onClick = [this]
			{
				currentBrushType = BrushType::Pencil;
			};


		addAndMakeVisible(brushButton);
		brushButton.setButtonText("Brush");
		brushButton.setRadioGroupId(1);
		brushButton.setClickingTogglesState(true);
		brushButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		brushButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		brushButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		brushButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		brushButton.onClick = [this]
			{
				currentBrushType = BrushType::Brush;
			};

		addAndMakeVisible(airbrushButton);
		airbrushButton.setButtonText("Spray");
		airbrushButton.setRadioGroupId(1);
		airbrushButton.setClickingTogglesState(true);
		airbrushButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		airbrushButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		airbrushButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		airbrushButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		airbrushButton.onClick = [this]
			{
				currentBrushType = BrushType::Airbrush;
			};

		addAndMakeVisible(eraserButton);
		eraserButton.setButtonText("Eraser");
		eraserButton.setRadioGroupId(1);
		eraserButton.setClickingTogglesState(true);
		eraserButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		eraserButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		eraserButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		eraserButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		eraserButton.onClick = [this]
			{
				currentBrushType = BrushType::Eraser;
			};

		addAndMakeVisible(fillButton);
		fillButton.setButtonText("Fill");
		fillButton.setRadioGroupId(1);
		fillButton.setClickingTogglesState(true);
		fillButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		fillButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary);
		fillButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		fillButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
		fillButton.onClick = [this]
			{
				currentBrushType = BrushType::Fill;
			};

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
		brushSizeSlider.onValueChange = [this]
			{
				currentBrushSize = (float)brushSizeSlider.getValue();
			};

		addAndMakeVisible(colorLabel);
		colorLabel.setText("Color:", juce::dontSendNotification);
		colorLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		colorLabel.setJustificationType(juce::Justification::centredRight);

		juce::Array<juce::Colour> colors = {
			   juce::Colours::black,
			   juce::Colours::red,
			   juce::Colours::blue,
			   juce::Colours::green,
			   juce::Colours::yellow,
			   juce::Colours::orange,
			   juce::Colours::purple,
			   juce::Colours::brown,
			   juce::Colours::grey,
			   juce::Colours::white
		};

		for (auto c : colors)
		{
			auto* b = new ColorSwatch(c);
			addAndMakeVisible(b);
			b->setClickingTogglesState(true);
			b->setRadioGroupId(2);

			if (c == juce::Colours::black)
			{
				b->setToggleState(true, juce::dontSendNotification);
			}

			b->onClick = [this, c]()
				{
					currentColor = c;
					DBG("Color changed to: " << c.toString());
				};

			colorSwatches.add(b);
		}

		addAndMakeVisible(clearButton);
		clearButton.setButtonText("Clear");
		clearButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDanger);
		clearButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
		clearButton.onClick = [this] { clearCanvasWithConfirmation(); };

		addAndMakeVisible(undoButton);
		undoButton.setButtonText("Undo");
		undoButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		undoButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		undoButton.onClick = [this] { undo(); };
		undoButton.setEnabled(false);

		addAndMakeVisible(redoButton);
		redoButton.setButtonText("Redo");
		redoButton.setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundLight);
		redoButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textSecondary);
		redoButton.onClick = [this] { redo(); };
		redoButton.setEnabled(false);

		addAndMakeVisible(generateButton);
		generateButton.setButtonText("Generate");
		generateButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonSuccess);
		generateButton.setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary);
		generateButton.onClick = [this]
			{
				if (onGenerate && !isGenerating)
				{
					onGenerate(getBase64Image());
				}
			};

		for (auto* swatch : colorSwatches)
		{
			swatch->setSize(28, 28);
		}
	}

	void resetHistory()
	{
		undoHistory.clear();
		historyIndex = -1;
		saveToHistory();
		updateUndoRedoButtons();
	}

	void saveToHistory(bool forceAdd = false)
	{
		if (!forceAdd && !undoHistory.empty() && historyIndex >= 0)
		{
			auto& lastImage = undoHistory[historyIndex];
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

	bool imagesAreEqual(const juce::Image& img1, const juce::Image& img2)
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

	void undo()
	{
		if (historyIndex > 0)
		{
			historyIndex--;
			canvas = undoHistory[historyIndex].createCopy();
			needsRepaint = true;
			updateUndoRedoButtons();
		}
	}

	void redo()
	{
		if (historyIndex < (int)undoHistory.size() - 1)
		{
			historyIndex++;
			canvas = undoHistory[historyIndex].createCopy();
			needsRepaint = true;
			updateUndoRedoButtons();
		}
	}

	void updateUndoRedoButtons()
	{
		undoButton.setEnabled(historyIndex > 0);
		redoButton.setEnabled(historyIndex < (int)undoHistory.size() - 1);
	}

	void floodFill(int x, int y, juce::Colour targetColor, juce::Colour replacementColor)
	{
		if (targetColor == replacementColor) return;
		if (!canvas.getBounds().contains(x, y)) return;

		std::vector<juce::Point<int>> stack;
		stack.push_back({ x, y });

		while (!stack.empty())
		{
			auto p = stack.back();
			stack.pop_back();

			if (!canvas.getBounds().contains(p.x, p.y)) continue;
			if (canvas.getPixelAt(p.x, p.y) != targetColor) continue;

			canvas.setPixelAt(p.x, p.y, replacementColor);

			stack.push_back({ p.x + 1, p.y });
			stack.push_back({ p.x - 1, p.y });
			stack.push_back({ p.x, p.y + 1 });
			stack.push_back({ p.x, p.y - 1 });
		}
		repaint();
	}

	bool keyPressed(const juce::KeyPress& key) override
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


	juce::Image canvas;
	juce::Rectangle<int> canvasAreaBounds;
	bool isDrawing = false;
	bool needsRepaint = false;
	juce::Point<int> lastPoint;
	juce::Random random;

	BrushType currentBrushType = BrushType::Pencil;
	float currentBrushSize = 5.0f;
	juce::Colour currentColor = juce::Colours::black;

	juce::TextButton pencilButton;
	juce::TextButton brushButton;
	juce::TextButton airbrushButton;
	juce::TextButton fillButton;
	juce::TextButton undoButton;
	juce::TextButton redoButton;
	juce::TextButton eraserButton;
	juce::Label brushSizeLabel;
	juce::Slider brushSizeSlider;
	juce::Label colorLabel;

	juce::TextButton clearButton;
	juce::TextButton generateButton;
	std::vector<juce::Image> undoHistory;
	juce::OwnedArray<juce::TextButton> colorSwatches;

	int historyIndex = -1;
	static constexpr int maxHistorySize = 10;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawingCanvas)
};