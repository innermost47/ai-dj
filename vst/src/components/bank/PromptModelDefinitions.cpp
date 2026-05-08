#include "PromptModelDefinitions.h"

const std::vector<PromptModelDefinitions::ModelInfo> &PromptModelDefinitions::getAllModels()
{
	static std::vector<ModelInfo> models = {
	    {"stable-audio-open-1.0",
	     "Natural language descriptive prompts. Format: [Style/Genre], [Key Elements], [Mood], [Details]",
	     {
	         {"Genres",
	          {"techno", "house", "trap", "drum and bass", "ambient", "hip-hop", "jungle", "dub", "industrial",
	           "hardcore", "breakbeat", "lo-fi", "synthwave", "psychedelic"}},
	         {"Elements",
	          {"kick drum loop", "bassline", "hi-hat loop", "snare pattern", "percussion loop", "synth melody", "lead",
	           "pad", "arpeggio", "vocal chops", "noise stabs"}},
	         {"Moods",
	          {"dark", "warm", "aggressive", "chill", "melancholic", "euphoric", "dreamy", "haunting", "uplifting",
	           "moody", "raw", "ethereal"}},
	         {"Qualities",
	          {"solo", "isolated", "dry", "rhythmic", "monophonic", "driving", "rolling", "punchy", "crisp", "deep",
	           "warm", "distorted", "metallic", "squelchy"}},
	         {"Negatives", {"no background elements", "no pads", "no melody", "no accompaniment", "no rhythm"}},
	     },
	     {
	         "deep dub kick loop, solo kick pattern, sub-heavy 4/4, dry, no background",
	         "psychedelic ambient flute melody, solo flute, ethereal phrasing, dry, no rhythm",
	         "rolling jungle break loop, solo amen percussion, chopped, dry, isolated",
	         "warm analog lead melody, solo monophonic synth, soft saw wave, dry",
	         "industrial noise stabs, solo distorted hits, rhythmic, dry, no melodic content",
	         "minimal techno hi-hat loop, solo metallic hi-hats, off-beat, dry, isolated",
	     }},

	    {"foundation-1",
	     "Structured tags. Format: [Family], [Timbre], [Notation], [FX]. Comma-separated.",
	     {
	         {"Families",
	          {"Synth", "Keys", "Bass", "Bowed Strings", "Mallet", "Wind", "Guitar", "Brass", "Vocal",
	           "Plucked Strings"}},
	         {"Sub-Families", {"Synth Lead", "Synth Bass", "Grand Piano", "Rhodes Piano", "Digital Piano",
	                           "Violin",     "Cello",      "Trumpet",     "Flute",        "Pan Flute",
	                           "Choir",      "Harp",       "Ocarina",     "Clarinet",     "French Horn",
	                           "Tuba",       "Oboe",       "Supersaw",    "Reese Bass",   "Wavetable Synth",
	                           "Pad",        "Atmosphere", "Texture",     "Bell",         "Pluck"}},
	         {"Timbre",
	          {"Warm",  "Bright", "Wide",    "Airy",   "Thick",  "Rich",    "Gritty", "Clean",     "Dark",  "Analog",
	           "Soft",  "Smooth", "Deep",    "Round",  "Punchy", "Vintage", "Dreamy", "Metallic",  "Crisp", "Focused",
	           "Buzzy", "Growl",  "Breathy", "Glassy", "Noisy",  "303",     "Acid",   "Bitcrushed"}},
	         {"Notation",
	          {"Chord Progression", "Melody", "Top Melody", "Arp", "Triplets", "Simple", "Complex", "Rising", "Falling",
	           "Strummed", "Sustained", "Catchy", "Epic", "Slow Speed", "Fast Speed", "Pitch Bend", "Bassline"}},
	         {"FX",
	          {"Low Reverb", "Medium Reverb", "High Reverb", "Plate Reverb", "Low Delay", "Medium Delay", "High Delay",
	           "Ping Pong Delay", "Stereo Delay", "Mono Delay", "Low Distortion", "Medium Distortion",
	           "High Distortion", "Phaser", "Bitcrush"}},
	     },
	     {
	         "Synth Lead, Wavetable Synth, Bright, Buzzy, Top Melody, Pitch Bend, Ping Pong Delay",
	         "Bass, Reese Bass, Wide, Growl, Bassline, Sustained, Medium Distortion",
	         "Pad, Atmosphere, Warm, Soft, Dreamy, Sustained, High Reverb",
	         "Bowed Strings, Cello, Warm, Rich, Melody, Slow Speed, Plate Reverb",
	         "Pluck, Bell, Bright, Glassy, Arp, Triplets, Stereo Delay",
	         "Synth Bass, 303, Acid, Buzzy, Bassline, Fast Speed",
	     }},

	    {"audialab-edm-elements",
	     "EDM components with speed controls. Format: [Sound Type], [Modifier], [Feel], [Speed], [FX]",
	     {
	         {"Sound Types",
	          {"Pluck", "Bell", "Lead", "Square", "Buzzy", "Legato", "Saw", "Warm", "Supersaw", "Synth", "Punchy Bass",
	           "Sub Bass"}},
	         {"Modifiers",
	          {"triplet feel", "syncopated", "off-beat", "complex motion", "minimalist", "upward motion",
	           "downward motion"}},
	         {"Speed", {"Slow Speed", "Medium Speed", "Fast Speed"}},
	         {"FX",
	          {"Small Reverb", "Medium Reverb", "High Reverb", "Rising Low-Pass", "Falling High-Cut",
	           "Quarter-Beat Gate", "Half-Beat Gate"}},
	     },
	     {
	         "Lead, Saw, Warm, Supersaw, complex motion, Medium Speed, Medium Reverb",
	         "Pluck, Bell, triplet feel, Medium Speed, Small Reverb",
	         "Lead, Square, Buzzy, Legato, syncopated, Fast Speed, Quarter-Beat Gate",
	         "Supersaw, Synth, Saw, upward motion, Fast Speed, Rising Low-Pass",
	         "Punchy Bass, minimalist, Medium Speed, Small Reverb",
	     }},

	    {"rc-infinite-pianos",
	     "Piano performance prompts. Format: [Type], [Modifier], [Phrase], [Tremolo], [Reverb]",
	     {
	         {"Piano Types", {"Grand Piano", "Soft E. Piano", "Medium E. Piano"}},
	         {"Performance",
	          {"simple", "complex", "jazzy", "dance plucky", "fast", "slow", "smooth", "rising", "falling",
	           "simple strummed", "rising strummed", "complex strummed", "jazzy strummed", "slow strummed"}},
	         {"Phrase Types",
	          {"chord progression only", "melody only", "chord progression with top catchy melody",
	           "alternating top arp melody"}},
	         {"Tremolo (E. Piano)", {"Low Tremolo", "Medium Tremolo", "High Tremolo"}},
	         {"Reverb", {"Low Reverb", "Medium Reverb", "High Reverb", "High Spacey Reverb"}},
	     },
	     {
	         "Grand Piano, jazzy, chord progression with top catchy melody, Medium Reverb",
	         "Soft E. Piano, smooth, chord progression only, Low Tremolo, Medium Reverb",
	         "Medium E. Piano, dance plucky, alternating top arp melody, Medium Tremolo, High Reverb",
	         "Grand Piano, slow strummed, melody only, High Spacey Reverb",
	         "Soft E. Piano, complex, chord progression with top catchy melody, Medium Tremolo, High Reverb",
	     }},

	    {"rc-vocal-textures",
	     "Vocal textures and choral progressions. Format: [Vocal Type], Chord Progression, [Tone], [Space]",
	     {
	         {"Vocal Types", {"Male Vocal Texture", "Female Vocal Texture", "Ensemble Vocal Texture"}},
	         {"Character",
	          {"long attacks", "atmospheric", "haunting", "angelic", "operatic", "deep", "rich", "pure", "high",
	           "full choir", "mixed"}},
	         {"Space", {"high reverb", "ethereal space", "washy textures", "cinematic"}},
	         {"Required", {"Chord Progression"}},
	     },
	     {
	         "Female Vocal Texture, Chord Progression, angelic, ethereal space, high reverb",
	         "Male Vocal Texture, Chord Progression, deep, haunting, washy textures, high reverb",
	         "Ensemble Vocal Texture, Chord Progression, operatic, long attacks, atmospheric",
	         "Female Vocal Texture, Chord Progression, pure, ethereal space, high reverb",
	         "Ensemble Vocal Texture, Chord Progression, full choir, cinematic, washy textures",
	     }},

	    {"sao-instrumental",
	     "Modern instrumental stems. Format: [Genre], [Main Inst], [Secondary], [Mood], [Contour]",
	     {
	         {"Genres",
	          {"Cloud Trap", "Melodic Trap", "Lofi Jazz Rap", "Neo-Soul", "Alternative Rock", "British Pop Rock",
	           "Hard Rock", "British 60s Oldies"}},
	         {"Instruments",
	          {"nostalgic piano", "plucked bass", "synth bells", "vocal adlibs", "electric guitar riffs",
	           "deep sub bass", "airy vocal pads", "live bass", "soft Rhodes keys", "warm analog grooves"}},
	         {"Moods",
	          {"Dark", "melancholic", "laid back", "chill", "smooth", "seductive", "romantic", "energetic", "raw",
	           "contemplative", "moody", "boomy"}},
	     },
	     {
	         "Cloud Trap, nostalgic piano, plucked bass, dreamy, melancholic",
	         "Neo-Soul, soft Rhodes keys, live bass, smooth, seductive",
	         "Lofi Jazz Rap, soft Rhodes keys, plucked bass, laid back, chill",
	         "Alternative Rock, electric guitar riffs, warm analog grooves, energetic, raw",
	         "Melodic Trap, synth bells, deep sub bass, dark, moody",
	         "British 60s Oldies, vocal adlibs, airy vocal pads, romantic, contemplative",
	     }},

	    {"stablebeat",
	     "Trap drum and percussion engine. Format: [Solo/Full Beat], Instruments: drum, [Style], [Timbre], [Density]",
	     {
	         {"Format", {"Solo", "Full Beat"}},
	         {"Required", {"Instruments: drum"}},
	         {"Styles",
	          {"cloud trap beat", "melodic trap beat", "boom bap", "jazzy chillhop", "industrial hip-hop", "r&b beat"}},
	         {"Timbre",
	          {"boomy bass", "deep sub", "punchy snare", "crisp hi-hats", "dirty piano loop", "distorted kick",
	           "industrial metallic percussion"}},
	         {"Rhythmic", {"driving 4/4 beat", "syncopated rhythm", "off-beat patterns", "boomy", "rhythmic density"}},
	     },
	     {
	         "Full Beat, Instruments: drum, cloud trap beat, boomy bass, syncopated rhythm",
	         "Solo, Instruments: drum, boom bap, dirty piano loop, driving 4/4 beat",
	         "Full Beat, Instruments: drum, industrial hip-hop, distorted kick, industrial metallic percussion",
	         "Solo, Instruments: drum, jazzy chillhop, crisp hi-hats, off-beat patterns",
	         "Full Beat, Instruments: drum, melodic trap beat, deep sub, punchy snare",
	         "Solo, Instruments: drum, r&b beat, boomy, rhythmic density",
	     }},

	    {"gluten-v1",
	     "Loopable musical phrases. Format: pipe-separated. NO BPM/Key.",
	     {
	         {"Genres", {"Trap", "Melodic Trap", "Wavy Trap", "Hip-Hop", "Boom Bap", "Pop", "Ambient"}},
	         {"Instruments", {"Piano", "Synth Pad", "Synth Lead", "808 Bass", "Bells", "Strings", "Ambient Pads"}},
	         {"Moods",
	          {"Melancholic", "Reflective", "Catchy", "Smooth", "Epic", "Dark", "Atmospheric", "Building", "Ethereal",
	           "Sad", "Heavy", "Driving", "Punchy", "Rhythmic"}},
	         {"Tempo", {"Slow", "Medium", "Fast"}},
	     },
	     {
	         "Format: Solo | Genre: Trap | Sub-Genre: Melodic Trap | Instruments: Piano, Synth Pad | Moods: "
	         "Melancholic | Styles: Catchy, Smooth | Tempo: Medium",
	         "Format: Solo | Genre: Ambient | Sub-Genre: Ambient | Instruments: Synth Pad, Strings | Moods: "
	         "Atmospheric, Ethereal | Styles: Building | Tempo: Slow",
	         "Format: Solo | Genre: Hip-Hop | Sub-Genre: Boom Bap | Instruments: Piano, 808 Bass | Moods: Sad | "
	         "Styles: Smooth | Tempo: Medium",
	         "Format: Solo | Genre: Pop | Sub-Genre: Pop | Instruments: Synth Lead, Bells | Moods: Catchy, Driving | "
	         "Styles: Punchy | Tempo: Fast",
	         "Format: Solo | Genre: Trap | Sub-Genre: Wavy Trap | Instruments: Bells, Synth Lead | Moods: Reflective, "
	         "Atmospheric | Styles: Ethereal | Tempo: Slow",
	     }},
	};
	return models;
}

const PromptModelDefinitions::ModelInfo *PromptModelDefinitions::getModel(const juce::String &modelName)
{
	for (const auto &m : getAllModels())
		if (m.modelName == modelName)
			return &m;
	return nullptr;
}