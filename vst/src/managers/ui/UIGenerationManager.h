#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>

class DjIaVstEditor;

class UIGenerationManager : public DjIaVstProcessor::GenerationListener
{
  public:
	explicit UIGenerationManager(DjIaVstEditor &editor);
	~UIGenerationManager() = default;

	void onGenerationComplete(const juce::String &trackId, const juce::String &message) override;
	void startGenerationUI(const juce::String &trackId);
	void stopGenerationUI(const juce::String &trackId, bool success = true, const juce::String &errorMessage = "");
	void onGenerateButtonClicked();
	void generateFromTrackComponent(const juce::String &trackId);
	void startGenerationButtonAnimation();
	void stopGenerationButtonAnimation();
	void setAllGenerateButtonsEnabled(bool enabled);

	bool isGenerating() const
	{
		return isGenerating_.load();
	}
	void setIsGenerating(bool value)
	{
		isGenerating_.store(value);
	}

	bool wasGenerating() const
	{
		return wasGenerating_.load();
	}
	void setWasGenerating(bool value)
	{
		wasGenerating_.store(value);
	}

	juce::String getGeneratingTrackId() const
	{
		return generatingTrackId;
	}
	void setGeneratingTrackId(const juce::String &trackId)
	{
		generatingTrackId = trackId;
	}

	juce::String getOriginalButtonText() const
	{
		return originalButtonText;
	}
	void setOriginalButtonText(const juce::String &text)
	{
		originalButtonText = text;
	}

  private:
	DjIaVstEditor &editor;
	std::atomic<bool> isGenerating_{false};
	std::atomic<bool> wasGenerating_{false};
	juce::String generatingTrackId;
	juce::String originalButtonText;
};