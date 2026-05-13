#include "SampleBankItem.h"
#include "ObsidianAlertManager.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

SampleBankItem::SampleBankItem(SampleBankEntry *entry, DjIaVstProcessor &processor)
    : sampleEntry(entry), audioProcessor(processor)
{
	setSize(400, 52);
}

SampleBankItem::~SampleBankItem()
{
}

void SampleBankItem::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	if (selected)
	{
		g.setColour(juce::Colours::white.withAlpha(0.06f));
		g.fillRect(bounds);
	}

	if (!sampleEntry)
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f, (float)bounds.getWidth() - 4.0f,
		           (float)bounds.getBottom() - 1.0f, 0.5f);
		return;
	}

	if (sampleEntry && !sampleEntry->category.isEmpty())
	{
		const float thickness = selected ? 4.0f : 1.0f;
		g.setColour(getCategoryColor(sampleEntry->category));
		g.fillRect(0.0f, 0.0f, thickness, (float)bounds.getHeight());
	}
	else if (selected)
	{
		g.setColour(ColourPalette::lightGrey);
		g.fillRect(0.0f, 0.0f, 4.0f, (float)bounds.getHeight());
	}

	{
		auto nameArea = bounds.removeFromTop(20).withTrimmedLeft(12).withTrimmedRight(48);

		juce::AttributedString attr;
		attr.setJustification(juce::Justification::centredLeft);

		juce::String prompt = sampleEntry->originalPrompt;
		juce::Font font(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
		float maxWidth = (float)nameArea.getWidth();

		juce::GlyphArrangement ga;
		ga.addLineOfText(font, prompt, 0.0f, 0.0f);
		float textWidth = ga.getBoundingBox(0, -1, true).getWidth();

		if (textWidth > maxWidth)
		{
			while (prompt.isNotEmpty())
			{
				ga.clear();
				ga.addLineOfText(font, prompt + "...", 0.0f, 0.0f);
				if (ga.getBoundingBox(0, -1, true).getWidth() <= maxWidth)
					break;
				prompt = prompt.dropLastCharacters(1);
			}
			prompt += "...";
		}

		attr.append(prompt, juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold),
		            ColourPalette::textPrimary);

		attr.draw(g, nameArea.toFloat());
	}

	auto modelArea = bounds.removeFromTop(18).withTrimmedLeft(12).withTrimmedRight(48);

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

	if (!sampleEntry->category.isEmpty())
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

void SampleBankItem::resized()
{
}

juce::Colour SampleBankItem::getCategoryColor(const juce::String &category)
{
	if (categoryColourResolver)
		return categoryColourResolver(category);

	static const std::map<juce::String, juce::Colour> colors = {{"Drums", ColourPalette::indigo},
	                                                            {"Bass", ColourPalette::teal},
	                                                            {"Melody", ColourPalette::coral},
	                                                            {"Ambient", ColourPalette::emerald},
	                                                            {"Percussion", ColourPalette::slate},
	                                                            {"Vocal", ColourPalette::amber},
	                                                            {"FX", ColourPalette::backgroundLight},
	                                                            {"Loops", ColourPalette::buttonSuccess},
	                                                            {"One-shots", ColourPalette::buttonSecondary},
	                                                            {"House", ColourPalette::buttonDangerDark},
	                                                            {"Techno", ColourPalette::lime},
	                                                            {"Hip-Hop", ColourPalette::violet},
	                                                            {"Jazz", ColourPalette::amber},
	                                                            {"Rock", ColourPalette::buttonDanger},
	                                                            {"Electronic", ColourPalette::cyan},
	                                                            {"Piano", ColourPalette::textSecondary},
	                                                            {"Guitar", ColourPalette::textWarning},
	                                                            {"Synth", ColourPalette::textSecondary}};
	auto it = colors.find(category);
	return it != colors.end() ? it->second : ColourPalette::backgroundLight;
}

void SampleBankItem::mouseEnter(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void SampleBankItem::mouseExit(const juce::MouseEvent &)
{
	setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SampleBankItem::mouseDown(const juce::MouseEvent &event)
{
	if (event.mods.isRightButtonDown())
	{
		showCategoryMenu();
		return;
	}

	if (event.getNumberOfClicks() == 2)
	{
		if (!selected && onItemClicked)
			onItemClicked(sampleEntry);
		if (onPromptEditRequested)
			onPromptEditRequested(sampleEntry);
		return;
	}

	if (onItemClicked)
		onItemClicked(sampleEntry);
}

void SampleBankItem::mouseDrag(const juce::MouseEvent &event)
{
	if (event.getDistanceFromDragStart() < 6 || isDragging)
		return;
	isDragging = true;

	if (event.mods.isCtrlDown() && sampleEntry)
	{
		juce::File f(sampleEntry->filePath);
		if (f.exists())
		{
			juce::StringArray files;
			files.add(f.getFullPathName());
			performExternalDragDropOfFiles(files, false);
			return;
		}
	}
	if (auto *dc = juce::DragAndDropContainer::findParentDragContainerFor(this))
		dc->startDragging(sampleEntry->id, this);
}

void SampleBankItem::mouseUp(const juce::MouseEvent &)
{
	isDragging = false;
}

void SampleBankItem::showCategoryMenu()
{
	if (!sampleEntry)
		return;

	juce::String sampleId = sampleEntry->id;

	std::vector<juce::String> avail;
	if (getCategoriesList)
		avail = getCategoriesList();
	else
		avail = {"Drums", "Bass",   "Melody",  "Ambient", "Percussion", "Vocal",      "FX",    "Loops",  "One-shots",
		         "House", "Techno", "Hip-Hop", "Jazz",    "Rock",       "Electronic", "Piano", "Guitar", "Synth"};

	ObsidianAlertManager::showCategoryEditor(
	    this, sampleEntry->originalPrompt, sampleEntry->category, avail,
	    [sampleId, &ap = audioProcessor, cb = onCategoryChanged](const juce::String &cat)
	    {
		    if (auto *bank = ap.getSampleBank())
		    {
			    if (auto *s = bank->getSample(sampleId))
			    {
				    s->category = cat;
				    if (cb)
					    cb(s, cat);
			    }
		    }
	    });
}