#pragma once
#include "GlitchTypes.h"
#include <JuceHeader.h>

struct TrackData;
class SequencerManager;

class GlitchSequencerEngine
{
  public:
	void setSequencerManager(SequencerManager *manager)
	{
		sequencerManager = manager;
	}

	void setSequencerActive(TrackData &track, bool shouldBeActive);
	void processBlock(TrackData &track, double ppqPosition, double hostBpm, double sampleRate, int numSamples);
	void refreshMasksAfterEdit(TrackData &track);

	static int effectBit(GlitchEffectType type)
	{
		return 1 << static_cast<int>(type);
	}

	static int rebuildOwnedEffectsMask(const GlitchSequence &seq);

	static void setUserFxEnabled(TrackData &track, GlitchEffectType type, bool enabled);
	static void syncUserMaskFromDsp(TrackData &track);
	static void applyEffectiveBypasses(TrackData &track);

  private:
	static constexpr int kBypassableMask =
	    (1 << static_cast<int>(GlitchEffectType::Gate)) | (1 << static_cast<int>(GlitchEffectType::Filter)) |
	    (1 << static_cast<int>(GlitchEffectType::Chorus)) | (1 << static_cast<int>(GlitchEffectType::Phaser)) |
	    (1 << static_cast<int>(GlitchEffectType::Flanger)) | (1 << static_cast<int>(GlitchEffectType::BitCrusher)) |
	    (1 << static_cast<int>(GlitchEffectType::Distortion));

	GlitchEffectType pickRandomEffect(TrackData &track);
	void triggerStep(TrackData &track, GlitchEffectType type, double hostBpm);
	void startBeatRepeat(TrackData &track, double hostBpm);
	void stopBeatRepeat(TrackData &track);
	void applyMetaStep(TrackData &, int);
	void advanceStepIfNeeded(TrackData &, GlitchSequence &, int, double);
	void clearTransportEffects(TrackData &track, int mask);
	void forceMix(TrackData &track, GlitchEffectType type);
	void restoreMix(TrackData &track, GlitchEffectType type);
	int computeReservedMask(TrackData &track);

	SequencerManager *sequencerManager = nullptr;
};