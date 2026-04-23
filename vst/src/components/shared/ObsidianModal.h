#pragma once
#include <JuceHeader.h>
#include "style/ColourPalette.h"

class ObsidianSvgButton : public juce::Button
{
public:
	ObsidianSvgButton(const juce::String& name, const juce::String& svgData, juce::Colour baseColour)
		: juce::Button(name), colour(baseColour)
	{
		if (svgData.isNotEmpty())
		{
			auto xml = juce::XmlDocument::parse(svgData);
			if (xml != nullptr)
				drawable = juce::Drawable::createFromSVG(*xml);
		}
	}

	void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
	{
		auto bounds = getLocalBounds().toFloat();

		juce::Colour bgColour = shouldDrawButtonAsDown ? colour.darker(0.2f) : (shouldDrawButtonAsHighlighted ? colour.brighter(0.1f) : colour);

		g.setColour(bgColour);
		g.fillRoundedRectangle(bounds, 4.0f);

		auto contentBounds = bounds.reduced(8.0f);
		float iconSize = 0.0f;

		if (drawable != nullptr)
		{
			iconSize = 16.0f;
			auto iconBounds = contentBounds.removeFromLeft(iconSize).withSizeKeepingCentre(iconSize, iconSize);
			drawable->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
			contentBounds.removeFromLeft(8.0f);
		}

		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::bold));
		g.drawText(getButtonText(), contentBounds, juce::Justification::centredLeft, true);
	}

private:
	juce::Colour colour;
	std::unique_ptr<juce::Drawable> drawable;
};

class ObsidianModalWindow : public juce::Component
{
public:
	ObsidianModalWindow(const juce::String& titleText, int width = 600, int height = 400)
		: title(titleText), targetWidth(width), targetHeight(height)
	{
	}

	int targetWidth;
	int targetHeight;

	void setContent(std::unique_ptr<juce::Component> newContent)
	{
		content = std::move(newContent);
		addAndMakeVisible(content.get());
		resized();
	}

	void addButton(const juce::String& text, const juce::String& svgData, juce::Colour colour, std::function<void()> onClick)
	{
		auto* btn = buttons.add(new ObsidianSvgButton(text, svgData, colour));
		addAndMakeVisible(btn);
		btn->onClick = onClick;
		resized();
	}

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();

		juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f), 12, juce::Point<int>(0, 6));
		shadow.drawForRectangle(g, bounds.toNearestInt());

		g.setColour(ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(bounds, 8.0f);

		g.setColour(ColourPalette::backgroundLight);
		g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

		auto titleBounds = bounds.removeFromTop(50.0f).reduced(20.0f, 0.0f);
		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions("Courier New", 18.0f, juce::Font::bold));
		g.drawText(title, titleBounds, juce::Justification::centredLeft, true);

		g.setColour(ColourPalette::backgroundLight);
		g.drawLine(20.0f, 50.0f, bounds.getWidth() - 20.0f, 50.0f, 1.0f);
	}

	void resized() override
	{
		auto bounds = getLocalBounds().reduced(20);
		bounds.removeFromTop(40);

		auto buttonArea = bounds.removeFromBottom(40);

		if (content != nullptr)
			content->setBounds(bounds.withTrimmedBottom(20));

		int btnWidth = 180;
		int spacing = 10;
		juce::FlexBox fb;
		fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;

		for (auto* btn : buttons)
		{
			if (btn == nullptr) continue;

			fb.items.add(juce::FlexItem(*btn)
				.withWidth(static_cast<float>(btnWidth))
				.withHeight(36.0f)
				.withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, static_cast<float>(spacing))));
		}
		fb.performLayout(buttonArea);
	}

private:
	juce::String title;
	std::unique_ptr<juce::Component> content;
	juce::OwnedArray<ObsidianSvgButton> buttons;
};

class ObsidianModalOverlay : public juce::Component
{
public:
	ObsidianModalOverlay(juce::Component* parentToOverlay, std::unique_ptr<ObsidianModalWindow> modal)
		: parent(parentToOverlay), modalWindow(std::move(modal))
	{
		addAndMakeVisible(modalWindow.get());
		parent->addAndMakeVisible(this);
		toFront(false);
		setBounds(parent->getLocalBounds());
		setInterceptsMouseClicks(true, true);
	}

	~ObsidianModalOverlay()
	{
		parent->removeChildComponent(this);
	}

	void paint(juce::Graphics& g) override
	{
		g.fillAll(ColourPalette::backgroundDeep.withAlpha(0.85f));
	}

	void resized() override
	{
		if (modalWindow != nullptr)
		{
			int width = juce::jmin(modalWindow->targetWidth, getWidth() - 40);
			int height = juce::jmin(modalWindow->targetHeight, getHeight() - 40);

			modalWindow->setBounds(getLocalBounds().withSizeKeepingCentre(width, height));
		}
	}

	void close()
	{
		juce::MessageManager::callAsync([this]()
			{ delete this; });
	}

	std::unique_ptr<ObsidianModalWindow> modalWindow;

private:
	juce::Component* parent;
};