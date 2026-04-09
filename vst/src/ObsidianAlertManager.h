#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"

class ObsidianAlertManager
{
public:
    static void initialize()
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&getAlertLookAndFeel());
    }

    static void shutdown()
    {
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    struct ConfigDialogResult
    {
        bool confirmed;
        bool useLocalModel;
        juce::String serverUrl;
        juce::String apiKey;
        int timeoutMs;
    };

    static void showInfo(
        const juce::String &title,
        const juce::String &message,
        const juce::String &buttonText = "OK",
        std::function<void()> onConfirm = nullptr)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle(title)
                .withMessage(message)
                .withButton(buttonText),
            [onConfirm](int)
            { if (onConfirm) onConfirm(); });
    }

    static void showError(
        const juce::String &title,
        const juce::String &message)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle(title)
                .withMessage(message)
                .withButton("OK"),
            nullptr);
    }

    static void showConfirm(
        const juce::String &title,
        const juce::String &message,
        const juce::String &confirmText,
        const juce::String &cancelText,
        std::function<void(bool confirmed)> callback)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle(title)
                .withMessage(message)
                .withButton(confirmText)
                .withButton(cancelText),
            [callback](int result)
            { if (callback) callback(result == 1); });
    }

    static void showConfigDialog(
        const juce::String &title,
        const juce::String &serverUrl,
        const juce::String &apiKey,
        bool currentUseLocal,
        int currentTimeoutMs,
        bool isFirstTime,
        std::function<void(const ConfigDialogResult &)> callback)
    {
        auto alertWindow = std::make_unique<juce::AlertWindow>(
            title,
            isFirstTime
                ? "Choose your generation method.\n\nNo API key yet? Get your free account at:\nobsidian-neural.com\n\n7-day free trial - 100 credits included - no credit card required."
                : "Update your settings:",
            juce::MessageBoxIconType::NoIcon);

        juce::StringArray modes;
        modes.add("Server/API (Full features + stems separation)");
        modes.add("Local Model (Basic - requires manual setup)");
        alertWindow->addComboBox("generationMode", modes, "Generation Mode:");
        if (auto *combo = alertWindow->getComboBoxComponent("generationMode"))
            combo->setSelectedItemIndex(currentUseLocal ? 1 : 0);

        alertWindow->addTextEditor("serverUrl",
                                   serverUrl.isEmpty() ? "http://localhost:8000" : serverUrl,
                                   "Server URL:");

        alertWindow->addTextEditor("apiKey", "",
                                   isFirstTime ? "API Key:" : "API Key (leave blank to keep current):");
        if (auto *keyEditor = alertWindow->getTextEditor("apiKey"))
            keyEditor->setPasswordCharacter('*');

        static const juce::Array<int> timeoutValues = {1, 2, 5, 10, 15, 20, 30, 45};
        juce::StringArray timeouts;
        for (int v : timeoutValues)
            timeouts.add(juce::String(v) + " minute" + (v > 1 ? "s" : ""));
        alertWindow->addComboBox("requestTimeout", timeouts, "Request Timeout:");
        if (auto *timeoutCombo = alertWindow->getComboBoxComponent("requestTimeout"))
        {
            int selectedIndex = 2;
            int currentMinutes = currentTimeoutMs / 60000;
            for (int i = 0; i < timeoutValues.size(); ++i)
                if (timeoutValues[i] == currentMinutes)
                {
                    selectedIndex = i;
                    break;
                }
            timeoutCombo->setSelectedItemIndex(selectedIndex);
        }

        alertWindow->addButton(isFirstTime ? "Save & Continue" : "Update", 1);
        alertWindow->addButton(isFirstTime ? "Skip for now" : "Cancel", 0);

        applyThemeToAlertWindow(alertWindow.get());

        auto *windowPtr = alertWindow.get();
        alertWindow.release()->enterModalState(true,
                                               juce::ModalCallbackFunction::create([windowPtr, callback](int result)
                                                                                   {
                ConfigDialogResult res{};
                res.confirmed = (result == 1);

                if (res.confirmed)
                {
                    auto* modeCombo    = windowPtr->getComboBoxComponent("generationMode");
                    auto* urlEditor    = windowPtr->getTextEditor("serverUrl");
                    auto* keyEditor    = windowPtr->getTextEditor("apiKey");
                    auto* timeoutCombo = windowPtr->getComboBoxComponent("requestTimeout");

                    res.useLocalModel = (modeCombo->getSelectedItemIndex() == 1);
                    res.serverUrl     = urlEditor->getText();
                    res.apiKey        = keyEditor->getText();
                    res.timeoutMs     = timeoutValues[timeoutCombo->getSelectedItemIndex()] * 60000;
                }

                windowPtr->exitModalState(result);
                delete windowPtr;

                if (callback) callback(res); }));
    }

    static void showEditPrompt(
        const juce::String &currentPrompt,
        std::function<void(const juce::String &newPrompt)> callback)
    {
        auto alertWindow = std::make_unique<juce::AlertWindow>(
            "Edit Custom Prompt", "Edit your prompt:",
            juce::MessageBoxIconType::NoIcon);
        alertWindow->addTextEditor("promptText", currentPrompt, "Prompt text:");
        alertWindow->addButton("Save", 1);
        alertWindow->addButton("Cancel", 0);

        applyThemeToAlertWindow(alertWindow.get());

        auto *windowPtr = alertWindow.get();
        alertWindow.release()->enterModalState(true,
                                               juce::ModalCallbackFunction::create([windowPtr, callback](int result)
                                                                                   {
                juce::String result_str;
                if (result == 1)
                    if (auto* editor = windowPtr->getTextEditor("promptText"))
                        result_str = editor->getText();
                windowPtr->exitModalState(result);
                delete windowPtr;
                if (callback && result == 1 && result_str.isNotEmpty())
                    callback(result_str); }));
    }

    static void showUpdateAvailable(
        const juce::String &latestTag,
        const juce::String &currentBuild)
    {
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::NoIcon)
                .withTitle("Update Available!")
                .withMessage(
                    "A new version of OBSIDIAN Neural is available: " + latestTag + "\n\n"
                                                                                    "Your current build: v" +
                    currentBuild + "\n\n"
                                   "Download the latest version at:\ngithub.com/innermost47/ai-dj/releases/latest")
                .withButton("Download Now")
                .withButton("Later"),
            [](int result)
            {
                if (result == 1)
                    juce::URL("https://github.com/innermost47/ai-dj/releases/latest")
                        .launchInDefaultBrowser();
            });
    }

    static void applyThemeToAlertWindow(juce::AlertWindow *aw)
    {
        aw->setColour(juce::AlertWindow::backgroundColourId, ColourPalette::backgroundDeep);
        aw->setColour(juce::AlertWindow::textColourId, ColourPalette::textPrimary);
        aw->setColour(juce::AlertWindow::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.6f));

        for (auto *child : aw->getChildren())
        {
            if (auto *te = dynamic_cast<juce::TextEditor *>(child))
            {
                te->setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
                te->setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
                te->setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.5f));
                te->setColour(juce::TextEditor::focusedOutlineColourId, ColourPalette::buttonPrimary);
                te->setColour(juce::TextEditor::highlightColourId, ColourPalette::buttonPrimary.withAlpha(0.3f));
                te->applyFontToAllText(juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain)));
            }
            if (auto *cb = dynamic_cast<juce::ComboBox *>(child))
            {
                cb->setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
                cb->setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
                cb->setColour(juce::ComboBox::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.5f));
                cb->setColour(juce::ComboBox::arrowColourId, ColourPalette::buttonPrimary);
            }
            if (auto *label = dynamic_cast<juce::Label *>(child))
            {
                label->setColour(juce::Label::textColourId, ColourPalette::textSecondary);
            }
        }
    }

private:
    class ObsidianLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ObsidianLookAndFeel()
        {
            setColour(juce::AlertWindow::backgroundColourId, ColourPalette::backgroundDeep);
            setColour(juce::AlertWindow::textColourId, ColourPalette::textPrimary);
            setColour(juce::AlertWindow::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.6f));

            setColour(juce::ResizableWindow::backgroundColourId, ColourPalette::backgroundDeep);

            setColour(juce::TextButton::buttonColourId, ColourPalette::buttonPrimary);
            setColour(juce::TextButton::buttonOnColourId, ColourPalette::buttonPrimary.darker(0.2f));
            setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            setColour(juce::TextButton::textColourOnId, juce::Colours::white);

            setColour(juce::TextEditor::backgroundColourId, ColourPalette::backgroundDark);
            setColour(juce::TextEditor::textColourId, ColourPalette::textPrimary);
            setColour(juce::TextEditor::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.5f));

            setColour(juce::ComboBox::backgroundColourId, ColourPalette::backgroundDark);
            setColour(juce::ComboBox::textColourId, ColourPalette::textPrimary);
            setColour(juce::ComboBox::outlineColourId, ColourPalette::buttonPrimary.withAlpha(0.5f));
            setColour(juce::ComboBox::arrowColourId, ColourPalette::buttonPrimary);

            setColour(juce::Label::textColourId, ColourPalette::textPrimary);
        }

        void drawAlertBox(juce::Graphics &g, juce::AlertWindow &alert,
                          const juce::Rectangle<int> &textArea,
                          juce::TextLayout &textLayout) override
        {
            LookAndFeel_V4::drawAlertBox(g, alert, textArea, textLayout);
        }

        int getAlertWindowButtonHeight() override { return 36; }

        juce::Font getAlertWindowTitleFont() override
        {
            return juce::Font(juce::FontOptions("Courier New", 16.0f, juce::Font::bold));
        }

        juce::Font getAlertWindowMessageFont() override
        {
            return juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
        }

        juce::Font getAlertWindowFont() override
        {
            return juce::Font(juce::FontOptions("Courier New", 13.0f, juce::Font::plain));
        }
    };

    static ObsidianLookAndFeel &getAlertLookAndFeel()
    {
        static ObsidianLookAndFeel instance;
        return instance;
    }
};