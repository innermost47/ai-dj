#pragma once
#include <JuceHeader.h>

class DjIaVstProcessor;

class BankCategoryOperations
{
  public:
	struct EditResult
	{
		juce::String oldName;
		juce::String newName;
		juce::Colour newColour;
		bool wasRenamed{false};
	};

	static void deleteCategory(DjIaVstProcessor &processor, const juce::String &categoryName);

	static void renameCategory(DjIaVstProcessor &processor, const juce::String &oldName, const juce::String &newName,
	                           juce::Colour newColour);

	static void promptDeleteCategoryWithDialog(DjIaVstProcessor &processor, juce::Component *parentForDialog,
	                                           const juce::String &categoryName, std::function<void()> onCompleted);

	static void promptEditCategoryWithDialog(DjIaVstProcessor &processor, juce::Component *parentForDialog,
	                                         const juce::String &categoryName, juce::Colour currentColour,
	                                         std::function<void(const EditResult &)> onCompleted);

  private:
	BankCategoryOperations() = delete;
};