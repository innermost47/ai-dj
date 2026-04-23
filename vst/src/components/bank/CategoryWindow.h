#pragma once
#include <JuceHeader.h>

class CategoryWindow : public juce::DocumentWindow
{
public:
	CategoryWindow(const juce::String& sampleName,
		const std::vector<juce::String>& currentCategories,
		const std::vector<juce::String>& availableCategories);

	~CategoryWindow() override;

	void closeButtonPressed() override;

	std::function<void(const std::vector<juce::String>&)> onCategoriesChanged;

private:
	class CategoryComponent;
	std::unique_ptr<CategoryComponent> categoryComponent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CategoryWindow)
};

class CategoryWindow::CategoryComponent : public juce::Component
{
public:
	CategoryComponent(const juce::String& sampleName,
		const std::vector<juce::String>& currentCategories,
		const std::vector<juce::String>& availableCategories);

	void paint(juce::Graphics& g) override;
	void resized() override;

	std::vector<juce::String> getSelectedCategories() const;

	std::function<void(const std::vector<juce::String>&)> onCategoriesChanged;

private:
	juce::TextButton clearAllButton;
	juce::TextButton closeButton;
	std::unique_ptr<juce::Viewport> viewport;
	std::unique_ptr<juce::Component> toggleContainer;

	struct CategoryToggle
	{
		juce::ToggleButton toggle;
		juce::String categoryName;
	};

	std::vector<std::unique_ptr<CategoryToggle>> categoryToggles;

	void updateCategories();
};
