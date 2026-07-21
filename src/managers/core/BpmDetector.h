#pragma once
#include "JuceHeader.h"
#include <memory>

class BpmDetector
{
  public:
	virtual ~BpmDetector() = default;
	virtual double detectTempo(double hostBpm, const float *channelData, int numSamples, double sampleRate) = 0;
};

std::unique_ptr<BpmDetector> createBpmDetector();