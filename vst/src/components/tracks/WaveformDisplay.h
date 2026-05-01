#pragma once
#include "components/ObsidianBase.h"
class DjIaVstProcessor;
struct TrackData;
class WaveformDisplay : public ObsidianComponent, public juce::ScrollBar::Listener, public juce::DragAndDropContainer
{
public:
	WaveformDisplay(DjIaVstProcessor& processor, TrackData& trackData);
	~WaveformDisplay();
	std::function<void(double, double)> onLoopPointsChanged;
	std::function<void(float)> onAdsrAttackChanged;
	std::function<void(float)> onAdsrDecayChanged;
	std::function<void(float)> onAdsrSustainChanged;
	std::function<void(float)> onAdsrReleaseChanged;

	void mouseDoubleClick(const juce::MouseEvent& e) override;
	void setOriginalBpm(float bpm);
	void setSampleBpm(float bpm);
	void lockLoopPoints(bool locked);
	void setPlaybackPosition(double timeInSeconds, bool isPlaying);
	void setAudioData(const juce::AudioBuffer<float>& newAudioBuffer, double newSampleRate);
	void setLoopPoints(double startTime, double endTime);
	void setAudioFile(const juce::File& file);
	void setAdsrParams(float attack, float decay, float sustain, float release);
private:
	juce::AudioBuffer<float> audioBuffer;
	juce::File currentAudioFile;
	juce::Point<int> dragStartPosition;
	std::unique_ptr<juce::ScrollBar> horizontalScrollBar;
	DjIaVstProcessor& audioProcessor;
	TrackData& track;
	std::vector<float> thumbnailLeft;
	std::vector<float> thumbnailRight;
	double loopStart = 0.0;
	double loopEnd = 4.0;
	double sampleRate = 48000.0;
	double zoomFactor = 1.0;
	double viewStartTime = 0.0;
	double playbackPosition = 0.0;
	bool scrollBarVisible = false;
	bool loopPointsLocked = false;
	bool draggingStart = false;
	bool draggingEnd = false;
	bool isDraggingAudio = false;
	bool isCurrentlyPlaying = false;
	float trackBpm = 126.0f;
	float sampleBpm = 126.0f;
	float stretchRatio = 1.0f;
	float originalBpm = 126.0f;
	float timeStretchRatio = 1.0f;

	juce::Image waveformCache;
	juce::Image gridCache;
	bool waveformCacheDirty = true;
	bool gridCacheDirty = true;
	float lastPlaybackHeadX = -1.0f;
	juce::Colour cachedModelColour;
	float cachedStretchRatioForColour = 1.0f;

	float adsrAttack = 0.01f;
	float adsrDecay = 4.0f;
	float adsrSustain = 1.0f;
	float adsrRelease = 0.0f;

	enum class AdsrHandle { None, AttackPeak, DecaySustain, ReleaseStart };

	struct AdsrLayout
	{
		float startX = 0.0f, endX = 0.0f;
		float x1 = 0.0f, x2 = 0.0f, x3 = 0.0f;
		float yPeak = 0.0f, ySustain = 0.0f;
		float scale = 1.0f;
		double sectionDuration = 0.0;
		bool valid = false;
	};

	AdsrLayout adsrLayout;
	AdsrHandle activeHandle = AdsrHandle::None;

	AdsrHandle hitTestAdsr(juce::Point<float> p) const;
	void updateAdsrFromMouse(juce::Point<float> p);

	void drawAdsrOverlay(juce::Graphics& g, float startX, float endX);
	void invalidateAllCaches();
	void invalidateWaveformCache();
	void invalidateGridCache();
	void rebuildWaveformCache();
	void rebuildGridCache();
	void repaintPlaybackHeadRegion(float oldX, float newX);
	void renderWaveformInto(juce::Graphics& g);
	void renderGridInto(juce::Graphics& g);
	float getHostBpm() const;
	float timeToX(double time);
	void generateThumbnail();
	void feedThumbnailStereo(int startSample, int point, int samplesPerPoint, int& retFlag);
	void drawWaveform(juce::Graphics& g);
	void setColorDependingTimeStretchRatio(juce::Colour& waveformColor) const;
	void drawLoopMarkers(juce::Graphics& g);
	void drawLoopTimeLabels(juce::Graphics& g, float startX, float endX);
	void drawLoopBarLabels(juce::Graphics& g, float startX, float endX) const;
	void drawPlaybackHead(juce::Graphics& g);
	void drawBeatMarkers(juce::Graphics& g);
	void calculateStretchRatio() const;
	void updateScrollBarVisibility();
	void updateScrollBar();
	void setViewStartTime(double newViewStartTime);
	void drawMeasureLine(double time, juce::Graphics& g, float barDuration, double viewDuration);
	void drawBeatLine(double time, juce::Graphics& g, double viewDuration);
	void drawSubdivisionLine(double time, juce::Graphics& g, double viewDuration);
	void paint(juce::Graphics& g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
	void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
	juce::Colour getModelAccentColour() const;
	double xToTime(float x);
	double getTotalDuration() const;
	double getViewStartTime() const;
	double getViewEndTime() const;
};