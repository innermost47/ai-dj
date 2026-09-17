#include "ModulationTypes.h"
#include "TrackData.h"

static const ModTargetDef kModTargets[] = {
    {"None", nullptr, nullptr, nullptr},

    {"Cutoff", "Cutoff", [](const TrackData &t) { return t.filter.isBypassed(); },
     [](TrackData &t, float v) { t.filter.setCutoffFrequency(v); }},
    {"Resonance", "Resonance", [](const TrackData &t) { return t.filter.isBypassed(); },
     [](TrackData &t, float v) { t.filter.setResonance(v); }},
    {"Filter Drive", "FilterDrive", [](const TrackData &t) { return t.filter.isBypassed(); },
     [](TrackData &t, float v) { t.filter.setDrive(v); }},

    {"Chorus Rate", "ChorusRate", [](const TrackData &t) { return t.chorus.isBypassed(); },
     [](TrackData &t, float v) { t.chorus.setRate(v); }},
    {"Chorus Depth", "ChorusDepth", [](const TrackData &t) { return t.chorus.isBypassed(); },
     [](TrackData &t, float v) { t.chorus.setDepth(v); }},
    {"Chorus Feedback", "ChorusFeedback", [](const TrackData &t) { return t.chorus.isBypassed(); },
     [](TrackData &t, float v) { t.chorus.setFeedback(v); }},
    {"Chorus Centre", "ChorusCentre", [](const TrackData &t) { return t.chorus.isBypassed(); },
     [](TrackData &t, float v) { t.chorus.setCentre(v); }},

    {"Phaser Rate", "PhaserRate", [](const TrackData &t) { return t.phaser.isBypassed(); },
     [](TrackData &t, float v) { t.phaser.setRate(v); }},
    {"Phaser Depth", "PhaserDepth", [](const TrackData &t) { return t.phaser.isBypassed(); },
     [](TrackData &t, float v) { t.phaser.setDepth(v); }},
    {"Phaser Feedback", "PhaserFeedback", [](const TrackData &t) { return t.phaser.isBypassed(); },
     [](TrackData &t, float v) { t.phaser.setFeedback(v); }},
    {"Phaser Centre", "PhaserCentre", [](const TrackData &t) { return t.phaser.isBypassed(); },
     [](TrackData &t, float v) { t.phaser.setCentre(v); }},

    {"Flanger Rate", "FlangerRate", [](const TrackData &t) { return t.flanger.isBypassed(); },
     [](TrackData &t, float v) { t.flanger.setRate(v); }},
    {"Flanger Depth", "FlangerDepth", [](const TrackData &t) { return t.flanger.isBypassed(); },
     [](TrackData &t, float v) { t.flanger.setDepth(v); }},
    {"Flanger Feedback", "FlangerFeedback", [](const TrackData &t) { return t.flanger.isBypassed(); },
     [](TrackData &t, float v) { t.flanger.setFeedback(v); }},
    {"Flanger Centre", "FlangerCentre", [](const TrackData &t) { return t.flanger.isBypassed(); },
     [](TrackData &t, float v) { t.flanger.setCentre(v); }},

    {"Bitcrush Depth", "BitCrusherBitDepth", [](const TrackData &t) { return t.bitCrusher.isBypassed(); },
     [](TrackData &t, float v) { t.bitCrusher.setBitDepth(v); }},
    {"Bitcrush Rate", "BitCrusherRate", [](const TrackData &t) { return t.bitCrusher.isBypassed(); },
     [](TrackData &t, float v) { t.bitCrusher.setSampleRateReduction(v); }},

    {"Dist PreGain", "DistortionPreGain", [](const TrackData &t) { return t.distortion.isBypassed(); },
     [](TrackData &t, float v) { t.distortion.setPre(v); }},
    {"Dist Cut", "DistortionCut", [](const TrackData &t) { return t.distortion.isBypassed(); },
     [](TrackData &t, float v) { t.distortion.setCut(v); }},

    {"Gate Duration", "GateDuration", [](const TrackData &t) { return t.gate.isBypassed(); },
     [](TrackData &t, float v) { t.gate.setDuration(v); }},
    {"Gate Depth", "GateDepth", [](const TrackData &t) { return t.gate.isBypassed(); },
     [](TrackData &t, float v) { t.gate.setDepth(v); }},

    {"Pan", "Pan", nullptr, [](TrackData &t, float v) { t.pan.store(v); }},
    {"Delay Send", "DelaySend", nullptr, [](TrackData &t, float v) { t.delaySend.store(v); }},
    {"Reverb Send", "ReverbSend", nullptr, [](TrackData &t, float v) { t.reverbSend.store(v); }},
};

const ModTargetDef *getModTargets()
{
	return kModTargets;
}

int getNumModTargets()
{
	return (int)(sizeof(kModTargets) / sizeof(kModTargets[0]));
}

juce::StringArray getModTargetNames()
{
	juce::StringArray names;
	for (int i = 0; i < getNumModTargets(); ++i)
		names.add(kModTargets[i].displayName);
	return names;
}