#include "ColourPalette.h"

constexpr auto COLOR_SUCCESS = 0xffF5F5F5;
constexpr auto COLOR_WARNING = 0xffFFFFFF;
constexpr auto COLOR_DANGER = 0xffB8605C;
constexpr auto COLOR_DANGER_LIGHT = 0xffC97571;
constexpr auto COLOR_DANGER_DARK = 0xff8B4545;
constexpr auto COLOR_PRIMARY = 0xffB8605C;
constexpr auto COLOR_SECONDARY = 0xffB8605C;
constexpr auto COLOR_BG_DEEP = 0xffFFFFFF;
constexpr auto COLOR_BG_DARK = 0xffFAFAFA;
constexpr auto COLOR_BG_MID = 0xffF5F5F5;
constexpr auto COLOR_BG_LIGHT = 0xffF0F0F0;
constexpr auto COLOR_TEXT_PRIMARY = 0xff1A1A1A;
constexpr auto COLOR_TEXT_SECONDARY = 0xff4A4A4A;
constexpr auto COLOR_TEXT_ACCENT = 0xffB8605C;
constexpr auto COLOR_INACTIVE = 0xffCCCCCC;
constexpr auto COLOR_SELECTED = 0xffB8605C;
constexpr auto COLOR_PLAY_ARMED = 0xffD4A5A0;
constexpr auto COLOR_SOLO_ACTIVE = 0xffB8605C;
constexpr auto COLOR_SOLO_TEXT = 0xffFFFFFF;
constexpr auto COLOR_STOP_ACTIVE = 0xff8B4545;
constexpr auto COLOR_TRACK1 = 0xffB8605C;
constexpr auto COLOR_TRACK2 = 0xffB8605C;
constexpr auto COLOR_TRACK3 = 0xffB8605C;
constexpr auto COLOR_TRACK4 = 0xffB8605C;
constexpr auto COLOR_TRACK5 = 0xffB8605C;
constexpr auto COLOR_TRACK6 = 0xffB8605C;
constexpr auto COLOR_TRACK7 = 0xffB8605C;
constexpr auto COLOR_TRACK8 = 0xffB8605C;
constexpr auto COLOR_SEQUENCER_ACCENT = 0xffB8605C;
constexpr auto COLOR_SEQUENCER_BEAT = 0xff1A1A1A;
constexpr auto COLOR_SEQUENCER_SUBBEAT = 0xffCCCCCC;
constexpr auto COLOR_CREDITS = 0xff4A4A4A;
constexpr auto COLOR_VIOLET = 0xff8B4545;
constexpr auto COLOR_EMERALD = 0xffB8605C;
constexpr auto COLOR_CORAL = 0xffB8605C;
constexpr auto COLOR_SLATE = 0xff1A1A1A;
constexpr auto COLOR_INDIGO = 0xff1A1A1A;
constexpr auto COLOR_TEAL = 0xffB8605C;
constexpr auto COLOR_AMBER = 0xffF5F5F5;
constexpr auto COLOR_VU_GREEN = 0xffD4A5A0;
constexpr auto COLOR_VU_ORANGE = 0xffC97571;
constexpr auto COLOR_VU_RED = 0xffB8605C;
constexpr auto COLOR_PLAY_ACTIVE = 0xff8B4545;
constexpr auto COLOR_SAMPLE_PENDING = COLOR_PLAY_ARMED;

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