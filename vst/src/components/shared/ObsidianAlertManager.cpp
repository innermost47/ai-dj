#include "ObsidianAlertManager.h"

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
	class CategoryDialogContent : public juce::Component
	{
	  public:
		juce::TextEditor nameEditor;
		juce::Label nameLbl, colourLbl;
		ColourPicker colourPicker;

		CategoryDialogContent(const juce::String &initialName, juce::Colour initialColour)
		{
			nameLbl.setText("Name:", juce::dontSendNotification);
			nameLbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
			addAndMakeVisible(nameLbl);

			nameEditor.setText(initialName);
			nameEditor.setTextToShowWhenEmpty("Category name...", ColourPalette::textSecondary);
			nameEditor.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
			nameEditor.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
			nameEditor.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
			addAndMakeVisible(nameEditor);

			colourLbl.setText("Colour:", juce::dontSendNotification);
			colourLbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
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

	overlay->modalWindow->addButton("Add", checkSvg, ColourPalette::emerald,
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

	class CategoryDialogContent : public juce::Component
	{
	  public:
		juce::TextEditor nameEditor;
		juce::Label nameLbl, colourLbl;
		ColourPicker colourPicker;

		CategoryDialogContent(const juce::String &initialName, juce::Colour initialColour)
		{
			nameLbl.setText("Name:", juce::dontSendNotification);
			nameLbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
			addAndMakeVisible(nameLbl);

			nameEditor.setText(initialName);
			nameEditor.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
			nameEditor.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
			nameEditor.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
			addAndMakeVisible(nameEditor);

			colourLbl.setText("Colour:", juce::dontSendNotification);
			colourLbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
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

	overlay->modalWindow->addButton("Save", checkSvg, ColourPalette::amber,
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

	overlay->modalWindow->addButton("OK", crossSvg, ColourPalette::buttonDanger, [overlay]() { overlay->close(); });
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

	overlay->modalWindow->addButton(confirmText, checkSvg, ColourPalette::buttonDanger,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback(true);
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showConfigDialog(juce::Component *parent, const juce::String &title,
                                            const juce::String &serverUrl, const juce::String &apiKey,
                                            bool currentUseLocal, int currentTimeoutMs, bool isFirstTime,
                                            std::function<void(const ConfigDialogResult &)> callback)
{
	auto modal = std::make_unique<ObsidianModalWindow>(title, 480, 420);

	class ConfigContent : public juce::Component
	{
	  public:
		juce::ComboBox modeCombo, timeoutCombo;
		juce::TextEditor urlEditor, keyEditor;
		juce::Label modeLbl, urlLbl, keyLbl, timeoutLbl;

		ConfigContent(bool useLocal, const juce::String &url, const juce::String & /*key */, int timeout,
		              bool firstTime)
		{
			auto styleEditor = [](juce::TextEditor &te, const juce::String &text)
			{
				te.setText(text);
				te.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
				te.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
				te.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
				te.applyFontToAllText(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
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
				lbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
				lbl.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
				addAndMakeVisible(lbl);
			};

			styleLabel(modeLbl, "Generation Mode:");
			modeCombo.addItem("Server/API", 1);
			modeCombo.addItem("Local Model", 2);
			modeCombo.setSelectedId(useLocal ? 2 : 1);
			styleCombo(modeCombo);
			addAndMakeVisible(modeCombo);

			styleLabel(urlLbl, "Server URL:");
			styleEditor(urlEditor, url.isEmpty() ? "http://localhost:8000" : url);
			addAndMakeVisible(urlEditor);

			styleLabel(keyLbl, firstTime ? "API Key:" : "API Key (leave blank to keep current):");
			styleEditor(keyEditor, "");
			keyEditor.setPasswordCharacter('*');
			addAndMakeVisible(keyEditor);

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
		}

		void resized() override
		{
			auto bounds = getLocalBounds().reduced(10);
			int rowH = 30;
			int spacing = 15;

			modeLbl.setBounds(bounds.removeFromTop(20));
			modeCombo.setBounds(bounds.removeFromTop(rowH));
			bounds.removeFromTop(spacing);

			urlLbl.setBounds(bounds.removeFromTop(20));
			urlEditor.setBounds(bounds.removeFromTop(rowH));
			bounds.removeFromTop(spacing);

			keyLbl.setBounds(bounds.removeFromTop(20));
			keyEditor.setBounds(bounds.removeFromTop(rowH));
			bounds.removeFromTop(spacing);

			timeoutLbl.setBounds(bounds.removeFromTop(20));
			timeoutCombo.setBounds(bounds.removeFromTop(rowH));
		}
	};

	auto formContent =
	    std::make_unique<ConfigContent>(currentUseLocal, serverUrl, apiKey, currentTimeoutMs, isFirstTime);
	auto *formPtr = formContent.get();
	modal->setContent(std::move(formContent));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton(isFirstTime ? "Skip for now" : "Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                ConfigDialogResult res{false, false, "", "", 0};
		                                callback(res);
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton(isFirstTime ? "Save & Continue" : "Update", checkSvg, ColourPalette::buttonPrimary,
	                                [overlay, formPtr, callback]()
	                                {
		                                ConfigDialogResult res;
		                                res.confirmed = true;
		                                res.useLocalModel = formPtr->modeCombo.getSelectedId() == 2;
		                                res.serverUrl = formPtr->urlEditor.getText();
		                                res.apiKey = formPtr->keyEditor.getText();

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
                                              const std::vector<juce::String> &currentCategories,
                                              const std::vector<juce::String> &availableCategories,
                                              std::function<void(const std::vector<juce::String> &)> onSave)
{
	auto modal = std::make_unique<ObsidianModalWindow>("Categories: " + sampleName, 480, 400);

	auto categoryContent = std::make_unique<CategoryPanel>(currentCategories, availableCategories);
	auto *panelPtr = categoryContent.get();
	modal->setContent(std::move(categoryContent));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Clear All", crossSvg, ColourPalette::buttonInactive,
	                                [panelPtr]() { panelPtr->clearAll(); });

	overlay->modalWindow->addButton("Done", checkSvg, ColourPalette::buttonPrimary,
	                                [overlay, panelPtr, onSave]()
	                                {
		                                if (onSave)
			                                onSave(panelPtr->getSelectedCategories());
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showEditPrompt(juce::Component *parent, const juce::String &currentPrompt,
                                          std::function<void(const juce::String &newPrompt)> callback)
{
	class EditPromptContent : public juce::Component
	{
	  public:
		juce::TextEditor editor;
		EditPromptContent(const juce::String &text)
		{
			editor.setText(text);
			editor.setMultiLine(true);
			editor.setReturnKeyStartsNewLine(true);
			editor.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
			editor.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
			editor.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
			editor.applyFontToAllText(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
			addAndMakeVisible(editor);
		}
		void resized() override
		{
			editor.setBounds(getLocalBounds().reduced(5));
		}
	};

	auto modal = std::make_unique<ObsidianModalWindow>("Edit Custom Prompt", 520, 220);

	auto content = std::make_unique<EditPromptContent>(currentPrompt);
	auto *editorPtr = &content->editor;
	modal->setContent(std::move(content));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive,
	                                [overlay, callback]()
	                                {
		                                if (callback)
			                                callback("");
		                                overlay->close();
	                                });

	overlay->modalWindow->addButton("Save", checkSvg, ColourPalette::buttonPrimary,
	                                [overlay, editorPtr, callback]()
	                                {
		                                juce::String resultStr = editorPtr->getText();
		                                if (callback && resultStr.isNotEmpty())
			                                callback(resultStr);
		                                overlay->close();
	                                });
}

void ObsidianAlertManager::showUpdateAvailable(juce::Component *parent, const juce::String &latestTag,
                                               const juce::String &currentBuild)
{
	auto modal = std::make_unique<ObsidianModalWindow>("Update Available!", 520, 280);
	juce::String message = "A new version of OBSIDIAN Neural is available: " + latestTag +
	                       "\n\n"
	                       "Your current build: v" +
	                       currentBuild +
	                       "\n\n"
	                       "Download the latest version at:\ngithub.com/innermost47/ai-dj/releases/latest";

	modal->setContent(std::make_unique<TextContent>(message));

	auto *overlay = createAndAttachOverlay(parent, std::move(modal));
	if (overlay == nullptr)
		return;

	overlay->modalWindow->addButton("Later", crossSvg, ColourPalette::buttonInactive,
	                                [overlay]() { overlay->close(); });

	overlay->modalWindow->addButton(
	    "Download Now", downloadSvg, ColourPalette::buttonPrimary,
	    [overlay]()
	    {
		    juce::URL("https://github.com/innermost47/ai-dj/releases/latest").launchInDefaultBrowser();
		    overlay->close();
	    });
}
