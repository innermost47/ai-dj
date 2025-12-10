#include "JuceHeader.h"
#include "WaveformDisplay.h"
#include "PluginProcessor.h"
#include "TrackData.h"
#include "ColourPalette.h" 

WaveformDisplay::WaveformDisplay(DjIaVstProcessor& processor, TrackData& trackData) : audioProcessor(processor), track(trackData)
{
	setSize(400, 80);

	zoomFactor = 1.0;
	viewStartTime = 0.0;
	sampleRate = 48000.0;
	loopPointsLocked = track.loopPointsLocked.load();
	horizontalScrollBar = std::make_unique<juce::ScrollBar>(false);
	horizontalScrollBar->setRangeLimits(0.0, 1.0);
	horizontalScrollBar->addListener(this);
}

WaveformDisplay::~WaveformDisplay()
{
}

void WaveformDisplay::setSampleBpm(float bpm)
{
	sampleBpm = bpm;
	calculateStretchRatio();
	juce::MessageManager::callAsync([this]()
		{ repaint(); });
}

void WaveformDisplay::setOriginalBpm(float bpm)
{
	originalBpm = bpm;
}

void WaveformDisplay::setAudioData(const juce::AudioBuffer<float>& newAudioBuffer, double newSampleRate)
{
	jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

	if (newAudioBuffer.getNumChannels() == 0 || newAudioBuffer.getNumSamples() == 0)
	{
		audioBuffer.setSize(0, 0);
		sampleRate = newSampleRate;
		thumbnail.clear();
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
		repaint();
	}
	catch (const std::exception& /*e*/)
	{
		audioBuffer.setSize(0, 0);
		sampleRate = newSampleRate;
		thumbnail.clear();
		repaint();
	}
}

void WaveformDisplay::setLoopPoints(double startTime, double endTime)
{
	loopStart = startTime;
	loopEnd = endTime;
	juce::MessageManager::callAsync([this]()
		{ repaint(); });
}

void WaveformDisplay::lockLoopPoints(bool locked)
{
	loopPointsLocked = locked;
	juce::MessageManager::callAsync([this]()
		{ repaint(); });
}

void WaveformDisplay::calculateStretchRatio() const
{
	if (originalBpm > 0.0f && sampleBpm > 0.0f)
	{
		const_cast<WaveformDisplay*>(this)->stretchRatio = sampleBpm / originalBpm;
	}
	else
	{
		const_cast<WaveformDisplay*>(this)->stretchRatio = 1.0f;
	}
}

void WaveformDisplay::setPlaybackPosition(double timeInSeconds, bool isPlaying)
{
	playbackPosition = timeInSeconds;
	isCurrentlyPlaying = isPlaying;

	juce::MessageManager::callAsync([this]()
		{ repaint(); });
}

void WaveformDisplay::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds();

	g.setColour(ColourPalette::backgroundMid);
	g.fillRect(bounds);

	if (thumbnail.empty())
	{
		g.setColour(ColourPalette::textSecondary);
		g.setFont(12.0f);
		g.drawText("No audio data", bounds.reduced(5).removeFromTop(20), juce::Justification::centred);

		g.setColour(ColourPalette::textSecondary);
		g.setFont(10.0f);
		g.drawText("Ctrl+Wheel: Zoom | Wheel: Scroll | Right-click: Lock/Unlock | Ctrl+Click: Drag and Drop in DAW",
			bounds.reduced(5).removeFromBottom(15), juce::Justification::centred);
		return;
	}

	drawWaveform(g);
	drawLoopMarkers(g);
	drawBeatMarkers(g);
	drawPlaybackHead(g);

	drawVisibleBarLabels(g);

	if (zoomFactor > 1.0)
	{
		g.setColour(ColourPalette::buttonWarning);
		g.setFont(10.0f);
		g.drawText("Zoom: " + juce::String(zoomFactor, 1) + "x", 5, getHeight() - 20, 60, 15, juce::Justification::left);
	}

	if (loopPointsLocked)
	{
		g.setColour(ColourPalette::muteActive);
		g.setFont(10.0f);
		g.drawText("LOCKED", getWidth() - 60, getHeight() - 20, 55, 15, juce::Justification::right);
	}
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		loopPointsLocked = !track.loopPointsLocked.load();
		track.loopPointsLocked.store(loopPointsLocked);
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

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
	if (!draggingStart && !draggingEnd && currentAudioFile.exists() && e.mods.isCtrlDown())
	{
		auto distanceFromStart = e.getDistanceFromDragStart();
		if (distanceFromStart > 10 && !isDraggingAudio)
		{
			isDraggingAudio = true;

			juce::File exportedFile = audioProcessor.exportSampleForDragDrop(currentAudioFile);

			if (exportedFile.existsAsFile())
			{
				juce::StringArray files;
				files.add(exportedFile.getFullPathName());
				performExternalDragDropOfFiles(files, false);

				DBG("Dragging exported copy: " + exportedFile.getFullPathName());
			}
			else
			{
				juce::StringArray files;
				files.add(currentAudioFile.getFullPathName());
				performExternalDragDropOfFiles(files, false);

				DBG("Export failed, dragging original file");
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
			loopStart = juce::jlimit(getViewStartTime(), loopEnd, newStart);
			repaint();
			if (onLoopPointsChanged)
			{
				onLoopPointsChanged(loopStart, loopEnd);
			}
		}
		else if (draggingEnd)
		{
			double newEnd = xToTime(static_cast<float>(e.x));
			loopEnd = juce::jlimit(loopStart, getViewEndTime(), newEnd);
			repaint();
			if (onLoopPointsChanged)
			{
				onLoopPointsChanged(loopStart, loopEnd);
			}
		}
	}
}

double WaveformDisplay::getMinLoopDuration() const
{
	if (trackBpm <= 0.0f)
		return 1.0;

	int numerator = audioProcessor.getTimeSignatureNumerator();

	double beatDuration = 60.0 / trackBpm;
	return beatDuration * numerator;
}

void WaveformDisplay::setAudioFile(const juce::File& file)
{
	currentAudioFile = file;
}

void WaveformDisplay::mouseUp(const juce::MouseEvent& /*e*/)
{
	draggingStart = false;
	draggingEnd = false;
	isDraggingAudio = false;
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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
		repaint();
	}
	else if (zoomFactor > 1.0)
	{
		double viewDuration = getTotalDuration() / zoomFactor;
		double scrollAmount = wheel.deltaY * viewDuration * 0.1;

		setViewStartTime(viewStartTime - scrollAmount);

		updateScrollBar();
		generateThumbnail();
		repaint();
	}
}

void WaveformDisplay::updateScrollBarVisibility()
{
	bool shouldShow = (zoomFactor > 1.0);

	if (shouldShow && !scrollBarVisible)
	{
		addAndMakeVisible(*horizontalScrollBar);
		horizontalScrollBar->setBounds(0, getHeight() - 8, getWidth(), 12);
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
		horizontalScrollBar->setBounds(0, getHeight() - 8, getWidth(), 12);
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

void WaveformDisplay::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	if (scrollBarThatHasMoved == horizontalScrollBar.get())
	{
		double totalDuration = getTotalDuration();
		double newViewStartTime = newRangeStart * totalDuration;

		setViewStartTime(newViewStartTime);
		generateThumbnail();
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
	thumbnail.clear();

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
		feedThumbnail(startSample, point, samplesPerPoint, retFlag);
		if (retFlag == 2)
			break;
	}
}

void WaveformDisplay::feedThumbnail(int startSample, int point, int samplesPerPoint, int& retFlag)
{
	retFlag = 1;
	int sampleStart = startSample + (point * samplesPerPoint);
	int sampleEnd = std::min(sampleStart + samplesPerPoint, audioBuffer.getNumSamples());
	if (sampleStart >= audioBuffer.getNumSamples())
	{
		{
			retFlag = 2;
			return;
		};
	}
	float rmsSum = 0.0f;
	float peak = 0.0f;
	int count = 0;

	for (int sample = sampleStart; sample < sampleEnd; ++sample)
	{

		if (sample >= audioBuffer.getNumSamples())
		{
			break;
		}
		for (int ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
		{
			float val = audioBuffer.getSample(ch, sample);
			rmsSum += val * val;
			peak = std::max(peak, std::abs(val));
			count++;
		}
	}

	float rms = count > 0 ? std::sqrt(rmsSum / count) : 0.0f;
	float finalValue = (rms * 0.7f) + (peak * 0.3f);

	thumbnail.push_back(finalValue);
}

void WaveformDisplay::drawWaveform(juce::Graphics& g)
{
	if (thumbnail.empty())
		return;

	juce::Colour waveformColor;
	setColorDependingTimeStretchRatio(waveformColor);

	g.setColour(waveformColor);

	juce::Path waveformPath;
	bool pathStarted = false;

	size_t thumbnailSize = thumbnail.size();
	float pixelsPerPoint = static_cast<float>(getWidth()) / static_cast<float>(thumbnailSize);

	for (int i = 0; i < thumbnailSize; ++i)
	{
		generateTopHalfPath(i, pixelsPerPoint, pathStarted, waveformPath, static_cast<int>(thumbnailSize));
	}

	g.strokePath(waveformPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

	juce::Path bottomPath;
	pathStarted = false;

	for (int i = 0; i < thumbnailSize; ++i)
	{
		generateBottomHalfPath(i, pixelsPerPoint, pathStarted, bottomPath, static_cast<int>(thumbnailSize));
	}

	g.strokePath(bottomPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	float centerY = getHeight() * 0.5f;
	float width = static_cast<float>(getWidth());
	g.drawLine(0.0f, centerY, width, centerY, 0.5f);
}

void WaveformDisplay::setColorDependingTimeStretchRatio(juce::Colour& waveformColor) const
{
	float deviation = std::abs(stretchRatio - 1.0f);

	if (deviation < 0.005f)
	{
		waveformColor = ColourPalette::buttonPrimary;
	}
	else if (deviation < 0.08f)
	{
		float normalizedDev = juce::jlimit(0.0f, 1.0f, (deviation - 0.005f) / 0.075f);

		if (stretchRatio > 1.0f)
		{
			waveformColor = ColourPalette::buttonPrimary.interpolatedWith(
				ColourPalette::buttonDangerLight, normalizedDev);
		}
		else
		{
			waveformColor = ColourPalette::buttonPrimary.interpolatedWith(
				ColourPalette::buttonDangerDark, normalizedDev);
		}
	}
	else
	{
		if (stretchRatio > 1.0f)
		{
			waveformColor = juce::Colour(0xffE6A5A5);
		}
		else
		{
			waveformColor = juce::Colour(0xff6B3535);
		}
	}
}

void WaveformDisplay::generateBottomHalfPath(int i, float pixelsPerPoint, bool& pathStarted, juce::Path& bottomPath, int thumbnailSize)
{
	float x = i * pixelsPerPoint;
	float amplitude = thumbnail[i];

	float centerY = getHeight() * 0.5f;
	float waveHeight = amplitude * centerY * 0.8f;
	float bottomY = centerY + waveHeight;

	if (!pathStarted)
	{
		bottomPath.startNewSubPath(x, centerY);
		pathStarted = true;
	}

	if (i > 0 && i < thumbnailSize - 1)
	{
		float prevX = (i - 1) * pixelsPerPoint;
		float nextX = (i + 1) * pixelsPerPoint;
		float controlX = (prevX + nextX) * 0.5f;
		bottomPath.quadraticTo(controlX, bottomY, x, bottomY);
	}
	else
	{
		bottomPath.lineTo(x, bottomY);
	}
}

void WaveformDisplay::generateTopHalfPath(int i, float pixelsPerPoint, bool& pathStarted, juce::Path& waveformPath, int thumbnailSize)
{
	float x = i * pixelsPerPoint;
	float amplitude = thumbnail[i];

	float centerY = getHeight() * 0.5f;
	float waveHeight = amplitude * centerY * 0.8f;

	float topY = centerY - waveHeight;

	if (!pathStarted)
	{
		waveformPath.startNewSubPath(x, centerY);
		pathStarted = true;
	}
	if (i > 0 && i < thumbnailSize - 1)
	{
		float prevX = (i - 1) * pixelsPerPoint;
		float nextX = (i + 1) * pixelsPerPoint;

		float controlX = (prevX + nextX) * 0.5f;
		waveformPath.quadraticTo(controlX, topY, x, topY);
	}
	else
	{
		waveformPath.lineTo(x, topY);
	}
}

void WaveformDisplay::drawLoopMarkers(juce::Graphics& g)
{
	float startX = timeToX(loopStart);
	float endX = timeToX(loopEnd);

	juce::Colour loopColour = loopPointsLocked ? ColourPalette::textAccent : ColourPalette::buttonPrimary;

	if (loopPointsLocked)
	{
		g.setColour(loopColour.darker(0.3f).withAlpha(0.5f));
	}
	else
	{
		g.setColour(loopColour.withAlpha(0.15f));
	}
	g.fillRect(startX, 0.0f, endX - startX, (float)getHeight());

	float lineWidth = loopPointsLocked ? 3.0f : 2.0f;
	g.setColour(loopColour);
	float height = static_cast<float>(getHeight());

	g.drawLine(startX, 0.0f, startX, height, lineWidth);
	g.drawLine(endX, 0.0f, endX, height, lineWidth);

	int triangleSize = 12;

	juce::Path startTriangle;
	startTriangle.addTriangle(
		startX, 0.0f,
		startX, static_cast<float>(triangleSize),
		startX + triangleSize, triangleSize / 2.0f
	);

	g.setColour(loopColour);
	g.fillPath(startTriangle);
	g.setColour(loopColour.brighter(0.3f));
	g.strokePath(startTriangle, juce::PathStrokeType(1.5f));

	juce::Path endTriangle;
	endTriangle.addTriangle(
		endX, 0.0f,
		endX, static_cast<float>(triangleSize),
		endX - triangleSize, triangleSize / 2.0f
	);

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

void WaveformDisplay::drawLoopTimeLabels(juce::Graphics& g, float startX, float endX)
{
	g.setColour(ColourPalette::textPrimary);
	g.setFont(10.0f);
	int startTextX = static_cast<int>(startX + 2);
	int endTextX = static_cast<int>(endX - 50);
	g.drawText(juce::String(loopStart, 2) + "s", startTextX, 2, 50, 15,
		juce::Justification::left);
	g.drawText(juce::String(loopEnd, 2) + "s", endTextX, 2, 48, 15,
		juce::Justification::right);
}

void WaveformDisplay::drawLoopBarLabels(juce::Graphics& g, float startX, float endX) const
{
	g.setColour(ColourPalette::textPrimary);
	g.setFont(10.0f);
	int startTextX = static_cast<int>(startX + 5);
	int endTextX = static_cast<int>(endX - 55);
	int textY = getHeight() - 30;
	g.drawText(juce::String(loopStart, 2) + "s", startTextX, textY, 50, 15,
		juce::Justification::left);
	g.drawText(juce::String(loopEnd, 2) + "s", endTextX, textY, 48, 15,
		juce::Justification::right);
}

void WaveformDisplay::drawVisibleBarLabels(juce::Graphics& g)
{
	if (trackBpm <= 0.0f)
		return;

	int numerator = audioProcessor.getTimeSignatureNumerator();

	float effectiveBpm = trackBpm;
	if (stretchRatio > 0.0f && stretchRatio != 1.0f)
	{
		effectiveBpm = trackBpm * stretchRatio;
	}

	double beatDuration = 60.0 / effectiveBpm;
	double barDuration = beatDuration * numerator;

	double viewStart = getViewStartTime();
	double viewEnd = getViewEndTime();

	int leftBar = (int)(viewStart / barDuration) + 1;
	int rightBar = (int)(viewEnd / barDuration) + 1;

	if (fmod(viewEnd, barDuration) < 0.01)
	{
		rightBar--;
	}

	int visibleBars = rightBar - leftBar + 1;
	if (visibleBars > 1)
	{
		g.setColour(ColourPalette::textSecondary);
		g.setFont(10.0f);
		g.drawText("(" + juce::String(visibleBars) + " bars visible)",
			getWidth() / 2 - 40, 2, 80, 15, juce::Justification::centred);
	}
}

void WaveformDisplay::drawPlaybackHead(juce::Graphics& g)
{
	if (isCurrentlyPlaying && playbackPosition >= 0.0)
	{
		float headX = timeToX(playbackPosition);

		double viewStart = getViewStartTime();
		double viewEnd = getViewEndTime();

		if (playbackPosition >= viewStart && playbackPosition <= viewEnd && headX >= 0 && headX <= getWidth())
		{
			g.setColour(ColourPalette::playArmed);
			float height = static_cast<float>(getHeight());
			g.drawLine(headX, 0.0f, headX, height, 4.0f);

			juce::Path triangle;
			triangle.addTriangle(headX - 8, 0.0f, headX + 8, 0.0f, headX, 16.0f);
			g.setColour(ColourPalette::buttonDangerLight);
			g.fillPath(triangle);
			triangle.clear();

			triangle.addTriangle(headX - 8, height, headX + 8, height, headX, height - 16.0f);
			g.fillPath(triangle);

			g.setColour(ColourPalette::textPrimary);
			g.setFont(14.0f);
			g.drawText(juce::String(playbackPosition, 2) + "s",
				static_cast<int>(headX - 40),
				getHeight() / 2 - 10,
				80, 20,
				juce::Justification::centred);
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

void WaveformDisplay::drawBeatMarkers(juce::Graphics& g)
{
	if (thumbnail.empty())
		return;
	float hostBpm = getHostBpm();
	if (hostBpm <= 0.0f)
		return;
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

	g.setColour(ColourPalette::sequencerAccent.withAlpha(0.9f));
	double firstBarTime = floor(extendedStart / barDuration) * barDuration;
	for (double time = firstBarTime; time <= extendedEnd; time += barDuration)
	{
		double shiftedTime = time + gridOffset;
		drawMeasureLine(shiftedTime, g, barDuration, viewDuration);
	}

	g.setColour(ColourPalette::sequencerAccent.withAlpha(0.6f));
	double firstBeatTime = floor(extendedStart / actualBeatDuration) * actualBeatDuration;
	for (double time = firstBeatTime; time <= extendedEnd; time += actualBeatDuration)
	{
		double shiftedTime = time + gridOffset;
		if (fmod(shiftedTime, barDuration) > 0.01)
		{
			drawBeatLine(shiftedTime, g, viewDuration);
		}
	}

	g.setColour(ColourPalette::sequencerAccent.withAlpha(0.5f));
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

	g.setColour(ColourPalette::sequencerAccent.withAlpha(0.35f));
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

void WaveformDisplay::drawMeasureLine(double time, juce::Graphics& g, float barDuration, double viewDuration)
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

void WaveformDisplay::drawBeatLine(double time, juce::Graphics& g, double viewDuration)
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

void WaveformDisplay::drawSubdivisionLine(double time, juce::Graphics& g, double viewDuration)
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

void WaveformDisplay::drawBeats(juce::Graphics& g, float beatDuration, float viewEndTime, float barDuration, double viewDuration)
{
	g.setColour(ColourPalette::sequencerBeat.withAlpha(0.4f));
	double firstBeatTime = floor(viewStartTime / beatDuration) * beatDuration;
	for (double time = firstBeatTime; time <= viewEndTime; time += beatDuration)
	{
		if (fmod(time, barDuration) > 0.01)
		{
			if (time >= viewStartTime)
			{
				double relativeTime = time - viewStartTime;
				float x = static_cast<float>((relativeTime / viewDuration) * getWidth());
				if (x >= 0 && x <= getWidth())
				{
					g.drawLine(x, 0.0f, x, static_cast<float>(getHeight()), 1.0f);
				}
			}
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
	return juce::jlimit(viewStartTime, getTotalDuration(),
		viewStartTime + (getTotalDuration() / zoomFactor));
}