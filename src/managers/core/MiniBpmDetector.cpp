#include "BpmDetector.h"
#include "MiniBpm.h"

class MiniBpmDetector : public BpmDetector
{
  public:
	double detectTempo(double hostBpm, const float *channelData, int numSamples, double sampleRate) override
	{
		breakfastquay::MiniBPM bpm(static_cast<float>(sampleRate));
		bpm.setBPMRange(hostBpm - 20.0, hostBpm + 20.0);
		bpm.setBeatsPerBar(4);
		bpm.process(channelData, numSamples);
		double tempo = bpm.estimateTempo();
		bpm.reset();
		return tempo;
	}
};

std::unique_ptr<BpmDetector> createBpmDetector()
{
	return std::make_unique<MiniBpmDetector>();
}