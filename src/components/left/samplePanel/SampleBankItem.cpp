#include "SampleBankItem.h"
#include "PluginProcessor.h"

SampleBankItem::SampleBankItem(SampleBankEntry *entry, DjIaVstProcessor &processor)
    : sampleEntry(entry), audioProcessor(processor)
{
	setSize(400, ObsidianSizes::SAMPLE_ROW_HEIGHT);

	dragPayloadProvider = [this]() -> juce::String
	{
		if (sampleEntry == nullptr)
			return {};
		return sampleEntry->id;
	};

	onBuildContextMenu = [this](const juce::MouseEvent &e) { buildSampleContextMenu(e); };
}

SampleBankItem::~SampleBankItem() = default;

void SampleBankItem::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	if (isSelected())
	{
		g.setColour(juce::Colours::white.withAlpha(0.06f));
		g.fillRect(bounds);
	}

	if (sampleEntry == nullptr)
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f, (float)bounds.getWidth() - 4.0f,
		           (float)bounds.getBottom() - 1.0f, 0.5f);
		return;
	}

	if (sampleEntry->category.isNotEmpty())
	{
		const float thickness = isSelected() ? 4.0f : 1.0f;
		juce::Colour catCol =
		    categoryColourResolver ? categoryColourResolver(sampleEntry->category) : ColourPalette::backgroundLight;
		g.setColour(catCol);
		g.fillRect(0.0f, 0.0f, thickness, (float)bounds.getHeight());
	}
	else if (isSelected())
	{
		g.setColour(ColourPalette::lightGrey);
		g.fillRect(0.0f, 0.0f, 4.0f, (float)bounds.getHeight());
	}

	{
		auto nameArea = bounds.removeFromTop(20).withTrimmedLeft(12).withTrimmedRight(12);

		juce::Font font(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
		juce::String prompt = sampleEntry->originalPrompt;
		const float maxWidth = (float)nameArea.getWidth();

		if (juce::GlyphArrangement::getStringWidth(font, prompt) > maxWidth)
		{
			while (prompt.isNotEmpty())
			{
				if (juce::GlyphArrangement::getStringWidth(font, prompt + "...") <= maxWidth)
					break;
				prompt = prompt.dropLastCharacters(1);
			}
			prompt += "...";
		}

		g.setColour(ColourPalette::textPrimary);
		g.setFont(font);
		g.drawText(prompt, nameArea, juce::Justification::centredLeft, false);
	}

	auto modelArea = bounds.removeFromTop(18).withTrimmedLeft(12).withTrimmedRight(12);

	juce::Colour modelColour = AiModelDefinitions::getColourForModel(sampleEntry->modelName);
	drawCircleWithEllipse(g, modelArea, modelColour);

	modelArea.removeFromLeft(14);

	juce::String displayName = sampleEntry->modelName.isNotEmpty() ? sampleEntry->modelName : "Unknown model";

	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SMALL, juce::Font::italic));
	g.setColour(ColourPalette::textSecondary);
	g.drawText(displayName, modelArea, juce::Justification::centredLeft, true);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f, (float)bounds.getWidth() - 4.0f,
	           (float)bounds.getBottom() - 1.0f, 0.5f);

	auto metaArea = bounds.removeFromBottom(22).withTrimmedLeft(12).withTrimmedRight(4);

	juce::StringArray parts;

	if (sampleEntry->category.isNotEmpty())
		parts.add("[" + sampleEntry->category + "]");

	if (sampleEntry->duration > 0.0f)
	{
		int sc = (int)sampleEntry->duration % 60;
		int ms = (int)((sampleEntry->duration - (int)sampleEntry->duration) * 10);
		parts.add(juce::String::formatted("%d.%ds", sc, ms));
	}

	if (sampleEntry->bpm > 0.0f)
		parts.add(juce::String(sampleEntry->bpm, 1) + " BPM");

	if (sampleEntry->key.isNotEmpty())
		parts.add(sampleEntry->key);

	const int usageCount = (int)sampleEntry->usedInProjects.size();
	if (usageCount == 0)
		parts.add("Unused");
	else
		parts.add(juce::String(usageCount) + " project(s)");

	g.setColour(ColourPalette::textSecondary.withAlpha(0.75f));
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SMALL));
	g.drawText(parts.joinIntoString(" - "), metaArea, juce::Justification::centredLeft, true);

	if (sampleEntry->description.isNotEmpty())
	{
		auto descArea = bounds.removeFromBottom(14).withTrimmedLeft(12).withTrimmedRight(4);
		g.setColour(ColourPalette::textSecondary.withAlpha(0.5f));
		g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SMALL));
		g.drawText(sampleEntry->description, descArea, juce::Justification::centredLeft, true);
	}
}

int SampleBankItem::getPreferredHeight(int /*width*/) const
{
	return ObsidianSizes::SAMPLE_ROW_HEIGHT;
}

void SampleBankItem::mouseDrag(const juce::MouseEvent &event)
{
	if (event.mods.isCtrlDown() && sampleEntry != nullptr)
	{
		if (event.getDistanceFromDragStart() < 6)
			return;

		juce::File f(sampleEntry->filePath);
		if (f.exists())
		{
			juce::StringArray files;
			files.add(f.getFullPathName());
			juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false);
			return;
		}
	}
	ObsidianListItem::mouseDrag(event);
}

void SampleBankItem::buildSampleContextMenu(const juce::MouseEvent &e)
{
	if (sampleEntry == nullptr)
		return;

	juce::PopupMenu menu;

	enum MenuIds
	{
		MenuEditPrompt = 1,
		MenuChangeCategory = 2,
		MenuDelete = 3
	};

	menu.addItem(MenuEditPrompt, "Edit prompt");
	menu.addItem(MenuChangeCategory, "Move to category...");
	menu.addSeparator();
	menu.addItem(MenuDelete, "Delete");

	auto screenPos = e.getScreenPosition();
	auto screenArea = juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1);

	juce::WeakReference<juce::Component> weakSelf(this);
	menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screenArea),
	                   [weakSelf](int result)
	                   {
		                   if (weakSelf == nullptr)
			                   return;
		                   auto *self = dynamic_cast<SampleBankItem *>(weakSelf.get());
		                   if (self == nullptr || self->sampleEntry == nullptr)
			                   return;

		                   switch (result)
		                   {
		                   case MenuEditPrompt:
			                   if (self->onPromptEditRequested)
				                   self->onPromptEditRequested(self->sampleEntry);
			                   break;
		                   case MenuChangeCategory:
			                   if (self->onChangeCategoryRequested)
				                   self->onChangeCategoryRequested(self->sampleEntry);
			                   break;
		                   case MenuDelete:
			                   if (self->onSampleDeleteRequested)
				                   self->onSampleDeleteRequested(self->sampleEntry);
			                   break;
		                   default:
			                   break;
		                   }
	                   });
}