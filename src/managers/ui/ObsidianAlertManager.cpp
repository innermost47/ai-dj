#include "ObsidianAlertManager.h"
#include "AiModelDefinitions.h"
#include "ObsidianBase.h"
#include "PromptModelDefinitions.h"

struct GroupLabelInfo
{
	juce::String label;
	juce::Rectangle<int> bounds;
};

static int measureTextHeight(const juce::String &text, int width, const juce::Font &f)
{
	juce::AttributedString a;
	a.append(text, f);
	a.setWordWrap(juce::AttributedString::byWord);
	juce::TextLayout l;
	l.createLayout(a, (float)width);
	return (int)std::ceil(l.getHeight());
}

class ModelCard : public ObsidianComponent
{
  public:
	ModelCard(const juce::String &name, juce::Colour col) : modelName(name), colour(col)
	{
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
	}

	void setSelected(bool s)
	{
		selected = s;
		repaint();
	}

	void mouseDown(const juce::MouseEvent &) override
	{
		if (onClick)
			onClick();
	}

	void mouseEnter(const juce::MouseEvent &) override
	{
		hovered = true;
		repaint();
	}
	void mouseExit(const juce::MouseEvent &) override
	{
		hovered = false;
		repaint();
	}

	void paint(juce::Graphics &g) override
	{
		auto bounds = getLocalBounds().toFloat();
		juce::Colour bg =
		    selected ? colour.withAlpha(0.25f)
			         : (hovered ? ColourPalette::backgroundDeep.brighter(0.05f) : ColourPalette::backgroundDeep);
		g.setColour(bg);
		g.fillRoundedRectangle(bounds, Obsidian::CORNER);

		g.setColour(selected ? colour : ColourPalette::backgroundLight.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, selected ? 2.0f : 1.0f);

		const float dotSize = 8.0f;
		auto dotRect = juce::Rectangle<float>(8.0f, bounds.getCentreY() - dotSize * 0.5f, dotSize, dotSize);
		g.setColour(colour);
		g.fillEllipse(dotRect);
		auto textArea = bounds.withTrimmedLeft(22).reduced(4, 0);
		g.setColour(selected ? ColourPalette::textPrimary : ColourPalette::textSecondary);
		g.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, selected ? juce::Font::bold : juce::Font::plain));
		g.drawText(modelName, textArea.toNearestInt(), juce::Justification::centredLeft, true);
	}

	juce::String getModelName() const
	{
		return modelName;
	}

	std::function<void()> onClick;

  private:
	juce::String modelName;
	juce::Colour colour;
	bool selected = false;
	bool hovered = false;
};
class ExampleCard : public ObsidianComponent
{
  public:
	ExampleCard(const juce::String &textIn) : text(textIn)
	{
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
	}

	void mouseDown(const juce::MouseEvent &) override
	{
		if (onClick)
			onClick(text);
	}

	void mouseEnter(const juce::MouseEvent &) override
	{
		hovered = true;
		repaint();
	}
	void mouseExit(const juce::MouseEvent &) override
	{
		hovered = false;
		repaint();
	}

	void paint(juce::Graphics &g) override
	{
		auto bounds = getLocalBounds().toFloat();

		g.setColour(hovered ? ColourPalette::backgroundDeep.brighter(0.08f) : ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(bounds, Obsidian::CORNER);

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 1.0f);

		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));

		juce::AttributedString attr;
		attr.append(text, juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain), ColourPalette::textPrimary);
		attr.setWordWrap(juce::AttributedString::byWord);
		attr.setJustification(juce::Justification::centredLeft);
		attr.draw(g, bounds.reduced(10, 6));
	}

	int getPreferredHeight(int width) const
	{
		juce::Font f(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
		juce::AttributedString attr;
		attr.append(text, f);
		attr.setWordWrap(juce::AttributedString::byWord);
		juce::TextLayout layout;
		layout.createLayout(attr, (float)(width - 20));
		return juce::jmax(36, (int)layout.getHeight() + 16);
	}

	std::function<void(const juce::String &)> onClick;

  private:
	juce::String text;
	bool hovered = false;
};
class KeywordsContainerComponent : public ObsidianComponent
{
  public:
	std::vector<GroupLabelInfo> *groupLabels = nullptr;

	void paint(juce::Graphics &g) override
	{
		if (!groupLabels)
			return;

		auto bounds = getLocalBounds().toFloat();
		paintBaseRoundedBackground(g, ColourPalette::backgroundDeep);

		for (const auto &gl : *groupLabels)
		{
			float y = (float)gl.bounds.getCentreY();
			float lineY = y - 1;

			g.setColour(ColourPalette::backgroundDeep.withAlpha(0.15f));
			g.drawLine(0.0f, lineY, (float)gl.bounds.getWidth(), lineY, 0.5f);

			juce::Font labelFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			g.setFont(labelFont);

			g.setColour(ColourPalette::slate);
			g.drawText(gl.label, gl.bounds, juce::Justification::left, false);
		}
	}
};

class PromptEditorContent : public ObsidianComponent
{
  public:
	juce::Label categoryLbl, promptLbl, examplesLbl, keywordsLbl, descLbl, displayHintLbl;
	juce::ComboBox categoryCombo;
	EscapableTextEditor promptEditor;
	juce::Viewport examplesViewport, keywordsViewport;
	juce::Component examplesContainer;
	KeywordsContainerComponent keywordsContainer;

	std::vector<std::unique_ptr<ModelCard>> modelCards;
	juce::String currentModel;
	bool localOnly;
	std::vector<std::unique_ptr<ExampleCard>> exampleCards;
	std::vector<std::unique_ptr<juce::TextButton>> keywordButtons;

	std::vector<GroupLabelInfo> groupLabels;

	PromptEditorContent(const juce::String &text, const juce::String &model, const juce::String &category,
	                    const juce::StringArray &categories, bool useLocalModel)
	    : currentModel(model), localOnly(useLocalModel)
	{
		if (currentModel.isEmpty())
			currentModel = Obsidian::STABLE_AUDIO_OPEN_V1();
		if (currentModel.toStdString() == Obsidian::STABLE_AUDIO_OPEN_LOCAL())
			currentModel = Obsidian::STABLE_AUDIO_OPEN_V3_MEDIUM();
		categoryLbl.setText("Category:", juce::dontSendNotification);
		categoryLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		categoryLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
		addAndMakeVisible(categoryLbl);

		displayHintLbl.setText(
		    "Note: structural tags like \"TrackType:\" or \"Format:\" are hidden in the track's prompt selector "
		    "for readability. Your full prompt is saved and used exactly as written here.",
		    juce::dontSendNotification);
		displayHintLbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
		displayHintLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
		displayHintLbl.setJustificationType(juce::Justification::topLeft);
		addAndMakeVisible(displayHintLbl);

		categoryCombo.addItem("Uncategorized", 1);
		juce::StringArray sortedCategories = categories;
		sortedCategories.sort(true);

		int catId = 2;
		int selectedId = 1;
		for (const auto &c : sortedCategories)
		{
			categoryCombo.addItem(c, catId);
			if (c == category)
				selectedId = catId;
			catId++;
		}
		categoryCombo.setSelectedId(selectedId);
		categoryCombo.setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
		categoryCombo.setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
		categoryCombo.setColour(juce::ComboBox::outlineColourId, ColourPalette::backgroundLight);
		addAndMakeVisible(categoryCombo);

		const auto &models = PromptModelDefinitions::getAllModels();
		for (const auto &m : models)
		{
			if (localOnly &&
			    AiModelDefinitions::normalize(m.modelName).toStdString() != Obsidian::STABLE_AUDIO_OPEN_V3_MEDIUM())
				continue;

			const juce::String shown = AiModelDefinitions::toDisplayName(m.modelName, localOnly);
			juce::Colour modelCol = AiModelDefinitions::getColourForModel(shown);
			auto card = std::make_unique<ModelCard>(shown, modelCol);

			card->setSelected(m.modelName == currentModel);
			juce::String modelNameCopy = m.modelName;
			card->onClick = [this, modelNameCopy]() { switchModel(modelNameCopy); };
			addAndMakeVisible(*card);
			modelCards.push_back(std::move(card));
		}

		if (modelCards.size() == 1)
		{
			modelCards[0]->setSelected(false);
			modelCards[0]->onClick = nullptr;
			modelCards[0]->setInterceptsMouseClicks(false, false);
		}

		descLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		descLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
		addAndMakeVisible(descLbl);

		promptLbl.setText("Prompt:", juce::dontSendNotification);
		promptLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		promptLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
		addAndMakeVisible(promptLbl);

		promptEditor.setText(text);
		promptEditor.setMultiLine(true);
		promptEditor.setReturnKeyStartsNewLine(true);
		promptEditor.setColour(EscapableTextEditor::backgroundColourId, ColourPalette::backgroundDark);
		promptEditor.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
		promptEditor.setColour(EscapableTextEditor::outlineColourId, ColourPalette::backgroundLight);
		promptEditor.applyFontToAllText(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
		addAndMakeVisible(promptEditor);

		examplesLbl.setText("Examples (click to use):", juce::dontSendNotification);
		examplesLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		examplesLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
		addAndMakeVisible(examplesLbl);

		keywordsLbl.setText("Keywords (click to insert):", juce::dontSendNotification);
		keywordsLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
		keywordsLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
		addAndMakeVisible(keywordsLbl);

		examplesViewport.setViewedComponent(&examplesContainer, false);
		examplesViewport.setScrollBarsShown(true, false);
		addAndMakeVisible(examplesViewport);

		keywordsViewport.setViewedComponent(&keywordsContainer, false);
		keywordsViewport.setScrollBarsShown(true, false);
		addAndMakeVisible(keywordsViewport);
		keywordsContainer.groupLabels = &groupLabels;
		rebuildModelContent();
	}

	void switchModel(const juce::String &newModel)
	{
		currentModel = newModel;
		for (auto &c : modelCards)
			c->setSelected(c->getModelName() == newModel);
		resized();
	}

	void rebuildModelContent()
	{
		const auto *info = PromptModelDefinitions::getModel(currentModel);

		descLbl.setText(info ? info->description : "", juce::dontSendNotification);

		examplesContainer.removeAllChildren();
		exampleCards.clear();

		if (info)
		{
			int y = 0;
			const int cardSpacing = 4;
			int containerW = examplesViewport.getWidth() - 12;

			for (const auto &ex : info->examples)
			{
				auto card = std::make_unique<ExampleCard>(ex);
				int h = card->getPreferredHeight(containerW);

				card->onClick = [this](const juce::String &txt) { promptEditor.setText(txt); };

				card->setBounds(0, y, containerW, h);
				examplesContainer.addAndMakeVisible(*card);
				exampleCards.push_back(std::move(card));

				y += h + cardSpacing;
			}

			examplesContainer.setSize(containerW, juce::jmax(y, examplesViewport.getHeight()));
		}

		keywordsContainer.removeAllChildren();
		keywordButtons.clear();
		groupLabels.clear();

		if (info)
		{
			const int padding = Obsidian::PADDING;
			int containerW = keywordsViewport.getWidth();
			int y = padding;
			const int kwH = 26;
			const int kwSpacing = 6;
			const int groupTopSpacing = 18;
			const int groupBottomSpacing = 8;
			const int groupLabelH = 22;
			const int startX = padding;
			const int availableW = containerW - (2 * padding);
			bool isFirstGroup = true;

			for (const auto &group : info->keywordGroups)
			{
				if (!isFirstGroup)
					y += groupTopSpacing;
				isFirstGroup = false;

				GroupLabelInfo gl;
				gl.label = group.label;
				gl.bounds = juce::Rectangle<int>(0, y, containerW, groupLabelH).reduced(Obsidian::PADDING);
				groupLabels.push_back(gl);

				y += groupLabelH + groupBottomSpacing;

				int x = startX;

				juce::Font kwFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
				for (const auto &kw : group.keywords)
				{
					int textW = (int)juce::GlyphArrangement::getStringWidth(kwFont, kw);
					int w = textW + 28;

					bool isOversized = (w > availableW);

					if (isOversized)
					{
						if (x > 0)
						{
							x = startX;
							y += kwH + kwSpacing;
						}
						w = availableW;
					}
					else if (x + w > availableW + startX)
					{
						x = startX;
						y += kwH + kwSpacing;
					}
					auto btn = std::make_unique<juce::TextButton>(kw);
					btn->setColour(juce::TextButton::buttonColourId, ColourPalette::backgroundDeep.brighter(0.04f));
					btn->setColour(juce::TextButton::buttonOnColourId, ColourPalette::backgroundDeep.brighter(0.12f));
					btn->setColour(juce::TextButton::textColourOffId, ColourPalette::textPrimary.withAlpha(0.85f));
					if (isOversized)
					{
						w -= 24;
						x = 12;
					}

					juce::String kwCopy = kw;
					btn->onClick = [this, kwCopy]() { insertKeyword(kwCopy); };

					btn->setBounds(x, y, w, kwH);
					keywordsContainer.addAndMakeVisible(*btn);
					keywordButtons.push_back(std::move(btn));

					if (isOversized)
					{
						x = startX;
						y += kwH + kwSpacing;
					}
					else
					{
						x += w + kwSpacing;
					}
				}
				y += kwH;
			}

			keywordsContainer.setSize(containerW, juce::jmax(y + 8, keywordsViewport.getHeight()));
			keywordsContainer.repaint();
		}
	}

	void insertKeyword(const juce::String &kw)
	{
		auto pos = promptEditor.getCaretPosition();
		juce::String currentText = promptEditor.getText();

		juce::String prefix;
		if (pos > 0 && currentText.length() > 0)
		{
			juce::juce_wchar prevChar = currentText[pos - 1];
			if (prevChar != ' ' && prevChar != ',' && prevChar != '\n')
				prefix = ", ";
		}

		promptEditor.insertTextAtCaret(prefix + kw);
		promptEditor.grabKeyboardFocus();
	}

	juce::String getPrompt() const
	{
		return promptEditor.getText().trim();
	}
	juce::String getModel() const
	{
		return currentModel;
	}
	juce::String getCategory() const
	{
		int sel = categoryCombo.getSelectedId();
		return sel <= 1 ? "" : categoryCombo.getText();
	}

	void resized() override
	{
		auto area = getLocalBounds().reduced(16);
		const int cardH = 30;
		const int cardSpacing = 6;
		const int numCols = 4;
		int totalCards = (int)modelCards.size();
		int numRows = (totalCards + numCols - 1) / numCols;
		int totalTabsHeight = numRows * cardH + (numRows - 1) * cardSpacing;
		auto tabsArea = area.removeFromTop(totalTabsHeight);
		int cardW = (tabsArea.getWidth() - (numCols - 1) * cardSpacing) / numCols;
		for (int i = 0; i < totalCards; ++i)
		{
			int row = i / numCols;
			int col = i % numCols;
			int x = tabsArea.getX() + col * (cardW + cardSpacing);
			int y = tabsArea.getY() + row * (cardH + cardSpacing);
			modelCards[i]->setBounds(x, y, cardW, cardH);
		}
		area.removeFromTop(8);

		const auto *info = PromptModelDefinitions::getModel(currentModel);
		const juce::String desc = info ? info->description : juce::String();
		descLbl.setText(desc, juce::dontSendNotification);
		descLbl.setJustificationType(juce::Justification::topLeft);

		juce::Font descFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::italic));
		int descH = juce::jlimit(18, 80, measureTextHeight(desc, area.getWidth(), descFont) + 4);
		descLbl.setBounds(area.removeFromTop(descH));

		area.removeFromTop(12);
		const int colSpacing = 16;
		int colW = (area.getWidth() - colSpacing) / 2;
		auto leftCol = area.removeFromLeft(colW);
		area.removeFromLeft(colSpacing);
		auto rightCol = area;
		categoryLbl.setBounds(leftCol.removeFromTop(16));
		leftCol.removeFromTop(Obsidian::GAP_4);
		categoryCombo.setBounds(leftCol.removeFromTop(28));
		leftCol.removeFromTop(12);
		promptLbl.setBounds(leftCol.removeFromTop(16));
		leftCol.removeFromTop(Obsidian::GAP_4);
		promptEditor.setBounds(leftCol.removeFromTop(140));

		const std::string model = currentModel.toStdString();
		const bool hasHiddenTags = model == Obsidian::STABLE_AUDIO_OPEN_V3_MEDIUM() ||
		                           model == Obsidian::STABLE_AUDIO_OPEN_LOCAL() || model == Obsidian::STABLEBEAT() ||
		                           model == Obsidian::GLUTEN_V1();

		displayHintLbl.setVisible(hasHiddenTags);
		if (hasHiddenTags)
		{
			leftCol.removeFromTop(6);
			displayHintLbl.setBounds(leftCol.removeFromTop(48));
		}
		else
			leftCol.removeFromTop(12);

		examplesLbl.setBounds(leftCol.removeFromTop(16));
		leftCol.removeFromTop(Obsidian::GAP_4);
		examplesViewport.setBounds(leftCol);
		keywordsLbl.setBounds(rightCol.removeFromTop(16));
		rightCol.removeFromTop(Obsidian::GAP_4);
		keywordsViewport.setBounds(rightCol);
		rebuildModelContent();
	}
};

juce::Component *ObsidianAlertManager::getSafePluginWindow(juce::Component *c)
{
	auto *current = c;
	while (current != nullptr)
	{
		if (dynamic_cast<juce::AudioProcessorEditor *>(current) != nullptr)
			return current;

		current = current->getParentComponent();
	}
	return c;
}

ObsidianModalOverlay *ObsidianAlertManager::createAndAttachOverlay(juce::Component *parent,
                                                                   std::unique_ptr<ObsidianModalWindow> modal)
{
	auto *root = getSafePluginWindow(parent);
	if (root == nullptr)
		return nullptr;

	auto *host = dynamic_cast<ModalHost *>(root);
	if (host == nullptr)
	{
		jassertfalse;
		return nullptr;
	}

	auto overlay = std::make_unique<ObsidianModalOverlay>(std::move(modal));
	auto *overlayPtr = overlay.get();
	host->addModal(std::move(overlay));
	return overlayPtr;
}

void ObsidianAlertManager::showAddCategoryDialog(
    juce::Component *parent, std::function<void(const juce::String &name, juce::Colour colour)> onAdd)
{
	class CategoryDialogContent : public ObsidianComponent
	{
	  public:
		EscapableTextEditor nameEditor;
		juce::Label nameLbl, colourLbl;
		ColourPicker colourPicker;

		CategoryDialogContent(const juce::String &initialName, juce::Colour initialColour)
		{
			nameLbl.setText("Name:", juce::dontSendNotification);
			nameLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			nameLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			addAndMakeVisible(nameLbl);

			nameEditor.setText(initialName);
			nameEditor.setTextToShowWhenEmpty("Category name...", ColourPalette::textSecondary);
			nameEditor.setColour(EscapableTextEditor::backgroundColourId, ColourPalette::backgroundDark);
			nameEditor.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
			nameEditor.setColour(EscapableTextEditor::outlineColourId, ColourPalette::backgroundLight);
			addAndMakeVisible(nameEditor);

			colourLbl.setText("Colour:", juce::dontSendNotification);
			colourLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			colourLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			addAndMakeVisible(colourLbl);

			colourPicker.setSelectedColour(initialColour);
			addAndMakeVisible(colourPicker);
		}

		juce::Colour getSelectedColour() const noexcept
		{
			return colourPicker.getSelectedColour();
		}

		void resized() override
		{
			auto area = getLocalBounds().reduced(12);
			nameLbl.setBounds(area.removeFromTop(18));
			area.removeFromTop(4);
			nameEditor.setBounds(area.removeFromTop(30));
			area.removeFromTop(12);
			colourLbl.setBounds(area.removeFromTop(18));
			area.removeFromTop(4);
			colourPicker.setBounds(area.removeFromTop(colourPicker.getPreferredHeight()));
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>("Add Category", 480, 320);
	auto content = std::make_unique<CategoryDialogContent>("", ColourPalette::indigo);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay]() { overlay->close(); });

	overlay->modalWindow->addButton("Add", checkSvg, ColourPalette::slate,
	                                [overlay, contentPtr, onAdd, parent]()
	                                {
		                                juce::String name = contentPtr->nameEditor.getText().trim();
		                                if (name.isEmpty())
		                                {
			                                showError(parent, "Add Category", "Please enter a name.");
			                                return;
		                                }
		                                if (onAdd)
			                                onAdd(name, contentPtr->getSelectedColour());
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showEditCategoryDialog(
    juce::Component *parent, const juce::String &currentName, juce::Colour currentColour,
    std::function<void(const juce::String &newName, juce::Colour newColour)> onSave)
{

	class CategoryDialogContent : public ObsidianComponent
	{
	  public:
		EscapableTextEditor nameEditor;
		juce::Label nameLbl, colourLbl;
		ColourPicker colourPicker;

		CategoryDialogContent(const juce::String &initialName, juce::Colour initialColour)
		{
			nameLbl.setText("Name:", juce::dontSendNotification);
			nameLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			nameLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			addAndMakeVisible(nameLbl);

			nameEditor.setText(initialName);
			nameEditor.setColour(EscapableTextEditor::backgroundColourId, ColourPalette::backgroundDark);
			nameEditor.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
			nameEditor.setColour(EscapableTextEditor::outlineColourId, ColourPalette::backgroundLight);
			addAndMakeVisible(nameEditor);

			colourLbl.setText("Colour:", juce::dontSendNotification);
			colourLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			colourLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			addAndMakeVisible(colourLbl);

			colourPicker.setSelectedColour(initialColour);
			addAndMakeVisible(colourPicker);
		}

		juce::Colour getSelectedColour() const noexcept
		{
			return colourPicker.getSelectedColour();
		}

		void resized() override
		{
			auto area = getLocalBounds().reduced(12);
			nameLbl.setBounds(area.removeFromTop(18));
			area.removeFromTop(4);
			nameEditor.setBounds(area.removeFromTop(30));
			area.removeFromTop(12);
			colourLbl.setBounds(area.removeFromTop(18));
			area.removeFromTop(4);
			colourPicker.setBounds(area.removeFromTop(colourPicker.getPreferredHeight()));
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>("Edit Category", 480, 320);
	auto content = std::make_unique<CategoryDialogContent>(currentName, currentColour);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay]() { overlay->close(); });

	overlay->modalWindow->addButton("Save", checkSvg, ColourPalette::slate,
	                                [overlay, contentPtr, onSave, parent]()
	                                {
		                                juce::String name = contentPtr->nameEditor.getText().trim();
		                                if (name.isEmpty())
		                                {
			                                showError(parent, "Edit Category", "Please enter a name.");
			                                return;
		                                }
		                                if (onSave)
			                                onSave(name, contentPtr->getSelectedColour());
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showInfo(juce::Component *parent, const juce::String &title, const juce::String &message,
                                    const juce::String &buttonText, std::function<void()> onConfirm)
{
	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 220);
	modal->setContent(std::make_unique<TextContent>(message));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton(buttonText, checkSvg, ColourPalette::buttonPrimary,
	                                [overlay, onConfirm]()
	                                {
		                                if (onConfirm)
			                                onConfirm();
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showError(juce::Component *parent, const juce::String &title, const juce::String &message)
{
	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 260);
	modal->setContent(std::make_unique<TextContent>(message));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("OK", crossSvg, ColourPalette::buttonDangerDark, [overlay]() { overlay->close(); });
}

void ObsidianAlertManager::showConfirm(juce::Component *parent, const juce::String &title, const juce::String &message,
                                       const juce::String &confirmText, const juce::String &cancelText,
                                       std::function<void(bool confirmed)> callback)
{
	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 220);
	modal->setContent(std::make_unique<TextContent>(message));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton(cancelText, crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback(false);
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton(confirmText, checkSvg, ColourPalette::buttonDangerDark,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback(true);
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showConfigDialog(juce::Component *parent, const juce::String &title,
                                            const juce::String &serverUrl, bool currentUseLocal, int currentTimeoutMs,
                                            bool isFirstTime, std::function<void(const ConfigDialogResult &)> callback)
{
	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 420);

	class ConfigContent : public ObsidianComponent
	{
	  public:
		juce::ComboBox modeCombo, timeoutCombo;
		EscapableTextEditor urlEditor;
		juce::Label modeLbl, urlLbl, timeoutLbl;

		ConfigContent(bool useLocal, const juce::String &url, int timeout)
		{
			auto styleEditor = [](EscapableTextEditor &te, const juce::String &text)
			{
				te.setText(text);
				te.setColour(EscapableTextEditor::backgroundColourId, ColourPalette::backgroundDark);
				te.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
				te.setColour(EscapableTextEditor::outlineColourId, ColourPalette::backgroundLight);
				te.applyFontToAllText(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
			};

			auto styleCombo = [](juce::ComboBox &cb)
			{
				cb.setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
				cb.setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
				cb.setColour(juce::ComboBox::outlineColourId, ColourPalette::backgroundLight);
				cb.setColour(juce::ComboBox::arrowColourId, ColourPalette::buttonPrimary);
			};

			auto styleLabel = [this](juce::Label &lbl, const juce::String &text)
			{
				lbl.setText(text, juce::dontSendNotification);
				lbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
				lbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
				addAndMakeVisible(lbl);
			};

			styleLabel(modeLbl, "Generation Mode:");
			modeCombo.addItem("Server/API", 1);
			modeCombo.addItem("Local Model", 2);
			modeCombo.setSelectedId(useLocal ? 2 : 1);
			styleCombo(modeCombo);
			addAndMakeVisible(modeCombo);
			modeCombo.onChange = [this]() { updateVisibility(); };

			styleLabel(urlLbl, "Server URL:");
			styleEditor(urlEditor, url.isEmpty() ? "http://localhost:8000" : url);
			addAndMakeVisible(urlEditor);

			styleLabel(timeoutLbl, "Request Timeout:");
			timeoutCombo.addItem("1 minute", 1);
			timeoutCombo.addItem("2 minutes", 2);
			timeoutCombo.addItem("5 minutes", 3);
			timeoutCombo.addItem("10 minutes", 4);

			int selectedIndex = 3;
			if (timeout == 60000)
				selectedIndex = 1;
			else if (timeout == 120000)
				selectedIndex = 2;
			else if (timeout == 600000)
				selectedIndex = 4;

			timeoutCombo.setSelectedId(selectedIndex);
			styleCombo(timeoutCombo);
			addAndMakeVisible(timeoutCombo);

			updateVisibility();
		}

		bool isLocalMode() const
		{
			return modeCombo.getSelectedId() == 2;
		}

		void updateVisibility()
		{
			bool local = isLocalMode();
			urlLbl.setVisible(!local);
			urlEditor.setVisible(!local);
			timeoutLbl.setVisible(!local);
			timeoutCombo.setVisible(!local);
			resized();
		}

		void resized() override
		{
			auto bounds = getLocalBounds().reduced(10);
			int rowH = 30;
			int spacing = 15;

			modeLbl.setBounds(bounds.removeFromTop(20));
			modeCombo.setBounds(bounds.removeFromTop(rowH));
			bounds.removeFromTop(spacing);

			if (!isLocalMode())
			{
				urlLbl.setBounds(bounds.removeFromTop(20));
				urlEditor.setBounds(bounds.removeFromTop(rowH));
				bounds.removeFromTop(spacing);

				timeoutLbl.setBounds(bounds.removeFromTop(20));
				timeoutCombo.setBounds(bounds.removeFromTop(rowH));
			}
		}
	};

	auto formContent = std::make_unique<ConfigContent>(currentUseLocal, serverUrl, currentTimeoutMs);
	auto *formPtr = formContent.get();
	modal->setContent(std::move(formContent));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton(isFirstTime ? "Skip for now" : "Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                ConfigDialogResult res{false, false, "", 0};
		                                callback(res);
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton(isFirstTime ? "Save & Continue" : "Update", checkSvg, ColourPalette::slate,
	                                [overlay, formPtr, callback]()
	                                {
		                                ConfigDialogResult res;
		                                res.confirmed = true;
		                                res.useLocalModel = formPtr->isLocalMode();
		                                res.serverUrl = formPtr->urlEditor.getText();
		                                int tid = formPtr->timeoutCombo.getSelectedId();
		                                if (tid == 1)
			                                res.timeoutMs = 60000;
		                                else if (tid == 2)
			                                res.timeoutMs = 120000;
		                                else if (tid == 4)
			                                res.timeoutMs = 600000;
		                                else
			                                res.timeoutMs = 300000;
		                                callback(res);
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showCategoryEditor(juce::Component *parent, const juce::String &sampleName,
                                              const juce::String &currentCategory,
                                              const std::vector<juce::String> &availableCategories,
                                              std::function<void(const juce::String &)> onSave)
{
	auto modal = std::make_unique<ObsidianModalWindow>("Categories: " + sampleName, 480, 400);

	auto categoryContent = std::make_unique<CategoryPanel>(currentCategory, availableCategories);
	auto *panelPtr = categoryContent.get();
	modal->setContent(std::move(categoryContent));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay]() { overlay->close(); });

	overlay->modalWindow->addButton("Done", checkSvg, ColourPalette::slate,
	                                [overlay, panelPtr, onSave]()
	                                {
		                                if (onSave)
			                                onSave(panelPtr->getSelectedCategory());
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showUpdateAvailable(juce::Component *parent, const juce::String &currentBuild,
                                               const juce::String &latestBuild, const juce::String &releaseUrl,
                                               std::function<void(bool)> onClose)
{
	class UpdateContent : public ObsidianComponent
	{
	  public:
		juce::Label messageLbl;
		juce::ToggleButton dontShowToggle;

		UpdateContent(const juce::String &message, const juce::String &latestBuild)
		{
			messageLbl.setText(message, juce::dontSendNotification);
			messageLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			messageLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
			messageLbl.setJustificationType(juce::Justification::topLeft);
			addAndMakeVisible(messageLbl);

			dontShowToggle.setButtonText("Don't remind me about build " + latestBuild + " again");
			dontShowToggle.setColour(juce::ToggleButton::textColourId, ColourPalette::textSecondary);
			dontShowToggle.setColour(juce::ToggleButton::tickColourId, ColourPalette::buttonPrimary);
			dontShowToggle.setColour(juce::ToggleButton::tickDisabledColourId, ColourPalette::backgroundLight);
			addAndMakeVisible(dontShowToggle);
		}

		bool dontShowAgain() const noexcept
		{
			return dontShowToggle.getToggleState();
		}

		void resized() override
		{
			auto area = getLocalBounds().reduced(12);
			dontShowToggle.setBounds(area.removeFromBottom(28));
			area.removeFromBottom(8);
			messageLbl.setBounds(area);
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>("Update Available!", 520, 320);

	juce::String message = "A new version of OBSIDIAN Neural is available!\n\n"
	                       "Installed version: " +
	                       currentBuild +
	                       "\n"
	                       "Available version: " +
	                       latestBuild +
	                       "\n\n"
	                       "Download it from the GitHub releases page.";

	auto content = std::make_unique<UpdateContent>(message, latestBuild);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Later", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, contentPtr, onClose]()
	                                {
		                                if (onClose)
			                                onClose(contentPtr->dontShowAgain());
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton("Open GitHub", downloadSvg, ColourPalette::buttonPrimary,
	                                [overlay, onClose, releaseUrl]()
	                                {
		                                if (onClose)
			                                onClose(false);
		                                juce::URL(releaseUrl).launchInDefaultBrowser();
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showPromptEditor(juce::Component *parent, const juce::String &initialText,
                                            const juce::String &initialModel, const juce::String &initialCategory,
                                            const juce::StringArray &availableCategories, bool useLocalModel,
                                            std::function<void(const PromptEditorResult &)> callback)
{

	juce::String title = initialText.isEmpty() ? "Create Prompt" : "Edit Prompt";
	auto modal = std::make_unique<ObsidianModalWindow>(title, 1100, 860);

	auto content = std::make_unique<PromptEditorContent>(initialText, initialModel, initialCategory,
	                                                     availableCategories, useLocalModel);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback({false, "", "", ""});
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton("Save", checkSvg, ColourPalette::slate,
	                                [overlay, contentPtr, parent, callback]()
	                                {
		                                juce::String txt = contentPtr->getPrompt();
		                                if (txt.isEmpty())
		                                {
			                                showError(parent, "Save Prompt", "Prompt cannot be empty.");
			                                return;
		                                }
		                                PromptEditorResult res;
		                                res.confirmed = true;
		                                res.text = txt;
		                                res.modelName = contentPtr->getModel();
		                                res.category = contentPtr->getCategory();
		                                if (callback)
			                                callback(res);
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showLicenseAgreement(juce::Component *parent, const std::vector<LicenseEntry> &licenses,
                                                std::function<void(bool accepted)> callback)
{
	auto modal = std::make_unique<ObsidianModalWindow>("License Agreements", 560, 480);

	auto content = std::make_unique<LicenseAgreementContent>(licenses);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	modal->addButton("Decline", crossSvg, ColourPalette::buttonInactive, nullptr);

	modal->addButton("Accept & Continue", checkSvg, ColourPalette::backgroundLight, nullptr);

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	auto *declineBtn = overlay->modalWindow->buttons[0];
	declineBtn->onClick = [overlay, callback]()
	{
		if (callback)
			callback(false);
		overlay->close();
	};

	auto *acceptBtn = overlay->modalWindow->buttons[1];
	acceptBtn->setEnabled(false);
	acceptBtn->onClick = [overlay, contentPtr, callback]()
	{
		if (!contentPtr->allAccepted())
			return;
		if (callback)
			callback(true);
		overlay->close();
	};

	juce::Component::SafePointer<ObsidianSvgButton> safeAccept(acceptBtn);
	contentPtr->onAcceptanceChanged = [safeAccept](bool allOk)
	{
		if (safeAccept != nullptr)
		{
			safeAccept->setEnabled(allOk);
			safeAccept->setColour(juce::TextButton::buttonColourId,
			                      allOk ? ColourPalette::slate : ColourPalette::backgroundLight);
			safeAccept->repaint();
		}
	};
}

void ObsidianAlertManager::showAssetDownloader(juce::Component *parent, const juce::File &destinationDir,
                                               std::function<void(bool success)> onComplete)
{
	auto modal = std::make_unique<ObsidianModalWindow>("Downloading Assets", 560, 400);
	auto content = std::make_unique<AssetDownloadContent>(destinationDir);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));
	modal->addButton("Cancel", crossSvg, ColourPalette::buttonInactive, nullptr);
	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	auto downloader = std::make_shared<AssetDownloadManager>();
	auto *cancelBtn = overlay->modalWindow->buttons[0];
	cancelBtn->onClick = [overlay, downloader, onComplete]()
	{
		downloader->cancel();
		if (onComplete)
			onComplete(false);
		overlay->close();
	};

	juce::Component::SafePointer<AssetDownloadContent> safeContent(contentPtr);
	juce::Component::SafePointer<ObsidianModalOverlay> safeOverlay(overlay);
	juce::Component::SafePointer<ObsidianSvgButton> safeCancel(cancelBtn);

	downloader->startFromManifest(
	    destinationDir,
	    [safeContent](const AssetDownloadProgress &p)
	    {
		    if (safeContent != nullptr)
			    safeContent->updateProgress(p);
	    },
	    [safeContent, safeOverlay, safeCancel, onComplete, parent](bool success, const juce::String &errorMessage)
	    {
		    if (safeContent != nullptr)
			    safeContent->setComplete(success, errorMessage);

		    if (success)
		    {
			    if (safeCancel != nullptr)
				    safeCancel->setEnabled(false);

			    juce::Timer::callAfterDelay(1500,
				                            [safeOverlay, onComplete, parent]()
				                            {
				                                if (safeOverlay != nullptr)
					                                safeOverlay->close();

				                                juce::Timer::callAfterDelay(
				                                    200,
				                                    [onComplete, parent]()
				                                    {
					                                    ObsidianAlertManager::showInfo(
					                                        parent, "Models Ready",
					                                        "All models have been downloaded "
					                                        "successfully.\nThey will be loaded "
					                                        "automatically on your first "
					                                        "generation.",
					                                        "OK",
					                                        [onComplete]()
					                                        {
						                                        if (onComplete)
							                                        onComplete(true);
					                                        });
				                                    });
			                                });
		    }
	    });
}

void ObsidianAlertManager::showCredits(juce::Component *parent)
{
	struct CreditEntry
	{
		juce::String name;
		juce::String copyright;
		juce::String license;
		juce::String url;
	};

	struct EarlyAdopterEntry
	{
		juce::String name;
		juce::String url;
	};

	static const std::vector<EarlyAdopterEntry> earlyAdopters = {
	    {"Brian Bullock", "https://rethinkstudios.tv"},
	    {"David Sablon", "https://www.linkedin.com/in/davidsablon/"},
	    {"Marc Wensauer", "https://ansolas.de/"},
	    {"Moteka", "https://linktr.ee/motekamusic"},
	    {"Steven Wagenheim", "https://soundcloud.com/steven-wagenheim"},
	    {"Yevgeni \"Jeff\" Birkhoff", "https://www.jbf.productions"},
	};

	static const std::vector<CreditEntry> entries = {
	    {"JUCE", "Copyright (c) Raw Material Software Limited.", "JUCE License", "https://juce.com"},
	    {"ONNX Runtime", "Copyright (c) Microsoft Corporation.", "MIT License",
		 "https://github.com/microsoft/onnxruntime"},
	    {"BeatDetektor", "Copyright (c) 2009 Charles J. Cliffe.", "MIT License", "http://opensource.org/licenses/MIT"},
	    {"Airwindows (Console6)", "Copyright (c) Chris Johnson (Airwindows).", "MIT License",
		 "https://www.airwindows.com/"},
	    {"nlohmann/json", "Copyright (c) 2013-2025 Niels Lohmann.", "MIT License", "https://github.com/nlohmann/json"},
	    {"Signalsmith Stretch", "Copyright (c) Signalsmith Audio Ltd.", "MIT License",
		 "https://github.com/Signalsmith-Audio/signalsmith-stretch"},
	    {"tokenizers-cpp", "Copyright (c) MLC AI / Apache TVM contributors.", "Apache License 2.0",
		 "https://github.com/mlc-ai/tokenizers-cpp"},
	    {"libsamplerate", "Copyright (c) 2012-2021, Erik de Castro Lopo.", "BSD 2-Clause License",
		 "https://github.com/libsndfile/libsamplerate"},
	    {"dr_wav (dr_libs)", "Copyright (c) David Reid.", "Public Domain / MIT-0",
		 "https://github.com/mackron/dr_libs"},
	    {"Ableton Link", "Copyright (c) Ableton AG, Berlin.", "Commercial License (Ableton AG)",
		 "https://github.com/Ableton/link"},
	    {"Monocypher", "Copyright (c) 2017-2023 Loup Vaillant, Michael Savage, Fabio Scotoni.",
		 "Public Domain (CC0) / BSD 2-Clause", "https://monocypher.org/"},
	};

	class CreditsContent : public ObsidianComponent
	{
	  public:
		juce::Viewport viewport;
		juce::Component container;

		struct Row : public ObsidianComponent
		{
			juce::Label nameLbl, copyrightLbl, licenseLbl;
			juce::HyperlinkButton urlBtn;
			juce::String url;

			Row(const CreditEntry &e) : url(e.url)
			{
				auto styleLabel = [](juce::Label &lbl, const juce::String &text, bool bold, juce::Colour col)
				{
					lbl.setText(text, juce::dontSendNotification);
					lbl.setColour(juce::Label::textColourId, col);
					lbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, bold ? juce::Font::bold : juce::Font::plain));
				};

				styleLabel(nameLbl, e.name, true, ColourPalette::textPrimary);
				styleLabel(copyrightLbl, e.copyright, false, ColourPalette::textSecondary);
				styleLabel(licenseLbl, e.license, false, ColourPalette::slate);

				urlBtn.setButtonText(e.url);
				urlBtn.setURL(juce::URL(e.url));
				urlBtn.setColour(juce::HyperlinkButton::textColourId, ColourPalette::buttonPrimary);
				urlBtn.setJustificationType(juce::Justification::centredLeft);
				urlBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
				addAndMakeVisible(urlBtn);

				addAndMakeVisible(nameLbl);
				addAndMakeVisible(copyrightLbl);
				addAndMakeVisible(licenseLbl);
				addAndMakeVisible(urlBtn);
			}

			void paint(juce::Graphics &g) override
			{
				g.setColour(ColourPalette::backgroundDeep);
				g.fillRoundedRectangle(getLocalBounds().toFloat(), Obsidian::CORNER);
				g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
				g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Obsidian::CORNER, 1.0f);
			}

			void resized() override
			{
				auto area = getLocalBounds().reduced(10, 6);
				nameLbl.setBounds(area.removeFromTop(18));
				area.removeFromTop(2);
				copyrightLbl.setBounds(area.removeFromTop(16));
				area.removeFromTop(2);

				auto bottom = area.removeFromTop(18);
				licenseLbl.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2));
				urlBtn.setBounds(bottom);
			}
		};

		struct AdopterRow : public ObsidianComponent
		{
			juce::Label nameLbl;
			juce::HyperlinkButton urlBtn;

			AdopterRow(const EarlyAdopterEntry &e)
			{
				nameLbl.setText(e.name, juce::dontSendNotification);
				nameLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
				nameLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
				addAndMakeVisible(nameLbl);

				urlBtn.setButtonText(e.url);
				urlBtn.setURL(juce::URL(e.url));
				urlBtn.setColour(juce::HyperlinkButton::textColourId, ColourPalette::buttonPrimary);
				urlBtn.setJustificationType(juce::Justification::centredLeft);
				urlBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
				addAndMakeVisible(urlBtn);
			}

			void paint(juce::Graphics &g) override
			{
				g.setColour(ColourPalette::backgroundDeep);
				g.fillRoundedRectangle(getLocalBounds().toFloat(), Obsidian::CORNER);
				g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
				g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Obsidian::CORNER, 1.0f);
			}

			void resized() override
			{
				auto area = getLocalBounds().reduced(10, 6);
				nameLbl.setBounds(area.removeFromTop(18));
				area.removeFromTop(2);
				urlBtn.setBounds(area.removeFromTop(18));
			}
		};

		std::vector<std::unique_ptr<Row>> rows;
		std::vector<std::unique_ptr<AdopterRow>> adopterRows;
		juce::Label header;
		juce::Label adoptersHeader;
		juce::HyperlinkButton authorLink;

		CreditsContent()
		{
			authorLink.setButtonText("Developed by innermost47");
			authorLink.setURL(juce::URL("https://github.com/innermost47"));
			authorLink.setColour(juce::HyperlinkButton::textColourId, ColourPalette::buttonPrimary);
			authorLink.setJustificationType(juce::Justification::centredLeft);
			authorLink.setMouseCursor(juce::MouseCursor::PointingHandCursor);
			container.addAndMakeVisible(authorLink);

			adoptersHeader.setText("Thanks to Obsidian Neural Early Adopters & Contributors",
			                       juce::dontSendNotification);
			adoptersHeader.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			adoptersHeader.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			adoptersHeader.setJustificationType(juce::Justification::centredLeft);
			container.addAndMakeVisible(adoptersHeader);

			for (const auto &a : earlyAdopters)
			{
				auto row = std::make_unique<AdopterRow>(a);
				container.addAndMakeVisible(*row);
				adopterRows.push_back(std::move(row));
			}

			header.setText("OBSIDIAN Neural - Third-Party Components", juce::dontSendNotification);
			header.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			header.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			header.setJustificationType(juce::Justification::centredLeft);
			container.addAndMakeVisible(header);

			for (const auto &e : entries)
			{
				auto row = std::make_unique<Row>(e);
				container.addAndMakeVisible(*row);
				rows.push_back(std::move(row));
			}

			viewport.setViewedComponent(&container, false);
			viewport.setScrollBarsShown(true, false);
			addAndMakeVisible(viewport);
		}

		void resized() override
		{
			viewport.setBounds(getLocalBounds());

			const int w = viewport.getWidth() - 12;
			const int rowH = 78;
			const int adopterRowH = 44;
			const int spacing = 6;
			const int headerH = 28;
			const int topPad = 8;

			int y = topPad;

			authorLink.setBounds(4, y, w, 20);
			y += 20 + 14;

			adoptersHeader.setBounds(0, y, w, headerH);
			y += headerH + 4;

			for (auto &row : adopterRows)
			{
				row->setBounds(0, y, w, adopterRowH);
				y += adopterRowH + spacing;
			}
			y += 10;

			header.setBounds(0, y, w, headerH);
			y += headerH + 4;

			for (auto &row : rows)
			{
				row->setBounds(0, y, w, rowH);
				y += rowH + spacing;
			}

			container.setSize(w, juce::jmax(y + topPad, viewport.getHeight()));
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>("Credits & Licenses", 600, 560);
	modal->setContent(std::make_unique<CreditsContent>());

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Close", crossSvg, ColourPalette::buttonInactive,
	                                [overlay]() { overlay->close(); });
}

void ObsidianAlertManager::showTextInput(juce::Component *parent, const juce::String &title, const juce::String &label,
                                         const juce::String &initialValue, const juce::String &confirmText,
                                         std::function<void(bool confirmed, const juce::String &text)> callback)
{
	class TextInputContent : public ObsidianComponent
	{
	  public:
		juce::Label fieldLbl;
		EscapableTextEditor textEditor;

		TextInputContent(const juce::String &labelText, const juce::String &initialText)
		{
			fieldLbl.setText(labelText, juce::dontSendNotification);
			fieldLbl.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			fieldLbl.setFont(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::bold));
			addAndMakeVisible(fieldLbl);

			textEditor.setText(initialText);
			textEditor.setColour(EscapableTextEditor::backgroundColourId, ColourPalette::backgroundDark);
			textEditor.setColour(EscapableTextEditor::textColourId, ColourPalette::textPrimary);
			textEditor.setColour(EscapableTextEditor::outlineColourId, ColourPalette::backgroundLight);
			textEditor.applyFontToAllText(juce::FontOptions(Obsidian::TEXT_REGULAR, juce::Font::plain));
			addAndMakeVisible(textEditor);
		}

		void resized() override
		{
			auto area = getLocalBounds().reduced(12);
			fieldLbl.setBounds(area.removeFromTop(18));
			area.removeFromTop(4);
			textEditor.setBounds(area.removeFromTop(30));
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 220);
	auto content = std::make_unique<TextInputContent>(label, initialValue);
	auto *contentPtr = content.get();
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback(false, "");
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton(confirmText, checkSvg, ColourPalette::slate,
	                                [overlay, contentPtr, parent, title, callback]()
	                                {
		                                juce::String text = contentPtr->textEditor.getText().trim();
		                                if (text.isEmpty())
		                                {
			                                showError(parent, title, "Please enter a name.");
			                                return;
		                                }
		                                if (callback)
			                                callback(true, text);
		                                overlay->close();
	                                });

	juce::Component::SafePointer<TextInputContent> safeContent(contentPtr);
	juce::Timer::callAfterDelay(150,
	                            [safeContent]()
	                            {
		                            if (safeContent != nullptr)
		                            {
			                            safeContent->textEditor.grabKeyboardFocus();
			                            safeContent->textEditor.selectAll();
		                            }
	                            });
}