#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

enum class GlitchEffectType
{
	None,
	Reverse,
	TransientScatter,
	BeatRepeat,
	Gate,
	Filter,
	Chorus,
	Phaser,
	Flanger,
	BitCrusher,
	Distortion,
	Random,
	Count
};

inline juce::String getGlitchEffectName(GlitchEffectType type)
{
	switch (type)
	{
	case GlitchEffectType::None:
		return "None";
	case GlitchEffectType::Reverse:
		return "Reverse";
	case GlitchEffectType::TransientScatter:
		return "Scatter";
	case GlitchEffectType::BeatRepeat:
		return "BeatRep";
	case GlitchEffectType::Gate:
		return "Gate";
	case GlitchEffectType::Filter:
		return "Filter";
	case GlitchEffectType::Chorus:
		return "Chorus";
	case GlitchEffectType::Phaser:
		return "Phaser";
	case GlitchEffectType::Flanger:
		return "Flanger";
	case GlitchEffectType::BitCrusher:
		return "BitCrush";
	case GlitchEffectType::Distortion:
		return "Distort";
	case GlitchEffectType::Random:
		return "Random";
	default:
		return "?";
	}
}

inline juce::String getGlitchEffectShortName(GlitchEffectType type)
{
	switch (type)
	{
	case GlitchEffectType::None:
		return "-";
	case GlitchEffectType::Reverse:
		return "REV";
	case GlitchEffectType::TransientScatter:
		return "SCT";
	case GlitchEffectType::BeatRepeat:
		return "BRP";
	case GlitchEffectType::Gate:
		return "GTE";
	case GlitchEffectType::Filter:
		return "FLT";
	case GlitchEffectType::Chorus:
		return "CHO";
	case GlitchEffectType::Phaser:
		return "PHS";
	case GlitchEffectType::Flanger:
		return "FLG";
	case GlitchEffectType::BitCrusher:
		return "BIT";
	case GlitchEffectType::Distortion:
		return "DST";
	case GlitchEffectType::Random:
		return "RND";
	default:
		return "?";
	}
}

struct GlitchSequence
{
	static constexpr int MAX_STEPS = 64;
	static constexpr int MAX_STEP_LENGTH = 16;

	GlitchSequence()
	{
		clearAllSteps();
	}

	GlitchSequence(const GlitchSequence &other)
	{
		copyFrom(other);
	}

	GlitchSequence &operator=(const GlitchSequence &other)
	{
		if (this != &other)
			copyFrom(other);
		return *this;
	}

	void copyFrom(const GlitchSequence &other)
	{
		for (int i = 0; i < MAX_STEPS; ++i)
			steps[i].store(other.steps[i].load(std::memory_order_relaxed), std::memory_order_relaxed);

		stepLengthIn16ths.store(other.stepLengthIn16ths.load(std::memory_order_relaxed), std::memory_order_relaxed);
		numSteps.store(other.numSteps.load(std::memory_order_acquire), std::memory_order_release);
	}

	GlitchEffectType getStep(int index) const noexcept
	{
		if (index < 0 || index >= MAX_STEPS)
			return GlitchEffectType::None;

		const auto raw = steps[index].load(std::memory_order_relaxed);
		const int v = static_cast<int>(raw);
		if (v < 0 || v >= static_cast<int>(GlitchEffectType::Count))
			return GlitchEffectType::None;

		return raw;
	}

	void setStep(int index, GlitchEffectType type) noexcept
	{
		if (index < 0 || index >= MAX_STEPS)
			return;

		const int v = static_cast<int>(type);
		if (v < 0 || v >= static_cast<int>(GlitchEffectType::Count))
			return;

		steps[index].store(type, std::memory_order_relaxed);
	}

	int getNumSteps() const noexcept
	{
		return numSteps.load(std::memory_order_acquire);
	}

	void setNumSteps(int n) noexcept
	{
		numSteps.store(juce::jlimit(1, MAX_STEPS, n), std::memory_order_release);
	}

	int getStepLength() const noexcept
	{
		return stepLengthIn16ths.load(std::memory_order_relaxed);
	}

	void setStepLength(int len) noexcept
	{
		stepLengthIn16ths.store(juce::jlimit(1, MAX_STEP_LENGTH, len), std::memory_order_relaxed);
	}

	void clearAllSteps() noexcept
	{
		for (auto &s : steps)
			s.store(GlitchEffectType::None, std::memory_order_relaxed);
	}

	bool isEmpty() const noexcept
	{
		const int n = getNumSteps();
		for (int i = 0; i < n; ++i)
			if (getStep(i) != GlitchEffectType::None)
				return false;
		return true;
	}

  private:
	std::array<std::atomic<GlitchEffectType>, MAX_STEPS> steps{};
	std::atomic<int> numSteps{16};
	std::atomic<int> stepLengthIn16ths{1};

	static_assert(std::atomic<GlitchEffectType>::is_always_lock_free,
	              "GlitchEffectType atomics must be lock-free for real-time audio use");
};

struct FxBypassSnapshot
{
	bool filterBypassed = true;
	bool chorusBypassed = true;
	bool phaserBypassed = true;
	bool flangerBypassed = true;
	bool bitCrusherBypassed = true;
	bool distortionBypassed = true;
	bool gateBypassed = true;
};

struct GlitchMetaSequence
{
	static constexpr int MAX_STEPS = 16;

	GlitchMetaSequence()
	{
		for (auto &s : steps)
			s.store(0, std::memory_order_relaxed);
	}

	GlitchMetaSequence(const GlitchMetaSequence &other)
	{
		copyFrom(other);
	}

	GlitchMetaSequence &operator=(const GlitchMetaSequence &other)
	{
		if (this != &other)
			copyFrom(other);
		return *this;
	}

	void copyFrom(const GlitchMetaSequence &other)
	{
		for (int i = 0; i < MAX_STEPS; ++i)
			steps[i].store(other.steps[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
		bypassed.store(other.bypassed.load(std::memory_order_relaxed), std::memory_order_relaxed);
		numSteps.store(other.numSteps.load(std::memory_order_acquire), std::memory_order_release);
	}

	int getStep(int index) const noexcept
	{
		if (index < 0 || index >= MAX_STEPS)
			return 0;
		return juce::jlimit(0, 8, steps[index].load(std::memory_order_relaxed));
	}

	void setStep(int index, int seqNumber) noexcept
	{
		if (index < 0 || index >= MAX_STEPS)
			return;
		steps[index].store(juce::jlimit(0, 8, seqNumber), std::memory_order_relaxed);
	}

	int getNumSteps() const noexcept
	{
		return numSteps.load(std::memory_order_acquire);
	}

	void setNumSteps(int n) noexcept
	{
		numSteps.store(juce::jlimit(1, MAX_STEPS, n), std::memory_order_release);
	}

	bool isBypassed() const noexcept
	{
		return bypassed.load(std::memory_order_relaxed);
	}

	void setBypassed(bool b) noexcept
	{
		bypassed.store(b, std::memory_order_relaxed);
	}

  private:
	std::array<std::atomic<int>, MAX_STEPS> steps{};
	std::atomic<int> numSteps{8};
	std::atomic<bool> bypassed{true};
};