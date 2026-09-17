#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

struct TrackData;

enum class ModShape
{
	Sine,
	Triangle,
	Saw,
	Square,
	Ramp,
	SampleHold,
	Count
};

inline juce::StringArray getModShapeNames()
{
	return juce::StringArray{"Sine", "Triangle", "Saw", "Square", "Ramp", "S&H"};
}

enum class ModRate
{
	FourBars,
	TwoBars,
	OneBar,
	Half,
	Quarter,
	Eighth,
	Sixteenth,
	Count
};

inline juce::StringArray getModRateNames()
{
	return juce::StringArray{"4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16"};
}

inline double getModRateInQuarterNotes(ModRate rate)
{
	switch (rate)
	{
	case ModRate::FourBars:
		return 16.0;
	case ModRate::TwoBars:
		return 8.0;
	case ModRate::OneBar:
		return 4.0;
	case ModRate::Half:
		return 2.0;
	case ModRate::Quarter:
		return 1.0;
	case ModRate::Eighth:
		return 0.5;
	case ModRate::Sixteenth:
		return 0.25;
	default:
		return 4.0;
	}
}

struct ModTargetDef
{
	const char *displayName;
	const char *paramSuffix;
	bool (*isFxBypassed)(const TrackData &track);
	void (*apply)(TrackData &track, float value);
};

const ModTargetDef *getModTargets();
int getNumModTargets();
juce::StringArray getModTargetNames();

struct Modulator
{
	Modulator() = default;

	Modulator(const Modulator &other)
	{
		copyFrom(other);
	}

	Modulator &operator=(const Modulator &other)
	{
		if (this != &other)
			copyFrom(other);
		return *this;
	}

	void copyFrom(const Modulator &other)
	{
		shape.store(other.shape.load(std::memory_order_relaxed), std::memory_order_relaxed);
		rate.store(other.rate.load(std::memory_order_relaxed), std::memory_order_relaxed);
		depth.store(other.depth.load(std::memory_order_relaxed), std::memory_order_relaxed);
		phase.store(other.phase.load(std::memory_order_relaxed), std::memory_order_relaxed);
		bipolar.store(other.bipolar.load(std::memory_order_relaxed), std::memory_order_relaxed);
		target.store(other.target.load(std::memory_order_acquire), std::memory_order_release);
		active.store(other.active.load(std::memory_order_acquire), std::memory_order_release);
	}

	int getTarget() const noexcept
	{
		return target.load(std::memory_order_acquire);
	}

	void setTarget(int index) noexcept
	{
		target.store(juce::jlimit(0, getNumModTargets() - 1, index), std::memory_order_release);
	}

	bool isActive() const noexcept
	{
		return active.load(std::memory_order_acquire);
	}

	void setActive(bool shouldBeActive) noexcept
	{
		active.store(shouldBeActive, std::memory_order_release);
	}

	ModShape getShape() const noexcept
	{
		const int v = shape.load(std::memory_order_relaxed);
		if (v < 0 || v >= static_cast<int>(ModShape::Count))
			return ModShape::Sine;
		return static_cast<ModShape>(v);
	}

	void setShape(ModShape s) noexcept
	{
		shape.store(static_cast<int>(s), std::memory_order_relaxed);
	}

	ModRate getRate() const noexcept
	{
		const int v = rate.load(std::memory_order_relaxed);
		if (v < 0 || v >= static_cast<int>(ModRate::Count))
			return ModRate::OneBar;
		return static_cast<ModRate>(v);
	}

	void setRate(ModRate r) noexcept
	{
		rate.store(static_cast<int>(r), std::memory_order_relaxed);
	}

	float getDepth() const noexcept
	{
		return depth.load(std::memory_order_relaxed);
	}

	void setDepth(float d) noexcept
	{
		depth.store(juce::jlimit(0.0f, 1.0f, d), std::memory_order_relaxed);
	}

	float getPhase() const noexcept
	{
		return phase.load(std::memory_order_relaxed);
	}

	void setPhase(float p) noexcept
	{
		phase.store(juce::jlimit(0.0f, 1.0f, p), std::memory_order_relaxed);
	}

	bool isBipolar() const noexcept
	{
		return bipolar.load(std::memory_order_relaxed);
	}

	void setBipolar(bool b) noexcept
	{
		bipolar.store(b, std::memory_order_relaxed);
	}

	float getCurrentValue() const noexcept
	{
		return currentValue.load(std::memory_order_relaxed);
	}

	void setCurrentValue(float v) noexcept
	{
		currentValue.store(v, std::memory_order_relaxed);
	}

	float getSampleHoldValue() const noexcept
	{
		return sampleHoldValue.load(std::memory_order_relaxed);
	}

	void setSampleHoldValue(float v) noexcept
	{
		sampleHoldValue.store(v, std::memory_order_relaxed);
	}

	int getLastSampleHoldCycle() const noexcept
	{
		return lastSampleHoldCycle.load(std::memory_order_relaxed);
	}

	void setLastSampleHoldCycle(int c) noexcept
	{
		lastSampleHoldCycle.store(c, std::memory_order_relaxed);
	}

	void reset() noexcept
	{
		currentValue.store(0.0f, std::memory_order_relaxed);
		sampleHoldValue.store(0.0f, std::memory_order_relaxed);
		lastSampleHoldCycle.store(-1, std::memory_order_relaxed);
	}

  private:
	std::atomic<int> target{0};
	std::atomic<int> shape{static_cast<int>(ModShape::Sine)};
	std::atomic<int> rate{static_cast<int>(ModRate::OneBar)};
	std::atomic<float> depth{0.0f};
	std::atomic<float> phase{0.0f};
	std::atomic<bool> bipolar{true};
	std::atomic<bool> active{false};

	std::atomic<float> currentValue{0.0f};
	std::atomic<float> sampleHoldValue{0.0f};
	std::atomic<int> lastSampleHoldCycle{-1};

	static_assert(std::atomic<int>::is_always_lock_free, "Modulator atomics must be lock-free for real-time audio use");
	static_assert(std::atomic<float>::is_always_lock_free,
	              "Modulator atomics must be lock-free for real-time audio use");
};

static constexpr int kNumModSlots = 8;
static const int kDefaultModTargets[kNumModSlots] = {1, 3, 5, 9, 13, 16, 21, 22};