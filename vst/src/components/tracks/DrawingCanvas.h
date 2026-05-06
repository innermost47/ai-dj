#pragma once
#include "BinaryData.h"
#include "ColourPalette.h"
#include "CustomLookAndFeel.h"
#include "IconButton.h"
#include "ObsidianAlertManager.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class KeywordBadge : public juce::TextButton
{
  public:
	KeywordBadge(const juce::String &text);

	std::function<void(const juce::MouseEvent &)> onRightClick;

	void mouseDown(const juce::MouseEvent &e) override;
};

class ColorSwatch : public juce::TextButton
{
  public:
	ColorSwatch(juce::Colour color);

	void paintButton(juce::Graphics &g, bool /*shouldDrawButtonAsHighlighted*/,
	                 bool /*shouldDrawButtonAsDown*/) override;

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

			auto *imageElement = xml.createNewChildElement("Image");
			imageElement->setAttribute("data", imageBase64);

			auto *keywordsElement = xml.createNewChildElement("Keywords");
			keywordsElement->setAttribute("data", selectedKeywords.joinIntoString("|"));

			return xml.toString();
		}

		static CanvasState fromXml(const juce::String &xmlString)
		{
			CanvasState state;

			if (auto xml = juce::parseXML(xmlString))
			{
				state.brushType = xml->getIntAttribute("brushType", 0);
				state.brushSize = (float)xml->getDoubleAttribute("brushSize", 5.0);
				state.brushColor = juce::Colour::fromString(xml->getStringAttribute("brushColor", "ff000000"));

				if (auto *imageElement = xml->getChildByName("Image"))
				{
					state.imageBase64 = imageElement->getStringAttribute("data");
				}

				if (auto *keywordsElement = xml->getChildByName("Keywords"))
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

	CanvasState getState() const;

	enum class BrushType
	{
		Pencil,
		Brush,
		Airbrush,
		Eraser,
		Fill
	};

	DrawingCanvas(DjIaVstProcessor &proc);
	~DrawingCanvas();
	void setGenerating(bool generating);
	void paint(juce::Graphics &g) override;
	void resized() override;
	void mouseMove(const juce::MouseEvent &e) override;
	void mouseDown(const juce::MouseEvent &e) override;
	void mouseDrag(const juce::MouseEvent &e) override;
	void mouseUp(const juce::MouseEvent &) override;
	void drawAtPoint(juce::Graphics &g, juce::Point<int> point);
	void drawLine(juce::Graphics &g, juce::Point<int> from, juce::Point<int> to);
	void clearCanvas();
	void clearCanvasWithConfirmation();
	juce::String getBase64Image();
	void loadFromBase64(const juce::String &base64Data);
	void setState(const CanvasState &state);

	std::function<void(const juce::String &)> onGenerate;
	std::function<void()> onClose;

  private:
	DjIaVstProcessor &audioProcessor;

	juce::StringArray selectedKeywords;
	juce::StringArray availableKeywords;

	juce::TextEditor keywordInput;
	juce::Label keywordsLabel;

	juce::OwnedArray<KeywordBadge> keywordBadges;

	juce::Viewport keywordsViewport;
	juce::Component keywordsBadgesContainer;

	static juce::StringArray getDefaultKeywords();
	void updateMouseCursor();
	void setupKeywordsUI();
	void updateKeywordBadges();
	void showKeywordContextMenu(KeywordBadge *badge, const juce::String &keyword);
	void editKeyword(const juce::String &oldKeyword);
	void deleteKeyword(const juce::String &keyword);
	void toggleKeyword(const juce::String &keyword);
	bool isKeywordValid(const juce::String &keyword) const;
	void addCustomKeyword();

	bool isGenerating = false;

	juce::TextButton *selectedColorSwatch = nullptr;

	void updateColorSwatchSelection();
	void timerCallback() override;
	bool isPointInCanvas(juce::Point<int> p);
	juce::Point<int> getCanvasPoint(juce::Point<int> screenPoint);
	void setupUI();
	void resetHistory();
	void saveToHistory(bool forceAdd = false);
	bool imagesAreEqual(const juce::Image &img1, const juce::Image &img2);
	void undo();
	void redo();
	void updateUndoRedoButtons();
	void floodFill(int x, int y, juce::Colour targetColor, juce::Colour replacementColor);
	bool keyPressed(const juce::KeyPress &key) override;

	juce::Image canvas;
	juce::Rectangle<int> canvasAreaBounds;
	bool isDrawing = false;
	bool needsRepaint = false;
	juce::Point<int> lastPoint;
	juce::Random random;

	juce::Rectangle<int> keywordsPanelBounds;
	juce::Rectangle<int> toolsPanelBounds;

	BrushType currentBrushType = BrushType::Pencil;
	float currentBrushSize = 5.0f;
	juce::Colour currentColor = juce::Colours::black;

	IconButtonSimple pencilButton{"Pencil", ""};
	IconButtonSimple brushButton{"Brush", ""};
	IconButtonSimple airbrushButton{"Spray", ""};
	IconButtonSimple fillButton{"Fill", ""};
	IconButtonSimple eraserButton{"Eraser", ""};
	IconButtonSimple undoButton{"Undo", ""};
	IconButtonSimple redoButton{"Redo", ""};
	IconButtonSimple clearButton{"Clear", ""};
	IconButtonSimple generateButton{"Generate", ""};
	IconButtonSimple addKeywordButton{"AddKeyword", ""};

	juce::Label brushSizeLabel;
	juce::Slider brushSizeSlider;
	juce::Label colorLabel;

	std::vector<juce::Image> undoHistory;
	juce::OwnedArray<juce::TextButton> colorSwatches;

	int historyIndex = -1;
	static constexpr int maxHistorySize = 10;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawingCanvas)
};

inline DrawingCanvas *ObsidianAlertManager::showDrawingCanvas(juce::Component *parent, DjIaVstProcessor &processor,
                                                              std::function<void(const juce::String &)> onGenerate,
                                                              std::function<void(DrawingCanvas *)> onClose)
{
	auto modal = std::make_unique<ObsidianModalWindow>("Draw to Audio", 980, 980);

	auto canvasContent = std::make_unique<DrawingCanvas>(processor);
	auto *canvasPtr = canvasContent.get();
	canvasPtr->onGenerate = onGenerate;
	modal->setContent(std::move(canvasContent));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return nullptr;

	juce::Component::SafePointer<ObsidianModalOverlay> safeOverlay(overlay);

	overlay->modalWindow->addButton("Close", ObsidianAlertManager::crossSvg, ColourPalette::buttonInactive,
	                                [safeOverlay, canvasPtr, onClose]()
	                                {
		                                if (onClose)
			                                onClose(canvasPtr);
		                                if (safeOverlay != nullptr)
			                                safeOverlay->close();
	                                });

	return canvasPtr;
}