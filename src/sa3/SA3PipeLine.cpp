#include "SA3PipeLine.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <algorithm>
#include <fstream>
#include <random>
#include <samplerate.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

const OrtApi *SA3PipeLine::g_onnxApi = nullptr;
std::unique_ptr<Ort::Env> SA3PipeLine::g_onnxEnv = nullptr;

namespace
{
#if defined(_WIN32)
constexpr const wchar_t *kOnnxLibNames[] = {L"onnxruntime.dll"};
#elif defined(__APPLE__)
constexpr const char *kOnnxLibNames[] = {"libonnxruntime.1.dylib", "libonnxruntime.dylib"};
#else
constexpr const char *kOnnxLibNames[] = {"libonnxruntime.so." ORT_VERSION_STR, "libonnxruntime.so"};
#endif
} // namespace

SA3PipeLine::SA3PipeLine()
{
	ensureOnnxLoaded();
	if (!g_onnxApi)
		return;

	g_envRefCount.fetch_add(1, std::memory_order_relaxed);

	OrtSessionOptions *raw_opts = nullptr;
	OrtStatus *st = g_onnxApi->CreateSessionOptions(&raw_opts);
	if (st == nullptr)
	{
		opts = std::unique_ptr<Ort::SessionOptions>(new Ort::SessionOptions(raw_opts));
		opts->DisablePerSessionThreads();
		opts->AddConfigEntry("session.intra_op.allow_spinning", "0");
		opts->AddConfigEntry("session.inter_op.allow_spinning", "0");
		opts->AddConfigEntry("memory.enable_memory_arena_shrinkage", "cpu:0");
		opts->DisableCpuMemArena();
	}
	else
	{
		g_onnxApi->ReleaseStatus(st);
	}
}

SA3PipeLine::~SA3PipeLine()
{
	sessionTextEncoder = nullptr;
	sessionDIT = nullptr;
	sessionDecoder = nullptr;
	sessionEncoder = nullptr;
	opts = nullptr;
	tokenizer = nullptr;

	if (g_onnxApi && g_envRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		releaseSharedEnv();
}

Ort::Env &SA3PipeLine::GetSharedEnv()
{
	if (g_onnxEnv == nullptr)
	{
		ensureOnnxLoaded();
		if (!g_onnxApi)
			throw std::runtime_error("ONNX not loaded");
		OrtThreadingOptions *threadOpts = nullptr;
		g_onnxApi->CreateThreadingOptions(&threadOpts);
		g_onnxApi->SetGlobalIntraOpNumThreads(threadOpts, 4);
		g_onnxApi->SetGlobalInterOpNumThreads(threadOpts, 1);
		g_onnxApi->SetGlobalSpinControl(threadOpts, 0);

		OrtEnv *rawEnv = nullptr;
		g_onnxApi->CreateEnvWithGlobalThreadPools(ORT_LOGGING_LEVEL_WARNING, "sa3pipeline", threadOpts, &rawEnv);
		g_onnxApi->SetLanguageProjection(rawEnv, OrtLanguageProjection::ORT_PROJECTION_CPLUSPLUS);

		g_onnxApi->ReleaseThreadingOptions(threadOpts);

		g_onnxEnv = std::make_unique<Ort::Env>(rawEnv);
	}
	return *g_onnxEnv;
}

void SA3PipeLine::releaseSharedEnv()
{
	if (g_onnxEnv != nullptr)
	{
		g_onnxEnv.reset();
	}
}

void SA3PipeLine::initModel(const std::filesystem::path &modelsDir)
{
	ditPath = modelsDir / Obsidian::FP32_DIT_ONNX();
	decoderPath = modelsDir / Obsidian::DEC_DYNAMIC_BF16();
	encoderPath = modelsDir / Obsidian::ENC_DYNAMIC_BF16();
	textEncoderPath = modelsDir / Obsidian::ENCODER();

	auto createSafeSession = [](const std::filesystem::path &path,
	                            const Ort::SessionOptions &options) -> std::unique_ptr<Ort::Session>
	{
		if (!g_onnxApi)
			return nullptr;
		if (!std::filesystem::exists(path))
			return nullptr;
		OrtSession *rawSession = nullptr;
		OrtStatus *status = g_onnxApi->CreateSession(GetSharedEnv(), ORT_PATH(path), options, &rawSession);
		if (status != nullptr)
		{
			g_onnxApi->ReleaseStatus(status);
			return nullptr;
		}
		if (rawSession == nullptr)
			return nullptr;
		return std::unique_ptr<Ort::Session>(new Ort::Session(rawSession));
	};

	sessionTextEncoder = createSafeSession(textEncoderPath, *opts);
	sessionDIT = createSafeSession(ditPath, *opts);
	sessionDecoder = createSafeSession(decoderPath, *opts);
	sessionEncoder = createSafeSession(encoderPath, *opts);

	tokenizer = std::make_unique<SA3Tokenizer>(modelsDir);
}

std::filesystem::path SA3PipeLine::getModuleDir()
{
#if defined(_WIN32)
	HMODULE hm = nullptr;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                   reinterpret_cast<LPCWSTR>(&SA3PipeLine::getModuleDir), &hm);
	wchar_t path[MAX_PATH] = {};
	GetModuleFileNameW(hm, path, MAX_PATH);
	return std::filesystem::path(path).parent_path();
#elif defined(__APPLE__)
	Dl_info info{};
	if (dladdr(reinterpret_cast<void *>(&SA3PipeLine::getModuleDir), &info) && info.dli_fname)
	{
		auto macosDir = std::filesystem::path(info.dli_fname).parent_path();
		return macosDir.parent_path() / "Frameworks";
	}
	return {};
#else
	Dl_info info{};
	if (dladdr(reinterpret_cast<void *>(&SA3PipeLine::getModuleDir), &info) && info.dli_fname)
		return std::filesystem::path(info.dli_fname).parent_path();
	return {};
#endif
}

void SA3PipeLine::ensureOnnxLoaded()
{
	if (g_onnxApi != nullptr)
		return;

	auto logError = [](const juce::String &m)
	{
		auto logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
		                   .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
		                   .getChildFile(Obsidian::LOG_DIR())
		                   .getChildFile(Obsidian::ONNX_LOG_FILE());
		logFile.getParentDirectory().createDirectory();
		logFile.appendText(m + "\n");
	};

	const OrtApiBase *(ORT_API_CALL * getApiBase)() = nullptr;
	auto moduleDir = getModuleDir();

	for (auto name : kOnnxLibNames)
	{
		auto libPath = moduleDir / name;

		if (!std::filesystem::exists(libPath))
			continue;

#if defined(_WIN32)
		HMODULE h = LoadLibraryW(libPath.c_str());
		if (!h)
			continue;
		getApiBase = reinterpret_cast<decltype(getApiBase)>(GetProcAddress(h, "OrtGetApiBase"));
#else
		void *h = dlopen(libPath.string().c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!h)
			continue;
		getApiBase = reinterpret_cast<decltype(getApiBase)>(dlsym(h, "OrtGetApiBase"));
#endif
		if (getApiBase)
			break;
	}

	if (!getApiBase)
	{
		logError("FATAL: no onnxruntime lib loaded");
		return;
	}

	const OrtApiBase *base = getApiBase();
	g_onnxApi = base->GetApi(ORT_API_VERSION);
	if (!g_onnxApi)
	{
		logError("FATAL: GetApi returned null (version mismatch)");
		return;
	}
	Ort::InitApi(g_onnxApi);
}

void SA3PipeLine::checkModelsThenExecute(std::function<void()> callback)
{
	if (sessionTextEncoder && sessionDIT && sessionDecoder && sessionEncoder && opts)
		callback();
}

bool SA3PipeLine::isReady() const
{
	return sessionTextEncoder && sessionDIT && sessionDecoder && sessionEncoder && tokenizer && opts;
}

SA3PipeLine::EncodedText SA3PipeLine::encodeText(std::string &prompt)
{
	Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	SA3Tokenizer::TokenizerOutput tokenizerOutputs = tokenizer->encode(prompt);
	std::vector<int64_t> ids64(tokenizerOutputs.inputIds.begin(), tokenizerOutputs.inputIds.end());
	std::vector<int64_t> mask64(tokenizerOutputs.attentionMask.begin(), tokenizerOutputs.attentionMask.end());
	std::vector<int64_t> shape = {1, 256};

	Ort::Value inputIdsTensor =
	    Ort::Value::CreateTensor<int64_t>(memInfo, ids64.data(), ids64.size(), shape.data(), shape.size());
	Ort::Value attentionMaskTensor =
	    Ort::Value::CreateTensor<int64_t>(memInfo, mask64.data(), mask64.size(), shape.data(), shape.size());

	const char *inputNames[] = {"input_ids", "attention_mask"};
	const char *outputNames[] = {"hidden_states"};

	std::vector<Ort::Value> inputs;
	inputs.push_back(std::move(inputIdsTensor));
	inputs.push_back(std::move(attentionMaskTensor));

	auto outputs =
	    sessionTextEncoder->Run(Ort::RunOptions{nullptr}, inputNames, inputs.data(), inputs.size(), outputNames, 1);
	return {std::move(outputs), mask64};
}

SA3PipeLine::NoiseOutput SA3PipeLine::generateNoise(int latenLen) const
{
	int totalSize = batch * channels * latenLen;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<float> dist(0.f, 1.f);
	std::vector<float> data(totalSize);

	for (int i = 0; i < totalSize; i++)
		data[i] = dist(gen);

	return {data, {(int64_t)batch, (int64_t)channels, (int64_t)latenLen}};
}

std::vector<float> SA3PipeLine::buildSchedule(int latentLen)
{
	int minLength = 256;
	int maxLength = 4096;
	float sigmaMax = 1.f;
	float baseShift = .5f;
	float maxShift = 1.15f;

	int seqLen = juce::jlimit(minLength, maxLength, latentLen);

	float mu = -(baseShift + (maxShift - baseShift) * (float)(seqLen - minLength) / (float)(maxLength - minLength));

	std::vector<float> values(steps + 1);

	for (int i = steps; i >= 0; i--)
	{
		if (i == 0)
			values[i] = 0.f;
		else
		{
			float value = sigmaMax * (float)i / (float)steps;
			float tShifted = 1.f - std::exp(mu) / (std::exp(mu) + (1.f / (1.f - value) - 1.f));
			values[i] = tShifted;
		}
	}

	values[steps] = 1.f;
	return values;
}

SA3PipeLine::LatentOutput SA3PipeLine::runDiffusion(EncodedText &encodedText, NoiseOutput &noise,
                                                    std::vector<float> &schedule, int latenLen, float duration) const
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<float> dist(0.f, 1.f);

	Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

	std::vector<Ort::Value> &t5Hidden = encodedText.hiddenStates;
	std::vector<int64_t> t5Mask = encodedText.attentionMask;
	std::vector<int64_t> maskShape = {1, 256};
	std::vector<float> x = noise.data;
	std::vector<float> secondsData = {duration};
	std::vector<int64_t> secondsShape = {1};
	std::vector<float> localAddCondData(1 * 257 * latenLen, 0.f);
	std::vector<int64_t> localAddCondShape = {1, 257, (int64_t)latenLen};

	float *t5HiddenData = t5Hidden[0].GetTensorMutableData<float>();
	auto t5HiddenShape = t5Hidden[0].GetTensorTypeAndShapeInfo().GetShape();

	std::vector<float> t5MaskFloat;
	t5MaskFloat.reserve(t5Mask.size());
	for (int64_t val : t5Mask)
		t5MaskFloat.push_back(static_cast<float>(val));

	Ort::Value t5HiddenTensor =
	    Ort::Value::CreateTensor<float>(memInfo, t5HiddenData, t5HiddenShape[0] * t5HiddenShape[1] * t5HiddenShape[2],
	                                    t5HiddenShape.data(), t5HiddenShape.size());

	Ort::Value t5MaskTensor = Ort::Value::CreateTensor<float>(memInfo, t5MaskFloat.data(), t5MaskFloat.size(),
	                                                          maskShape.data(), maskShape.size());

	Ort::Value secondsTotalTensor = Ort::Value::CreateTensor<float>(memInfo, secondsData.data(), secondsData.size(),
	                                                                secondsShape.data(), secondsShape.size());

	Ort::Value localAddCondTensor = Ort::Value::CreateTensor<float>(
	    memInfo, localAddCondData.data(), localAddCondData.size(), localAddCondShape.data(), localAddCondShape.size());

	std::vector<float> tData = {0.f};
	std::vector<int64_t> tShape = {1};
	Ort::Value tTensor =
	    Ort::Value::CreateTensor<float>(memInfo, tData.data(), tData.size(), tShape.data(), tShape.size());

	const char *inputNames[] = {"x", "t", "t5_hidden", "t5_mask", "seconds_total", "local_add_cond"};
	const char *outputNames[] = {"velocity"};

	for (int i = 0; i < steps; i++)
	{
		float tCurrent = schedule[steps - i];
		float tNext = schedule[steps - i - 1];

		Ort::Value xTensor =
		    Ort::Value::CreateTensor<float>(memInfo, x.data(), x.size(), noise.shape.data(), noise.shape.size());

		tData[0] = tCurrent;

		Ort::Value inputs[] = {
		    std::move(xTensor),
		    Ort::Value::CreateTensor<float>(memInfo, tData.data(), tData.size(), tShape.data(), tShape.size()),
		    Ort::Value::CreateTensor<float>(memInfo, t5HiddenData,
		                                    t5HiddenShape[0] * t5HiddenShape[1] * t5HiddenShape[2],
		                                    t5HiddenShape.data(), t5HiddenShape.size()),
		    Ort::Value::CreateTensor<float>(memInfo, t5MaskFloat.data(), t5MaskFloat.size(), maskShape.data(),
		                                    maskShape.size()),
		    Ort::Value::CreateTensor<float>(memInfo, secondsData.data(), secondsData.size(), secondsShape.data(),
		                                    secondsShape.size()),
		    Ort::Value::CreateTensor<float>(memInfo, localAddCondData.data(), localAddCondData.size(),
		                                    localAddCondShape.data(), localAddCondShape.size())};

		auto ditOutputs = sessionDIT->Run(Ort::RunOptions{nullptr}, inputNames, inputs, 6, outputNames, 1);

		float *velocity = ditOutputs[0].GetTensorMutableData<float>();

		bool lastStep = (i == steps - 1);
		if (!lastStep)
		{
			for (size_t j = 0; j < x.size(); j++)
			{
				float denoised = x[j] - tCurrent * velocity[j];
				x[j] = (1.f - tNext) * denoised + tNext * dist(gen);
			}
		}
		else
		{
			for (size_t j = 0; j < x.size(); j++)
				x[j] = x[j] - tCurrent * velocity[j];
		}
	}
	return {x, noise.shape};
}

SA3PipeLine::PcmOutput SA3PipeLine::decodeAudio(LatentOutput &latents)
{
	const char *inputNames[] = {"latent"};
	const char *outputNames[] = {"pcm"};

	Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	std::vector<float> data = latents.data;
	std::vector<int64_t> shape = latents.shape;

	Ort::Value latentsTensor =
	    Ort::Value::CreateTensor<float>(memInfo, data.data(), data.size(), shape.data(), shape.size());

	std::vector<Ort::Value> inputs;
	inputs.push_back(std::move(latentsTensor));

	std::vector<Ort::Value> outputs =
	    sessionDecoder.get()->Run(Ort::RunOptions{nullptr}, inputNames, inputs.data(), inputs.size(), outputNames, 1);

	int32_t *pcmData = outputs[0].GetTensorMutableData<int32_t>();
	auto pcmShape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
	size_t totalSize = pcmShape[0] * pcmShape[1] * pcmShape[2];

	std::vector<float> pcm(totalSize);
	for (size_t i = 0; i < totalSize; i++)
	{
		int32_t v = std::clamp(pcmData[i], -32767, 32767);
		pcm[i] = (float)v / 32767.f;
	}

	return {pcm, {pcmShape[0], pcmShape[1], pcmShape[2]}};
}

void SA3PipeLine::resamplePcm(PcmOutput &pcm, int targetRate)
{
	int numChannels = (int)pcm.shape[2];
	long inFrames = (long)pcm.shape[1];
	double ratio = (double)targetRate / (double)sampleRate;
	long outFrames = (long)std::ceil(inFrames * ratio);

	std::vector<float> output(outFrames * numChannels);

	SRC_DATA src;
	src.data_in = pcm.data.data();
	src.input_frames = inFrames;
	src.data_out = output.data();
	src.output_frames = outFrames;
	src.src_ratio = ratio;
	src.end_of_input = 1;

	int err = src_simple(&src, SRC_SINC_BEST_QUALITY, numChannels);
	if (err != 0)
		return;

	pcm.data = std::move(output);
	pcm.shape[1] = src.output_frames_gen;
}

std::vector<uint8_t> SA3PipeLine::writePcmToWav(PcmOutput &pcm, int outRate) const
{
	void *wavData = nullptr;
	size_t wavSize = 0;

	drwav_data_format format;
	format.container = drwav_container_riff;
	format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
	format.channels = (drwav_uint32)pcm.shape[2];
	format.sampleRate = outRate;
	format.bitsPerSample = 32;

	drwav wav;
	drwav_init_memory_write(&wav, &wavData, &wavSize, &format, nullptr);
	drwav_write_pcm_frames(&wav, pcm.shape[1], pcm.data.data());
	drwav_uninit(&wav);

	std::vector<uint8_t> result(static_cast<uint8_t *>(wavData), static_cast<uint8_t *>(wavData) + wavSize);
	drwav_free(wavData, nullptr);
	return result;
}

std::vector<uint8_t> SA3PipeLine::generate(std::string &prompt, float duration, int targetSampleRate)
{
	const float durationPaddingSec = 6.0f;
	const float paddedDuration = duration + durationPaddingSec;

	const int dsRatio = 4096;
	int paddedAudioSamples = static_cast<int>(std::ceil(paddedDuration * (float)sampleRate));
	paddedAudioSamples = ((paddedAudioSamples + dsRatio - 1) / dsRatio) * dsRatio;

	int latenLen = paddedAudioSamples / dsRatio;

	int effectiveLatenLen = static_cast<int>(std::ceil(duration * (float)sampleRate / (float)dsRatio));

	EncodedText encodedText = encodeText(prompt);
	NoiseOutput noise = generateNoise(latenLen);

	std::vector<float> schedule = buildSchedule(effectiveLatenLen);
	LatentOutput latents = runDiffusion(encodedText, noise, schedule, latenLen, duration);
	PcmOutput decoded = decodeAudio(latents);

	if (targetSampleRate != sampleRate)
		resamplePcm(decoded, targetSampleRate);

	{
		int outChannels = (int)decoded.shape[2];
		long targetFrames = (long)std::floor((double)duration * (double)targetSampleRate);
		long currentFrames = (long)decoded.shape[1];

		if (targetFrames > 0 && targetFrames < currentFrames)
		{
			std::vector<float> trimmed((size_t)targetFrames * outChannels);
			std::copy(decoded.data.begin(), decoded.data.begin() + (size_t)targetFrames * outChannels, trimmed.begin());
			decoded.data = std::move(trimmed);
			decoded.shape[1] = targetFrames;
		}
	}

	std::vector<uint8_t> wavData = writePcmToWav(decoded, targetSampleRate);
	return wavData;
}