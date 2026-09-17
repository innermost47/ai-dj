#include "ModulationEngine.h"
#include "PluginProcessor.h"
#include "TrackData.h"
#include <cmath>

ModulationEngine::ModulationEngine(DjIaVstProcessor &processor) : audioProcessor(processor)
{
}

int ModulationEngine::targetBit(int targetIndex)
{
	if (targetIndex <= 0 || targetIndex >= 31)
		return 0;
	return 1 << targetIndex;
}

bool ModulationEngine::isTargetModulated(const TrackData &track, int targetIndex)
{
	return (track.modulatedTargetsMask.load() & targetBit(targetIndex)) != 0;
}

float ModulationEngine::evaluateShape(Modulator &mod, double ppqPosition, double cycleLength)
{
	if (cycleLength <= 0.0)
		return 0.0f;

	const double cycles = ppqPosition / cycleLength + mod.getPhase();
	const double t = cycles - std::floor(cycles);

	switch (mod.getShape())
	{
	case ModShape::Sine:
		return (float)std::sin(t * juce::MathConstants<double>::twoPi);

	case ModShape::Triangle:
		return (float)(t < 0.5 ? (t * 4.0 - 1.0) : (3.0 - t * 4.0));

	case ModShape::Saw:
		return (float)(t * 2.0 - 1.0);

	case ModShape::Ramp:
		return (float)(1.0 - t * 2.0);

	case ModShape::Square:
		return t < 0.5 ? 1.0f : -1.0f;

	case ModShape::SampleHold:
	{
		const int currentCycle = (int)std::floor(cycles);
		if (currentCycle != mod.getLastSampleHoldCycle())
		{
			mod.setLastSampleHoldCycle(currentCycle);
			mod.setSampleHoldValue(juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
		}
		return mod.getSampleHoldValue();
	}

	default:
		return 0.0f;
	}
}

void ModulationEngine::applyModulator(TrackData &track, Modulator &mod, double ppqPosition)
{
	const int targetIndex = mod.getTarget();
	if (targetIndex <= 0 || targetIndex >= getNumModTargets())
		return;

	const auto &def = getModTargets()[targetIndex];
	if (def.paramSuffix == nullptr || def.apply == nullptr)
		return;

	if (track.slotIndex < 0 || track.slotIndex >= Obsidian::MAX_TRACKS)
		return;

	if (def.isFxBypassed != nullptr && def.isFxBypassed(track))
	{
		mod.setCurrentValue(0.0f);
		return;
	}

	const juce::String paramId = "slot" + juce::String(track.slotIndex + 1) + def.paramSuffix;
	auto &apvts = audioProcessor.getParameterTreeState();

	auto *param = apvts.getParameter(paramId);
	if (param == nullptr)
		return;

	const double cycleLength = getModRateInQuarterNotes(mod.getRate());
	float raw = evaluateShape(mod, ppqPosition, cycleLength);

	if (!mod.isBipolar())
		raw = (raw + 1.0f) * 0.5f;

	const float offset = raw * mod.getDepth();
	mod.setCurrentValue(offset);

	const float baseNorm = param->getValue();
	const float outNorm = juce::jlimit(0.0f, 1.0f, baseNorm + offset);

	const auto range = apvts.getParameterRange(paramId);
	def.apply(track, range.convertFrom0to1(outNorm));
}

void ModulationEngine::releaseTarget(TrackData &track, int targetIndex)
{
	if (targetIndex <= 0 || targetIndex >= getNumModTargets())
		return;

	const auto &def = getModTargets()[targetIndex];
	if (def.paramSuffix == nullptr || def.apply == nullptr)
		return;

	if (track.slotIndex < 0 || track.slotIndex >= Obsidian::MAX_TRACKS)
		return;

	const juce::String paramId = "slot" + juce::String(track.slotIndex + 1) + def.paramSuffix;
	auto &apvts = audioProcessor.getParameterTreeState();

	auto *param = apvts.getParameter(paramId);
	if (param == nullptr)
		return;

	const auto range = apvts.getParameterRange(paramId);
	def.apply(track, range.convertFrom0to1(param->getValue()));
}

void ModulationEngine::processBlock(TrackData &track, double ppqPosition)
{
	int newMask = 0;

	for (int i = 0; i < kNumModSlots; ++i)
	{
		auto &mod = track.modulators[i];
		if (!mod.isActive())
			continue;

		const int targetIndex = mod.getTarget();
		if (targetIndex <= 0)
			continue;

		newMask |= targetBit(targetIndex);
	}

	const int oldMask = track.modulatedTargetsMask.load();
	const int leaving = oldMask & ~newMask;

	if (newMask != oldMask)
		track.modulatedTargetsMask.store(newMask);

	if (leaving != 0)
	{
		for (int i = 1; i < getNumModTargets(); ++i)
			if (leaving & targetBit(i))
				releaseTarget(track, i);
	}

	if (newMask == 0)
		return;

	for (int i = 0; i < kNumModSlots; ++i)
	{
		auto &mod = track.modulators[i];
		if (!mod.isActive())
		{
			mod.setCurrentValue(0.0f);
			continue;
		}

		applyModulator(track, mod, ppqPosition);
	}
}

float ModulationEngine::getModulationForTarget(const TrackData &track, int targetIndex)
{
	if (targetIndex <= 0)
		return 0.0f;

	float total = 0.0f;
	for (int i = 0; i < kNumModSlots; ++i)
	{
		const auto &mod = track.modulators[i];
		if (mod.isActive() && mod.getTarget() == targetIndex)
			total += mod.getCurrentValue();
	}
	return juce::jlimit(-1.0f, 1.0f, total);
}