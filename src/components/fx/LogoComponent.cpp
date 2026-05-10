#include "LogoComponent.h"
#include "BinaryData.h"

LogoComponent::LogoComponent()
{
	auto imagePtr = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

	if (imagePtr.isValid())
		logoImage = imagePtr;
}

void LogoComponent::paint(juce::Graphics &g)
{
	if (logoImage.isValid())
	{
		auto bounds = getLocalBounds().toFloat().reduced(ObsidianSizes::PADDING);

		auto imageArea = bounds.removeFromTop(bounds.getHeight() * 0.55f);

		g.drawImageWithin(logoImage, (int)imageArea.getX(), (int)imageArea.getY(), (int)imageArea.getWidth(),
		                  (int)imageArea.getHeight(),
		                  juce::RectanglePlacement::yTop | juce::RectanglePlacement::onlyReduceInSize, false);

		float valSize = 14.0f;
		g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), valSize, juce::Font::bold));

		g.setColour(ColourPalette::textPrimary);
		g.drawText("OBSIDIAN Neural", bounds, juce::Justification::centredTop);

		bounds.removeFromTop(16.0f);

		g.setFont(juce::FontOptions(juce::Font::getSystemUIFontName(), valSize, juce::Font::italic));
		g.setColour(ColourPalette::textAccent);
		g.drawText("Sound Engine", bounds, juce::Justification::centredTop);
	}
	else
	{
		g.setColour(juce::Colours::red);
		g.drawText("Logo Missing", getLocalBounds(), juce::Justification::centred);
	}
}
