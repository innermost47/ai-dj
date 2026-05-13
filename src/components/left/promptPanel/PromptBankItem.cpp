#include "PromptBankItem.h"
#include "Sizes.h"

static float measureTextWidth(const juce::Font &font, const juce::String &text)
{
	juce::GlyphArrangement ga;
	ga.addLineOfText(font, text, 0.0f, 0.0f);
	return ga.getBoundingBox(0, -1, true).getWidth();
}

PromptBankItem::PromptBankItem(PromptBankEntry *entryIn) : entry(entryIn)
{
	setSize(400, ObsidianSizes::ACCORDION_ITEM_MIN_HEIGHT);

	if (entry && entry->isBuiltIn)
		setEditable(false);

	dragPayloadProvider = [this]() -> juce::String
	{
		if (entry == nullptr)
			return {};
		return "prompt:" + entry->id;
	};
}

PromptBankItem::~PromptBankItem() = default;

void PromptBankItem::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	if (isSelected())
	{
		g.setColour(juce::Colours::white.withAlpha(0.06f));
		g.fillRect(bounds);
	}

	if (!entry)
	{
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
		g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f, (float)bounds.getWidth() - 4.0f,
		           (float)bounds.getBottom() - 1.0f, 0.5f);
		return;
	}

	if (!getEditable())
	{
		const int lockW = ObsidianSizes::ACCORDION_LOCK_AREA_WIDTH;
		auto lockArea = juce::Rectangle<int>(bounds.getRight() - lockW, bounds.getY(), lockW, lockW);
		auto lockBounds = lockArea.withSizeKeepingCentre(ObsidianSizes::ACCORDION_LOCK_ICON_SIZE,
		                                                 ObsidianSizes::ACCORDION_LOCK_ICON_SIZE);
		auto lockSvg = juce::Drawable::createFromImageData(BinaryData::lockfill_svg, BinaryData::lockfill_svgSize);
		if (lockSvg != nullptr)
		{
			lockSvg->replaceColour(juce::Colours::black, ColourPalette::textSecondary);
			lockSvg->drawWithin(g, lockBounds.toFloat(), juce::RectanglePlacement::xRight, ObsidianShades::ALPHA_04);
		}
	}

	if (entry->category.isNotEmpty())
	{
		const float thickness = isSelected() ? 4.0f : 1.0f;
		juce::Colour catCol =
		    categoryColourResolver ? categoryColourResolver(entry->category) : ColourPalette::backgroundLight;
		g.setColour(catCol);
		g.fillRect(0.0f, 0.0f, thickness, (float)bounds.getHeight());
	}
	else if (isSelected())
	{
		g.setColour(ColourPalette::lightGrey);
		g.fillRect(0.0f, 0.0f, 4.0f, (float)bounds.getHeight());
	}

	const int rightPad = !getEditable() ? (ObsidianSizes::ACCORDION_LOCK_AREA_WIDTH + 4) : 12;
	auto textArea = bounds.withTrimmedLeft(12).withTrimmedRight(rightPad).withTrimmedTop(6);
	auto promptArea = textArea.removeFromTop(bounds.getHeight() - 30);

	const float fontSize = ObsidianSizes::TEXT_REGULAR;
	juce::Font promptFont(juce::FontOptions(fontSize, juce::Font::plain));
	const float lineHeight = promptFont.getHeight();
	const int maxLines = ObsidianSizes::ACCORDION_ITEM_MAX_LINES;
	const float maxWidth = (float)promptArea.getWidth();

	juce::StringArray words;
	words.addTokens(entry->text, " \t\n", "");

	juce::StringArray lines;
	juce::String currentLine;

	for (auto &word : words)
	{
		juce::String test = currentLine.isEmpty() ? word : currentLine + " " + word;
		if (measureTextWidth(promptFont, test) <= maxWidth)
		{
			currentLine = test;
		}
		else
		{
			if (currentLine.isNotEmpty())
				lines.add(currentLine);
			currentLine = word;

			if ((int)lines.size() >= maxLines)
				break;
		}
	}
	if (currentLine.isNotEmpty() && (int)lines.size() < maxLines)
		lines.add(currentLine);

	if ((int)lines.size() == maxLines)
	{
		juce::String fullJoined = lines.joinIntoString(" ");
		if (fullJoined.length() < entry->text.length())
		{
			juce::String &last = lines.getReference(maxLines - 1);
			while (last.isNotEmpty() && measureTextWidth(promptFont, last + "...") > maxWidth)
				last = last.dropLastCharacters(1);
			last += "...";
		}
	}

	g.setColour(ColourPalette::textPrimary);
	g.setFont(promptFont);

	float y = (float)promptArea.getY();
	for (const auto &line : lines)
	{
		g.drawText(line, juce::Rectangle<float>((float)promptArea.getX(), y, maxWidth, lineHeight),
		           juce::Justification::topLeft, false);
		y += lineHeight;
		if (y + lineHeight > (float)promptArea.getBottom())
			break;
	}

	auto metaArea = bounds.withTrimmedLeft(12).withTrimmedRight(rightPad).removeFromBottom(20);
	juce::Colour modelColour = AiModelDefinitions::getColourForModel(entry->modelName);
	drawCircleWithEllipse(g, metaArea, modelColour);

	metaArea.removeFromLeft(14);

	juce::StringArray parts;
	parts.add(entry->modelName.isNotEmpty() ? entry->modelName : "unknown model");
	if (entry->usageCount > 0)
		parts.add(juce::String(entry->usageCount) + " uses");
	auto now = juce::Time::getCurrentTime();
	auto delta = now - entry->creationTime;
	int days = (int)delta.inDays();
	if (days < 1)
		parts.add("today");
	else if (days == 1)
		parts.add("yesterday");
	else if (days < 30)
		parts.add(juce::String(days) + " days ago");
	else if (days < 365)
		parts.add(juce::String(days / 30) + " months ago");
	else
		parts.add(juce::String(days / 365) + " years ago");

	g.setColour(ColourPalette::textSecondary.withAlpha(0.75f));
	g.setFont(juce::FontOptions(ObsidianSizes::TEXT_SMALL, juce::Font::italic));
	g.drawText(parts.joinIntoString(" - "), metaArea, juce::Justification::centredLeft, true);

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawLine(4.0f, (float)bounds.getBottom() - 1.0f, (float)bounds.getWidth() - 4.0f,
	           (float)bounds.getBottom() - 1.0f, 0.5f);
}

int PromptBankItem::getPreferredHeight(int width) const
{
	if (!entry)
		return ObsidianSizes::ACCORDION_ITEM_MIN_HEIGHT;

	const float fontSize = ObsidianSizes::TEXT_REGULAR;
	juce::Font promptFont(juce::FontOptions(fontSize, juce::Font::plain));
	const float maxWidth = (float)(width);

	if (maxWidth <= 0)
		return ObsidianSizes::ACCORDION_ITEM_MIN_HEIGHT;

	juce::AttributedString attr;
	attr.append(entry->text, promptFont);
	attr.setWordWrap(juce::AttributedString::byWord);
	attr.setLineSpacing(0.0f);

	juce::TextLayout layout;
	layout.createLayout(attr, maxWidth);

	int lineCount = juce::jlimit(1, (int)ObsidianSizes::ACCORDION_ITEM_MAX_LINES, layout.getNumLines());
	const float lineHeight = promptFont.getHeight() * 1.15f;

	return 6 + (int)(lineCount * lineHeight) + 20 + 4;
}