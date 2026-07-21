#pragma once
#include <JuceHeader.h>

#ifndef OBSIDIAN_DATA_H
#define OBSIDIAN_DATA_H

namespace Obsidian
{
inline constexpr int MAX_STEPS_PER_MEASURE = 32;
inline constexpr int MAX_MEASURES = 4;
inline constexpr int MAX_PAGES = 4;
inline constexpr int MAX_TRACKS = 8;
inline constexpr int MAX_SEQUENCES = 8;
inline constexpr int MAX_CROSSFADER_PAIR = 4;
inline constexpr int MAX_BLOCK_SIZE = 512;
inline constexpr int RNDM_RTRGR_INTRVL = 3;
inline constexpr int SAFETY_FADE_LENGTH = 512;

inline constexpr int ASPECT_W = 27;
inline constexpr int ASPECT_H = 14;

inline constexpr int heightForWidth(int w)
{
	return w * ASPECT_H / ASPECT_W;
}

inline constexpr int BASE_PLUGIN_WIDTH = 1620;
inline constexpr int BASE_PLUGIN_HEIGHT = heightForWidth(BASE_PLUGIN_WIDTH);

inline constexpr int MIN_PLUGIN_WIDTH = 1080;
inline constexpr int MIN_PLUGIN_HEIGHT = heightForWidth(MIN_PLUGIN_WIDTH);

inline constexpr int MAX_PLUGIN_WIDTH = 3240;
inline constexpr int MAX_PLUGIN_HEIGHT = heightForWidth(MAX_PLUGIN_WIDTH);

inline constexpr double ASPECT_RATIO = (double)ASPECT_W / (double)ASPECT_H;

inline constexpr int BLINKING_DURATION_TIME = 350;

inline constexpr double SAMPLERATE = 48000.0;

inline constexpr float COMPRESSOR_THRESHOLD = -12.f;
inline constexpr float COMPRESSOR_RATIO = 4.f;
inline constexpr float COMPRESSOR_ATTACK = 10.f;
inline constexpr float COMPRESSOR_RELEASE = 100.f;
inline constexpr float COMPRESSOR_MAKEUP_GAIN = 1.f;

inline constexpr float CHORUS_RATE = 1.5f;
inline constexpr float CHORUS_DEPTH = 0.25f;
inline constexpr float CHORUS_CENTRE = 7.f;
inline constexpr float CHORUS_FEEDBACK = 0.f;
inline constexpr float CHORUS_MIX = 0.f;

inline constexpr float PHASER_RATE = .2f;
inline constexpr float PHASER_DEPTH = .7f;
inline constexpr float PHASER_CENTRE = 800.f;
inline constexpr float PHASER_FEEDBACK = .6f;
inline constexpr float PHASER_MIX = 0.f;

inline constexpr float FLANGER_RATE = .3f;
inline constexpr float FLANGER_DEPTH = 0.8f;
inline constexpr float FLANGER_CENTRE = 2.f;
inline constexpr float FLANGER_FEEDBACK = .6f;
inline constexpr float FLANGER_MIX = 0.f;

inline constexpr float BITCRUSHER_BIT_DEPTH = 8.f;
inline constexpr float BITCRUSHER_SAMPLE_RATE_REDUCTION = 4.f;
inline constexpr float BITCRUSHER_MIX = 0.f;

inline constexpr float DISTORTION_PRE = 0.f;
inline constexpr float DISTORTION_POST = 0.f;
inline constexpr float DISTORTION_CUT = 1000.f;

inline constexpr float FILTER_DRIVE = 1.f;
inline constexpr float FILTER_CUT = 20000.0f;
inline constexpr float FILTER_RES = 0.f;
inline constexpr float TRIM_THRESHOLD = 0.08f;
inline constexpr int FILTER_MODE = 0;

inline constexpr float LIMITER_THRESHOLD = -3.f;
inline constexpr float LIMITER_RELEASE = 50.f;
inline constexpr float LIMITER_MAKEUP_GAIN = 1.f;

inline constexpr float EQ_BANDS_GAIN = 1.f;
inline constexpr float EQ_SUB_BAS_FRQ = 40.f;
inline constexpr float EQ_BASS_FRQ = 120.f;
inline constexpr float EQ_LOW_MID_FRQ = 350.f;
inline constexpr float EQ_MID_FRQ = 1000.f;
inline constexpr float EQ_HI_MID_FRQ = 3000.f;
inline constexpr float EQ_PRESENCE_FRQ = 5000.f;
inline constexpr float EQ_HI_FRQ = 8000.f;
inline constexpr float EQ_AIR_FRQ = 15000.f;
inline constexpr float EQ_BASE_RESONANCE = 0.707f;

inline constexpr bool COMPRESSOR_BYPASSED = false;
inline constexpr bool LIMITER_BYPASSED = false;
inline constexpr bool EQ_BYPASSED = false;
inline constexpr bool FILTER_BYPASSED = true;
inline constexpr bool DISTORTION_BYPASSED = true;
inline constexpr bool CHORUS_BYPASSED = true;
inline constexpr bool PHASER_BYPASSED = true;
inline constexpr bool FLANGER_BYPASSED = true;
inline constexpr bool BITCRUSHER_BYPASSED = true;

inline static const std::string STABLE_AUDIO_OPEN_V1 = "stable-audio-open-1.0";
inline static const std::string STABLE_AUDIO_OPEN_V3_MEDIUM = "stable-audio-3-medium";
inline static const std::string FOUNDATION_1 = "foundation-1";
inline static const std::string AUDIOLAB_EDM = "audialab-edm-elements";
inline static const std::string INFINITE_PIANO = "rc-infinite-pianos";
inline static const std::string RC_VOCAL = "rc-vocal-textures";
inline static const std::string SAO_INSTRUMENTAL = "sao-instrumental";
inline static const std::string STABLEBEAT = "stablebeat";
inline static const std::string GLUTEN_V1 = "gluten-v1";
inline static const std::string STABLE_AUDIO_OPEN_V1_TFLITE = "stable-audio-open-small-tflite";

inline static const std::string OBSIDIAN_BASE_DIR = "OBSIDIAN-Neural";
inline static const std::string EXPORTS_DIR = "OBSIDIAN_Exports";
inline static const std::string SAMPLE_BANK_DIR = "SampleBank";
inline static const std::string SESSIONS_DIR = "Sessions";
inline static const std::string STABLE_AUDIO_DIR = "stable-audio";
inline static const std::string CATEGORIES_FILE = "categories.json";
inline static const std::string GLOBAL_CONFIG_FILE = "global_config.json";
inline static const std::string PROMPTS_FILE = "prompts.json";
inline static const std::string AUDIO_CACHE_DIR = "AudioCache";
inline static const std::string FORKS_FILE = "session.forks";
inline static const std::string MAGIC = "OBSIDIAN";
inline static const std::string LINEAGE_FILE = "session.lineage";

struct ADSRDefaultValues
{
	static constexpr float ATTACK_DEFAULT = 0.001f;
	static constexpr float ATTACK_MIN = 0.001f;
	static constexpr float ATTACK_MAX = 4.f;

	static constexpr float DECAY_DEFAULT = 4.f;
	static constexpr float DECAY_MIN = 0.001f;
	static constexpr float DECAY_MAX = 4.f;

	static constexpr float SUSTAIN_DEFAULT = 1.f;
	static constexpr float SUSTAIN_MIN = 0.f;
	static constexpr float SUSTAIN_MAX = 1.f;

	static constexpr float RELEASE_DEFAULT = 0.001f;
	static constexpr float RELEASE_MIN = 0.001f;
	static constexpr float RELEASE_MAX = 4.f;
};

enum RadioGroupIDs
{
	FilterTypeGroup = 1,
	DelayDivisionGroup = 2,
	DelayModeGroup = 3,
	TrackFXSelector = 4,
	DistortionType = 5
};

enum eqBands
{
	subBass = 0,
	bass = 1,
	lowMid = 2,
	mid = 3,
	highMid = 4,
	presence = 5,
	high = 6,
	air = 7
};

enum filterType
{
	lowShelf = 0,
	peakFilter = 1,
	highShelf = 2,
};

enum distortionChain
{
	filter = 0,
	preGain = 1,
	waveshaper = 2,
	postGain = 3
};

enum compressorChain
{
	compressor = 0,
	makeUpGain = 1
};

enum limiterChain
{
	limiter = 0,
	limiterGain = 1
};

enum chorusChain
{
	chorus = 0,
};

enum distortionType
{
	soft = 0,
	hard = 1,
	tube = 2,
	fold = 3,
	diode = 4,
	cubic = 5
};

enum phaserChain
{
	phaser = 0,
};

enum flangerChain
{
	flanger = 0,
};

} // namespace Obsidian
#endif