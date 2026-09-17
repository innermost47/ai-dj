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

inline constexpr float GATE_DURATION = .2f;
inline constexpr float GATE_DEPTH = 0.f;
inline constexpr float GATE_ATTACK = 2.f;
inline constexpr float GATE_RELEASE = 8.f;

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

inline constexpr bool COMPRESSOR_BYPASSED = false;
inline constexpr bool LIMITER_BYPASSED = false;
inline constexpr bool EQ_BYPASSED = false;
inline constexpr bool FILTER_BYPASSED = true;
inline constexpr bool DISTORTION_BYPASSED = true;
inline constexpr bool CHORUS_BYPASSED = true;
inline constexpr bool PHASER_BYPASSED = true;
inline constexpr bool FLANGER_BYPASSED = true;
inline constexpr bool BITCRUSHER_BYPASSED = true;
inline constexpr bool GATE_BYPASSED = true;
inline constexpr bool TAPE_STOP_BYPASSED = true;

inline std::string FP32_DIT_ONNX_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/dit.onnx";
}

inline std::string FP32_DIT_ONNX_DATA_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/dit.onnx.data";
}

inline std::string DEC_DYNAMIC_BF16_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/dec_dynamic_bf16.onnx";
}

inline std::string ENC_DYNAMIC_BF16_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/enc_dynamic_bf16.onnx";
}

inline std::string ENCODER_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/encoder.onnx";
}

inline std::string TOKENIZER_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/tokenizer.json";
}

inline std::string MODELS_DOWNLOAD_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx";
}

inline std::string MODEL_MANIFEST_URL()
{
	return "https://huggingface.co/innermost47/stable-audio-open-3-medium-onnx/resolve/main/manifest.json";
}

inline std::string GITHUB_LATEST_RELEASE_API_URL()
{
	return "https://api.github.com/repos/innermost47/ai-dj/releases/latest";
}

inline std::string UPDATE_URL_HEADERS()
{
	return "User-Agent: OBSIDIAN-Neural-Plugin";
}

inline std::string FP32_DIT_ONNX()
{
	return "dit.onnx";
}
inline std::string FP32_DIT_ONNX_DATA()
{
	return "dit.onnx.data";
}
inline std::string DEC_DYNAMIC_BF16()
{
	return "dec_dynamic_bf16.onnx";
}
inline std::string ENC_DYNAMIC_BF16()
{
	return "enc_dynamic_bf16.onnx";
}
inline std::string ENCODER()
{
	return "encoder.onnx";
}
inline std::string TOKENIZER()
{
	return "tokenizer.json";
}

inline std::string STABLE_AUDIO_OPEN_V1()
{
	return "stable-audio-open-1.0";
}
inline std::string STABLE_AUDIO_OPEN_V3_MEDIUM()
{
	return "stable-audio-3-medium";
}
inline std::string FOUNDATION_1()
{
	return "foundation-1";
}
inline std::string AUDIOLAB_EDM()
{
	return "audialab-edm-elements";
}
inline std::string INFINITE_PIANO()
{
	return "rc-infinite-pianos";
}
inline std::string RC_VOCAL()
{
	return "rc-vocal-textures";
}
inline std::string SAO_INSTRUMENTAL()
{
	return "sao-instrumental";
}
inline std::string STABLEBEAT()
{
	return "stablebeat";
}
inline std::string GLUTEN_V1()
{
	return "gluten-v1";
}
inline std::string STABLE_AUDIO_OPEN_LOCAL()
{
	return "stable-audio-3-medium-onnx";
}

inline std::string OBSIDIAN_BASE_DIR()
{
	return "OBSIDIAN-Neural";
}
inline std::string EXPORTS_DIR()
{
	return "OBSIDIAN_Exports";
}
inline std::string SAMPLE_BANK_DIR()
{
	return "SampleBank";
}
inline std::string SESSIONS_DIR()
{
	return "Sessions";
}
inline std::string STABLE_AUDIO_DIR()
{
	return "stable-audio";
}
inline std::string MAGIC()
{
	return "OBSIDIAN";
}
inline std::string CATEGORIES_FILE()
{
	return "categories.json";
}
inline std::string GLOBAL_CONFIG_FILE()
{
	return "global_config.json";
}
inline std::string PROMPTS_FILE()
{
	return "prompts.json";
}
inline std::string ONNX_LOG_FILE()
{
	return "obsidian_onnx.log";
}
inline std::string AUDIO_CACHE_DIR()
{
	return "AudioCache";
}
inline std::string LINEAGE_FILE()
{
	return "session.lineage";
}
inline std::string FORKS_FILE()
{
	return "session.forks";
}
inline std::string LOG_DIR()
{
	return "logs";
}

enum RadioGroupIDs
{
	FilterTypeGroup = 1,
	DelayDivisionGroup = 2,
	DelayModeGroup = 3,
	TrackFXSelector = 4,
	DistortionType = 5,
	GlitchSeqSelector = 6,
	ModShapeGroup = 7,
	ModRateGroup = 8,
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

enum phaserChain
{
	phaser = 0,
};

enum flangerChain
{
	flanger = 0,
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

} // namespace Obsidian
#endif