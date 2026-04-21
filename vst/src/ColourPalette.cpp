#include "ColourPalette.h"

// === CORE BACKGROUNDS — deep obsidian blacks, layered for depth ===
constexpr auto COLOR_BG_DEEP = 0xff1A1A1C;   // deepest — main window background
constexpr auto COLOR_BG_DARK = 0xff222225;   // slightly lifted — track rows
constexpr auto COLOR_BG_MID = 0xff2B2B2F;   // panels / button inactive
constexpr auto COLOR_BG_LIGHT = 0xff3A3A3F;  // borders / subtle dividers

// === TEXT — warm whites and cool greys ===
constexpr auto COLOR_TEXT_PRIMARY = 0xffE8E6E1;  // warm off-white (not pure white — less harsh)
constexpr auto COLOR_TEXT_SECONDARY = 0xff9A9A9E;  // mid grey for labels
constexpr auto COLOR_TEXT_ACCENT = 0xffD96850;  // brick — for highlights
constexpr auto COLOR_INACTIVE = 0xff55555A;  // disabled / dimmed

// === BRAND ACCENT — saturated brick red (punches on dark bg) ===
constexpr auto COLOR_PRIMARY = 0xffD96850;  // main brick accent (saturated)
constexpr auto COLOR_SECONDARY = 0xffD96850;
constexpr auto COLOR_DANGER = 0xffE07060;  // slightly warmer for errors
constexpr auto COLOR_DANGER_LIGHT = 0xffEB8777;
constexpr auto COLOR_DANGER_DARK = 0xffA04840;  // deeper brick for pressed states
constexpr auto COLOR_SUCCESS = 0xff6BB38A;  // muted teal-green (not neon)
constexpr auto COLOR_WARNING = 0xffE8A860;  // warm amber

// === SEQUENCER ===
constexpr auto COLOR_SEQUENCER_ACCENT = 0xffD96850;
constexpr auto COLOR_SEQUENCER_BEAT = 0xff4A4A50;
constexpr auto COLOR_SEQUENCER_SUBBEAT = 0xff32323A;

// === VU METERS — saturated for contrast on dark bg ===
constexpr auto COLOR_VU_GREEN = 0xff6BB38A;  // lively green
constexpr auto COLOR_VU_ORANGE = 0xffE8A860;  // warm amber
constexpr auto COLOR_VU_RED = 0xffE07060;  // saturated brick

// === UTILITY / BRAND ACCENTS (for model palette variety) ===
constexpr auto COLOR_VIOLET = 0xff8B6AB5;  // purple — distinctive
constexpr auto COLOR_EMERALD = 0xff4FA88C;  // teal-green
constexpr auto COLOR_CORAL = 0xffD96850;  // main brick
constexpr auto COLOR_SLATE = 0xff6B8299;  // cool blue-grey
constexpr auto COLOR_INDIGO = 0xff5568A0;  // deep blue
constexpr auto COLOR_TEAL = 0xff4DA3B3;  // cyan teal
constexpr auto COLOR_AMBER = 0xffD9A54E;  // gold amber	
constexpr auto COLOR_CHARCOAL = 0xff2B2B2F;

// === TRACK / SELECTION INDICATORS ===
constexpr auto COLOR_SELECTED = 0xff9A9A9A;
constexpr auto COLOR_PLAY_ARMED = 0xffE8A35E;  // amber (armed state — warm warning)
constexpr auto COLOR_PLAY_ACTIVE = 0xff6BB38A;  // green (currently playing)
constexpr auto COLOR_STOP_ACTIVE = 0xffA04840;  // deep brick
constexpr auto COLOR_SOLO_ACTIVE = 0xffD96850;
constexpr auto COLOR_SOLO_TEXT = 0xffE8E6E1;

constexpr auto COLOR_CREDITS = 0xff9A9A9E;
constexpr auto COLOR_SAMPLE_PENDING = COLOR_TEAL;

// === TRACK COLOURS (8 distinct accents for track identity) ===
constexpr auto COLOR_TRACK1 = 0xffD96850;  // brick (main)
constexpr auto COLOR_TRACK2 = 0xff4DA3B3;  // teal
constexpr auto COLOR_TRACK3 = 0xff8B6AB5;  // violet
constexpr auto COLOR_TRACK4 = 0xffD9A54E;  // amber
constexpr auto COLOR_TRACK5 = 0xff6BB38A;  // green
constexpr auto COLOR_TRACK6 = 0xff5568A0;  // indigo
constexpr auto COLOR_TRACK7 = 0xffCB7AA8;  // rose
constexpr auto COLOR_TRACK8 = 0xff6B8299;  // slate

constexpr auto COLOR_LIME = 0xff8BBF5A;
constexpr auto COLOR_ROSE = 0xffCB7AA8;
constexpr auto COLOR_CYAN = 0xff4DC4D4;

const juce::Colour ColourPalette::track1(COLOR_TRACK1);
const juce::Colour ColourPalette::track2(COLOR_TRACK2);
const juce::Colour ColourPalette::track3(COLOR_TRACK3);
const juce::Colour ColourPalette::track4(COLOR_TRACK4);
const juce::Colour ColourPalette::track5(COLOR_TRACK5);
const juce::Colour ColourPalette::track6(COLOR_TRACK6);
const juce::Colour ColourPalette::track7(COLOR_TRACK7);
const juce::Colour ColourPalette::track8(COLOR_TRACK8);

const juce::Colour ColourPalette::buttonPrimary(COLOR_PRIMARY);
const juce::Colour ColourPalette::buttonSecondary(COLOR_SECONDARY);
const juce::Colour ColourPalette::buttonDanger(COLOR_DANGER);
const juce::Colour ColourPalette::buttonSuccess(COLOR_SUCCESS);
const juce::Colour ColourPalette::buttonWarning(COLOR_WARNING);
const juce::Colour ColourPalette::buttonDangerLight(COLOR_DANGER_LIGHT);
const juce::Colour ColourPalette::buttonDangerDark(COLOR_DANGER_DARK);

const juce::Colour ColourPalette::backgroundDark(COLOR_BG_DARK);
const juce::Colour ColourPalette::backgroundMid(COLOR_BG_MID);
const juce::Colour ColourPalette::backgroundLight(COLOR_BG_LIGHT);
const juce::Colour ColourPalette::backgroundDeep(COLOR_BG_DEEP);

const juce::Colour ColourPalette::textPrimary(COLOR_TEXT_PRIMARY);
const juce::Colour ColourPalette::textSecondary(COLOR_TEXT_SECONDARY);
const juce::Colour ColourPalette::textDanger(COLOR_DANGER_LIGHT);
const juce::Colour ColourPalette::textSuccess(COLOR_SUCCESS);
const juce::Colour ColourPalette::textWarning(COLOR_WARNING);
const juce::Colour ColourPalette::textAccent(COLOR_TEXT_ACCENT);

const juce::Colour ColourPalette::sliderThumb(COLOR_PRIMARY);
const juce::Colour ColourPalette::sliderTrack(COLOR_INACTIVE);

const juce::Colour ColourPalette::vuPeak(COLOR_TEXT_PRIMARY);
const juce::Colour ColourPalette::vuClipping(COLOR_DANGER);
const juce::Colour ColourPalette::vuGreen(COLOR_VU_GREEN);
const juce::Colour ColourPalette::vuOrange(COLOR_VU_ORANGE);
const juce::Colour ColourPalette::vuRed(COLOR_VU_RED);

const juce::Colour ColourPalette::playActive(COLOR_PLAY_ACTIVE);
const juce::Colour ColourPalette::playArmed(COLOR_PLAY_ARMED);
const juce::Colour ColourPalette::muteActive(COLOR_DANGER);
const juce::Colour ColourPalette::soloActive(COLOR_SOLO_ACTIVE);
const juce::Colour ColourPalette::soloText(COLOR_SOLO_TEXT);
const juce::Colour ColourPalette::stopActive(COLOR_STOP_ACTIVE);
const juce::Colour ColourPalette::buttonInactive(COLOR_INACTIVE);
const juce::Colour ColourPalette::trackSelected(COLOR_SELECTED);

const juce::Colour ColourPalette::sequencerAccent(COLOR_SEQUENCER_ACCENT);
const juce::Colour ColourPalette::sequencerBeat(COLOR_SEQUENCER_BEAT);
const juce::Colour ColourPalette::sequencerSubBeat(COLOR_SEQUENCER_SUBBEAT);

const juce::Colour ColourPalette::credits(COLOR_CREDITS);

const juce::Colour ColourPalette::violet(COLOR_VIOLET);
const juce::Colour ColourPalette::emerald(COLOR_EMERALD);
const juce::Colour ColourPalette::coral(COLOR_CORAL);
const juce::Colour ColourPalette::slate(COLOR_SLATE);
const juce::Colour ColourPalette::indigo(COLOR_INDIGO);
const juce::Colour ColourPalette::teal(COLOR_TEAL);
const juce::Colour ColourPalette::amber(COLOR_AMBER);
const juce::Colour ColourPalette::textInactive(COLOR_INACTIVE);

const juce::Colour ColourPalette::samplePending(COLOR_SAMPLE_PENDING);

const juce::Colour ColourPalette::modelStableAudio(COLOR_SLATE);
const juce::Colour ColourPalette::modelFoundation(COLOR_CORAL);
const juce::Colour ColourPalette::modelEdm(COLOR_TEAL);
const juce::Colour ColourPalette::modelPianos(COLOR_AMBER);
const juce::Colour ColourPalette::modelVocals(COLOR_VIOLET);
const juce::Colour ColourPalette::modelInstrumental(COLOR_EMERALD);
const juce::Colour ColourPalette::modelBeats(COLOR_INDIGO);
const juce::Colour ColourPalette::modelGluten(0xffCB7AA8);

const juce::Colour ColourPalette::charcoal(COLOR_CHARCOAL);

const juce::Colour ColourPalette::lime(COLOR_LIME);
const juce::Colour ColourPalette::cyan(COLOR_CYAN);

juce::Colour ColourPalette::getTrackColour(int trackIndex)
{
	static const std::vector<juce::Colour> trackColours = {
		track1, track2, track3, track4, track5, track6, track7, track8 };
	return trackColours[trackIndex % trackColours.size()];
}

juce::Colour ColourPalette::withAlpha(const juce::Colour& colour, float alpha)
{
	return colour.withAlpha(alpha);
}

juce::Colour ColourPalette::darken(const juce::Colour& colour, float amount)
{
	return colour.darker(amount);
}

juce::Colour ColourPalette::lighten(const juce::Colour& colour, float amount)
{
	return colour.brighter(amount);
}

juce::Colour ColourPalette::getModelColourByIndex(int index)
{
	switch (index % 8)
	{
	case 0: return modelStableAudio;
	case 1: return modelFoundation;
	case 2: return modelEdm;
	case 3: return modelPianos;
	case 4: return modelVocals;
	case 5: return modelInstrumental;
	case 6: return modelBeats;
	case 7: return modelGluten;
	default: return modelFoundation;
	}
}