#include "SplashScreen.h"
#include "BinaryData.h"
#include "config/version.h"

SplashScreen::SplashScreen()
{
	auto imagePtr = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

	if (imagePtr.isValid())
		logoImage = imagePtr;
}

void SplashScreen::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	if (logoImage.isValid())
	{
		auto centre = bounds.getCentre();
		auto logoArea = bounds.withSize(250.0f, 250.0f).withCentre(centre.translated(0.0f, -20.0f));

		g.fillAll(ColourPalette::backgroundDeep);
		g.setColour(ColourPalette::backgroundDeep);
		g.drawRect(logoArea, 2.0f);

		g.drawImageWithin(logoImage, (int)logoArea.getX(), (int)logoArea.getY(), (int)logoArea.getWidth(),
		                  (int)logoArea.getHeight(), juce::RectanglePlacement::centred);
	}
	g.setFont(juce::FontOptions(ObsidianFonts::NOTO_REGULAR).withHeight(ObsidianSizes::TEXT_XS));
	g.setColour(ColourPalette::textAccent);
	g.drawText(Version::BUILD, bounds.removeFromBottom(40.0f), juce::Justification::centred);

	g.setColour(ColourPalette::textPrimary);
	g.setFont(juce::FontOptions(ObsidianFonts::MICHROMA).withHeight(ObsidianSizes::TEXT_SPLASH));
	g.drawText("OBSIDIAN Neural - " + Version::VERSION, bounds.removeFromBottom(70.0f), juce::Justification::centred);
}