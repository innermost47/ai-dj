#pragma once
#include "ObsidianBase.h"
#include "PromptBankPanel.h"
#include "SampleBankPanel.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;

class LeftPanelWrapper : public ObsidianComponent
{
  public:
	enum class Tab
	{
		Prompt = 0,
		Sample = 1
	};

	LeftPanelWrapper(DjIaVstProcessor &processor, DjIaVstEditor &editor);
	~LeftPanelWrapper() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	void setActiveTab(Tab tab);
	Tab getActiveTab() const
	{
		return activeTab;
	}

	SampleBankPanel *getSampleBankPanel()
	{
		return sampleBank.get();
	}
	PromptBankPanel *getPromptBankPanel()
	{
		return promptBank.get();
	}

	juce::var saveUIState() const;
	void restoreUIState(const juce::var &state);

  private:
	void updateTabVisibility();

	std::unique_ptr<SampleBankPanel> sampleBank;
	std::unique_ptr<PromptBankPanel> promptBank;

	IconButton promptTabButton{"prompt"};
	IconButton sampleTabButton{"sample"};

	Tab activeTab = Tab::Prompt;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftPanelWrapper)
};