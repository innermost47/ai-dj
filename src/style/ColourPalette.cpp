#include "ColourPalette.h"

constexpr auto OBSIDIAN = 0xff0D0D0E;
constexpr auto CHARCOAL_DARK = 0xff141416;
constexpr auto CHARCOAL = 0xff1C1C1E;
constexpr auto CHARCOAL_LIGHT = 0xff28282B;

constexpr auto WARM_WHITE = 0xffE8E6E1;
constexpr auto MEDIUM_GREY = 0xff9A9A9E;
constexpr auto DARK_GREY = 0xff55555A;
constexpr auto STEEL_GREY = 0xff9A9A9A;
constexpr auto COOL_GREY = 0xff7A8590;

constexpr auto TERRACOTTA = 0xffD96850;
constexpr auto TERRACOTTA_LIGHT = 0xffE07060;
constexpr auto TERRACOTTA_PALE = 0xffEB8777;
constexpr auto TERRACOTTA_DARK = 0xffA04840;

constexpr auto SAGE_GREEN = 0xff6BB38A;
constexpr auto AMBER = 0xffE8A860;
constexpr auto AMBER_WARM = 0xffE8A35E;
constexpr auto GOLD = 0xffD9A54E;

constexpr auto MAUVE = 0xff8B6AB5;
constexpr auto TEAL = 0xff4DA3B3;
constexpr auto CYAN = 0xff4DC4D4;
constexpr auto EMERALD = 0xff4FA88C;
constexpr auto SLATE_BLUE = 0xff6B8299;
constexpr auto INDIGO = 0xff5568A0;
constexpr auto ROSE = 0xffCB7AA8;
constexpr auto LIME = 0xff8BBF5A;

constexpr auto SEQUENCER_BEAT = 0xff4A4A50;
constexpr auto SEQUENCER_SUBBEAT = 0xff32323A;
constexpr auto SAMPLE_PENDING = 0x40aaaaaa;

constexpr auto MUTED_STEEL = 0xff5A6770;
constexpr auto MOSS_GREEN = 0xff7B8A5E;

constexpr auto DEEP_SLATE = 0xff2D3640;

const juce::Colour ColourPalette::track1(TERRACOTTA);
const juce::Colour ColourPalette::track2(TEAL);
const juce::Colour ColourPalette::track3(MAUVE);
const juce::Colour ColourPalette::track4(GOLD);
const juce::Colour ColourPalette::track5(SAGE_GREEN);
const juce::Colour ColourPalette::track6(INDIGO);
const juce::Colour ColourPalette::track7(ROSE);
const juce::Colour ColourPalette::track8(SLATE_BLUE);

const juce::Colour ColourPalette::buttonPrimary(TERRACOTTA);
const juce::Colour ColourPalette::buttonSecondary(TERRACOTTA);
const juce::Colour ColourPalette::buttonDanger(TERRACOTTA_LIGHT);
const juce::Colour ColourPalette::buttonSuccess(SAGE_GREEN);
const juce::Colour ColourPalette::buttonWarning(AMBER);
const juce::Colour ColourPalette::buttonDangerLight(TERRACOTTA_PALE);
const juce::Colour ColourPalette::buttonDangerDark(TERRACOTTA_DARK);

const juce::Colour ColourPalette::backgroundDark(CHARCOAL_DARK);
const juce::Colour ColourPalette::backgroundMid(CHARCOAL);
const juce::Colour ColourPalette::backgroundLight(CHARCOAL_LIGHT);
const juce::Colour ColourPalette::backgroundDeep(OBSIDIAN);

const juce::Colour ColourPalette::textPrimary(WARM_WHITE);
const juce::Colour ColourPalette::textSecondary(MEDIUM_GREY);
const juce::Colour ColourPalette::textDanger(TERRACOTTA_PALE);
const juce::Colour ColourPalette::textSuccess(SAGE_GREEN);
const juce::Colour ColourPalette::textWarning(AMBER);
const juce::Colour ColourPalette::textAccent(TERRACOTTA);
const juce::Colour ColourPalette::textInactive(DARK_GREY);

const juce::Colour ColourPalette::sliderThumb(TERRACOTTA);
const juce::Colour ColourPalette::sliderTrack(DARK_GREY);

const juce::Colour ColourPalette::vuPeak(WARM_WHITE);
const juce::Colour ColourPalette::vuClipping(TERRACOTTA_LIGHT);
const juce::Colour ColourPalette::vuGreen(SAGE_GREEN);
const juce::Colour ColourPalette::vuOrange(AMBER);
const juce::Colour ColourPalette::vuRed(TERRACOTTA_LIGHT);

const juce::Colour ColourPalette::playActive(MOSS_GREEN);
const juce::Colour ColourPalette::playArmed(AMBER_WARM);
const juce::Colour ColourPalette::muteActive(MUTED_STEEL);
const juce::Colour ColourPalette::soloActive(AMBER_WARM);
const juce::Colour ColourPalette::stopActive(TERRACOTTA);
const juce::Colour ColourPalette::soloText(WARM_WHITE);
const juce::Colour ColourPalette::buttonInactive(DARK_GREY);
const juce::Colour ColourPalette::lightGrey(STEEL_GREY);

const juce::Colour ColourPalette::sequencerAccent(TERRACOTTA);
const juce::Colour ColourPalette::sequencerBeat(SEQUENCER_BEAT);
const juce::Colour ColourPalette::sequencerSubBeat(SEQUENCER_SUBBEAT);

const juce::Colour ColourPalette::credits(MEDIUM_GREY);

const juce::Colour ColourPalette::violet(MAUVE);
const juce::Colour ColourPalette::emerald(EMERALD);
const juce::Colour ColourPalette::coral(TERRACOTTA);
const juce::Colour ColourPalette::slate(SLATE_BLUE);
const juce::Colour ColourPalette::indigo(INDIGO);
const juce::Colour ColourPalette::teal(TEAL);
const juce::Colour ColourPalette::amber(GOLD);
const juce::Colour ColourPalette::charcoal(CHARCOAL);
const juce::Colour ColourPalette::lime(LIME);
const juce::Colour ColourPalette::cyan(CYAN);
const juce::Colour ColourPalette::mossGreen(MOSS_GREEN);

const juce::Colour ColourPalette::samplePending(SAMPLE_PENDING);
const juce::Colour ColourPalette::loopLocked(COOL_GREY);

const juce::Colour ColourPalette::modelStableAudio(SLATE_BLUE);
const juce::Colour ColourPalette::modelFoundation(TERRACOTTA);
const juce::Colour ColourPalette::modelEdm(TEAL);
const juce::Colour ColourPalette::modelPianos(GOLD);
const juce::Colour ColourPalette::modelVocals(MAUVE);
const juce::Colour ColourPalette::modelInstrumental(EMERALD);
const juce::Colour ColourPalette::modelBeats(INDIGO);
const juce::Colour ColourPalette::modelGluten(ROSE);
const juce::Colour ColourPalette::modelStableAudioTflite(CYAN);
const juce::Colour ColourPalette::modelStableAudio3(LIME);

const juce::Colour ColourPalette::modalHeader(DEEP_SLATE);

juce::Colour ColourPalette::getTrackColour(int trackIndex)
{
	static const std::vector<juce::Colour> trackColours = {track1, track2, track3, track4,
	                                                       track5, track6, track7, track8};
	return trackColours[trackIndex % trackColours.size()];
}

juce::Colour ColourPalette::withAlpha(const juce::Colour &colour, float alpha)
{
	return colour.withAlpha(alpha);
}

juce::Colour ColourPalette::getModelColourByIndex(int index)
{
	switch (index % 8)
	{
	case 0:
		return modelStableAudio;
	case 1:
		return modelStableAudio3;
	case 2:
		return modelFoundation;
	case 3:
		return modelEdm;
	case 4:
		return modelPianos;
	case 5:
		return modelVocals;
	case 6:
		return modelInstrumental;
	case 7:
		return modelBeats;
	case 8:
		return modelGluten;
	default:
		return modelFoundation;
	}
}