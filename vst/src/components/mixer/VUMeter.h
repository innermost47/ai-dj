#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include "style/ColourPalette.h"

class VuMeter : public juce::Component
{
public:
	VuMeter() = default;
	~VuMeter() override = default;

	float getLevelLeft() const { return currentAudioLevelLeft; }
	float getLevelRight() const { return currentAudioLevelRight; }

	void updateMeter(const juce::AudioBuffer<float>* buffer, double readPos, float volume, bool isPlaying)
	{
		hasSource = (buffer != nullptr);

		if (!hasSource || !isPlaying)
		{
			currentAudioLevelLeft *= 0.88f;
			currentAudioLevelRight *= 0.88f;

			if (peakHoldTimerLeft > 0)
			{
				peakHoldTimerLeft--;
				if (peakHoldTimerLeft == 0)
					peakHoldLeft *= 0.9f;
			}

			if (peakHoldTimerRight > 0)
			{
				peakHoldTimerRight--;
				if (peakHoldTimerRight == 0)
					peakHoldRight *= 0.9f;
			}

			repaint();
			return;
		}

		float instantNormLeft = 0.0f;
		float instantNormRight = 0.0f;

		int sampleIndex = static_cast<int>(readPos);
		int numSamples = buffer->getNumSamples();

		if (sampleIndex >= 0 && sampleIndex < numSamples)
		{
			int numChannels = buffer->getNumChannels();
			int windowSize = 8;
			int endSample = std::min(sampleIndex + windowSize, numSamples);

			float peakLeft = 0.0f;
			float peakRight = 0.0f;

			for (int i = sampleIndex; i < endSample; ++i)
			{
				if (numChannels >= 1)
					peakLeft = std::max(peakLeft, std::abs(buffer->getSample(0, i)));

				if (numChannels >= 2)
					peakRight = std::max(peakRight, std::abs(buffer->getSample(1, i)));
				else
					peakRight = peakLeft;
			}

			peakLeft *= volume;
			peakRight *= volume;

			auto linearToDb = [](float linear) -> float {
				if (linear <= 0.00001f)
					return -100.0f;
				return 20.0f * std::log10(linear);
				};

			auto dbToNormalized = [](float db) -> float {
				return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
				};

			instantNormLeft = dbToNormalized(linearToDb(peakLeft));
			instantNormRight = dbToNormalized(linearToDb(peakRight));
		}

		if (instantNormLeft > currentAudioLevelLeft)
			currentAudioLevelLeft = instantNormLeft;
		else
			currentAudioLevelLeft = currentAudioLevelLeft * 0.92f + instantNormLeft * 0.08f;

		if (currentAudioLevelLeft > peakHoldLeft)
		{
			peakHoldLeft = currentAudioLevelLeft;
			peakHoldTimerLeft = 45;
		}

		if (instantNormRight > currentAudioLevelRight)
			currentAudioLevelRight = instantNormRight;
		else
			currentAudioLevelRight = currentAudioLevelRight * 0.92f + instantNormRight * 0.08f;

		if (currentAudioLevelRight > peakHoldRight)
		{
			peakHoldRight = currentAudioLevelRight;
			peakHoldTimerRight = 45;
		}

		repaint();
	}

	void paint(juce::Graphics& g) override
	{
		float meterWidth = 5.0f;
		float meterSpacing = 2.0f;

		float vuStartY = 0.0f;
		float vuHeight = static_cast<float>(getHeight());
		float startX = 0.0f;

		auto vuAreaLeft = juce::Rectangle<float>(startX, vuStartY, meterWidth, vuHeight);
		auto vuAreaRight = juce::Rectangle<float>(startX + meterWidth + meterSpacing, vuStartY, meterWidth, vuHeight);

		g.setColour(ColourPalette::backgroundDeep);
		g.fillRoundedRectangle(vuAreaLeft, 2.0f);
		g.fillRoundedRectangle(vuAreaRight, 2.0f);

		g.setColour(ColourPalette::backgroundLight);
		g.drawRoundedRectangle(vuAreaLeft, 2.0f, 0.5f);
		g.drawRoundedRectangle(vuAreaRight, 2.0f, 0.5f);

		if (!hasSource)
			return;

		int numSegments = 20;
		float segmentHeight = (vuAreaLeft.getHeight() - 10) / numSegments;

		for (int i = 0; i < numSegments; ++i)
		{
			fillMeterSegment(g, vuAreaLeft, i, segmentHeight, numSegments, currentAudioLevelLeft);
		}

		if (peakHoldLeft > 0.0f)
		{
			drawPeakSegments(numSegments, vuAreaLeft, segmentHeight, g, peakHoldLeft);
		}

		drawClipRect(vuAreaLeft, g, currentAudioLevelLeft);

		for (int i = 0; i < numSegments; ++i)
		{
			fillMeterSegment(g, vuAreaRight, i, segmentHeight, numSegments, currentAudioLevelRight);
		}

		if (peakHoldRight > 0.0f)
		{
			drawPeakSegments(numSegments, vuAreaRight, segmentHeight, g, peakHoldRight);
		}

		drawClipRect(vuAreaRight, g, currentAudioLevelRight);
	}

	void drawClipRect(juce::Rectangle<float>& vuArea, juce::Graphics& g, float currentAudioLevel)
	{
		auto clipRect = juce::Rectangle<float>(vuArea.getX() + 1, vuArea.getY() + 2, vuArea.getWidth() - 2, 4);

		if (currentAudioLevel >= 0.95f)
			g.setColour(ColourPalette::vuRed);
		else
			g.setColour(ColourPalette::vuRed.withAlpha(0.1f));

		g.fillRoundedRectangle(clipRect, 1.0f);
	}

	void drawPeakSegments(int numSegments, juce::Rectangle<float>& vuArea, float segmentHeight, juce::Graphics& g, float peakValue)
	{
		int peakSegment = (int)(peakValue * numSegments);
		if (peakSegment < numSegments)
		{
			float peakY = vuArea.getBottom() - 2 - (peakSegment + 1) * segmentHeight;
			juce::Rectangle<float> peakRect(vuArea.getX() + 1, peakY, vuArea.getWidth() - 2, 2);
			g.setColour(ColourPalette::vuPeak);
			g.fillRect(peakRect);
		}
	}

	void updateFromRawLevels(float rawLeft, float rawRight)
	{
		hasSource = true;

		if (rawLeft > currentAudioLevelLeft)
			currentAudioLevelLeft = rawLeft;
		else
			currentAudioLevelLeft = currentAudioLevelLeft * 0.92f + rawLeft * 0.08f;

		if (currentAudioLevelLeft > peakHoldLeft)
		{
			peakHoldLeft = currentAudioLevelLeft;
			peakHoldTimerLeft = 45;
		}

		if (rawRight > currentAudioLevelRight)
			currentAudioLevelRight = rawRight;
		else
			currentAudioLevelRight = currentAudioLevelRight * 0.92f + rawRight * 0.08f;

		if (currentAudioLevelRight > peakHoldRight)
		{
			peakHoldRight = currentAudioLevelRight;
			peakHoldTimerRight = 45;
		}

		juce::MessageManager::callAsync([this]() {
			repaint();
			});
	}


private:
	float currentAudioLevelLeft = 0.0f;
	float currentAudioLevelRight = 0.0f;
	float peakHoldLeft = 0.0f;
	float peakHoldRight = 0.0f;
	int peakHoldTimerLeft = 0;
	int peakHoldTimerRight = 0;
	bool hasSource = false;

	void fillMeterSegment(juce::Graphics& g, juce::Rectangle<float>& vuArea,
		int i, float segmentHeight, int numSegments,
		float currentLevel)
	{
		float segmentY = vuArea.getBottom() - 2 - (i + 1) * segmentHeight;
		float segmentLevel = (float)i / numSegments;

		juce::Rectangle<float> segmentRect(
			vuArea.getX() + 1, segmentY, vuArea.getWidth() - 2, segmentHeight - 1);

		juce::Colour segmentColour;
		if (segmentLevel < 0.67f)
			segmentColour = ColourPalette::vuGreen;
		else if (segmentLevel < 0.90f)
			segmentColour = ColourPalette::vuOrange;
		else
			segmentColour = ColourPalette::vuRed;

		if (currentLevel >= segmentLevel)
		{
			g.setColour(segmentColour);
			g.fillRoundedRectangle(segmentRect, 1.0f);
		}
		else
		{
			g.setColour(segmentColour.withAlpha(0.1f));
			g.fillRoundedRectangle(segmentRect, 1.0f);
		}
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeter)
};