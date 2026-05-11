#include "IconButton.h"
#include "Sizes.h"

std::unique_ptr<juce::Drawable> IconButtonBase::loadSVG(const char *data, size_t size)
{
	auto svgString = juce::String::fromUTF8(data, (int)size);
	svgString = svgString.replace("currentColor", "#000000");
	auto xml = juce::parseXML(svgString);
	if (xml != nullptr)
		return juce::Drawable::createFromSVG(*xml);
	return nullptr;
}

void IconButtonBase::paintIconButton(juce::Graphics &g, juce::Button &btn, bool isMouseOver, bool isButtonDown)
{
	auto fullBounds = btn.getLocalBounds().toFloat();
	const float cornerSize = ObsidianSizes::CORNER;
	bool toggled = btn.getToggleState();
	bool enabled = btn.isEnabled();

	if (showBackground)
	{
		if (showBorder)
		{
			auto bounds = fullBounds.reduced(0.5f);

			if (!isButtonDown)
			{
				g.setColour(juce::Colours::black.withAlpha(0.25f));
				g.fillRoundedRectangle(bounds.translated(0, 1.0f), cornerSize);
			}

			juce::Colour baseColour = toggled ? btn.findColour(juce::TextButton::buttonOnColourId)
			                                  : btn.findColour(juce::TextButton::buttonColourId);

			g.setColour(baseColour);
			g.fillRoundedRectangle(bounds, cornerSize);

			g.setColour(isMouseOver ? borderColour.withAlpha(0.6f) : borderColour);
			g.drawRoundedRectangle(bounds, cornerSize, 0.8f);
		}
		else
		{
			juce::Colour bgColour = toggled ? btn.findColour(juce::TextButton::buttonOnColourId)
			                                : btn.findColour(juce::TextButton::buttonColourId);
			if (!enabled)
				bgColour = bgColour.withMultipliedAlpha(0.4f);
			else if (isButtonDown)
				bgColour = bgColour.darker(0.08f);
			else if (isMouseOver)
				bgColour = bgColour.darker(0.03f);
			g.setColour(bgColour);
			g.fillRoundedRectangle(fullBounds.reduced(1.0f), cornerSize);
			g.setColour(ColourPalette::backgroundLight.darker(0.15f).withAlpha(enabled ? 0.8f : 0.3f));
			g.drawRoundedRectangle(fullBounds.reduced(1.0f), cornerSize, 1.0f);
		}
	}

	auto *drawable =
	    (toggled && hasToggledIcon && iconDrawableToggled) ? iconDrawableToggled.get() : iconDrawable.get();
	const bool hasIcon = (drawable != nullptr);

	const float topPadding = hasAccentBar ? 3.0f : 2.0f;
	const float bottomPadding = hasAccentBar ? (showBorder ? 4.0f : 2.0f) : 2.0f;
	const float accentBarSlot = hasAccentBar ? 3.0f : 0.0f;
	const float accentGap = hasAccentBar ? 2.0f : 0.0f;
	const bool drawAccentBar = hasAccentBar && toggled && enabled;
	const float labelHeight =
	    labelText.isNotEmpty() ? (hasIcon ? (isCompact ? 8.0f : 10.0f) : (isCompact ? 10.0f : 12.0f)) : 0.0f;
	const float labelGap = (labelHeight > 0.0f && hasIcon) ? 2.0f : 0.0f;

	auto contentArea = fullBounds.reduced(2.0f, 0.0f);
	contentArea.removeFromTop(topPadding);
	contentArea.removeFromBottom(bottomPadding);

	if (accentBarSlot > 0.0f)
	{
		auto barArea = contentArea.removeFromBottom(accentBarSlot);
		if (drawAccentBar)
		{
			const float barInset = 4.0f;
			g.setColour(btn.findColour(juce::TextButton::textColourOnId));
			g.fillRoundedRectangle(
			    {barArea.getX() + barInset, barArea.getY(), barArea.getWidth() - 2.0f * barInset, accentBarSlot},
			    accentBarSlot * 0.5f);
		}
		contentArea.removeFromBottom(accentGap);
	}

	juce::Rectangle<float> labelArea;
	if (labelHeight > 0.0f)
	{
		if (hasIcon)
		{
			labelArea = contentArea.removeFromBottom(labelHeight);
			contentArea.removeFromBottom(labelGap);
		}
		else
		{
			labelArea = contentArea;
		}
	}

	if (hasIcon && contentArea.getHeight() > 2.0f)
	{
		const float marginH = isCompact ? 0.10f : 0.18f;
		const float marginV = isCompact ? 0.05f : 0.10f;
		auto iconBounds = contentArea.reduced(contentArea.getWidth() * marginH, contentArea.getHeight() * marginV);
		float side = std::min(iconBounds.getWidth(), iconBounds.getHeight());

		if (customIconSize > 0.0f)
			side = customIconSize;
		else if (isCompact)
			side = std::min(side, 9.0f);

		juce::Rectangle<float> square(iconBounds.getCentreX() - side * 0.5f, iconBounds.getCentreY() - side * 0.5f,
		                              side, side);

		juce::Colour iconColour;
		if (toggled && hasCustomIconColourToggled)
			iconColour = customIconColourToggled;
		else if (!toggled && hasCustomIconColour)
			iconColour = customIconColour;
		else
			iconColour = toggled ? btn.findColour(juce::TextButton::textColourOnId)
			                     : btn.findColour(juce::TextButton::textColourOffId);

		if (!enabled)
			iconColour = iconColour.withMultipliedAlpha(0.3f);

		auto copy = drawable->createCopy();

		copy->replaceColour(juce::Colours::black, iconColour);
		copy->replaceColour(juce::Colours::white, iconColour);
		copy->replaceColour(juce::Colour(0xff000000), iconColour);
		copy->replaceColour(juce::Colour(0xffffffff), iconColour);

		if (auto *dc = dynamic_cast<juce::DrawableComposite *>(copy.get()))
		{
			for (int i = 0; i < dc->getNumChildComponents(); ++i)
				if (auto *child = dynamic_cast<juce::DrawablePath *>(dc->getChildComponent(i)))
				{
					if (child->getFill().isInvisible() || child->getFill().colour.isTransparent())
						child->setFill(juce::FillType(juce::Colours::transparentBlack));
					else
						child->setFill(iconColour);
					child->setStrokeFill(iconColour);
				}
		}
		if (auto *dp = dynamic_cast<juce::DrawablePath *>(copy.get()))
		{
			if (dp->getFill().isInvisible() || dp->getFill().colour.isTransparent())
				dp->setFill(juce::FillType(juce::Colours::transparentBlack));
			else
				dp->setFill(iconColour);
			dp->setStrokeFill(iconColour);
		}

		copy->drawWithin(g, square, juce::RectanglePlacement::centred, 1.0f);
	}

	if (labelText.isNotEmpty())
	{
		juce::Colour labelCol = toggled ? btn.findColour(juce::TextButton::textColourOnId)
		                                : btn.findColour(juce::TextButton::textColourOffId);
		g.setColour(enabled ? labelCol : labelCol.withAlpha(0.3f));

		const float fontSize =
		    hasIcon ? (isCompact ? 6.5f : 8.5f) : (isCompact ? ObsidianSizes::TEXT_INFO : ObsidianSizes::TEXT_REGULAR);

		g.setFont(juce::FontOptions(fontSize, juce::Font::bold));
		g.drawText(labelText, labelArea.toNearestInt(), juce::Justification::centred, false);
	}
}

IconButton::IconButton(const juce::String &name, const juce::String &label)
{
	setName(name);
	setButtonText({});
	labelText = label;
	setAccessible(false);
}

void IconButton::paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown)
{
	paintIconButton(g, *this, isMouseOver, isButtonDown);
}

IconButtonSimple::IconButtonSimple(const juce::String &name, const juce::String &label)
{
	setName(name);
	setButtonText({});
	labelText = label;
}

void IconButtonSimple::paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown)
{
	paintIconButton(g, *this, isMouseOver, isButtonDown);
}

IconButtonRepeat::IconButtonRepeat(const juce::String &name, const juce::String &label)
{
	setName(name);
	setButtonText({});
	labelText = label;
	setAccessible(false);
}

void IconButtonRepeat::paintButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown)
{
	paintIconButton(g, *this, isMouseOver, isButtonDown);
}

void IconButtonRepeat::mouseDown(const juce::MouseEvent &e)
{
	juce::TextButton::mouseDown(e);
	if (isEnabled())
	{
		repeatCount = 0;
		startTimer(400);
	}
}

void IconButtonRepeat::mouseUp(const juce::MouseEvent &e)
{
	stopTimer();
	juce::TextButton::mouseUp(e);
}

void IconButtonRepeat::timerCallback()
{
	++repeatCount;
	if (onClick)
		onClick();

	if (repeatCount == 1)
		startTimer(120);
	else if (repeatCount == 10)
		startTimer(60);
	else if (repeatCount == 25)
		startTimer(30);
	else if (repeatCount == 50)
		startTimer(15);
}