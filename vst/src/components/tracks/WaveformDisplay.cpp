#include "WaveformDisplay.h"
#include "AiModelDefinitions.h"
#include "ColourPalette.h"
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "TrackData.h"

WaveformDisplay::WaveformDisplay(DjIaVstProcessor &processor, TrackData *trackData)
    : audioProcessor(processor), track(trackData)
{
	setSize(400, 80);
	setAccessible(false);
	zoomFactor = 1.0;
	viewStartTime = 0.0;
	sampleRate = 48000.0;
	auto &currentPage = track->getCurrentPage();
	loopPointsLocked = currentPage.loopPointsLocked.load();
	horizontalScrollBar = std::make_unique<juce::ScrollBar>(false);
	horizontalScrollBar->setRangeLimits(0.0, 1.0);
	horizontalScrollBar->addListener(this);
	cachedModelColour = getModelAccentColour();
}

WaveformDisplay::~WaveformDisplay()
{
	setVisible(false);
}

void WaveformDisplay::invalidateAllCaches()
{
	waveformCacheDirty = true;
	gridCacheDirty = true;
}

void WaveformDisplay::invalidateGridCache()
{
	gridCacheDirty = true;
}

void WaveformDisplay::resized()
{
	invalidateAllCaches();
	if (scrollBarVisible && horizontalScrollBar)
		horizontalScrollBar->setBounds(0, getHeight() - 8, getWidth(), 12);
}

void WaveformDisplay::setSampleBpm(float bpm)
{
	if (std::abs(sampleBpm - bpm) < 0.001f)
		return;

	sampleBpm = bpm;
	float oldRatio = stretchRatio;
	calculateStretchRatio();

	if (std::abs(stretchRatio - oldRatio) >= 0.001f)
	{
		invalidateAllCaches();
	}

	juce::MessageManager::callAsync(
	    [safeThis = juce::Component::SafePointer<WaveformDisplay>(this)]()
	    {
		    if (safeThis != nullptr)
			    safeThis->repaint();
	    });
}

void WaveformDisplay::setOriginalBpm(float bpm)
{
	if (std::abs(originalBpm - bpm) < 0.001f)
		return;
	originalBpm = bpm;
	float oldRatio = stretchRatio;
	calculateStretchRatio();
	if (std::abs(stretchRatio - oldRatio) >= 0.001f)
	{
		invalidateAllCaches();
	}
}

void WaveformDisplay::setAudioData(const juce::AudioBuffer<float> &newAudioBuffer, double newSampleRate)
{
	jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

	if (newAudioBuffer.getNumChannels() == 0 || newAudioBuffer.getNumSamples() == 0)
	{
		audioBuffer.setSize(0, 0);
		sampleRate = newSampleRate;
		thumbnailLeft.clear();
		thumbnailRight.clear();
		invalidateAllCaches();
		repaint();
		return;
	}

	try
	{
		audioBuffer.setSize(newAudioBuffer.getNumChannels(), newAudioBuffer.getNumSamples(), false, true, true);

		for (int channel = 0; channel < newAudioBuffer.getNumChannels(); ++channel)
		{
			audioBuffer.copyFrom(channel, 0, newAudioBuffer, channel, 0, newAudioBuffer.getNumSamples());
		}

		sampleRate = newSampleRate;
		zoomFactor = 1.0;
		viewStartTime = 0.0;

		generateThumbnail();
		invalidateAllCaches();
		repaint();
	}
	catch (const std::exception &)
	{
		audioBuffer.setSize(0, 0);
		sampleRate = newSampleRate;
		thumbnailLeft.clear();
		thumbnailRight.clear();
		invalidateAllCaches();
		repaint();
	}
}

void WaveformDisplay::setLoopPoints(double startTime, double endTime)
{
	if (std::abs(loopStart - startTime) < 1e-6 && std::abs(loopEnd - endTime) < 1e-6)
		return;

	bool startChanged = std::abs(loopStart - startTime) >= 1e-6;
	loopStart = startTime;
	loopEnd = endTime;

	if (startChanged)
	{
		invalidateGridCache();
	}

	if (juce::MessageManager::getInstance()->isThisTheMessageThread())
	{
		repaint();
	}
	else
	{
		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (isShowing())
				    repaint();
		    });
	}
}

void WaveformDisplay::lockLoopPoints(bool locked)
{
	if (loopPointsLocked == locked)
		return;
	loopPointsLocked = locked;
	juce::MessageManager::callAsync([this]() { repaint(); });
}

void WaveformDisplay::calculateStretchRatio() const
{
	if (originalBpm > 0.0f && sampleBpm > 0.0f)
	{
		const_cast<WaveformDisplay *>(this)->stretchRatio = sampleBpm / originalBpm;
	}
	else
	{
		const_cast<WaveformDisplay *>(this)->stretchRatio = 1.0f;
	}
}

void WaveformDisplay::repaintPlaybackHeadRegion(float oldX, float newX)
{
	const int margin = 50;
	const int h = getHeight();

	if (oldX >= 0)
	{
		auto oldRect = juce::Rectangle<int>(static_cast<int>(oldX) - margin, 0, margin * 2, h);
		repaint(oldRect);
	}

	if (newX >= 0)
	{
		auto newRect = juce::Rectangle<int>(static_cast<int>(newX) - margin, 0, margin * 2, h);
		repaint(newRect);
	}
}

void WaveformDisplay::setPlaybackPosition(double timeInSeconds, bool isPlaying)
{
	if (!isPlaying && !isCurrentlyPlaying)
		return;

	if (std::abs(timeInSeconds - playbackPosition) < 0.005 && isPlaying == isCurrentlyPlaying)
		return;

	float oldX = (isCurrentlyPlaying && playbackPosition >= 0.0) ? timeToX(playbackPosition) : -1.0f;

	playbackPosition = timeInSeconds;
	bool wasPlaying = isCurrentlyPlaying;
	isCurrentlyPlaying = isPlaying;

	float newX = (isPlaying && playbackPosition >= 0.0) ? timeToX(playbackPosition) : -1.0f;

	juce::MessageManager::callAsync(
	    [safe = juce::Component::SafePointer<WaveformDisplay>(this), oldX, newX, wasPlaying, isPlaying]()
	    {
		    if (safe == nullptr)
			    return;

		    if (wasPlaying != isPlaying)
		    {
			    safe->repaint();
			    safe->lastPlaybackHeadX = newX;
			    return;
		    }

		    safe->repaintPlaybackHeadRegion(oldX, newX);
		    safe->lastPlaybackHeadX = newX;
	    });
}

void WaveformDisplay::rebuildWaveformCache()
{
	if (getWidth() <= 0 || getHeight() <= 0)
		return;

	waveformCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
	juce::Graphics g(waveformCache);
	renderWaveformInto(g);
	waveformCacheDirty = false;
	cachedStretchRatioForColour = stretchRatio;
	cachedModelColour = getModelAccentColour();
}

void WaveformDisplay::rebuildGridCache()
{
	if (getWidth() <= 0 || getHeight() <= 0)
		return;

	gridCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
	juce::Graphics g(gridCache);
	renderGridInto(g);
	gridCacheDirty = false;
}

void WaveformDisplay::renderWaveformInto(juce::Graphics &g)
{
	g.setColour(ColourPalette::backgroundMid);
	g.fillRect(getLocalBounds());

	if (thumbnailLeft.empty() && thumbnailRight.empty())
		return;

	drawWaveform(g);
}

void WaveformDisplay::renderGridInto(juce::Graphics &g)
{
	if (thumbnailLeft.empty() && thumbnailRight.empty())
		return;
	drawBeatMarkers(g);
}

void WaveformDisplay::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();

	if (thumbnailLeft.empty() && thumbnailRight.empty())
	{
		g.setColour(ColourPalette::backgroundMid);
		g.fillRect(bounds);
		g.setColour(ColourPalette::textSecondary);
		g.setFont(12.0f);
		g.drawText("No audio data", bounds.reduced(5).removeFromTop(20), juce::Justification::centred);

		g.setColour(ColourPalette::textSecondary);
		g.setFont(10.0f);
		g.drawText("Ctrl+Wheel: Zoom | Wheel: Scroll | Right-click: Lock/Unlock | Ctrl+Click: Drag and Drop in DAW",
		           bounds.reduced(5).removeFromBottom(15), juce::Justification::centred);
		return;
	}

	auto currentColour = getModelAccentColour();
	if (currentColour != cachedModelColour || std::abs(stretchRatio - cachedStretchRatioForColour) >= 0.001f)
	{
		invalidateAllCaches();
	}

	if (waveformCacheDirty || waveformCache.getWidth() != getWidth() || waveformCache.getHeight() != getHeight())
	{
		rebuildWaveformCache();
	}

	if (gridCacheDirty || gridCache.getWidth() != getWidth() || gridCache.getHeight() != getHeight())
	{
		rebuildGridCache();
	}

	g.drawImageAt(waveformCache, 0, 0);
	g.drawImageAt(gridCache, 0, 0);

	drawLoopMarkers(g);
	drawPlaybackHead(g);

	if (zoomFactor > 1.0)
	{
		g.setColour(ColourPalette::textPrimary);
		g.setFont(10.0f);
		g.drawText("Zoom: " + juce::String(zoomFactor, 1) + "x", 5, getHeight() - 20, 60, 15,
		           juce::Justification::left);
	}

	if (loopPointsLocked)
	{
		g.setColour(ColourPalette::loopLocked);
		g.setFont(10.0f);
		g.drawText("LOCKED", getWidth() - 60, getHeight() - 20, 55, 15, juce::Justification::right);
	}
}

void WaveformDisplay::mouseDown(const juce::MouseEvent &e)
{
	auto &currentPage = track->getCurrentPage();
	if (!e.mods.isRightButtonDown() && !loopPointsLocked)
	{
		auto handle = hitTestAdsr(e.position);
		if (handle != AdsrHandle::None)
		{
			activeHandle = handle;
			updateAdsrFromMouse(e.position);
			return;
		}
	}
	if (e.mods.isRightButtonDown())
	{
		loopPointsLocked = !currentPage.loopPointsLocked.load();
		currentPage.loopPointsLocked.store(loopPointsLocked);
		lockLoopPoints(loopPointsLocked);
		return;
	}

	if (loopPointsLocked)
		return;

	float startX = timeToX(loopStart);
	float endX = timeToX(loopEnd);
	float tolerance = 15.0f;

	if (std::abs(e.x - startX) < tolerance)
	{
		draggingStart = true;
		return;
	}
	else if (std::abs(e.x - endX) < tolerance)
	{
		draggingEnd = true;
		return;
	}
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent &e)
{
	if (activeHandle != AdsrHandle::None)
	{
		updateAdsrFromMouse(e.position);
		return;
	}
	if (!draggingStart && !draggingEnd && currentAudioFile.exists() && e.mods.isCtrlDown())
	{
		auto distanceFromStart = e.getDistanceFromDragStart();
		if (distanceFromStart > 10 && !isDraggingAudio)
		{
			isDraggingAudio = true;

			juce::File exportedFile = audioProcessor.getAudioManager().exportSampleForDragDrop(currentAudioFile);

			if (exportedFile.existsAsFile())
			{
				juce::StringArray files;
				files.add(exportedFile.getFullPathName());
				performExternalDragDropOfFiles(files, false);
			}
			else
			{
				juce::StringArray files;
				files.add(currentAudioFile.getFullPathName());
				performExternalDragDropOfFiles(files, false);
			}

			return;
		}
	}

	if (loopPointsLocked || trackBpm <= 0.0f)
		return;

	if (!e.mods.isCtrlDown())
	{
		if (draggingStart)
		{
			double newStart = xToTime(static_cast<float>(e.x));
			double clamped = juce::jlimit(getViewStartTime(), loopEnd, newStart);
			if (std::abs(clamped - loopStart) >= 1e-6)
			{
				loopStart = clamped;
				invalidateGridCache();
				repaint();
				if (onLoopPointsChanged)
				{
					onLoopPointsChanged(loopStart, loopEnd);
				}
			}
		}
		else if (draggingEnd)
		{
			double newEnd = xToTime(static_cast<float>(e.x));
			double clamped = juce::jlimit(loopStart, getViewEndTime(), newEnd);
			if (std::abs(clamped - loopEnd) >= 1e-6)
			{
				loopEnd = clamped;
				repaint();
				if (onLoopPointsChanged)
				{
					onLoopPointsChanged(loopStart, loopEnd);
				}
			}
		}
	}
}

void WaveformDisplay::setAudioFile(const juce::File &file)
{
	currentAudioFile = file;
}

void WaveformDisplay::mouseUp(const juce::MouseEvent &)
{
	activeHandle = AdsrHandle::None;
	draggingStart = false;
	draggingEnd = false;
	isDraggingAudio = false;
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
	if (e.mods.isCtrlDown())
	{
		if (getTotalDuration() <= 0.0)
			return;

		double totalDuration = getTotalDuration();
		double currentViewDuration = totalDuration / zoomFactor;
		double mouseRatio = (double)e.x / (double)getWidth();
		double mouseTime = viewStartTime + (mouseRatio * currentViewDuration);
		double oldZoomFactor = zoomFactor;

		if (wheel.deltaY > 0)
		{
			zoomFactor = juce::jlimit(1.0, 10.0, zoomFactor * 1.2);
		}
		else
		{
			zoomFactor = juce::jlimit(1.0, 10.0, zoomFactor / 1.2);
		}

		if (zoomFactor == oldZoomFactor)
			return;

		double newViewDuration = totalDuration / zoomFactor;
		viewStartTime = mouseTime - (mouseRatio * newViewDuration);

		setViewStartTime(viewStartTime);

		updateScrollBarVisibility();
		generateThumbnail();
		invalidateAllCaches();
		repaint();
	}
	else if (zoomFactor > 1.0)
	{
		double viewDuration = getTotalDuration() / zoomFactor;
		double scrollAmount = wheel.deltaY * viewDuration * 0.1;
		double oldViewStart = viewStartTime;

		setViewStartTime(viewStartTime - scrollAmount);

		if (std::abs(viewStartTime - oldViewStart) < 1e-9)
			return;

		updateScrollBar();
		generateThumbnail();
		invalidateAllCaches();
		repaint();
	}
}

WaveformDisplay::AdsrHandle WaveformDisplay::hitTestAdsr(juce::Point<float> p) const
{
	if (!adsrLayout.valid)
		return AdsrHandle::None;

	const float radius = 8.0f;
	const float r2 = radius * radius;
	auto dist2 = [](juce::Point<float> a, juce::Point<float> b)
	{
		float dx = a.x - b.x, dy = a.y - b.y;
		return dx * dx + dy * dy;
	};

	if (dist2(p, {adsrLayout.x1, adsrLayout.yPeak}) <= r2)
		return AdsrHandle::AttackPeak;
	if (dist2(p, {adsrLayout.x2, adsrLayout.ySustain}) <= r2)
		return AdsrHandle::DecaySustain;
	if (dist2(p, {adsrLayout.x3, adsrLayout.ySustain}) <= r2)
		return AdsrHandle::ReleaseStart;
	return AdsrHandle::None;
}

void WaveformDisplay::updateAdsrFromMouse(juce::Point<float> p)
{
	if (!adsrLayout.valid || adsrLayout.scale <= 0.0f)
		return;

	const float zoneWidth = adsrLayout.endX - adsrLayout.startX;
	if (zoneWidth <= 0.0f)
		return;

	const float pixPerSec = zoneWidth / (float)adsrLayout.sectionDuration;
	if (pixPerSec <= 0.0f)
		return;

	const float h = (float)getHeight();
	const float margin = h * 0.08f;
	auto yToAmp = [&](float y) -> float
	{
		float amp = (h - margin - y) / (h - margin * 2.0f);
		return juce::jlimit(0.0f, 1.0f, amp);
	};

	const float invScale = 1.0f / adsrLayout.scale;

	const float maxParamSec = (float)adsrLayout.sectionDuration;

	switch (activeHandle)
	{
	case AdsrHandle::AttackPeak:
	{
		float clampedX = juce::jlimit(adsrLayout.startX, adsrLayout.endX, p.x);
		float attackPxDrawn = clampedX - adsrLayout.startX;
		float attackSecDrawn = attackPxDrawn / pixPerSec;
		float attackParam = juce::jlimit(0.0f, maxParamSec, attackSecDrawn * invScale);
		if (onAdsrAttackChanged)
			onAdsrAttackChanged(attackParam);
		break;
	}
	case AdsrHandle::DecaySustain:
	{
		float clampedX = juce::jlimit(adsrLayout.x1, adsrLayout.endX, p.x);
		float decayPxDrawn = clampedX - adsrLayout.x1;
		float decaySecDrawn = decayPxDrawn / pixPerSec;
		float decayParam = juce::jlimit(0.0f, maxParamSec, decaySecDrawn * invScale);
		if (onAdsrDecayChanged)
			onAdsrDecayChanged(decayParam);

		float sustain = yToAmp(p.y);
		if (onAdsrSustainChanged)
			onAdsrSustainChanged(sustain);
		break;
	}
	case AdsrHandle::ReleaseStart:
	{
		float clampedX = juce::jlimit(adsrLayout.startX, adsrLayout.endX, p.x);
		float releasePxDrawn = adsrLayout.endX - clampedX;
		float releaseSecDrawn = releasePxDrawn / pixPerSec;
		float releaseParam = juce::jlimit(0.0f, maxParamSec, releaseSecDrawn * invScale);
		if (onAdsrReleaseChanged)
			onAdsrReleaseChanged(releaseParam);
		break;
	}
	default:
		break;
	}

	repaint();
}

void WaveformDisplay::mouseDoubleClick(const juce::MouseEvent &e)
{
	auto handle = hitTestAdsr(e.position);
	if (handle != AdsrHandle::None)
	{
		const float defA = 0.0f, defD = 4.0f, defS = 1.0f, defR = 0.0f;

		switch (handle)
		{
		case AdsrHandle::AttackPeak:
			if (onAdsrAttackChanged)
				onAdsrAttackChanged(defA);
			break;
		case AdsrHandle::DecaySustain:
			if (onAdsrDecayChanged)
				onAdsrDecayChanged(defD);
			if (onAdsrSustainChanged)
				onAdsrSustainChanged(defS);
			break;
		case AdsrHandle::ReleaseStart:
			if (onAdsrReleaseChanged)
				onAdsrReleaseChanged(defR);
			break;
		default:
			break;
		}
		repaint();
		return;
	}

	if (loopPointsLocked)
		return;

	float startX = timeToX(loopStart);
	float endX = timeToX(loopEnd);

	if ((float)e.x < startX)
	{
		double newStart = xToTime((float)e.x);
		newStart = juce::jlimit(getViewStartTime(), loopEnd, newStart);
		if (std::abs(newStart - loopStart) >= 1e-6)
		{
			loopStart = newStart;
			invalidateGridCache();
			repaint();
			if (onLoopPointsChanged)
				onLoopPointsChanged(loopStart, loopEnd);
		}
	}
	else if ((float)e.x > endX)
	{
		double newEnd = xToTime((float)e.x);
		newEnd = juce::jlimit(loopStart, getViewEndTime(), newEnd);
		if (std::abs(newEnd - loopEnd) >= 1e-6)
		{
			loopEnd = newEnd;
			repaint();
			if (onLoopPointsChanged)
				onLoopPointsChanged(loopStart, loopEnd);
		}
	}
}

void WaveformDisplay::updateScrollBarVisibility()
{
	bool shouldShow = (zoomFactor > 1.0);

	const int barHeight = 5;
	const int barY = getHeight() - barHeight;

	auto applyScrollBarStyle = [this]()
	{
		juce::Colour modelColour = getModelAccentColour();
		horizontalScrollBar->setColour(juce::ScrollBar::thumbColourId, modelColour.withAlpha(0.85f));
	};

	if (shouldShow && !scrollBarVisible)
	{
		addAndMakeVisible(*horizontalScrollBar);
		horizontalScrollBar->setBounds(0, barY, getWidth(), barHeight);
		applyScrollBarStyle();
		scrollBarVisible = true;
		updateScrollBar();
	}
	else if (!shouldShow && scrollBarVisible)
	{
		removeChildComponent(horizontalScrollBar.get());
		scrollBarVisible = false;
	}
	else if (shouldShow)
	{
		horizontalScrollBar->setBounds(0, barY, getWidth(), barHeight);
		applyScrollBarStyle();
		updateScrollBar();
	}
}

void WaveformDisplay::updateScrollBar()
{
	if (!scrollBarVisible)
		return;

	double totalDuration = getTotalDuration();

	if (totalDuration <= 0.0)
	{
		horizontalScrollBar->setCurrentRange(0.0, 1.0);
		return;
	}

	double viewProportionOfTotal = 1.0 / zoomFactor;
	double currentRangeStart = viewStartTime / totalDuration;

	horizontalScrollBar->setCurrentRange(currentRangeStart, viewProportionOfTotal);
}

void WaveformDisplay::scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart)
{
	if (scrollBarThatHasMoved == horizontalScrollBar.get())
	{
		double totalDuration = getTotalDuration();
		double newViewStartTime = newRangeStart * totalDuration;
		double oldViewStart = viewStartTime;

		setViewStartTime(newViewStartTime);

		if (std::abs(viewStartTime - oldViewStart) < 1e-9)
			return;

		generateThumbnail();
		invalidateAllCaches();
		repaint();
	}
}

void WaveformDisplay::setViewStartTime(double newViewStartTime)
{
	double totalDuration = getTotalDuration();
	if (totalDuration <= 0.0)
	{
		viewStartTime = 0.0;
		return;
	}

	double viewDuration = totalDuration / zoomFactor;
	double maxViewStartTime = totalDuration - viewDuration;

	if (maxViewStartTime < 0.0)
	{
		maxViewStartTime = 0.0;
	}

	viewStartTime = juce::jlimit(0.0, maxViewStartTime, newViewStartTime);
}

float WaveformDisplay::getHostBpm() const
{
	return static_cast<float>(audioProcessor.getHostBpm());
}

void WaveformDisplay::generateThumbnail()
{
	thumbnailLeft.clear();
	thumbnailRight.clear();

	if (audioBuffer.getNumSamples() == 0)
		return;

	double totalDuration = getTotalDuration();
	double viewDuration = totalDuration / zoomFactor;
	double viewEndTime = juce::jlimit(viewStartTime, totalDuration, viewStartTime + viewDuration);

	int startSample = (int)(viewStartTime * sampleRate);
	int endSample = (int)(viewEndTime * sampleRate);
	startSample = juce::jlimit(0, audioBuffer.getNumSamples() - 1, startSample);
	endSample = juce::jlimit(startSample + 1, audioBuffer.getNumSamples(), endSample);

	int viewSamples = endSample - startSample;

	if (viewSamples <= 0)
		return;

	int targetPoints = getWidth() * 2;
	targetPoints = juce::jlimit(getWidth(), getWidth() * 10, targetPoints);

	int samplesPerPoint = juce::jmax(1, viewSamples / targetPoints);

	for (int point = 0; point < targetPoints; ++point)
	{
		int retFlag;
		feedThumbnailStereo(startSample, point, samplesPerPoint, retFlag);
		if (retFlag == 2)
			break;
	}
}

void WaveformDisplay::feedThumbnailStereo(int startSample, int point, int samplesPerPoint, int &retFlag)
{
	retFlag = 1;
	int sampleStart = startSample + (point * samplesPerPoint);
	int sampleEnd = std::min(sampleStart + samplesPerPoint, audioBuffer.getNumSamples());

	if (sampleStart >= audioBuffer.getNumSamples())
	{
		retFlag = 2;
		return;
	}

	float rmsSumLeft = 0.0f;
	float rmsSumRight = 0.0f;
	float peakLeft = 0.0f;
	float peakRight = 0.0f;
	int count = 0;

	int numChannels = audioBuffer.getNumChannels();
	bool isMono = (numChannels == 1);

	for (int sample = sampleStart; sample < sampleEnd; ++sample)
	{
		if (sample >= audioBuffer.getNumSamples())
			break;

		float valLeft = audioBuffer.getSample(0, sample);
		rmsSumLeft += valLeft * valLeft;
		peakLeft = std::max(peakLeft, std::abs(valLeft));

		float valRight = isMono ? valLeft : audioBuffer.getSample(1, sample);
		rmsSumRight += valRight * valRight;
		peakRight = std::max(peakRight, std::abs(valRight));

		count++;
	}

	float rmsLeft = count > 0 ? std::sqrt(rmsSumLeft / count) : 0.0f;
	float rmsRight = count > 0 ? std::sqrt(rmsSumRight / count) : 0.0f;

	float finalLeft = (rmsLeft * 0.7f) + (peakLeft * 0.3f);
	float finalRight = (rmsRight * 0.7f) + (peakRight * 0.3f);

	thumbnailLeft.push_back(finalLeft);
	thumbnailRight.push_back(finalRight);
}

void WaveformDisplay::drawWaveform(juce::Graphics &g)
{
	if (thumbnailLeft.empty() || thumbnailRight.empty())
		return;

	juce::Colour waveformColor;
	setColorDependingTimeStretchRatio(waveformColor);

	g.setColour(waveformColor);

	size_t thumbnailSize = std::min(thumbnailLeft.size(), thumbnailRight.size());
	float pixelsPerPoint = static_cast<float>(getWidth()) / static_cast<float>(thumbnailSize);

	float centerY = getHeight() * 0.5f;
	float quarterY = getHeight() * 0.25f;
	float threeQuarterY = getHeight() * 0.75f;

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.5f));
	g.drawLine(0.0f, centerY, static_cast<float>(getWidth()), centerY, 1.0f);

	juce::Path leftPathTop, leftPathBottom;
	bool leftTopStarted = false, leftBottomStarted = false;

	for (size_t i = 0; i < thumbnailSize; ++i)
	{
		float x = i * pixelsPerPoint;
		float amplitude = thumbnailLeft[i];
		float waveHeight = amplitude * quarterY * 0.9f;

		float topY = quarterY - waveHeight;
		float bottomY = quarterY + waveHeight;

		if (!leftTopStarted)
		{
			leftPathTop.startNewSubPath(x, quarterY);
			leftTopStarted = true;
		}
		if (i > 0 && i < thumbnailSize - 1)
		{
			float prevX = (i - 1) * pixelsPerPoint;
			float nextX = (i + 1) * pixelsPerPoint;
			float controlX = (prevX + nextX) * 0.5f;
			leftPathTop.quadraticTo(controlX, topY, x, topY);
		}
		else
		{
			leftPathTop.lineTo(x, topY);
		}

		if (!leftBottomStarted)
		{
			leftPathBottom.startNewSubPath(x, quarterY);
			leftBottomStarted = true;
		}
		if (i > 0 && i < thumbnailSize - 1)
		{
			float prevX = (i - 1) * pixelsPerPoint;
			float nextX = (i + 1) * pixelsPerPoint;
			float controlX = (prevX + nextX) * 0.5f;
			leftPathBottom.quadraticTo(controlX, bottomY, x, bottomY);
		}
		else
		{
			leftPathBottom.lineTo(x, bottomY);
		}
	}

	g.setColour(waveformColor);
	g.strokePath(leftPathTop, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
	g.strokePath(leftPathBottom, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawLine(0.0f, quarterY, static_cast<float>(getWidth()), quarterY, 0.5f);

	juce::Path rightPathTop, rightPathBottom;
	bool rightTopStarted = false, rightBottomStarted = false;

	for (size_t i = 0; i < thumbnailSize; ++i)
	{
		float x = i * pixelsPerPoint;
		float amplitude = thumbnailRight[i];
		float waveHeight = amplitude * quarterY * 0.9f;

		float topY = threeQuarterY - waveHeight;
		float bottomY = threeQuarterY + waveHeight;

		if (!rightTopStarted)
		{
			rightPathTop.startNewSubPath(x, threeQuarterY);
			rightTopStarted = true;
		}
		if (i > 0 && i < thumbnailSize - 1)
		{
			float prevX = (i - 1) * pixelsPerPoint;
			float nextX = (i + 1) * pixelsPerPoint;
			float controlX = (prevX + nextX) * 0.5f;
			rightPathTop.quadraticTo(controlX, topY, x, topY);
		}
		else
		{
			rightPathTop.lineTo(x, topY);
		}

		if (!rightBottomStarted)
		{
			rightPathBottom.startNewSubPath(x, threeQuarterY);
			rightBottomStarted = true;
		}
		if (i > 0 && i < thumbnailSize - 1)
		{
			float prevX = (i - 1) * pixelsPerPoint;
			float nextX = (i + 1) * pixelsPerPoint;
			float controlX = (prevX + nextX) * 0.5f;
			rightPathBottom.quadraticTo(controlX, bottomY, x, bottomY);
		}
		else
		{
			rightPathBottom.lineTo(x, bottomY);
		}
	}

	g.setColour(waveformColor);
	g.strokePath(rightPathTop, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
	g.strokePath(rightPathBottom, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawLine(0.0f, threeQuarterY, static_cast<float>(getWidth()), threeQuarterY, 0.5f);

	g.setColour(ColourPalette::textSecondary.withAlpha(0.7f));
	g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
	g.drawText("L", 5, 5, 15, 15, juce::Justification::centred);
	g.drawText("R", 5, static_cast<int>(centerY) + 5, 15, 15, juce::Justification::centred);
}

void WaveformDisplay::setColorDependingTimeStretchRatio(juce::Colour &waveformColor) const
{
	juce::Colour baseColour = getModelAccentColour();

	float deviation = std::abs(stretchRatio - 1.0f);

	if (deviation < 0.005f)
	{
		waveformColor = baseColour;
	}
	else if (deviation < 0.08f)
	{
		float normalizedDev = juce::jlimit(0.0f, 1.0f, (deviation - 0.005f) / 0.075f);

		if (stretchRatio > 1.0f)
		{
			waveformColor = baseColour.interpolatedWith(baseColour.brighter(0.4f), normalizedDev);
		}
		else
		{
			waveformColor = baseColour.interpolatedWith(baseColour.darker(0.4f), normalizedDev);
		}
	}
	else
	{
		if (stretchRatio > 1.0f)
		{
			waveformColor = baseColour.brighter(0.6f).withSaturation(baseColour.getSaturation() * 0.7f);
		}
		else
		{
			waveformColor = baseColour.darker(0.6f).withSaturation(baseColour.getSaturation() * 0.7f);
		}
	}
}

void WaveformDisplay::drawLoopMarkers(juce::Graphics &g)
{
	float startX = timeToX(loopStart);
	float endX = timeToX(loopEnd);

	juce::Colour modelColour = getModelAccentColour();
	juce::Colour loopColour = loopPointsLocked ? ColourPalette::loopLocked : modelColour;

	drawAdsrOverlay(g, startX, endX);

	if (loopPointsLocked)
	{
		g.setColour(ColourPalette::loopLocked.withAlpha(0.22f));
		g.fillRect(startX, 0.0f, endX - startX, (float)getHeight());

		juce::Path hatchPath;
		const float spacing = 14.0f;
		const float zoneW = endX - startX;
		const float h = (float)getHeight();
		for (float offset = -h; offset < zoneW + h; offset += spacing)
		{
			hatchPath.startNewSubPath(startX + offset, h);
			hatchPath.lineTo(startX + offset + h, 0.0f);
		}
		g.setColour(ColourPalette::loopLocked.withAlpha(0.18f));
		juce::Graphics::ScopedSaveState save(g);
		g.reduceClipRegion(juce::Rectangle<float>(startX, 0.0f, zoneW, h).toNearestInt());
		g.strokePath(hatchPath, juce::PathStrokeType(1.0f));
	}

	float lineWidth = loopPointsLocked ? 3.0f : 2.0f;
	g.setColour(loopColour);
	float height = static_cast<float>(getHeight());

	g.drawLine(startX, 0.0f, startX, height, lineWidth);
	g.drawLine(endX, 0.0f, endX, height, lineWidth);

	const int markerSize = 12;
	const int scrollBarReserve = scrollBarVisible ? 4 : 0;
	const float markerBottom = height - scrollBarReserve;
	const float markerTop = markerBottom - markerSize;

	juce::Path startTriangle;
	startTriangle.addTriangle(startX, markerTop, startX, markerBottom, startX + markerSize, markerBottom);

	g.setColour(loopColour);
	g.fillPath(startTriangle);
	g.setColour(loopColour.brighter(0.3f));
	g.strokePath(startTriangle, juce::PathStrokeType(1.5f));

	juce::Path endTriangle;
	endTriangle.addTriangle(endX, markerTop, endX, markerBottom, endX - markerSize, markerBottom);

	g.setColour(loopColour);
	g.fillPath(endTriangle);
	g.setColour(loopColour.brighter(0.3f));
	g.strokePath(endTriangle, juce::PathStrokeType(1.5f));

	if (trackBpm > 0.0f)
	{
		drawLoopBarLabels(g, startX, endX);
	}
	else
	{
		drawLoopTimeLabels(g, startX, endX);
	}
}

void WaveformDisplay::drawAdsrOverlay(juce::Graphics &g, float startX, float endX)
{
	float zoneWidth = endX - startX;
	if (zoneWidth <= 0.0f)
		return;
	float h = (float)getHeight();

	double sectionDuration = loopEnd - loopStart;
	if (sectionDuration <= 0.0)
		return;

	float totalADR = adsrAttack + adsrDecay + adsrRelease;
	float scale = 1.0f;
	if (totalADR > (float)sectionDuration * 0.95f)
		scale = (float)sectionDuration * 0.95f / totalADR;

	float a = adsrAttack * scale;
	float d = adsrDecay * scale;
	float r = adsrRelease * scale;
	double releaseStart = sectionDuration - (double)r;

	float pixPerSec = zoneWidth / (float)sectionDuration;
	float attackPx = a * pixPerSec;
	float decayPx = d * pixPerSec;
	float releasePx = r * pixPerSec;
	float sustainPx = zoneWidth - attackPx - decayPx - releasePx;
	if (sustainPx < 0.0f)
		sustainPx = 0.0f;

	float margin = h * 0.08f;
	auto ampToY = [&](float amp) -> float { return h - margin - amp * (h - margin * 2.0f); };

	float x0 = startX;
	float x1 = x0 + attackPx;
	float x2 = x1 + decayPx;
	float x3 = startX + (float)(releaseStart * pixPerSec);
	float x4 = startX + zoneWidth;

	float yBottom = ampToY(0.0f);
	float yPeak = ampToY(1.0f);
	float ySustain = ampToY(adsrSustain);

	juce::Path adsrPath;
	adsrPath.startNewSubPath(x0, yBottom);
	adsrPath.lineTo(x1, yPeak);
	adsrPath.lineTo(x2, ySustain);
	adsrPath.lineTo(x3, ySustain);
	adsrPath.lineTo(x4, yBottom);

	juce::Path fillPath = adsrPath;
	fillPath.lineTo(x4, yBottom);
	fillPath.lineTo(x0, yBottom);
	fillPath.closeSubPath();

	juce::Colour modelColour = getModelAccentColour();
	g.setColour(modelColour.withAlpha(0.25f));
	g.fillPath(fillPath);
	g.setColour(modelColour.withAlpha(0.9f));
	g.strokePath(adsrPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

	float dotR = 4.0f;
	g.setColour(modelColour);
	g.fillEllipse(x1 - dotR, yPeak - dotR, dotR * 2, dotR * 2);
	g.fillEllipse(x2 - dotR, ySustain - dotR, dotR * 2, dotR * 2);
	g.fillEllipse(x3 - dotR, ySustain - dotR, dotR * 2, dotR * 2);

	g.setColour(modelColour.withAlpha(0.85f));
	g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
	int labelY = (int)(yPeak)-14;
	if (labelY < 2)
		labelY = 2;
	g.drawText("A", (int)x0, labelY, (int)attackPx, 12, juce::Justification::centred);
	g.drawText("D", (int)x1, labelY, (int)decayPx, 12, juce::Justification::centred);
	if (sustainPx > 12.0f)
		g.drawText("S", (int)x2, labelY, (int)sustainPx, 12, juce::Justification::centred);
	if (releasePx > 12.0f)
		g.drawText("R", (int)x3, labelY, (int)releasePx, 12, juce::Justification::centred);

	adsrLayout.startX = startX;
	adsrLayout.endX = endX;
	adsrLayout.x1 = x1;
	adsrLayout.x2 = x2;
	adsrLayout.x3 = x3;
	adsrLayout.yPeak = yPeak;
	adsrLayout.ySustain = ySustain;
	adsrLayout.scale = scale;
	adsrLayout.sectionDuration = sectionDuration;
	adsrLayout.valid = true;
}

void WaveformDisplay::drawLoopTimeLabels(juce::Graphics &g, float startX, float endX)
{
	g.setColour(ColourPalette::textPrimary);
	g.setFont(10.0f);
	int startTextX = static_cast<int>(startX + 2);
	int endTextX = static_cast<int>(endX - 50);
	g.drawText(juce::String(loopStart, 2) + "s", startTextX, 2, 50, 15, juce::Justification::left);
	g.drawText(juce::String(loopEnd, 2) + "s", endTextX, 2, 48, 15, juce::Justification::right);
}

void WaveformDisplay::drawLoopBarLabels(juce::Graphics &g, float startX, float endX) const
{
	g.setColour(ColourPalette::textPrimary);
	g.setFont(10.0f);
	int startTextX = static_cast<int>(startX + 5);
	int endTextX = static_cast<int>(endX - 55);
	int textY = getHeight() - 30;
	g.drawText(juce::String(loopStart, 2) + "s", startTextX, textY, 50, 15, juce::Justification::left);
	g.drawText(juce::String(loopEnd, 2) + "s", endTextX, textY, 48, 15, juce::Justification::right);
}

void WaveformDisplay::drawPlaybackHead(juce::Graphics &g)
{
	if (isCurrentlyPlaying && playbackPosition >= 0.0)
	{
		float headX = timeToX(playbackPosition);

		double viewStart = getViewStartTime();
		double viewEnd = getViewEndTime();

		if (playbackPosition >= viewStart && playbackPosition <= viewEnd && headX >= 0 && headX <= getWidth())
		{
			g.setColour(juce::Colours::white.withAlpha(0.3f));
			float height = static_cast<float>(getHeight());
			g.drawLine(headX, 0.0f, headX, height, 4.0f);

			g.setColour(juce::Colours::white.withAlpha(0.6f));
			juce::Path triangle;
			triangle.addTriangle(headX - 8, 0.0f, headX + 8, 0.0f, headX, 16.0f);
			g.fillPath(triangle);
			triangle.clear();

			triangle.addTriangle(headX - 8, height, headX + 8, height, headX, height - 16.0f);
			g.fillPath(triangle);

			g.setFont(14.0f);
			g.drawText(juce::String(playbackPosition, 2) + "s", static_cast<int>(headX - 40), getHeight() / 2 - 10, 80,
			           20, juce::Justification::centred);
		}
	}
}

float WaveformDisplay::timeToX(double time)
{
	double totalDuration = getTotalDuration();
	if (totalDuration <= 0.0)
		return 0.0f;
	double viewDuration = totalDuration / zoomFactor;
	double relativeTime = time - viewStartTime;
	return static_cast<float>(juce::jmap(relativeTime, 0.0, viewDuration, 0.0, static_cast<double>(getWidth())));
}

void WaveformDisplay::drawBeatMarkers(juce::Graphics &g)
{
	if (thumbnailLeft.empty() || thumbnailRight.empty())
		return;
	float hostBpm = getHostBpm();
	if (hostBpm <= 0.0f)
		return;

	juce::Colour modelColour = getModelAccentColour();
	int numerator = audioProcessor.getTimeSignatureNumerator();
	int denominator = audioProcessor.getTimeSignatureDenominator();
	double totalDuration = getTotalDuration();
	double viewDuration = totalDuration / zoomFactor;
	double viewEndTime = juce::jlimit(viewStartTime, totalDuration, viewStartTime + viewDuration);
	float baseBeatDuration = 60.0f / hostBpm;
	float actualBeatDuration;
	float barDuration;
	if (denominator == 8)
	{
		actualBeatDuration = baseBeatDuration * 0.5f * stretchRatio;
		barDuration = actualBeatDuration * numerator;
	}
	else
	{
		actualBeatDuration = baseBeatDuration * stretchRatio;
		barDuration = actualBeatDuration * numerator;
	}
	double measureAtLoopStart = floor(loopStart / barDuration);
	double gridOffset = loopStart - (measureAtLoopStart * barDuration);
	double extendedStart = viewStartTime - (actualBeatDuration * 50);
	double extendedEnd = viewEndTime + (actualBeatDuration * 50);
	extendedStart -= gridOffset;
	extendedEnd -= gridOffset;

	g.setColour(modelColour.withAlpha(0.9f));
	double firstBarTime = floor(extendedStart / barDuration) * barDuration;
	for (double time = firstBarTime; time <= extendedEnd; time += barDuration)
	{
		double shiftedTime = time + gridOffset;
		drawMeasureLine(shiftedTime, g, barDuration, viewDuration);
	}

	g.setColour(modelColour.withAlpha(0.55f));
	double firstBeatTime = floor(extendedStart / actualBeatDuration) * actualBeatDuration;
	for (double time = firstBeatTime; time <= extendedEnd; time += actualBeatDuration)
	{
		double shiftedTime = time + gridOffset;
		if (fmod(shiftedTime, barDuration) > 0.01)
		{
			drawBeatLine(shiftedTime, g, viewDuration);
		}
	}

	g.setColour(modelColour.withAlpha(0.4f));
	double subdivisionDuration = actualBeatDuration * 0.5f;
	double firstSubTime = floor(extendedStart / subdivisionDuration) * subdivisionDuration;
	for (double time = firstSubTime; time <= extendedEnd; time += subdivisionDuration)
	{
		double shiftedTime = time + gridOffset;
		bool isOnBeat = (fmod(shiftedTime, actualBeatDuration) < 0.01);
		bool isOnBar = (fmod(shiftedTime, barDuration) < 0.01);
		if (!isOnBeat && !isOnBar)
		{
			drawSubdivisionLine(shiftedTime, g, viewDuration);
		}
	}

	g.setColour(modelColour.withAlpha(0.25f));
	subdivisionDuration = actualBeatDuration * 0.25f;
	firstSubTime = floor(extendedStart / subdivisionDuration) * subdivisionDuration;
	for (double time = firstSubTime; time <= extendedEnd; time += subdivisionDuration)
	{
		double shiftedTime = time + gridOffset;
		bool isOnBeat = (fmod(shiftedTime, actualBeatDuration) < 0.01);
		bool isOnHalfBeat = (fmod(shiftedTime, actualBeatDuration * 0.5f) < 0.01);
		bool isOnBar = (fmod(shiftedTime, barDuration) < 0.01);
		if (!isOnBeat && !isOnHalfBeat && !isOnBar)
		{
			drawSubdivisionLine(shiftedTime, g, viewDuration);
		}
	}
}

void WaveformDisplay::drawMeasureLine(double time, juce::Graphics &g, float barDuration, double viewDuration)
{
	if (time >= viewStartTime && time <= (viewStartTime + viewDuration))
	{
		double relativeTime = time - viewStartTime;
		float x = (static_cast<float>(relativeTime) / static_cast<float>(viewDuration)) * getWidth();
		if (x >= 0 && x <= getWidth())
		{
			float height = static_cast<float>(getHeight());
			g.drawLine(x, 0.0f, x, height, 2.0f);
			int measureNumber = static_cast<int>(time / barDuration) + 1;
			g.setFont(10.0f);
			int textX = static_cast<int>(x + 2);
			g.drawText(juce::String(measureNumber), textX, 2, 30, 15, juce::Justification::left);
		}
	}
}

void WaveformDisplay::drawBeatLine(double time, juce::Graphics &g, double viewDuration)
{
	if (time >= viewStartTime && time <= (viewStartTime + viewDuration))
	{
		double relativeTime = time - viewStartTime;
		float x = static_cast<float>((relativeTime / viewDuration) * getWidth());
		if (x >= 0 && x <= getWidth())
		{
			g.drawLine(x, 0.0f, x, static_cast<float>(getHeight()), 1.0f);
		}
	}
}

void WaveformDisplay::drawSubdivisionLine(double time, juce::Graphics &g, double viewDuration)
{
	if (time >= viewStartTime && time <= (viewStartTime + viewDuration))
	{
		double relativeTime = time - viewStartTime;
		float x = static_cast<float>((relativeTime / viewDuration) * getWidth());
		if (x >= 0 && x <= getWidth())
		{
			g.drawLine(x, getHeight() * 0.2f, x, getHeight() * 0.8f, 0.5f);
		}
	}
}

double WaveformDisplay::xToTime(float x)
{
	double totalDuration = getTotalDuration();
	if (totalDuration <= 0.0)
	{
		return 0.0;
	}

	double viewDuration = totalDuration / zoomFactor;
	if (viewDuration <= 0.0)
	{
		return 0.0;
	}

	double relativeTime = juce::jmap(static_cast<double>(x), 0.0, static_cast<double>(getWidth()), 0.0, viewDuration);
	double result = viewStartTime + relativeTime;

	return juce::jlimit(0.0, totalDuration, result);
}

double WaveformDisplay::getTotalDuration() const
{
	if (audioBuffer.getNumSamples() == 0 || sampleRate <= 0)
		return 0.0;

	return audioBuffer.getNumSamples() / sampleRate;
}

double WaveformDisplay::getViewStartTime() const
{
	return viewStartTime;
}

double WaveformDisplay::getViewEndTime() const
{
	return juce::jlimit(viewStartTime, getTotalDuration(), viewStartTime + (getTotalDuration() / zoomFactor));
}

void WaveformDisplay::setAdsrParams(float attack, float decay, float sustain, float release)
{
	adsrAttack = attack;
	adsrDecay = decay;
	adsrSustain = sustain;
	adsrRelease = release;
	repaint();
}

juce::Colour WaveformDisplay::getModelAccentColour() const
{
	if (!track)
		ColourPalette::getModelColourByIndex(0);
	juce::String modelName;

	modelName = track->getCurrentPage().selectedModel;

	if (modelName.isEmpty())
		return ColourPalette::buttonPrimary;
	auto &models = AiModelDefinitions::getAvailableModels();
	int modelIndex = models.indexOf(modelName);

	if (modelIndex < 0)
		return ColourPalette::buttonPrimary;

	return ColourPalette::getModelColourByIndex(modelIndex);
}