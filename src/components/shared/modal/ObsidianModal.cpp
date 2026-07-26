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
	const float corner = Obsidian::CORNER;

	juce::Colour bgColour = colour;
	if (shouldDrawButtonAsDown)
		bgColour = colour.darker(0.2f);
	else if (shouldDrawButtonAsHighlighted)
		bgColour = colour.brighter(0.12f);

	g.setColour(bgColour);
	g.fillRoundedRectangle(bounds, corner);

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
	g.setFont(juce::FontOptions(Obsidian::notoBold()).withHeight(Obsidian::TEXT_REGULAR));
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
	const float corner = Obsidian::CORNER;
	const float titleHeight = 38.0f;

	g.setColour(ColourPalette::backgroundDark);
	g.fillRoundedRectangle(bounds, corner);

	juce::Path titleBarPath;
	titleBarPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight, corner, corner, true,
	                                 true, false, false);

	g.setColour(ColourPalette::modalHeader);
	g.fillPath(titleBarPath);

	auto titleBounds =
	    juce::Rectangle<float>(bounds.getX() + 30.0f, bounds.getY(), bounds.getWidth() - 60.0f, titleHeight);

	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_XL));
	g.drawText(title, titleBounds, juce::Justification::centredLeft, true);

	float lineY = bounds.getY() + titleHeight;
	g.setColour(ColourPalette::modalHeader.withAlpha(Obsidian::ALPHA_04));
	g.fillRect(bounds.getX(), lineY, bounds.getWidth(), 1.0f);
}

void ObsidianModalWindow::resized()
{
	const int titleHeight = 32;
	const int padding = 12;
	const int buttonAreaHeight = 32;
	const int buttonAreaPadding = 18;

	auto bounds = getLocalBounds();
	bounds.removeFromTop(titleHeight);
	bounds = bounds.reduced(padding, padding - 4);

	auto buttonArea = bounds.removeFromBottom(buttonAreaHeight);
	bounds.removeFromBottom(buttonAreaPadding);

	if (content != nullptr)
		content->setBounds(bounds);

	const int btnWidth = 170;
	const int btnHeight = 32;
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
}

ObsidianModalOverlay::~ObsidianModalOverlay()
{
	auto &animator = juce::Desktop::getInstance().getAnimator();
	animator.cancelAnimation(this, false);
	if (modalWindow != nullptr)
		animator.cancelAnimation(modalWindow.get(), false);
	modalWindow.reset();
}

void ObsidianModalOverlay::startFadeIn()
{
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
	if (modalWindow == nullptr || modalWindow->getBounds().contains(e.getPosition()))
		return;
	if (shaking)
		return;

	shaking = true;
	const auto target = modalWindow->getBounds();
	juce::Component::SafePointer<ObsidianModalOverlay> safeThis(this);
	juce::Component::SafePointer<juce::Component> safeModal(modalWindow.get());
	auto &animator = juce::Desktop::getInstance().getAnimator();

	animator.animateComponent(safeModal, target.translated(6, 0), 1.0f, 50, false, 1.0, 0.0);

	juce::Timer::callAfterDelay(
	    55,
	    [safeThis, safeModal, target]()
	    {
		    if (safeModal == nullptr)
			    return;
		    juce::Desktop::getInstance().getAnimator().animateComponent(safeModal, target.translated(-6, 0), 1.0f, 50,
		                                                                false, 1.0, 0.0);

		    juce::Timer::callAfterDelay(55,
		                                [safeThis, safeModal, target]()
		                                {
			                                if (safeModal != nullptr)
				                                juce::Desktop::getInstance().getAnimator().animateComponent(
				                                    safeModal, target, 1.0f, 60, false, 1.0, 0.0);
			                                if (safeThis != nullptr)
				                                safeThis->shaking = false;
		                                });
	    });
}

void ObsidianModalOverlay::close()
{
	if (closing)
		return;
	closing = true;

	juce::Component::SafePointer<ObsidianModalOverlay> safeThis(this);
	juce::MessageManager::callAsync(
	    [safeThis]()
	    {
		    auto *self = safeThis.getComponent();
		    if (self == nullptr)
			    return;

		    if (auto *host = self->findParentComponentOfClass<ModalHost>())
			    host->removeModal(self);
		    else if (auto *p = self->getParentComponent())
			    p->removeChildComponent(self);
	    });
}