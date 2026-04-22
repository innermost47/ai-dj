#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"
#include "ObsidianModal.h"

class ObsidianAlertManager
{
private:
	static inline const juce::String checkSvg = R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>)";
	static inline const juce::String crossSvg = R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)";
	static inline const juce::String downloadSvg = R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="7 10 12 15 17 10"></polyline><line x1="12" y1="15" x2="12" y2="3"></line></svg>)";
	static inline const juce::String infoSvg = R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>)";

	class TextContent : public juce::Component
	{
		juce::Label label;
	public:
		TextContent(const juce::String& text) {
			label.setText(text, juce::dontSendNotification);
			label.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			label.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
			label.setJustificationType(juce::Justification::centredLeft);
			addAndMakeVisible(label);
		}
		void resized() override { label.setBounds(getLocalBounds()); }
	};

public:
	static void initialize() {}
	static void shutdown() {}

	struct ConfigDialogResult
	{
		bool confirmed;
		bool useLocalModel;
		juce::String serverUrl;
		juce::String apiKey;
		int timeoutMs;
	};

	static void showInfo(
		juce::Component* parent,
		const juce::String& title,
		const juce::String& message,
		const juce::String& buttonText = "OK",
		std::function<void()> onConfirm = nullptr)
	{
		auto modal = std::make_unique<ObsidianModalWindow>(title);
		modal->setContent(std::make_unique<TextContent>(message));
		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton(buttonText, checkSvg, ColourPalette::buttonPrimary, [overlay, onConfirm]() {
			if (onConfirm) onConfirm();
			overlay->close();
			});
	}

	static void showError(
		juce::Component* parent,
		const juce::String& title,
		const juce::String& message)
	{
		auto modal = std::make_unique<ObsidianModalWindow>(title);
		modal->setContent(std::make_unique<TextContent>(message));
		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton("OK", crossSvg, ColourPalette::buttonDanger, [overlay]() {
			overlay->close();
			});
	}

	static void showConfirm(
		juce::Component* parent,
		const juce::String& title,
		const juce::String& message,
		const juce::String& confirmText,
		const juce::String& cancelText,
		std::function<void(bool confirmed)> callback)
	{
		auto modal = std::make_unique<ObsidianModalWindow>(title);
		modal->setContent(std::make_unique<TextContent>(message));
		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton(cancelText, crossSvg, ColourPalette::buttonInactive, [overlay, callback]() {
			if (callback) callback(false);
			overlay->close();
			});

		overlay->modalWindow->addButton(confirmText, checkSvg, ColourPalette::buttonDanger, [overlay, callback]() {
			if (callback) callback(true);
			overlay->close();
			});
	}

	static void showConfigDialog(
		juce::Component* parent,
		const juce::String& title,
		const juce::String& serverUrl,
		const juce::String& apiKey,
		bool currentUseLocal,
		int currentTimeoutMs,
		bool isFirstTime,
		std::function<void(const ConfigDialogResult&)> callback)
	{
		auto modal = std::make_unique<ObsidianModalWindow>(title);

		class ConfigContent : public juce::Component
		{
		public:
			juce::ComboBox modeCombo, timeoutCombo;
			juce::TextEditor urlEditor, keyEditor;
			juce::Label modeLbl, urlLbl, keyLbl, timeoutLbl;

			ConfigContent(bool useLocal, const juce::String& url, const juce::String& key, int timeout, bool firstTime)
			{
				auto styleEditor = [](juce::TextEditor& te, const juce::String& text) {
					te.setText(text);
					te.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
					te.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
					te.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
					te.applyFontToAllText(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
					};

				auto styleCombo = [](juce::ComboBox& cb) {
					cb.setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
					cb.setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
					cb.setColour(juce::ComboBox::outlineColourId, ColourPalette::backgroundLight);
					cb.setColour(juce::ComboBox::arrowColourId, ColourPalette::buttonPrimary);
					};

				auto styleLabel = [this](juce::Label& lbl, const juce::String& text) {
					lbl.setText(text, juce::dontSendNotification);
					lbl.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
					lbl.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
					addAndMakeVisible(lbl);
					};

				styleLabel(modeLbl, "Generation Mode:");
				modeCombo.addItem("Server/API (Full features + stems separation)", 1);
				modeCombo.addItem("Local Model (Basic - requires manual setup)", 2);
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
				if (timeout == 60000) selectedIndex = 1;
				else if (timeout == 120000) selectedIndex = 2;
				else if (timeout == 600000) selectedIndex = 4;

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

		auto formContent = std::make_unique<ConfigContent>(currentUseLocal, serverUrl, apiKey, currentTimeoutMs, isFirstTime);
		auto* formPtr = formContent.get();
		modal->setContent(std::move(formContent));

		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton(isFirstTime ? "Skip for now" : "Cancel", crossSvg, ColourPalette::buttonInactive, [overlay, callback]() {
			ConfigDialogResult res{ false, false, "", "", 0 };
			callback(res);
			overlay->close();
			});

		overlay->modalWindow->addButton(isFirstTime ? "Save & Continue" : "Update", checkSvg, ColourPalette::buttonPrimary, [overlay, formPtr, callback]() {
			ConfigDialogResult res;
			res.confirmed = true;
			res.useLocalModel = formPtr->modeCombo.getSelectedId() == 2;
			res.serverUrl = formPtr->urlEditor.getText();
			res.apiKey = formPtr->keyEditor.getText();

			int tid = formPtr->timeoutCombo.getSelectedId();
			if (tid == 1) res.timeoutMs = 60000;
			else if (tid == 2) res.timeoutMs = 120000;
			else if (tid == 4) res.timeoutMs = 600000;
			else res.timeoutMs = 300000;

			callback(res);
			overlay->close();
			});
	}

	static void showEditPrompt(
		juce::Component* parent,
		const juce::String& currentPrompt,
		std::function<void(const juce::String& newPrompt)> callback)
	{
		auto modal = std::make_unique<ObsidianModalWindow>("Edit Custom Prompt");

		class EditPromptContent : public juce::Component
		{
		public:
			juce::TextEditor editor;
			EditPromptContent(const juce::String& text) {
				editor.setText(text);
				editor.setMultiLine(true);
				editor.setReturnKeyStartsNewLine(true);
				editor.setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
				editor.setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
				editor.setColour(juce::TextEditor::outlineColourId, ColourPalette::backgroundLight);
				editor.applyFontToAllText(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
				addAndMakeVisible(editor);
			}
			void resized() override { editor.setBounds(getLocalBounds().reduced(5)); }
		};

		auto content = std::make_unique<EditPromptContent>(currentPrompt);
		auto* editorPtr = &content->editor;
		modal->setContent(std::move(content));

		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton("Cancel", crossSvg, ColourPalette::buttonInactive, [overlay, callback]() {
			if (callback) callback("");
			overlay->close();
			});

		overlay->modalWindow->addButton("Save", checkSvg, ColourPalette::buttonPrimary, [overlay, editorPtr, callback]() {
			juce::String resultStr = editorPtr->getText();
			if (callback && resultStr.isNotEmpty()) callback(resultStr);
			overlay->close();
			});
	}

	static void showUpdateAvailable(
		juce::Component* parent,
		const juce::String& latestTag,
		const juce::String& currentBuild)
	{
		auto modal = std::make_unique<ObsidianModalWindow>("Update Available!");
		juce::String message = "A new version of OBSIDIAN Neural is available: " + latestTag + "\n\n"
			"Your current build: v" + currentBuild + "\n\n"
			"Download the latest version at:\ngithub.com/innermost47/ai-dj/releases/latest";

		modal->setContent(std::make_unique<TextContent>(message));
		auto* overlay = new ObsidianModalOverlay(parent, std::move(modal));

		overlay->modalWindow->addButton("Later", crossSvg, ColourPalette::buttonInactive, [overlay]() {
			overlay->close();
			});

		overlay->modalWindow->addButton("Download Now", downloadSvg, ColourPalette::buttonPrimary, [overlay]() {
			juce::URL("https://github.com/innermost47/ai-dj/releases/latest").launchInDefaultBrowser();
			overlay->close();
			});
	}
};