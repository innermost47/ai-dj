#include "MasterWaveformDisplay.h"

MasterWaveformDisplay::MasterWaveformDisplay()
{
	writeBuffer.resize(2048, 0.0f);
	readBuffer.resize(2048, 0.0f);
	vBlankAttachment = std::make_unique<juce::VBlankAttachment>(this, [this]() { handleVBlank(); });
}

MasterWaveformDisplay::~MasterWaveformDisplay()
{
	vBlankAttachment.reset();
}

void MasterWaveformDisplay::pushSamples(const float *left, const float *right, int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		float mono = (left[i] + (right ? right[i] : left[i])) * 0.5f;
		writeBuffer[writePos % writeBuffer.size()] = mono;
		++writePos;
	}
	hasNewData.store(true);
}

void MasterWaveformDisplay::setPositionInBeats(double ppqPosition)
{
	positionInBeats.store(ppqPosition);
	hasNewData.store(true);
}

void MasterWaveformDisplay::handleVBlank()
{
	if (!hasNewData.load())
		return;
	hasNewData.store(false);

	size_t pos = writePos.load();
	size_t sz = readBuffer.size();
	for (size_t i = 0; i < sz; ++i)
		readBuffer[i] = writeBuffer[(pos + i) % sz];

	pathsDirty = true;
	repaint();
}

void MasterWaveformDisplay::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(bounds, Obsidian::CORNER);
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.6f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::CORNER, 0.8f);

	auto inner = bounds.reduced(4.0f, 3.0f);
	int w = (int)inner.getWidth();
	int h = (int)inner.getHeight();
	if (w <= 0 || h <= 0)
		return;

	float cy = inner.getCentreY();
	float hH = inner.getHeight() * 0.45f;
	float ppx = (float)readBuffer.size() / (float)w;

	if (pathsDirty || lastW != w || lastH != h)
	{
		rebuildPaths(inner, w, cy, hH, ppx);
		lastW = w;
		lastH = h;
		pathsDirty = false;
	}

	g.setColour(ColourPalette::sequencerSubBeat.withAlpha(0.5f));
	for (int i = 1; i < Obsidian::MAX_TRACKS; ++i)
	{
		float gx = inner.getX() + (inner.getWidth() * (float)i / 8.0f);
		g.drawLine(gx, inner.getY() + 2.0f, gx, inner.getBottom() - 2.0f, 0.5f);
	}

	g.setColour(ColourPalette::textPrimary.withAlpha(0.15f));
	g.drawLine(inner.getX(), cy, inner.getRight(), cy, 0.8f);

	g.setColour(ColourPalette::textPrimary.withAlpha(0.85f));
	g.strokePath(cachedTop, juce::PathStrokeType(1.2f));
	g.strokePath(cachedBot, juce::PathStrokeType(1.2f));

	float phNorm = (float)(std::fmod(positionInBeats.load(), 4.0) / 4.0);
	if (phNorm < 0.0f)
		phNorm += 1.0f;
	float phX = inner.getX() + inner.getWidth() * phNorm;

	g.setColour(ColourPalette::playArmed.withAlpha(0.85f));
	g.drawLine(phX, inner.getY(), phX, inner.getBottom(), 1.2f);
}

void MasterWaveformDisplay::rebuildPaths(juce::Rectangle<float> inner, int w, float cy, float hH, float ppx)
{
	cachedTop.clear();
	cachedBot.clear();

	bool started = false;

	for (int i = 0; i < w; ++i)
	{
		float x = inner.getX() + i;

		int idx = (int)(i * ppx) % (int)readBuffer.size();
		float vScaled = std::abs(readBuffer[idx]) * hH;

		if (!started)
		{
			cachedTop.startNewSubPath(x, cy);
			cachedBot.startNewSubPath(x, cy);
			started = true;
		}
		cachedTop.lineTo(x, cy - vScaled);
		cachedBot.lineTo(x, cy + vScaled);
	}
}