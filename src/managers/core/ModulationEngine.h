#pragma once
#include "ModulationTypes.h"
#include <JuceHeader.h>

struct TrackData;
class DjIaVstProcessor;

class ModulationEngine
{
  public:
	explicit ModulationEngine(DjIaVstProcessor &processor);

	void processBlock(TrackData &track, double ppqPosition);

	static bool isTargetModulated(const TrackData &track, int targetIndex);
	static int targetBit(int targetIndex);
	static float getModulationForTarget(const TrackData &track, int targetIndex);

  private:
	float evaluateShape(Modulator &mod, double ppqPosition, double cycleLength);
	void applyModulator(TrackData &track, Modulator &mod, double ppqPosition);
	void releaseTarget(TrackData &track, int targetIndex);

	DjIaVstProcessor &audioProcessor;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationEngine)
};