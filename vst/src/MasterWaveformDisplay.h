#pragma once
#include <JuceHeader.h>
#include "ColourPalette.h"

class MasterWaveformDisplay : public juce::Component, public juce::Timer
{
public:
	MasterWaveformDisplay()
	{
		writeBuffer.resize(2048, 0.0f);
		readBuffer.resize(2048, 0.0f);
		startTimerHz(30);
	}

	~MasterWaveformDisplay() override { stopTimer(); }

	void pushSamples(const float* left, const float* right, int numSamples)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			float mono = (left[i] + (right ? right[i] : left[i])) * 0.5f;
			writeBuffer[writePos % writeBuffer.size()] = mono;
			++writePos;
		}
	}

	void timerCallback() override
	{
		size_t pos = writePos.load();
		size_t sz = readBuffer.size();
		for (size_t i = 0; i < sz; ++i)
			readBuffer[i] = writeBuffer[(pos + i) % sz];
		repaint();
	}

	void paint(juce::Graphics& g) override
	{
		auto bounds = getLocalBounds().toFloat();
		g.setColour(ColourPalette::backgroundDark);
		g.fillRoundedRectangle(bounds, 4.0f);
		g.setColour(ColourPalette::trackSelected.withAlpha(0.4f));
		g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 0.8f);

		auto inner = bounds.reduced(4.0f, 3.0f);
		int w = (int)inner.getWidth();
		if (w <= 0) return;

		float cy = inner.getCentreY();
		float hH = inner.getHeight() * 0.45f;
		float ppx = (float)readBuffer.size() / (float)w;

		juce::Path top, bot;
		bool started = false;
		for (int i = 0; i < w; ++i)
		{
			float x = inner.getX() + i;
			int idx = (int)(i * ppx) % (int)readBuffer.size();
			float h = std::abs(readBuffer[idx]) * hH;
			if (!started)
			{
				top.startNewSubPath(x, cy);
				bot.startNewSubPath(x, cy);
				started = true;
			}
			top.lineTo(x, cy - h);
			bot.lineTo(x, cy + h);
		}

		g.setColour(ColourPalette::buttonPrimary.withAlpha(0.8f));
		g.strokePath(top, juce::PathStrokeType(1.0f));
		g.strokePath(bot, juce::PathStrokeType(1.0f));

		g.setColour(ColourPalette::backgroundLight.withAlpha(0.2f));
		g.drawLine(inner.getX(), cy, inner.getRight(), cy, 0.5f);
	}

private:
	std::vector<float> writeBuffer;
	std::vector<float> readBuffer;
	std::atomic<size_t> writePos{ 0 };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterWaveformDisplay)
};