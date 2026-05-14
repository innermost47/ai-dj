#include "BasePanel.h"
#include "ColourPalette.h"
#include "PluginProcessor.h"

BasePanel::BasePanel(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

void BasePanel::transferOpenCategoryState(const juce::String &oldName, const juce::String &newName)
{
	if (oldName == newName)
		return;
	if (openCategories.erase(oldName) > 0)
		openCategories.insert(newName);
}

void BasePanel::expandAll(std::function<void(ObsidianAccordion *accordion, const juce::String &categoryName)> callback)
{
	openCategories.clear();
	for (auto &acc : accordions)
	{
		acc->setExpanded(true, false);
		if (callback != nullptr)
		{
			callback(acc.get(), acc->getName());
		}
		openCategories.insert(acc->getName());
	}
	header.setExpanded(true);
	resized();
}

void BasePanel::collapseAll()
{
	openCategories.clear();
	for (auto &acc : accordions)
		acc->setExpanded(false, false);
	header.setExpanded(false);
	resized();
}

void BasePanel::resized()
{
}

juce::Colour BasePanel::resolveCategoryColour(const juce::String &name) const
{
	auto *bank = audioProcessor.getPromptBank();
	if (!bank)
		return ColourPalette::backgroundLight;

	for (const auto &c : bank->getCategories())
		if (c.name == name)
			return c.colour != juce::Colour(0) ? c.colour : ColourPalette::backgroundLight;

	return ColourPalette::backgroundLight;
}

juce::var BasePanel::saveUIState(int sortType) const
{
	juce::DynamicObject::Ptr o = new juce::DynamicObject();
	juce::Array<juce::var> openArr;
	for (const auto &cat : openCategories)
		openArr.add(juce::var(cat));
	o->setProperty("openCategories", juce::var(openArr));
	o->setProperty("sort", sortType);
	o->setProperty("search", currentSearch);
	return juce::var(o.get());
}

void BasePanel::restoreUIState(const juce::var &state, std::function<void()> refreshCallback, int min, int max)
{
	if (!state.isObject())
		return;

	auto *o = state.getDynamicObject();
	if (o == nullptr)
		return;

	openCategories.clear();
	auto arr = o->getProperty("openCategories");
	if (arr.isArray())
		for (int i = 0; i < arr.getArray()->size(); ++i)
			openCategories.insert(arr.getArray()->getUnchecked(i).toString());

	header.setSelectedSortId(1, false);
	int s = (int)o->getProperty("sort");
	if (s >= min && s <= max)
		header.setSelectedSortId(s, false);

	juce::String savedSearch = o->getProperty("search").toString();
	currentSearch = savedSearch;
	header.setSearchText(savedSearch, false);

	if (refreshCallback != nullptr)
		refreshCallback();

	juce::MessageManager::callAsync(
	    [safe = juce::Component::SafePointer(this)]()
	    {
		    if (safe)
			    safe->resized();
	    });
}