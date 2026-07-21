#include "SplashScreen.h"
#include "BinaryData.h"
#include "config/version.h"

SplashScreen::SplashScreen()
{
	auto imagePtr = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

	if (imagePtr.isValid())
		logoImage = imagePtr;

	lastFrameTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;

	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

SplashScreen::~SplashScreen()
{
	vBlankAttachment.reset();
}

void SplashScreen::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();
	auto contentBounds = bounds.reduced(12.0f);

	if (logoImage.isValid())
	{
		auto centre = contentBounds.getCentre();
		auto logoArea = contentBounds.withSize(250.0f, 250.0f).withCentre(centre.translated(0.0f, -40.0f));

		juce::DropShadow shadow(juce::Colours::black.withAlpha(0.5f), 12, {0, 4});
		juce::Path shadowPath;
		shadowPath.addRoundedRectangle(contentBounds, 16.0f);
		shadow.drawForPath(g, shadowPath);

		g.setColour(ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(contentBounds, 8.0f);

		g.drawRect(logoArea, 2.0f);

		g.drawImageWithin(logoImage, (int)logoArea.getX(), (int)logoArea.getY(), (int)logoArea.getWidth(),
		                  (int)logoArea.getHeight(), juce::RectanglePlacement::centred);
	}

	g.setFont(juce::FontOptions(Obsidian::notoRegular()).withHeight(Obsidian::TEXT_XS));
	g.setColour(ColourPalette::textAccent.withAlpha(Obsidian::ALPHA_08));
	g.drawText(Version::BUILD, contentBounds.removeFromBottom(40.0f), juce::Justification::centred);

	contentBounds.removeFromBottom(20.0f);

	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(Obsidian::michroma()).withHeight(Obsidian::TEXT_SPLASH));
	auto titleArea = contentBounds.removeFromBottom(60.0f);
	g.drawText("OBSIDIAN Neural - " + Version::VERSION, titleArea, juce::Justification::centred);

	auto barArea =
	    juce::Rectangle<float>(contentBounds.getCentreX() - 150.0f, titleArea.getBottom() - 5.0f, 300.0f, 3.0f);
	g.setColour(ColourPalette::backgroundDeep.brighter(0.15f));
	g.fillRoundedRectangle(barArea, 1.5f);
	g.setColour(ColourPalette::textAccent);
	g.fillRoundedRectangle(barArea.withWidth(barArea.getWidth() * progress), 1.5f);
}

void SplashScreen::handleVBlank()
{
	double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	double dt = now - lastFrameTime;
	lastFrameTime = now;

	float factor = 1.0f - std::pow(1.0f - 0.015f, (float)(dt / 0.033333));

	progress += (1.0f - progress) * factor;
	repaint();
}