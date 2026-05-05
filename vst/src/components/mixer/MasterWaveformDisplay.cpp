#pragma once
#include "MasterWaveformDisplay.h"

MasterWaveformDisplay::MasterWaveformDisplay()
{
	writeBuffer.resize(2048, 0.0f);
	readBuffer.resize(2048, 0.0f);
	startTimerHz(30);
}

MasterWaveformDisplay::~MasterWaveformDisplay() { stopTimer(); }

void MasterWaveformDisplay::pushSamples(const float* left, const float* right, int numSamples)
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

void MasterWaveformDisplay::timerCallback()
{
	if (!hasNewData.load())
	{
		idleFrames++;
		if (idleFrames == 30)
			startTimerHz(5);
		return;
	}

	hasNewData.store(false);
	idleFrames = 0;
	startTimerHz(30);

	size_t pos = writePos.load();
	size_t sz = readBuffer.size();
	for (size_t i = 0; i < sz; ++i)
		readBuffer[i] = writeBuffer[(pos + i) % sz];

	animPhase += 1.0f / 30.0f;
	pathsDirty = true;
	repaint();
}

void MasterWaveformDisplay::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRoundedRectangle(bounds, 4.0f);
	g.setColour(ColourPalette::backgroundLight.withAlpha(0.6f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 0.8f);

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
	for (int i = 1; i < 8; ++i)
	{
		float gx = inner.getX() + (inner.getWidth() * (float)i / 8.0f);
		g.drawLine(gx, inner.getY() + 2.0f, gx, inner.getBottom() - 2.0f, 0.5f);
	}

	g.setColour(ColourPalette::amber.withAlpha(0.7f));
	for (int i = 1; i < 8; ++i)
	{
		float gx = inner.getX() + (inner.getWidth() * (float)i / 8.0f);
		float r = (i == 4) ? 2.0f : 1.2f;
		g.fillEllipse(gx - r, inner.getY() + 2.0f, r * 2.0f, r * 2.0f);
	}

	g.setColour(ColourPalette::textPrimary.withAlpha(0.15f));
	g.drawLine(inner.getX(), cy, inner.getRight(), cy, 0.8f);

	g.setColour(ColourPalette::teal.withAlpha(0.30f));
	g.strokePath(cachedEchoTop, juce::PathStrokeType(0.8f));
	g.strokePath(cachedEchoBot, juce::PathStrokeType(0.8f));

	g.setColour(ColourPalette::textPrimary.withAlpha(0.12f));
	g.strokePath(cachedTop, juce::PathStrokeType(3.5f));
	g.strokePath(cachedBot, juce::PathStrokeType(3.5f));

	g.setColour(ColourPalette::textPrimary.withAlpha(0.85f));
	g.strokePath(cachedTop, juce::PathStrokeType(1.2f));
	g.strokePath(cachedBot, juce::PathStrokeType(1.2f));

	if (cachedMaxPeakVal > 0.15f)
	{
		float peakY1 = cy - cachedMaxPeakVal * hH;
		float peakY2 = cy + cachedMaxPeakVal * hH;
		g.setColour(ColourPalette::buttonDangerLight);
		g.fillEllipse(cachedMaxPeakX - 1.8f, peakY1 - 1.8f, 3.6f, 3.6f);
		g.fillEllipse(cachedMaxPeakX - 1.8f, peakY2 - 1.8f, 3.6f, 3.6f);

		float ringAlpha = 0.4f + 0.3f * std::sin(animPhase * 6.0f);
		g.setColour(ColourPalette::buttonDangerLight.withAlpha(ringAlpha));
		g.drawEllipse(cachedMaxPeakX - 4.0f, peakY1 - 4.0f, 8.0f, 8.0f, 0.6f);
	}

	const int numParticles = 8;
	double pos = positionInBeats.load();
	float beatFrac = (float)(pos - std::floor(pos));
	float baseAlpha = 0.25f + 0.35f * juce::jmin(1.0f, cachedPeakAbs * 2.0f);
	float transientKick = juce::jmax(0.0f, cachedPeakAbs - lastPeak) * 3.0f;
	lastPeak = cachedPeakAbs * 0.92f + cachedPeakAbs * 0.08f;

	for (int i = 0; i < numParticles; ++i)
	{
		float particleBeatOffset = (float)i / (float)numParticles;
		float distanceFromBeat = std::abs(beatFrac - particleBeatOffset);
		distanceFromBeat = juce::jmin(distanceFromBeat, 1.0f - distanceFromBeat);
		float beatPulse = std::exp(-distanceFromBeat * 12.0f);

		float t = animPhase * 0.3f + (float)i * 0.78539f;
		float px = inner.getX() + inner.getWidth() * (0.1f + 0.8f * (0.5f + 0.5f * std::sin(t + (float)i)));
		float py = cy + std::sin(t * 1.3f + (float)i * 1.1f) * (hH * 0.75f);
		float pr = 1.3f + beatPulse * 2.8f + transientKick * 1.5f;
		float alpha = juce::jlimit(0.0f, 1.0f, baseAlpha + beatPulse * 0.6f + transientKick * 0.4f);

		auto col = ColourPalette::getTrackColour(i);
		g.setColour(col.withAlpha(alpha));
		g.fillEllipse(px - pr, py - pr, pr * 2.0f, pr * 2.0f);

		if (beatPulse > 0.5f)
		{
			g.setColour(col.withAlpha(beatPulse * 0.35f));
			float ringR = pr + 3.0f + beatPulse * 2.0f;
			g.drawEllipse(px - ringR, py - ringR, ringR * 2.0f, ringR * 2.0f, 0.8f);
		}
	}

	float phNorm = (float)(std::fmod(pos, 4.0) / 4.0);
	if (phNorm < 0.0f) phNorm += 1.0f;
	float phX = inner.getX() + inner.getWidth() * phNorm;

	g.setColour(ColourPalette::playArmed.withAlpha(0.12f));
	g.fillRect(phX - 2.5f, inner.getY(), 5.0f, inner.getHeight());
	g.setColour(ColourPalette::playArmed.withAlpha(0.85f));
	g.drawLine(phX, inner.getY(), phX, inner.getBottom(), 1.2f);
	g.setColour(ColourPalette::playArmed);
	g.fillEllipse(phX - 2.5f, cy - 2.5f, 5.0f, 5.0f);

	g.setColour(ColourPalette::textSecondary.withAlpha(0.4f));
	const float tick = 4.0f;
	g.drawLine(inner.getX(), inner.getY() + tick, inner.getX(), inner.getY(), 0.5f);
	g.drawLine(inner.getX(), inner.getY(), inner.getX() + tick, inner.getY(), 0.5f);
	g.drawLine(inner.getRight() - tick, inner.getY(), inner.getRight(), inner.getY(), 0.5f);
	g.drawLine(inner.getRight(), inner.getY(), inner.getRight(), inner.getY() + tick, 0.5f);
	g.drawLine(inner.getX(), inner.getBottom() - tick, inner.getX(), inner.getBottom(), 0.5f);
	g.drawLine(inner.getX(), inner.getBottom(), inner.getX() + tick, inner.getBottom(), 0.5f);
	g.drawLine(inner.getRight() - tick, inner.getBottom(), inner.getRight(), inner.getBottom(), 0.5f);
	g.drawLine(inner.getRight(), inner.getBottom() - tick, inner.getRight(), inner.getBottom(), 0.5f);
}

void MasterWaveformDisplay::rebuildPaths(juce::Rectangle<float> inner, int w, float cy, float hH, float ppx)
{
	cachedTop.clear();
	cachedBot.clear();
	cachedEchoTop.clear();
	cachedEchoBot.clear();

	cachedMaxPeakVal = 0.0f;
	cachedMaxPeakX = inner.getX();
	cachedPeakAbs = 0.0f;

	for (size_t i = 0; i < readBuffer.size(); ++i)
		cachedPeakAbs = juce::jmax(cachedPeakAbs, std::abs(readBuffer[i]));

	const int echoOffset = 96;
	bool echoStarted = false, mainStarted = false;

	for (int i = 0; i < w; ++i)
	{
		float x = inner.getX() + i;

		int echoIdx = ((int)(i * ppx) + echoOffset) % (int)readBuffer.size();
		float echoVal = std::abs(readBuffer[echoIdx]) * hH * 0.85f;
		if (!echoStarted)
		{
			cachedEchoTop.startNewSubPath(x, cy);
			cachedEchoBot.startNewSubPath(x, cy);
			echoStarted = true;
		}
		cachedEchoTop.lineTo(x, cy - echoVal);
		cachedEchoBot.lineTo(x, cy + echoVal);

		int idx = (int)(i * ppx) % (int)readBuffer.size();
		float val = std::abs(readBuffer[idx]);
		float vScaled = val * hH;

		if (val > cachedMaxPeakVal)
		{
			cachedMaxPeakVal = val;
			cachedMaxPeakX = x;
		}

		if (!mainStarted)
		{
			cachedTop.startNewSubPath(x, cy);
			cachedBot.startNewSubPath(x, cy);
			mainStarted = true;
		}
		cachedTop.lineTo(x, cy - vScaled);
		cachedBot.lineTo(x, cy + vScaled);
	}
}