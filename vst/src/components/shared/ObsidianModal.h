#pragma once
#include <JuceHeader.h>
#include "style/ColourPalette.h"

class ObsidianSvgButton : public juce::Button
{
public:
	ObsidianSvgButton(const juce::String& name, const juce::String& svgData, juce::Colour baseColour);
	void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
	juce::Colour colour;
	std::unique_ptr<juce::Drawable> drawable;
};

class ObsidianModalWindow : public juce::Component
{
public:
	ObsidianModalWindow(const juce::String& titleText, int width = 600, int height = 400);
	int targetWidth;
	int targetHeight;

	void setContent(std::unique_ptr<juce::Component> newContent);
	void addButton(const juce::String& text, const juce::String& svgData,
		juce::Colour colour, std::function<void()> onClick);
	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	juce::String title;
	std::unique_ptr<juce::Component> content;
	juce::OwnedArray<ObsidianSvgButton> buttons;
};

class ObsidianModalOverlay;

class ModalHost
{
public:
	virtual ~ModalHost() = default;
	virtual void addModal(std::unique_ptr<ObsidianModalOverlay> overlay) = 0;
	virtual void removeModal(ObsidianModalOverlay* overlay) = 0;
};

class ObsidianModalOverlay : public juce::Component
{
public:
	ObsidianModalOverlay(std::unique_ptr<ObsidianModalWindow> modal);
	~ObsidianModalOverlay() override;
	void startFadeIn();
	void paint(juce::Graphics& g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& e) override;
	void close();

	std::unique_ptr<ObsidianModalWindow> modalWindow;

private:
	bool closing = false;
	JUCE_DECLARE_WEAK_REFERENCEABLE(ObsidianModalOverlay)
};