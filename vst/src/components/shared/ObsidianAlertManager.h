#pragma once
#include "CategoryPanel.h"
#include "ColourPalette.h"
#include "ColourPicker.h"
#include "ObsidianModal.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DrawingCanvas;

class ObsidianAlertManager
{
  private:
	class TextContent : public juce::Component
	{
		juce::Label label;

	  public:
		TextContent(const juce::String &text)
		{
			label.setText(text, juce::dontSendNotification);
			label.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
			label.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::plain));
			label.setJustificationType(juce::Justification::centredLeft);
			addAndMakeVisible(label);
		}
		void resized() override
		{
			label.setBounds(getLocalBounds());
		}
	};

	static juce::Component *getSafePluginWindow(juce::Component *c);

	static ObsidianModalOverlay *createAndAttachOverlay(juce::Component *parent,
	                                                    std::unique_ptr<ObsidianModalWindow> modal);

  public:
	static inline const juce::String checkSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>)";
	static inline const juce::String crossSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>)";
	static inline const juce::String downloadSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="7 10 12 15 17 10"></polyline><line x1="12" y1="15" x2="12" y2="3"></line></svg>)";
	static inline const juce::String infoSvg =
	    R"(<svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>)";

	static void initialize()
	{
	}
	static void shutdown()
	{
	}
	static void showMidiMappingEditor(juce::Component *parent, class MidiLearnManager *manager);
	static DrawingCanvas *showDrawingCanvas(juce::Component *parent, DjIaVstProcessor &processor,
	                                        std::function<void(const juce::String &)> onGenerate,
	                                        std::function<void(DrawingCanvas *)> onClose = nullptr);

	static void showAddCategoryDialog(juce::Component *parent,
	                                  std::function<void(const juce::String &name, juce::Colour colour)> onAdd);

	static void showEditCategoryDialog(juce::Component *parent, const juce::String &currentName,
	                                   juce::Colour currentColour,
	                                   std::function<void(const juce::String &newName, juce::Colour newColour)> onSave);

	struct ConfigDialogResult
	{
		bool confirmed;
		bool useLocalModel;
		juce::String serverUrl;
		juce::String apiKey;
		int timeoutMs;
	};

	struct PromptEditorResult
	{
		bool confirmed = false;
		juce::String text;
		juce::String modelName;
		juce::String category;
	};

	static void showPromptEditor(juce::Component *parent, const juce::String &initialText,
	                             const juce::String &initialModel, const juce::String &initialCategory,
	                             const juce::StringArray &availableCategories,
	                             std::function<void(const PromptEditorResult &)> callback);

	static void showInfo(juce::Component *parent, const juce::String &title, const juce::String &message,
	                     const juce::String &buttonText = "OK", std::function<void()> onConfirm = nullptr);

	static void showError(juce::Component *parent, const juce::String &title, const juce::String &message);

	static void showConfirm(juce::Component *parent, const juce::String &title, const juce::String &message,
	                        const juce::String &confirmText, const juce::String &cancelText,
	                        std::function<void(bool confirmed)> callback);

	static void showConfigDialog(juce::Component *parent, const juce::String &title, const juce::String &serverUrl,
	                             const juce::String &apiKey, bool currentUseLocal, int currentTimeoutMs,
	                             bool isFirstTime, std::function<void(const ConfigDialogResult &)> callback);

	static void showCategoryEditor(juce::Component *parent, const juce::String &sampleName,
	                               const std::vector<juce::String> &currentCategories,
	                               const std::vector<juce::String> &availableCategories,
	                               std::function<void(const std::vector<juce::String> &)> onSave);

	static void showEditPrompt(juce::Component *parent, const juce::String &currentPrompt,
	                           std::function<void(const juce::String &newPrompt)> callback);

	static void showUpdateAvailable(juce::Component *parent, const juce::String &latestTag,
	                                const juce::String &currentBuild);
};