#pragma once
#include "DataConst.h"
#include "SA3Tokenizer.h"
#include <filesystem>
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>

#ifdef _WIN32
#define ORT_PATH(p) (p).wstring().c_str()
#else
#define ORT_PATH(p) (p).string().c_str()
#endif

class SA3PipeLine
{
  public:
	SA3PipeLine();
	~SA3PipeLine();

	const std::unique_ptr<Ort::SessionOptions> &getOpts() const
	{
		return opts;
	}

	const std::unique_ptr<Ort::Session> &getSessionTextEncoder() const
	{
		return sessionTextEncoder;
	}

	const std::unique_ptr<Ort::Session> &getSessionDIT() const
	{
		return sessionDIT;
	}

	const std::unique_ptr<Ort::Session> &getSessionDecoder() const
	{
		return sessionDecoder;
	}

	const std::unique_ptr<Ort::Session> &getSessionEncoder() const
	{
		return sessionEncoder;
	}

	const std::unique_ptr<SA3Tokenizer> &getTokenizer() const
	{
		return tokenizer;
	}

	std::vector<uint8_t> generate(std::string &prompt, float duration, int sampleRate);

	void initModel(const std::filesystem::path &modelsDir);

	bool isReady() const;

	static void releaseSharedEnv();

  private:
	int sampleRate = 44100;
	int steps = 8;
	int batch = 1;
	int channels = 256;
	static std::filesystem::path getModuleDir();
	static void ensureOnnxLoaded();
	static Ort::Env &GetSharedEnv();
	static const OrtApi *g_onnxApi;
	static std::unique_ptr<Ort::Env> g_onnxEnv;
	static inline std::atomic<int> g_envRefCount{0};

	struct EncodedText
	{
		std::vector<Ort::Value> hiddenStates;
		std::vector<int64_t> attentionMask;
	};

	struct NoiseOutput
	{
		std::vector<float> data;
		std::vector<int64_t> shape;
	};

	struct LatentOutput
	{
		std::vector<float> data;
		std::vector<int64_t> shape;
	};

	struct PcmOutput
	{
		std::vector<float> data;
		std::vector<int64_t> shape;
	};

	std::unique_ptr<Ort::SessionOptions> opts;

	std::unique_ptr<Ort::Session> sessionTextEncoder;
	std::unique_ptr<Ort::Session> sessionDIT;
	std::unique_ptr<Ort::Session> sessionDecoder;
	std::unique_ptr<Ort::Session> sessionEncoder;

	std::unique_ptr<SA3Tokenizer> tokenizer;

	std::filesystem::path textEncoderPath;
	std::filesystem::path ditPath;
	std::filesystem::path decoderPath;
	std::filesystem::path encoderPath;

	EncodedText encodeText(std::string &prompt);

	std::vector<float> buildSchedule(int latentLen);

	LatentOutput runDiffusion(EncodedText &encodedText, NoiseOutput &noise, std::vector<float> &schedule, int latenLen,
	                          float duration) const;
	NoiseOutput generateNoise(int latenLen) const;

	PcmOutput decodeAudio(LatentOutput &latents);

	std::vector<uint8_t> writePcmToWav(PcmOutput &pcm, int outRate) const;

	void resamplePcm(PcmOutput &pcm, int targetRate);

	void checkModelsThenExecute(std::function<void()> callback);
};