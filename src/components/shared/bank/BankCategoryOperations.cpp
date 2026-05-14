#include "BankCategoryOperations.h"
#include "ObsidianAlertManager.h"
#include "PluginProcessor.h"
#include "PromptBank.h"
#include "SampleBank.h"

void BankCategoryOperations::deleteCategory(DjIaVstProcessor &processor, const juce::String &categoryName)
{
	if (auto *pb = processor.getPromptBank())
		pb->removeCategory(categoryName);

	if (auto *sb = processor.getSampleBank())
	{
		for (auto *s : sb->getAllSamples())
			if (s->category == categoryName)
				s->category.clear();
		sb->saveBankData();
	}
}

void BankCategoryOperations::renameCategory(DjIaVstProcessor &processor, const juce::String &oldName,
                                            const juce::String &newName, juce::Colour newColour)
{
	if (auto *pb = processor.getPromptBank())
		pb->renameCategory(oldName, newName, newColour);

	if (auto *sb = processor.getSampleBank())
	{
		for (auto *s : sb->getAllSamples())
			if (s->category == oldName)
				s->category = newName;
		sb->saveBankData();
	}
}

void BankCategoryOperations::promptDeleteCategoryWithDialog(DjIaVstProcessor &processor,
                                                            juce::Component *parentForDialog,
                                                            const juce::String &categoryName,
                                                            std::function<void()> onCompleted)
{
	ObsidianAlertManager::showConfirm(parentForDialog, "Delete Category",
	                                  "Delete '" + categoryName +
	                                      "'?\n\n"
	                                      "Samples and prompts in this category will become Uncategorized.",
	                                  "Delete", "Cancel",
	                                  [&processor, categoryName, onCompleted](bool ok)
	                                  {
		                                  if (!ok)
			                                  return;
		                                  deleteCategory(processor, categoryName);
		                                  if (onCompleted)
			                                  onCompleted();
	                                  });
}

void BankCategoryOperations::promptEditCategoryWithDialog(DjIaVstProcessor &processor, juce::Component *parentForDialog,
                                                          const juce::String &categoryName, juce::Colour currentColour,
                                                          std::function<void(const EditResult &)> onCompleted)
{
	ObsidianAlertManager::showEditCategoryDialog(
	    parentForDialog, categoryName, currentColour,
	    [&processor, categoryName, onCompleted](const juce::String &newName, juce::Colour newColour)
	    {
		    renameCategory(processor, categoryName, newName, newColour);

		    EditResult result;
		    result.oldName = categoryName;
		    result.newName = newName;
		    result.newColour = newColour;
		    result.wasRenamed = (categoryName != newName);

		    if (onCompleted)
			    onCompleted(result);
	    });
}