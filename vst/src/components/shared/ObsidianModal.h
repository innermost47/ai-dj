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
		auto bounds = getLocalBounds().toFloat().reduced(0.5f);
		const float corner = 5.0f;

		juce::Colour bgColour = colour;
		if (shouldDrawButtonAsDown)
			bgColour = colour.darker(0.2f);
		else if (shouldDrawButtonAsHighlighted)
			bgColour = colour.brighter(0.12f);

		if (!shouldDrawButtonAsDown)
		{
			g.setColour(juce::Colours::black.withAlpha(0.35f));
			g.fillRoundedRectangle(bounds.translated(0, 1.5f), corner);
		}

		juce::ColourGradient bgGradient(
			bgColour.brighter(0.08f), bounds.getX(), bounds.getY(),
			bgColour.darker(0.08f), bounds.getX(), bounds.getBottom(),
			false);
		g.setGradientFill(bgGradient);
		g.fillRoundedRectangle(bounds, corner);

		if (!shouldDrawButtonAsDown)
		{
			g.setColour(juce::Colours::white.withAlpha(0.08f));
			auto topHighlight = bounds.withHeight(bounds.getHeight() * 0.45f);
			g.fillRoundedRectangle(topHighlight, corner);
		}

		g.setColour(bgColour.brighter(0.25f).withAlpha(0.5f));
		g.drawRoundedRectangle(bounds, corner, 0.8f);

		auto contentBounds = bounds.reduced(10.0f, 6.0f);

		if (drawable != nullptr)
		{
			const float iconSize = 14.0f;
			auto iconBounds = contentBounds.removeFromLeft(iconSize)
				.withSizeKeepingCentre(iconSize, iconSize);
			drawable->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
			contentBounds.removeFromLeft(8.0f);
		}

		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::bold));
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

	void addButton(const juce::String& text, const juce::String& svgData,
		juce::Colour colour, std::function<void()> onClick)
	{
		auto* btn = buttons.add(new ObsidianSvgButton(text, svgData, colour));
		addAndMakeVisible(btn);
		btn->onClick = onClick;
		resized();
	}

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();
		const float corner = 10.0f;
		const float titleHeight = 56.0f;

		juce::DropShadow shadow(juce::Colours::black.withAlpha(0.7f), 24,
			juce::Point<int>(0, 8));
		shadow.drawForRectangle(g, bounds.toNearestInt());

		juce::ColourGradient bgGradient(
			ColourPalette::backgroundDeep.brighter(0.03f),
			bounds.getX(), bounds.getY(),
			ColourPalette::backgroundDeep.darker(0.05f),
			bounds.getX(), bounds.getBottom(),
			false);
		g.setGradientFill(bgGradient);
		g.fillRoundedRectangle(bounds, corner);

		juce::Path titleBarPath;
		titleBarPath.addRoundedRectangle(
			bounds.getX(), bounds.getY(),
			bounds.getWidth(), titleHeight,
			corner, corner, true, true, false, false);

		juce::ColourGradient titleGradient(
			ColourPalette::buttonPrimary.withAlpha(0.18f),
			bounds.getX(), bounds.getY(),
			ColourPalette::buttonPrimary.withAlpha(0.05f),
			bounds.getX(), bounds.getY() + titleHeight,
			false);
		g.setGradientFill(titleGradient);
		g.fillPath(titleBarPath);

		auto titleBounds = juce::Rectangle<float>(
			bounds.getX() + 30.0f, bounds.getY(),
			bounds.getWidth() - 60.0f, titleHeight);
		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions("Courier New", 17.0f, juce::Font::bold));
		g.drawText(title, titleBounds, juce::Justification::centredLeft, true);

		float lineY = bounds.getY() + titleHeight;
		juce::ColourGradient lineGradient(
			ColourPalette::trackSelected.withAlpha(0.0f),
			bounds.getX(), lineY,
			ColourPalette::trackSelected.withAlpha(0.0f),
			bounds.getRight(), lineY,
			false);
		lineGradient.addColour(0.5, ColourPalette::trackSelected.withAlpha(0.6f));
		g.setGradientFill(lineGradient);
		g.fillRect(bounds.getX(), lineY, bounds.getWidth(), 1.0f);

		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);

		g.setColour(juce::Colours::white.withAlpha(0.03f));
		auto topHighlight = juce::Rectangle<float>(
			bounds.getX(), bounds.getY(),
			bounds.getWidth(), titleHeight * 0.5f);
		juce::Path highlightPath;
		highlightPath.addRoundedRectangle(
			topHighlight.getX(), topHighlight.getY(),
			topHighlight.getWidth(), topHighlight.getHeight(),
			corner, corner, true, true, false, false);
		g.fillPath(highlightPath);
	}

	void resized() override
	{
		const int titleHeight = 56;
		const int padding = 24;
		const int buttonAreaHeight = 48;
		const int buttonAreaPadding = 16;

		auto bounds = getLocalBounds();
		bounds.removeFromTop(titleHeight);
		bounds = bounds.reduced(padding, padding - 4);

		auto buttonArea = bounds.removeFromBottom(buttonAreaHeight);
		bounds.removeFromBottom(buttonAreaPadding);

		if (content != nullptr)
			content->setBounds(bounds);

		const int btnWidth = 170;
		const int btnHeight = 38;
		const int spacing = 10;

		juce::FlexBox fb;
		fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
		fb.alignItems = juce::FlexBox::AlignItems::center;

		for (auto* btn : buttons)
		{
			if (btn == nullptr) continue;

			fb.items.add(juce::FlexItem(*btn)
				.withWidth(static_cast<float>(btnWidth))
				.withHeight(static_cast<float>(btnHeight))
				.withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f,
					static_cast<float>(spacing))));
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
	ObsidianModalOverlay(juce::Component* parentToOverlay,
		std::unique_ptr<ObsidianModalWindow> modal)
		: parent(parentToOverlay), modalWindow(std::move(modal))
	{
		addAndMakeVisible(modalWindow.get());
		parent->addAndMakeVisible(this);
		toFront(false);
		setBounds(parent->getLocalBounds());
		setInterceptsMouseClicks(true, true);

		setAlpha(0.0f);
		juce::Desktop::getInstance().getAnimator().fadeIn(this, 180);
	}

	~ObsidianModalOverlay()
	{
		parent->removeChildComponent(this);
	}

	void paint(juce::Graphics& g) override
	{
		juce::ColourGradient backdrop(
			ColourPalette::backgroundDeep.withAlpha(0.75f),
			(float)getWidth() * 0.5f, (float)getHeight() * 0.5f,
			ColourPalette::backgroundDeep.withAlpha(0.92f),
			0.0f, 0.0f,
			true);
		g.setGradientFill(backdrop);
		g.fillAll();
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

	void mouseDown(const juce::MouseEvent& e) override
	{
		if (modalWindow != nullptr && !modalWindow->getBounds().contains(e.getPosition()))
		{
			auto& animator = juce::Desktop::getInstance().getAnimator();
			auto target = modalWindow->getBounds();
			animator.animateComponent(modalWindow.get(),
				target.translated(6, 0), 1.0f, 50, false, 1.0, 0.0);
			animator.animateComponent(modalWindow.get(),
				target.translated(-6, 0), 1.0f, 50, false, 1.0, 0.0);
			animator.animateComponent(modalWindow.get(),
				target, 1.0f, 50, false, 1.0, 0.0);
		}
	}

	void close()
	{
		auto& animator = juce::Desktop::getInstance().getAnimator();
		animator.fadeOut(this, 150);
		juce::MessageManager::callAsync([this]()
			{ delete this; });
	}

	std::unique_ptr<ObsidianModalWindow> modalWindow;

private:
	juce::Component* parent;
};