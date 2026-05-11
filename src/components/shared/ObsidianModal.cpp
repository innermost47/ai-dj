#include "ObsidianModal.h"
#include "Fonts.h"

ObsidianSvgButton::ObsidianSvgButton(const juce::String &name, const juce::String &svgData, juce::Colour baseColour)
    : juce::Button(name), colour(baseColour)
{
	if (svgData.isNotEmpty())
	{
		auto xml = juce::XmlDocument::parse(svgData);
		if (xml != nullptr)
			drawable = juce::Drawable::createFromSVG(*xml);
	}
}

void ObsidianSvgButton::paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto bounds = getLocalBounds().toFloat().reduced(0.5f);
	const float corner = ObsidianSizes::CORNER;

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

	juce::ColourGradient bgGradient(bgColour.brighter(0.08f), bounds.getX(), bounds.getY(), bgColour.darker(0.08f),
	                                bounds.getX(), bounds.getBottom(), false);
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
		auto iconBounds = contentBounds.removeFromLeft(iconSize).withSizeKeepingCentre(iconSize, iconSize);
		drawable->drawWithin(g, iconBounds, juce::RectanglePlacement::centred, 1.0f);
		contentBounds.removeFromLeft(8.0f);
	}

	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(ObsidianFonts::NOTO_BOLD).withHeight(13.0f));
	g.drawText(getButtonText(), contentBounds, juce::Justification::centredLeft, true);
}

ObsidianModalWindow::ObsidianModalWindow(const juce::String &titleText, int width, int height)
    : title(titleText), targetWidth(width), targetHeight(height)
{
}

void ObsidianModalWindow::setContent(std::unique_ptr<juce::Component> newContent)
{
	content = std::move(newContent);
	addAndMakeVisible(content.get());
	resized();
}

void ObsidianModalWindow::addButton(const juce::String &text, const juce::String &svgData, juce::Colour colour,
                                    std::function<void()> onClick)
{
	auto *btn = buttons.add(new ObsidianSvgButton(text, svgData, colour));
	addAndMakeVisible(btn);
	btn->onClick = onClick;
	resized();
}

void ObsidianModalWindow::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	const float corner = 10.0f;
	const float titleHeight = 56.0f;

	juce::DropShadow shadow(juce::Colours::black.withAlpha(0.7f), 24, juce::Point<int>(0, 8));
	shadow.drawForRectangle(g, bounds.toNearestInt());

	juce::ColourGradient bgGradient(ColourPalette::backgroundDeep.brighter(0.03f), bounds.getX(), bounds.getY(),
	                                ColourPalette::backgroundDeep.darker(0.05f), bounds.getX(), bounds.getBottom(),
	                                false);
	g.setGradientFill(bgGradient);
	g.fillRoundedRectangle(bounds, corner);

	juce::Path titleBarPath;
	titleBarPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight, corner, corner, true,
	                                 true, false, false);

	juce::ColourGradient titleGradient(ColourPalette::buttonPrimary.withAlpha(0.18f), bounds.getX(), bounds.getY(),
	                                   ColourPalette::buttonPrimary.withAlpha(0.05f), bounds.getX(),
	                                   bounds.getY() + titleHeight, false);
	g.setGradientFill(titleGradient);
	g.fillPath(titleBarPath);

	auto titleBounds =
	    juce::Rectangle<float>(bounds.getX() + 30.0f, bounds.getY(), bounds.getWidth() - 60.0f, titleHeight);
	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(17.0f));
	g.drawText(title, titleBounds, juce::Justification::centredLeft, true);

	float lineY = bounds.getY() + titleHeight;
	juce::ColourGradient lineGradient(ColourPalette::lightGrey.withAlpha(0.0f), bounds.getX(), lineY,
	                                  ColourPalette::lightGrey.withAlpha(0.0f), bounds.getRight(), lineY, false);
	lineGradient.addColour(0.5, ColourPalette::lightGrey.withAlpha(0.6f));
	g.setGradientFill(lineGradient);
	g.fillRect(bounds.getX(), lineY, bounds.getWidth(), 1.0f);

	g.setColour(ColourPalette::buttonPrimary.withAlpha(0.4f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);

	g.setColour(juce::Colours::white.withAlpha(0.03f));
	auto topHighlight = juce::Rectangle<float>(bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight * 0.5f);
	juce::Path highlightPath;
	highlightPath.addRoundedRectangle(topHighlight.getX(), topHighlight.getY(), topHighlight.getWidth(),
	                                  topHighlight.getHeight(), corner, corner, true, true, false, false);
	g.fillPath(highlightPath);
}

void ObsidianModalWindow::resized()
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

	for (auto *btn : buttons)
	{
		if (btn == nullptr)
			continue;

		fb.items.add(juce::FlexItem(*btn)
		                 .withWidth(static_cast<float>(btnWidth))
		                 .withHeight(static_cast<float>(btnHeight))
		                 .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, static_cast<float>(spacing))));
	}
	fb.performLayout(buttonArea);
}

ObsidianModalOverlay::ObsidianModalOverlay(std::unique_ptr<ObsidianModalWindow> modal) : modalWindow(std::move(modal))
{
	addAndMakeVisible(modalWindow.get());
	setInterceptsMouseClicks(true, true);
	setAlpha(0.0f);
}

ObsidianModalOverlay::~ObsidianModalOverlay()
{
	auto &animator = juce::Desktop::getInstance().getAnimator();
	animator.cancelAnimation(this, false);
	if (modalWindow != nullptr)
		animator.cancelAnimation(modalWindow.get(), false);
}

void ObsidianModalOverlay::startFadeIn()
{
	juce::Desktop::getInstance().getAnimator().fadeIn(this, 180);
}

void ObsidianModalOverlay::paint(juce::Graphics &g)
{
	juce::ColourGradient backdrop(ColourPalette::backgroundDeep.withAlpha(0.75f), (float)getWidth() * 0.5f,
	                              (float)getHeight() * 0.5f, ColourPalette::backgroundDeep.withAlpha(0.92f), 0.0f, 0.0f,
	                              true);
	g.setGradientFill(backdrop);
	g.fillAll();
}

void ObsidianModalOverlay::resized()
{
	if (modalWindow != nullptr)
	{
		int width = juce::jmin(modalWindow->targetWidth, getWidth() - 40);
		int height = juce::jmin(modalWindow->targetHeight, getHeight() - 40);
		modalWindow->setBounds(getLocalBounds().withSizeKeepingCentre(width, height));
	}
}

void ObsidianModalOverlay::mouseDown(const juce::MouseEvent &e)
{
	if (modalWindow != nullptr && !modalWindow->getBounds().contains(e.getPosition()))
	{
		juce::WeakReference<juce::Component> safeModal(modalWindow.get());
		auto &animator = juce::Desktop::getInstance().getAnimator();
		auto target = modalWindow->getBounds();

		animator.animateComponent(modalWindow.get(), target.translated(6, 0), 1.0f, 50, false, 1.0, 0.0);
		if (safeModal != nullptr)
			animator.animateComponent(safeModal, target.translated(-6, 0), 1.0f, 50, false, 1.0, 0.0);
		if (safeModal != nullptr)
			animator.animateComponent(safeModal, target, 1.0f, 50, false, 1.0, 0.0);
	}
}

void ObsidianModalOverlay::close()
{
	if (closing)
		return;
	closing = true;

	juce::WeakReference<juce::Component> safeThis(this);
	juce::Desktop::getInstance().getAnimator().fadeOut(this, 150);

	juce::MessageManager::callAsync(
	    [safeThis]()
	    {
		    if (safeThis == nullptr)
			    return;

		    auto *self = static_cast<ObsidianModalOverlay *>(safeThis.get());
		    if (auto *host = self->findParentComponentOfClass<ModalHost>())
			    host->removeModal(self);
	    });
}
