#include "CategoryWindow.h"

CategoryWindow::CategoryWindow(const juce::String& sampleName,
	const std::vector<juce::String>& currentCategories,
	const std::vector<juce::String>& availableCategories)
	: DocumentWindow("Categories - " + sampleName,
		juce::Colour(0xff1e1e1e),
		DocumentWindow::closeButton)
{
	categoryComponent = std::make_unique<CategoryComponent>(sampleName, currentCategories, availableCategories);

	categoryComponent->onCategoriesChanged = [this](const std::vector<juce::String>& newCategories)
		{
			if (onCategoriesChanged)
				onCategoriesChanged(newCategories);
		};

	setContentOwned(categoryComponent.get(), true);
	setResizable(false, false);
	setSize(300, 400);
	centreWithSize(300, 400);
	setVisible(true);
}

CategoryWindow::~CategoryWindow() = default;

void CategoryWindow::closeButtonPressed()
{
	if (onCategoriesChanged)
	{
		onCategoriesChanged(categoryComponent->getSelectedCategories());
	}
	delete this;
}

CategoryWindow::CategoryComponent::CategoryComponent(const juce::String& /*sampleName*/,
	const std::vector<juce::String>& currentCategories,
	const std::vector<juce::String>& availableCategories)
{

	addAndMakeVisible(clearAllButton);
	clearAllButton.setButtonText("Clear All");
	clearAllButton.onClick = [this]()
		{
			for (auto& toggle : categoryToggles)
			{
				toggle->toggle.setToggleState(false, juce::dontSendNotification);
			}
			updateCategories();
		};

	addAndMakeVisible(closeButton);
	closeButton.setButtonText("Done");
	closeButton.onClick = [this]()
		{
			if (auto* window = findParentComponentOfClass<CategoryWindow>())
			{
				window->closeButtonPressed();
			}
		};

	for (const auto& category : availableCategories)
	{
		auto toggle = std::make_unique<CategoryToggle>();
		toggle->categoryName = category;
		toggle->toggle.setButtonText(category);
		toggle->toggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
		toggle->toggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightgreen);

		bool isAssigned = std::find(currentCategories.begin(), currentCategories.end(), category) != currentCategories.end();
		toggle->toggle.setToggleState(isAssigned, juce::dontSendNotification);

		toggle->toggle.onClick = [this]() { updateCategories(); };

		categoryToggles.push_back(std::move(toggle));
	}
}

void CategoryWindow::CategoryComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colour(0xff2d2d2d));
	g.setColour(juce::Colour(0xff404040));
	g.drawRect(getLocalBounds(), 2);
}

void CategoryWindow::CategoryComponent::resized()
{
	auto area = getLocalBounds().reduced(10);

	auto buttonArea = area.removeFromBottom(30);
	clearAllButton.setBounds(buttonArea.removeFromLeft(80));
	buttonArea.removeFromLeft(10);
	closeButton.setBounds(buttonArea.removeFromRight(80));

	area.removeFromBottom(10);

	if (!viewport)
	{
		viewport = std::make_unique<juce::Viewport>();
		toggleContainer = std::make_unique<juce::Component>();

		addAndMakeVisible(*viewport);
		viewport->setViewedComponent(toggleContainer.get(), false);
		viewport->setScrollBarsShown(true, false);
	}

	viewport->setBounds(area);

	int toggleHeight = 25;
	int spacing = 5;
	int totalHeight = static_cast<int>(categoryToggles.size()) * (toggleHeight + spacing);

	toggleContainer->setSize(area.getWidth() - viewport->getScrollBarThickness(), totalHeight);
	for (auto& toggle : categoryToggles)
	{
		if (!toggle->toggle.getParentComponent())
		{
			toggleContainer->addAndMakeVisible(toggle->toggle);
		}
	}
	int yPos = 0;
	for (auto& toggle : categoryToggles)
	{
		toggle->toggle.setBounds(5, yPos, toggleContainer->getWidth() - 10, toggleHeight);
		yPos += toggleHeight + spacing;
	}
}

std::vector<juce::String> CategoryWindow::CategoryComponent::getSelectedCategories() const
{
	std::vector<juce::String> selected;
	for (const auto& toggle : categoryToggles)
	{
		if (toggle->toggle.getToggleState())
		{
			selected.push_back(toggle->categoryName);
		}
	}
	return selected;
}

void CategoryWindow::CategoryComponent::updateCategories()
{
	if (onCategoriesChanged)
	{
		onCategoriesChanged(getSelectedCategories());
	}
}