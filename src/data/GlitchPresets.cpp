#include "GlitchPresets.h"
#include "TrackData.h"

static const GlitchPreset kFactoryPresets[] = {
    {"Dub Siren",
     {"---------------F", "F-----------C---", "-------P-------F", "----C------F----", "--P---------F--C",
      "F----C-----P----", "---F--C-P------L", "P--F----C--L----"}},

    {"Jungle Fold",
     {"R--S--RS-B---SBR", "RB-S----SB---S-R", "R--SB-R--BS--SBR", "RBS---RS-BS----R", "R--SB-RS-B-R--BR",
      "RBSS--R--BS--S-R", "R--S-BRS-BSR-S-R", "RB-SB-R--B---SBR"}},

    {"Hollow Room",
     {"--L-C-P-C-P-PL--", "----C---C-P-P---", "--L-C---C---PL--", "P-L-C-P-C-P-P---", "--L-C-P-----PL-P",
      "--L---P-C-P-PL--", "P---C-P-C---PL-P", "--LPC---C-P--L--"}},

    {"Broken Console",
     {"IDD-I-DD-D--IDD-", "I-D-I-------I-D-", "IDD-I--D----IDD-", "I-D-IDDD-D--I-D-", "IDD-I-DD----IDDD",
      "I-DDI--D-D--I-D-", "IDD-ID-D-D--IDD-", "I-D-I-DD-D--IDDD"}},

    {"Half Time Ghost",
     {"B---G---RB---G--", "B-------RB------", "B---G---R----G--", "B-S-G---RB-S----", "B---G---RB---G-F",
      "B-S-----RB-S-G--", "BF--G---R--F-G--", "B---GF--RB---GF-"}},

    {"Needle Drop",
     {"-B-S---R-B-S---R", "---S---R-------R", "-B-S-G-R-B-S---R", "---S---R-B-S-G-R", "-B-S---R---S---R",
      "-BFS---R-B-S--FR", "---S-G-R-B-S---R", "-B-S---RFB-S---R"}},

    {"Sub Rattle",
     {"DI--DI--DI--DI--", "D---D---D---D---", "DI--D---DI--D---", "DIG-DI--DI--DI-G", "D-I-DI--D-I-DI--",
      "DI--DIG-DI--DI--", "DIB-DI--DIB-DI--", "DI--D-I-DI--D-I-"}},

    {"Vocal Cut",
     {"--G-GB--B---GBG-", "--G-G-------G-G-", "--G-G---B---G-G-", "R-G-GB------GBG-", "--G-GB--B-S-GBG-",
      "--G-G-R-B---G-G-", "R-G-GB--BR--GBG-", "--G-GB--B---GSG-"}},

    {"Tape Bleed",
     {"L---C-I-I-L-C-I-", "----C---I---C---", "L---C---I-L-C---", "L-D-C-I-I-L-C-ID", "L---C-I---L-C-I-",
      "LR--C-I-I-L-C-I-", "L---C-IDI-L-C---", "L---CDI-I-LDC-I-"}},

    {"Kick Killer",
     {"G-B-G---G-B-G---", "G-------G-------", "G-B-----G-B-----", "G-B-GD--G-B-G---", "G-B-G---G-B-GD--",
      "GBB-G---GBB-G---", "G-B-G-R-G-B-G---", "G-B-GD--GBB-G---"}},

    {"Slow Bloom",
     {"----------C-----", "------C---------", "--------C-----P-", "----P-----C-----", "------C-------P-",
      "--P-------C---L-", "------L---C-----", "----C---------P-"}},

    {"Panic Room",
     {"XX--X---XX--X---", "X---X-------X---", "XX------XX------", "X-X-X---X-X-X---", "XX--X---X---X-X-",
      "X---XX--XX--X---", "XX--X-X-XX--X---", "X-X-X---XX--XX--"}},

    {"Rubber Band",
     {"P-L-P---P-L-P---", "P---P-------P---", "P-L-----P-L-----", "PLL-P---P-L-P---", "P-L-P---PLL-P---",
      "P-L-PL--P-L-P---", "PL--P---P-L-PL--", "P-L-P-L-P-L-P---"}},

    {"Grain Storm",
     {"-S-S--S--S-S--S-", "-S----S----S----", "-S-S--S--S------", "SS-S--S--S-S--SS", "-S-S---S-S-S--S-",
      "-SBS--S--S-S-BS-", "-S-S--SB-S-S--S-", "SS-S--S--SBS--S-"}},

    {"Filter Sweep",
     {"F---F---F---F---", "F-------F-------", "F---F-------F---", "FF--F---F---FF--", "F---F--FF---F---",
      "F-C-F---F-C-F---", "F---FC--F---F-C-", "FF--F---FF--F---"}},

    {"Cassette Warp",
     {"--I---L---I---L-", "--I-------I-----", "------L---I---L-", "--I-C-L---I---L-", "--I---L-C-I---L-",
      "-DI---L---I--DL-", "--I---LD--I---L-", "--I-C-L---I-C-L-"}},

    {"Trip Wire",
     {"-B--R---B---R---", "-B------B-------", "----R-------R---", "-B--R---B-S-R---", "-B--RS--B---R---",
      "-BG-R---B---RG--", "-B--R---BG--R-S-", "-BS-R---B---R---"}},

    {"Metal Sheet",
     {"D-I-D---D-I-D---", "D---D-------D---", "D-I-----D-I-----", "DDI-D---D-I-DD--", "D-I-D--DD-I-D---",
      "D-I-DL--D-I-D---", "DLI-D---D-I-DL--", "D-I-D---DDI-D-L-"}},

    {"Empty Church",
     {"--------C-------", "----C-----------", "--------C-----C-", "--P-----C-------", "----C---C-----P-",
      "--------CP------", "----C-------C---", "--P-----C-----P-"}},

    {"Hard Chop",
     {"GB--GB--GB--GB--", "G---G---G---G---", "GB--G---GB--G---", "GBB-GB--GB--GBB-", "GB--GBB-GB--GB--",
      "GB-DGB--GB--GB-D", "GB--GB-DGBB-GB--", "GBD-GB--GB--GBD-"}},
};

const GlitchPreset *getFactoryGlitchPresets()
{
	return kFactoryPresets;
}

int getNumFactoryGlitchPresets()
{
	return (int)(sizeof(kFactoryPresets) / sizeof(kFactoryPresets[0]));
}

GlitchEffectType glitchEffectFromChar(char c)
{
	switch (c)
	{
	case 'R':
		return GlitchEffectType::Reverse;
	case 'S':
		return GlitchEffectType::TransientScatter;
	case 'B':
		return GlitchEffectType::BeatRepeat;
	case 'G':
		return GlitchEffectType::Gate;
	case 'F':
		return GlitchEffectType::Filter;
	case 'C':
		return GlitchEffectType::Chorus;
	case 'P':
		return GlitchEffectType::Phaser;
	case 'L':
		return GlitchEffectType::Flanger;
	case 'I':
		return GlitchEffectType::BitCrusher;
	case 'D':
		return GlitchEffectType::Distortion;
	case 'X':
		return GlitchEffectType::Random;
	default:
		return GlitchEffectType::None;
	}
}

char charFromGlitchEffect(GlitchEffectType type)
{
	switch (type)
	{
	case GlitchEffectType::Reverse:
		return 'R';
	case GlitchEffectType::TransientScatter:
		return 'S';
	case GlitchEffectType::BeatRepeat:
		return 'B';
	case GlitchEffectType::Gate:
		return 'G';
	case GlitchEffectType::Filter:
		return 'F';
	case GlitchEffectType::Chorus:
		return 'C';
	case GlitchEffectType::Phaser:
		return 'P';
	case GlitchEffectType::Flanger:
		return 'L';
	case GlitchEffectType::BitCrusher:
		return 'I';
	case GlitchEffectType::Distortion:
		return 'D';
	case GlitchEffectType::Random:
		return 'X';
	default:
		return '-';
	}
}

void applyGlitchSequencesFromStrings(TrackData &track, const juce::String sequences[8])
{
	for (int s = 0; s < 8; ++s)
	{
		auto &seq = track.glitchSequences[s];
		const juce::String &pattern = sequences[s];

		seq.clearAllSteps();
		seq.setStepLength(1);

		const int len = juce::jmin(pattern.length(), GlitchSequence::MAX_STEPS);
		for (int i = 0; i < len; ++i)
			seq.setStep(i, glitchEffectFromChar((char)pattern[i]));

		seq.setNumSteps(len > 0 ? len : 16);
	}

	auto &meta = track.glitchMetaSequence;
	for (int i = 0; i < GlitchMetaSequence::MAX_STEPS; ++i)
		meta.setStep(i, i < 8 ? i + 1 : 0);
	meta.setNumSteps(8);

	track.metaCurrentStep.store(-1);
	track.glitchCycleStartPpq.store(-1.0);
	track.lastTriggeredGlitchStep.store(-1);
}

void applyFactoryGlitchPreset(TrackData &track, int presetIndex)
{
	if (presetIndex < 0 || presetIndex >= getNumFactoryGlitchPresets())
		return;

	const auto &preset = kFactoryPresets[presetIndex];

	juce::String sequences[8];
	for (int s = 0; s < 8; ++s)
		sequences[s] = preset.sequences[s];

	applyGlitchSequencesFromStrings(track, sequences);
}