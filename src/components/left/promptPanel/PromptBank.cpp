#include "PromptBank.h"
#include "ColourPalette.h"

PromptBank::PromptBank()
{
	loadFromFile();
}

PromptBank::~PromptBank() = default;

juce::File PromptBank::getPromptBankFile()
{
	return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
	    .getChildFile(Obsidian::OBSIDIAN_BASE_DIR())
	    .getChildFile(Obsidian::PROMPTS_FILE());
}

juce::String PromptBank::generateId()
{
	return juce::Uuid().toString();
}

PromptBankEntry *PromptBank::addPrompt(const juce::String &text, const juce::String &modelName,
                                       const juce::String &category)
{
	if (text.isEmpty())
		return nullptr;

	auto entry = std::make_unique<PromptBankEntry>();
	entry->id = generateId();
	entry->text = text;
	entry->modelName = modelName;
	entry->category = category;
	entry->creationTime = juce::Time::getCurrentTime();
	entry->usageCount = 0;

	auto *raw = entry.get();
	prompts[entry->id] = std::move(entry);

	saveToFile();
	if (onBankChanged)
		onBankChanged();

	return raw;
}

bool PromptBank::removePrompt(const juce::String &id)
{
	auto it = prompts.find(id);
	if (it == prompts.end())
		return false;

	prompts.erase(it);
	saveToFile();
	if (onBankChanged)
		onBankChanged();
	return true;
}

bool PromptBank::updatePrompt(const juce::String &id, const juce::String &text, const juce::String &modelName,
                              const juce::String &category)
{
	auto *entry = getPrompt(id);
	if (!entry)
		return false;

	entry->text = text;
	entry->modelName = modelName;
	entry->category = category;

	saveToFile();
	if (onBankChanged)
		onBankChanged();
	return true;
}

void PromptBank::incrementUsage(const juce::String &id)
{
	if (auto *entry = getPrompt(id))
	{
		entry->usageCount++;
		saveToFile();
	}
}

PromptBankEntry *PromptBank::getPrompt(const juce::String &id)
{
	auto it = prompts.find(id);
	return it != prompts.end() ? it->second.get() : nullptr;
}

std::vector<PromptBankEntry *> PromptBank::getAllPrompts()
{
	std::vector<PromptBankEntry *> result;
	result.reserve(prompts.size());
	for (auto &pair : prompts)
		result.push_back(pair.second.get());
	return result;
}

std::vector<PromptBankEntry *> PromptBank::getPromptsByCategory(const juce::String &category)
{
	std::vector<PromptBankEntry *> result;
	for (auto &pair : prompts)
		if (pair.second->category == category)
			result.push_back(pair.second.get());
	return result;
}

void PromptBank::addCategory(const juce::String &name, juce::Colour colour)
{
	for (const auto &c : categories)
		if (c.name.compareIgnoreCase(name) == 0)
			return;

	PromptCategoryInfo info;
	info.id = getNextCategoryId();
	info.name = name;
	info.colour = colour;
	info.isBuiltIn = false;
	categories.push_back(info);

	saveToFile();
	if (onBankChanged)
		onBankChanged();
}

bool PromptBank::renameCategory(const juce::String &oldName, const juce::String &newName, juce::Colour colour)
{
	auto it = std::find_if(categories.begin(), categories.end(),
	                       [&oldName](const PromptCategoryInfo &c) { return c.name == oldName; });
	if (it == categories.end())
		return false;

	for (const auto &c : categories)
		if (c.name != oldName && c.name.compareIgnoreCase(newName) == 0)
			return false;

	it->name = newName;
	it->colour = colour;

	for (auto &pair : prompts)
		if (pair.second->category == oldName)
			pair.second->category = newName;

	saveToFile();
	if (onBankChanged)
		onBankChanged();
	return true;
}

bool PromptBank::removeCategory(const juce::String &name)
{
	auto it = std::find_if(categories.begin(), categories.end(),
	                       [&name](const PromptCategoryInfo &c) { return c.name == name; });
	if (it == categories.end())
		return false;

	categories.erase(it);

	for (auto &pair : prompts)
		if (pair.second->category == name)
			pair.second->category = "";

	saveToFile();
	if (onBankChanged)
		onBankChanged();
	return true;
}

int PromptBank::getNextCategoryId()
{
	int mx = 0;
	for (const auto &c : categories)
		mx = std::max(mx, c.id);
	return mx + 1;
}

void PromptBank::loadFromFile()
{
	auto file = getPromptBankFile();
	if (!file.existsAsFile())
		return;

	auto json = juce::JSON::parse(file);
	auto *obj = json.getDynamicObject();
	if (!obj)
		return;

	migrated = obj->getProperty("migrated").toString() == "true";
	seeded = obj->getProperty("seeded").toString() == "true";
	seededSA3 = obj->getProperty("seededSA3").toString() == "true";

	auto catVar = obj->getProperty("categories");
	if (catVar.isArray())
	{
		categories.clear();
		for (int i = 0; i < catVar.getArray()->size(); ++i)
		{
			auto v = catVar.getArray()->getUnchecked(i);
			if (auto *o = v.getDynamicObject())
			{
				PromptCategoryInfo info;
				info.id = (int)o->getProperty("id");
				info.name = o->getProperty("name").toString();
				auto builtInVar = o->getProperty("isBuiltIn");
				if (!builtInVar.isVoid())
					info.isBuiltIn = builtInVar.toString() == "true";
				auto colourVar = o->getProperty("colour");
				if (!colourVar.isVoid())
					info.colour = juce::Colour((juce::uint32)(int)colourVar);
				categories.push_back(info);
			}
		}
	}

	auto promptsVar = obj->getProperty("prompts");
	if (promptsVar.isArray())
	{
		prompts.clear();
		for (int i = 0; i < promptsVar.getArray()->size(); ++i)
		{
			auto v = promptsVar.getArray()->getUnchecked(i);
			if (auto *o = v.getDynamicObject())
			{
				auto entry = std::make_unique<PromptBankEntry>();
				entry->id = o->getProperty("id").toString();
				entry->text = o->getProperty("text").toString();
				entry->modelName = o->getProperty("modelName").toString();
				entry->category = o->getProperty("category").toString();

				auto timeVar = o->getProperty("creationTime");
				if (!timeVar.isVoid())
					entry->creationTime = juce::Time((juce::int64)timeVar);
				else
					entry->creationTime = juce::Time::getCurrentTime();

				entry->usageCount = (int)o->getProperty("usageCount");

				auto builtInVar = o->getProperty("isBuiltIn");
				if (!builtInVar.isVoid())
					entry->isBuiltIn = builtInVar.toString() == "true";

				if (entry->id.isEmpty())
					entry->id = generateId();

				prompts[entry->id] = std::move(entry);
			}
		}
	}
}

void PromptBank::saveToFile()
{
	auto file = getPromptBankFile();
	file.getParentDirectory().createDirectory();

	juce::DynamicObject::Ptr root = new juce::DynamicObject();
	root->setProperty("migrated", migrated ? "true" : "false");
	root->setProperty("seeded", seeded ? "true" : "false");
	root->setProperty("seededSA3", seededSA3 ? "true" : "false");

	juce::Array<juce::var> catsArray;
	for (const auto &c : categories)
	{
		juce::DynamicObject::Ptr o = new juce::DynamicObject();
		o->setProperty("id", c.id);
		o->setProperty("name", c.name);
		o->setProperty("colour", (int)c.colour.getARGB());
		o->setProperty("isBuiltIn", c.isBuiltIn ? "true" : "false");
		catsArray.add(o.get());
	}
	root->setProperty("categories", juce::var(catsArray));

	juce::Array<juce::var> promptsArray;
	for (auto &pair : prompts)
	{
		juce::DynamicObject::Ptr o = new juce::DynamicObject();
		o->setProperty("id", pair.second->id);
		o->setProperty("text", pair.second->text);
		o->setProperty("modelName", pair.second->modelName);
		o->setProperty("category", pair.second->category);
		o->setProperty("creationTime", (juce::int64)pair.second->creationTime.toMilliseconds());
		o->setProperty("usageCount", pair.second->usageCount);
		o->setProperty("isBuiltIn", pair.second->isBuiltIn ? "true" : "false");
		promptsArray.add(o.get());
	}
	root->setProperty("prompts", juce::var(promptsArray));

	file.replaceWithText(juce::JSON::toString(juce::var(root.get())));
}

void PromptBank::migrateFromCustomPrompts(const juce::StringArray &existing, const juce::String &defaultModel)
{
	if (migrated)
		return;

	for (const auto &text : existing)
	{
		if (text.isEmpty())
			continue;

		bool exists = false;
		for (auto &pair : prompts)
			if (pair.second->text == text)
			{
				exists = true;
				break;
			}
		if (exists)
			continue;

		auto entry = std::make_unique<PromptBankEntry>();
		entry->id = generateId();
		entry->text = text;
		entry->modelName = defaultModel;
		entry->category = "";
		entry->creationTime = juce::Time::getCurrentTime();
		entry->usageCount = 0;

		prompts[entry->id] = std::move(entry);
	}

	migrated = true;
	saveToFile();
	if (onBankChanged)
		onBankChanged();
}

bool PromptBank::seedDefaultPromptsAndCategories()
{
	if (seeded)
		return false;
	struct DefaultCat
	{
		const char *name;
		juce::Colour colour;
	};

	const DefaultCat defaultCats[] = {
	    {"Drums", ColourPalette::coral},        {"Bass", ColourPalette::teal},           {"Lead", ColourPalette::amber},
	    {"Pads", ColourPalette::violet},        {"Percussion", ColourPalette::lime},     {"FX", ColourPalette::slate},
	    {"Vocal", ColourPalette::buttonDanger}, {"Piano & Keys", ColourPalette::indigo},
	};

	for (const auto &dc : defaultCats)
	{
		bool exists = false;
		for (const auto &c : categories)
			if (c.name.compareIgnoreCase(dc.name) == 0)
			{
				exists = true;
				break;
			}
		if (!exists)
		{
			PromptCategoryInfo info;
			info.id = getNextCategoryId();
			info.name = dc.name;
			info.colour = dc.colour;
			info.isBuiltIn = true;
			categories.push_back(info);
		}
	}

	const DefaultPrompt defaults[] = {
	    {"deep techno kick drum loop, solo kick pattern, driving 4/4 beat, dry, no background elements",
	     "stable-audio-open-1.0", "Drums"},
	    {"hardcore kick pattern, distorted solo kick, aggressive 4/4, dry, isolated rhythm", "stable-audio-open-1.0",
	     "Drums"},
	    {"trap hi-hat loop, fast rolling solo hi-hats, crisp metallic percussion, dry, isolated rhythm",
	     "stable-audio-open-1.0", "Percussion"},
	    {"glitchy percussion loop, broken IDM rhythm, solo percussion, dry, no melodic content",
	     "stable-audio-open-1.0", "Percussion"},
	    {"acid bassline loop, solo squelchy 303 synth bass, rhythmic monophonic sequence, dry, no pads",
	     "stable-audio-open-1.0", "Bass"},
	    {"deep rolling sub bassline, solo dub-style bass, rhythmic monophonic, dry, no background",
	     "stable-audio-open-1.0", "Bass"},
	    {"vintage analog lead, solo monophonic synth melody, warm saw wave, dry, no accompaniment",
	     "stable-audio-open-1.0", "Lead"},
	    {"distorted noise chops, solo industrial noise stabs, rhythmic, dry, no melody", "stable-audio-open-1.0", "FX"},
	    {"dark atmospheric pad, slow evolving solo pad, cinematic, dry, no rhythm", "stable-audio-open-1.0", "Pads"},
	    {"ambient flute psychedelic, solo flute melody, ethereal phrasing, dry, no accompaniment",
	     "stable-audio-open-1.0", "Lead"},

	    {"minimal techno drum loop, solo dry kick with rim shot, sparse hypnotic groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"dub techno drum loop, solo muffled kick with shuffled closed hat, deep hypnotic groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"peak time techno drums, solo punchy kick with off-beat open hat, driving relentless rhythm, dry, no pads",
	     "stable-audio-open-1.0", "Drums"},
	    {"industrial techno drum loop, solo distorted kick with metallic snare, brutal warehouse rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"acid techno drum pattern, solo punchy kick with snappy clap, raw 4/4 groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"berlin techno drums, solo dry analog kick with tight hi-hat, dark hypnotic loop, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"raw warehouse techno, solo distorted kick with industrial percussion, aggressive groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},

	    {"classic house drum loop, solo kick with crisp open hat, swung 4/4 groove, dry, isolated rhythm",
	     "stable-audio-open-1.0", "Drums"},
	    {"deep house drum loop, solo warm kick with shaker and rim, groovy laid-back rhythm, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"tech house drum loop, solo tight kick with tambourine and clap, bouncy groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"afro house drum loop, solo organic kick with congas and shaker, tribal rolling groove, dry, no background",
	     "stable-audio-open-1.0", "Percussion"},
	    {"disco house drum loop, solo four-on-the-floor kick with open hat and tambourine, uplifting groove, dry, "
	     "isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"chicago house drum machine loop, solo 909 kick with classic clap, raw groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"french touch house drums, solo punchy kick with filtered hat, funky groove, dry, isolated rhythm",
	     "stable-audio-open-1.0", "Drums"},

	    {"drum and bass amen break, solo chopped breakbeat, fast rolling rhythm, dry, isolated drums",
	     "stable-audio-open-1.0", "Drums"},
	    {"liquid dnb drum loop, solo smooth breakbeat with crisp snare, rolling groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"neurofunk drum loop, solo tight technical breakbeat, precise snare rolls, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"jungle break, solo chopped amen pattern, fast syncopated rhythm, dry, no melody", "stable-audio-open-1.0",
	     "Drums"},
	    {"ragga jungle drum loop, solo rapid breakbeat with snare rolls, rough groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"breakcore drum pattern, solo chaotic chopped breaks, fast glitchy rhythm, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"footwork drum loop, solo syncopated 160bpm kick and clap, stuttering chicago rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"juke drum loop, solo rapid kick and snare with hi-hat triplets, hyperactive groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},

	    {"dubstep drum loop, solo half-time kick and snare, heavy syncopated groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"uk garage drum loop, solo syncopated 2-step kick and snare, shuffled hats, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"grime drum loop, solo skippy kick pattern with crisp snare, raw urban rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"2-step garage drums, solo offbeat kick and snare with shuffled hats, bouncy groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"future garage drum loop, solo soft kick with crisp snare, atmospheric rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},

	    {"trap drum loop, solo 808 kick with crisp snare and rolling hi-hats, dry, isolated rhythm",
	     "stable-audio-open-1.0", "Drums"},
	    {"boom bap drum loop, solo dusty kick and snare, classic hip-hop break, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"lofi hip-hop drum loop, solo soft kick with vinyl snare, mellow swung groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"drill drum loop, solo sliding 808 with sharp snare and triplet hats, dark groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"uk drill drum loop, solo sliding bass kick with skippy hats and rim, dark groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"memphis trap drums, solo deep 808 with crisp snare, rolling hi-hats, dark groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"phonk drum loop, solo cowbell percussion with punchy kick, memphis-style groove, dry, isolated",
	     "stable-audio-open-1.0", "Percussion"},

	    {"afrobeat drum loop, solo congas and shekere with snare, polyrhythmic groove, dry, isolated",
	     "stable-audio-open-1.0", "Percussion"},
	    {"amapiano drum loop, solo log drums with shaker, bouncy south african groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"gqom drum loop, solo deep kick with tribal percussion, raw south african rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"baile funk drum loop, solo tamborzao pattern, rapid syncopated brazilian rhythm, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"reggaeton drum loop, solo dembow pattern with crisp snare, latin groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"cumbia drum loop, solo congas and timbales, latin rhythmic groove, dry, no melody", "stable-audio-open-1.0",
	     "Percussion"},
	    {"salsa percussion loop, solo congas and bongos, fast latin rhythm, dry, isolated percussion",
	     "stable-audio-open-1.0", "Percussion"},
	    {"samba batucada loop, solo surdo and tamborim, brazilian percussion ensemble, dry, no background",
	     "stable-audio-open-1.0", "Percussion"},
	    {"tribal drum loop, solo djembe and shaker, organic acoustic groove, dry, isolated rhythm",
	     "stable-audio-open-1.0", "Percussion"},
	    {"ghettotech drum loop, solo fast detroit kick and snare with claps, syncopated groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},

	    {"rock drum loop, solo punchy kick with crisp snare and crash, driving 4/4 groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"funk drum break, solo syncopated kick and snare with ghost notes, groovy pocket, dry, no background",
	     "stable-audio-open-1.0", "Drums"},
	    {"jazz drum brush loop, solo brushed snare with ride cymbal, swung groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"metal drum loop, solo double kick with blast beat snare, fast aggressive rhythm, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"punk drum loop, solo fast kick and snare with crash, raw energetic groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"motown drum loop, solo tight kick with snappy snare and tambourine, vintage groove, dry, no background",
	     "stable-audio-open-1.0", "Drums"},

	    {"idm glitch drum loop, solo chopped percussion with stutter edits, broken rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"experimental percussion loop, solo metallic clangs and clicks, abstract rhythm, dry, no melody",
	     "stable-audio-open-1.0", "Percussion"},
	    {"clicks and cuts percussion, solo microsound clicks and pops, minimal glitchy rhythm, dry, isolated",
	     "stable-audio-open-1.0", "Percussion"},
	    {"granular drum loop, solo fragmented chopped textures, experimental rhythm, dry, no background",
	     "stable-audio-open-1.0", "Drums"},

	    {"trance drum loop, solo punchy kick with off-beat open hat, uplifting 4/4 groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"psytrance drum loop, solo tight kick with rolling 16th hats, hypnotic groove, dry, no melody",
	     "stable-audio-open-1.0", "Drums"},
	    {"hardstyle drum loop, solo distorted reverse kick with crisp clap, aggressive groove, dry, isolated",
	     "stable-audio-open-1.0", "Drums"},
	    {"gabber drum loop, solo distorted kick at 180bpm, brutal hardcore rhythm, dry, no background",
	     "stable-audio-open-1.0", "Drums"},

	    {"reese bass loop, solo growling detuned sawtooth bass, dnb-style monophonic, dry, no background",
	     "stable-audio-open-1.0", "Bass"},
	    {"wobble dubstep bass, solo modulated lfo bass, rhythmic monophonic sequence, dry, no melody",
	     "stable-audio-open-1.0", "Bass"},
	    {"trap 808 bass loop, solo sliding 808 sub bass, rhythmic monophonic, dry, isolated", "stable-audio-open-1.0",
	     "Bass"},

	    {"detuned saw lead, solo monophonic supersaw melody, bright trance-style, dry, no accompaniment",
	     "stable-audio-open-1.0", "Lead"},
	    {"riser fx, solo white noise sweep with rising pitch, tension build-up, dry, no melody",
	     "stable-audio-open-1.0", "FX"},
	    {"impact fx, solo cinematic boom hit, single impact, dry, no background", "stable-audio-open-1.0", "FX"},

	    {"Synth Bass, 303, Acid, Buzzy, Bassline, Fast Speed, Medium Distortion", "foundation-1", "Bass"},
	    {"Synth Bass, Reese Bass, Wide, Thick, Growl, Sustained, Bassline", "foundation-1", "Bass"},
	    {"Synth Lead, Supersaw, Bright, Wide, Catchy, Melody, Medium Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Wavetable Synth, Gritty, Buzzy, Top Melody, Pitch Bend, Medium Delay", "foundation-1", "Lead"},
	    {"Pad, Atmosphere, Warm, Soft, Dreamy, Sustained, High Reverb", "foundation-1", "Pads"},
	    {"Pad, Texture, Dark, Airy, Slow Speed, Sustained, Plate Reverb", "foundation-1", "Pads"},
	    {"Pluck, Bell, Bright, Glassy, Arp, Triplets, Ping Pong Delay", "foundation-1", "Lead"},
	    {"Bowed Strings, Cello, Warm, Deep, Melody, Slow Speed, Medium Reverb", "foundation-1", "Lead"},

	    {"Synth Bass, Reese Bass, Dark, Gritty, Growl, Bassline, Medium Distortion", "foundation-1", "Bass"},
	    {"Synth Bass, Wavetable Synth, Deep, Round, Sustained, Bassline, Low Reverb", "foundation-1", "Bass"},
	    {"Synth Bass, Analog, Warm, Thick, Bassline, Slow Speed, Mono Delay", "foundation-1", "Bass"},
	    {"Synth Bass, Pluck, Punchy, Focused, Bassline, Fast Speed", "foundation-1", "Bass"},
	    {"Synth Bass, 303, Acid, Buzzy, Bassline, Pitch Bend, High Distortion", "foundation-1", "Bass"},
	    {"Synth Bass, Sub, Deep, Round, Sustained, Bassline, Low Reverb", "foundation-1", "Bass"},
	    {"Synth Bass, Bitcrushed, Gritty, Noisy, Bassline, Fast Speed, Bitcrush", "foundation-1", "Bass"},

	    {"Synth Lead, Supersaw, Wide, Bright, Epic, Melody, Rising, High Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Wavetable Synth, Metallic, Focused, Top Melody, Fast Speed, Stereo Delay", "foundation-1",
	     "Lead"},
	    {"Synth Lead, Analog, Warm, Vintage, Melody, Slow Speed, Plate Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Square, Buzzy, Gritty, Top Melody, Pitch Bend, Medium Distortion", "foundation-1", "Lead"},
	    {"Pluck, Bell, Glassy, Bright, Arp, Fast Speed, Ping Pong Delay", "foundation-1", "Lead"},
	    {"Pluck, Bell, Dreamy, Soft, Top Melody, Triplets, High Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Saw, Warm, Rich, Melody, Catchy, Medium Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Wavetable Synth, Noisy, Bitcrushed, Top Melody, Complex, Bitcrush", "foundation-1", "Lead"},

	    {"Pad, Atmosphere, Bright, Wide, Dreamy, Sustained, High Reverb, Phaser", "foundation-1", "Pads"},
	    {"Pad, Texture, Warm, Analog, Vintage, Sustained, Plate Reverb", "foundation-1", "Pads"},
	    {"Pad, Atmosphere, Dark, Deep, Slow Speed, Sustained, High Delay", "foundation-1", "Pads"},
	    {"Pad, Texture, Airy, Soft, Breathy, Sustained, High Reverb", "foundation-1", "Pads"},
	    {"Pad, Atmosphere, Gritty, Noisy, Dark, Sustained, Medium Distortion, Plate Reverb", "foundation-1", "Pads"},
	    {"Pad, Texture, Glassy, Bright, Rising, Sustained, Ping Pong Delay, High Reverb", "foundation-1", "Pads"},

	    {"Keys, Grand Piano, Warm, Rich, Chord Progression, Slow Speed, Medium Reverb", "foundation-1", "Piano & Keys"},
	    {"Keys, Rhodes Piano, Smooth, Dreamy, Melody, Slow Speed, Medium Reverb", "foundation-1", "Piano & Keys"},
	    {"Keys, Digital Piano, Clean, Bright, Chord Progression, Catchy, Low Reverb", "foundation-1", "Piano & Keys"},
	    {"Keys, Rhodes Piano, Warm, Vintage, Chord Progression, Strummed, Plate Reverb", "foundation-1",
	     "Piano & Keys"},

	    {"Bowed Strings, Violin, Bright, Focused, Melody, Fast Speed, Medium Reverb", "foundation-1", "Lead"},
	    {"Bowed Strings, Cello, Dark, Deep, Sustained, Slow Speed, High Reverb", "foundation-1", "Pads"},
	    {"Wind, Flute, Airy, Breathy, Melody, Slow Speed, Medium Reverb", "foundation-1", "Lead"},
	    {"Wind, Pan Flute, Soft, Dreamy, Melody, Slow Speed, High Reverb", "foundation-1", "Lead"},
	    {"Wind, Clarinet, Warm, Round, Melody, Medium Speed, Low Reverb", "foundation-1", "Lead"},
	    {"Wind, Ocarina, Soft, Breathy, Melody, Simple, Medium Reverb", "foundation-1", "Lead"},
	    {"Brass, Trumpet, Bright, Focused, Melody, Fast Speed, Plate Reverb", "foundation-1", "Lead"},
	    {"Brass, French Horn, Warm, Rich, Sustained, Slow Speed, High Reverb", "foundation-1", "Pads"},
	    {"Brass, Tuba, Deep, Round, Bassline, Slow Speed, Low Reverb", "foundation-1", "Bass"},
	    {"Mallet, Marimba, Warm, Round, Arp, Fast Speed, Medium Reverb", "foundation-1", "Lead"},
	    {"Mallet, Vibraphone, Glassy, Soft, Melody, Slow Speed, Plate Reverb", "foundation-1", "Lead"},
	    {"Plucked Strings, Harp, Glassy, Bright, Arp, Falling, High Reverb", "foundation-1", "Lead"},
	    {"Guitar, Acoustic, Warm, Rich, Strummed, Medium Speed, Low Reverb", "foundation-1", "Lead"},
	    {"Guitar, Acoustic, Soft, Vintage, Chord Progression, Slow Speed, Plate Reverb", "foundation-1", "Lead"},

	    {"Vocal, Choir, Airy, Dreamy, Chord Progression, Sustained, High Reverb", "foundation-1", "Vocal"},
	    {"Vocal, Choir, Dark, Deep, Sustained, Slow Speed, Plate Reverb", "foundation-1", "Vocal"},

	    {"Lead, Saw, Warm, Supersaw, Epic, Medium Speed, Medium Reverb", "audialab-edm-elements", "Lead"},
	    {"Supersaw, Synth, Saw, Rising, Fast Speed, Rising Low-Pass", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Triplets, Bounce, Medium Speed, Small Reverb", "audialab-edm-elements", "Lead"},
	    {"Bass, Punchy, Pluck, Sub, Simple, Medium Speed", "audialab-edm-elements", "Bass"},
	    {"Lead, Square, Buzzy, Legato, Complex, Fast Speed, Quarter-Beat Gate", "audialab-edm-elements", "Lead"},

	    {"Lead, Supersaw, Wide, Epic, Rising, Fast Speed, High Reverb", "audialab-edm-elements", "Lead"},
	    {"Lead, Saw, Warm, Catchy, Simple, Medium Speed, Medium Reverb", "audialab-edm-elements", "Lead"},
	    {"Lead, Square, Buzzy, Bounce, Triplets, Fast Speed, Half-Beat Gate", "audialab-edm-elements", "Lead"},
	    {"Lead, Saw, Bright, Legato, Complex, Medium Speed, Stereo Delay", "audialab-edm-elements", "Lead"},
	    {"Supersaw, Synth, Wide, Falling, Medium Speed, Falling High-Cut", "audialab-edm-elements", "Lead"},
	    {"Supersaw, Synth, Epic, Rising, Fast Speed, Quarter-Beat Gate, High Reverb", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Bright, Bounce, Triplets, Fast Speed, Ping Pong Delay", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Dreamy, Simple, Slow Speed, High Reverb", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Glassy, Complex, Fast Speed, Stereo Delay", "audialab-edm-elements", "Lead"},
	    {"Lead, Square, Gritty, Legato, Complex, Fast Speed, Half-Beat Gate", "audialab-edm-elements", "Lead"},
	    {"Lead, Saw, Warm, Rising, Medium Speed, Rising Low-Pass, Medium Reverb", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Catchy, Bounce, Medium Speed, Small Reverb, Ping Pong Delay", "audialab-edm-elements", "Lead"},

	    {"Bass, Punchy, Pluck, Sub, Simple, Fast Speed", "audialab-edm-elements", "Bass"},
	    {"Bass, Reese, Growl, Wide, Complex, Medium Speed, Medium Distortion", "audialab-edm-elements", "Bass"},
	    {"Bass, Pluck, Punchy, Sub, Bounce, Triplets, Medium Speed", "audialab-edm-elements", "Bass"},
	    {"Bass, Reese, Buzzy, Wide, Sustained, Medium Speed, Half-Beat Gate", "audialab-edm-elements", "Bass"},
	    {"Bass, Sub, Deep, Simple, Slow Speed, Low Reverb", "audialab-edm-elements", "Bass"},
	    {"Bass, Wobble, Modulated, Complex, Medium Speed, Half-Beat Gate", "audialab-edm-elements", "Bass"},
	    {"Bass, Punchy, Pluck, Bright, Bounce, Fast Speed, Small Reverb", "audialab-edm-elements", "Bass"},

	    {"Pad, Warm, Wide, Rising, Slow Speed, Rising Low-Pass, High Reverb", "audialab-edm-elements", "Pads"},
	    {"Pad, Bright, Dreamy, Simple, Slow Speed, High Reverb", "audialab-edm-elements", "Pads"},
	    {"Pad, Dark, Wide, Falling, Slow Speed, Falling High-Cut, Plate Reverb", "audialab-edm-elements", "Pads"},
	    {"Pad, Airy, Soft, Rising, Slow Speed, High Reverb", "audialab-edm-elements", "Pads"},

	    {"Riser, Noise, Rising, Fast Speed, Rising Low-Pass, High Reverb", "audialab-edm-elements", "FX"},
	    {"Riser, Synth, Epic, Rising, Fast Speed, Rising Low-Pass", "audialab-edm-elements", "FX"},
	    {"Drop FX, Bass, Punchy, Falling, Medium Speed, Falling High-Cut", "audialab-edm-elements", "FX"},
	    {"Impact FX, Noise, Punchy, Simple, Small Reverb", "audialab-edm-elements", "FX"},

	    {"Grand Piano, simple, melody only, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, jazzy, chord progression with top catchy melody, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Soft E. Piano, smooth, chord progression only, Low Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Medium E. Piano, dance plucky, alternating top arp melody, Medium Tremolo, High Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Grand Piano, complex strummed, chord progression with top catchy melody, High Spacey Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},

	    {"Grand Piano, complex, melody only, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, fast, melody only, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, slow, chord progression only, High Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, smooth, chord progression with top catchy melody, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Grand Piano, rising, melody only, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, falling, melody only, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, jazzy, alternating top arp melody, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, simple strummed, chord progression only, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, rising strummed, chord progression with top catchy melody, High Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Grand Piano, slow strummed, chord progression only, High Spacey Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Grand Piano, complex, alternating top arp melody, High Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, dance plucky, melody only, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},

	    {"Soft E. Piano, simple, melody only, Low Tremolo, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Soft E. Piano, jazzy, chord progression with top catchy melody, Medium Tremolo, Medium Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},
	    {"Soft E. Piano, slow, chord progression only, High Tremolo, High Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Soft E. Piano, smooth, alternating top arp melody, Medium Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Soft E. Piano, complex, melody only, Low Tremolo, Medium Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Soft E. Piano, rising, chord progression only, Medium Tremolo, High Spacey Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Soft E. Piano, slow strummed, chord progression with top catchy melody, High Tremolo, High Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},

	    {"Medium E. Piano, simple, melody only, Low Tremolo, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Medium E. Piano, jazzy, chord progression with top catchy melody, Medium Tremolo, Medium Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},
	    {"Medium E. Piano, fast, alternating top arp melody, High Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Medium E. Piano, complex, melody only, Medium Tremolo, High Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Medium E. Piano, smooth, chord progression only, Low Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Medium E. Piano, rising strummed, chord progression with top catchy melody, Medium Tremolo, High Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},
	    {"Medium E. Piano, dance plucky, melody only, High Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Medium E. Piano, complex strummed, chord progression with top catchy melody, Medium Tremolo, High Spacey "
	     "Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},

	    {"Female Vocal Texture, Chord Progression, angelic, ethereal space, high reverb", "rc-vocal-textures", "Vocal"},
	    {"Male Vocal Texture, Chord Progression, deep, haunting, washy textures, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, operatic, long attacks, atmospheric, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Female Vocal Texture, Chord Progression, pure, atmospheric, ethereal space", "rc-vocal-textures", "Vocal"},

	    {"Female Vocal Texture, Chord Progression, pure, angelic, long attacks, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, breathy, ethereal space, washy textures", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, haunting, dark, atmospheric, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, soft, dreamy, ethereal space, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, operatic, long attacks, cinematic, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, warm, intimate, washy textures, medium reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Female Vocal Texture, Chord Progression, ghostly, haunting, dark space, high reverb", "rc-vocal-textures",
	     "Vocal"},

	    {"Male Vocal Texture, Chord Progression, deep, operatic, long attacks, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Male Vocal Texture, Chord Progression, warm, rich, atmospheric, medium reverb", "rc-vocal-textures", "Vocal"},
	    {"Male Vocal Texture, Chord Progression, haunting, dark, washy textures, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Male Vocal Texture, Chord Progression, breathy, intimate, ethereal space, medium reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Male Vocal Texture, Chord Progression, monastic, long attacks, cathedral space, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Male Vocal Texture, Chord Progression, ghostly, atmospheric, washy textures, high reverb",
	     "rc-vocal-textures", "Vocal"},

	    {"Ensemble Vocal Texture, Chord Progression, angelic, long attacks, ethereal space, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, dark, haunting, washy textures, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, cinematic, epic, long attacks, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, monastic, operatic, cathedral space, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, pure, soft, atmospheric, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, full choir, mixed voices, ethereal space, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, ghostly, haunting, dark space, washy textures",
	     "rc-vocal-textures", "Vocal"},

	    {"Cloud Trap, nostalgic piano, plucked bass, dreamy, melancholic", "sao-instrumental", "Piano & Keys"},
	    {"Melodic Trap, synth bells, deep sub bass, dark, moody", "sao-instrumental", "Bass"},
	    {"Lofi Jazz Rap, soft Rhodes keys, plucked bass, laid back, chill", "sao-instrumental", "Piano & Keys"},
	    {"Neo-Soul, electric guitar riffs, live bass, smooth, seductive", "sao-instrumental", "Lead"},
	    {"Alternative Rock, electric guitar riffs, warm analog grooves, energetic, raw", "sao-instrumental", "Lead"},
	    {"British 60s Oldies, vocal adlibs, airy vocal pads, romantic, contemplative", "sao-instrumental", "Vocal"},

	    {"Full Beat, Instruments: drum, cloud trap beat, boomy bass, crisp hi-hats, syncopated rhythm", "stablebeat",
	     "Drums"},
	    {"Full Beat, Instruments: drum, melodic trap beat, deep sub, punchy snare, off-beat patterns", "stablebeat",
	     "Drums"},
	    {"Solo, Instruments: drum, boom bap, dirty piano loop, driving 4/4 beat", "stablebeat", "Drums"},
	    {"Full Beat, Instruments: drum, industrial hip-hop, distorted kick, industrial metallic percussion",
	     "stablebeat", "Drums"},
	    {"Solo, Instruments: drum, jazzy chillhop, crisp hi-hats, syncopated rhythm", "stablebeat", "Percussion"},

	    {"Format: Solo | Genre: Trap | Sub-Genre: Melodic Trap | Instruments: Piano, Synth Pad | Moods: Melancholic | "
	     "Styles: Catchy, Smooth",
	     "gluten-v1", "Piano & Keys"},
	    {"Format: Solo | Genre: Trap | Sub-Genre: Wavy Trap | Instruments: Bells, Synth Lead | Moods: Reflective, "
	     "Atmospheric | Styles: Ethereal",
	     "gluten-v1", "Lead"},
	    {"Format: Solo | Genre: Hip-Hop | Sub-Genre: Boom Bap | Instruments: Piano, 808 Bass | Moods: Sad, Melancholic "
	     "| Styles: Smooth",
	     "gluten-v1", "Piano & Keys"},
	    {"Format: Solo | Genre: Ambient | Sub-Genre: Ambient | Instruments: Synth Pad, Strings | Moods: Atmospheric, "
	     "Ethereal | Styles: Building",
	     "gluten-v1", "Pads"},
	    {"Format: Solo | Genre: Pop | Sub-Genre: Pop | Instruments: Synth Lead, Bells | Moods: Catchy, Driving | "
	     "Styles: Punchy, Rhythmic",
	     "gluten-v1", "Lead"},
	};

	seeded = addDefaultPromptsArray(defaults, juce::numElementsInArray(defaults));
	return seeded;
}

bool PromptBank::addDefaultPromptsArray(const DefaultPrompt *array, int arraySize)
{
	bool entriesAdded = false;

	for (int i = 0; i < arraySize; ++i)
	{
		const auto &dp = array[i];

		bool exists = false;
		for (auto &pair : prompts)
		{
			if (pair.second->text == dp.text && pair.second->modelName == dp.model)
			{
				exists = true;
				break;
			}
		}

		if (exists)
			continue;

		auto entry = std::make_unique<PromptBankEntry>();
		entry->id = generateId();
		entry->text = dp.text;
		entry->modelName = dp.model;
		entry->category = dp.category;
		entry->creationTime = juce::Time::getCurrentTime();
		entry->usageCount = 0;
		entry->isBuiltIn = true;

		prompts[entry->id] = std::move(entry);
		entriesAdded = true;
	}

	return entriesAdded;
}

bool PromptBank::seedStableAudio3Medium()
{
	if (seededSA3)
		return false;
	const DefaultPrompt sa3Defaults[] = {
	    {"TrackType: Instrument, Format: Solo, a raw delta blues slide guitar riff, resonator acoustic guitar, dusty "
	     "vintage tone, fingerpicked with stompbox rhythm",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a smoky blues harmonica solo, soulful pitch bends, gritty amplifier "
	     "distortion, intimate bar room acoustics",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, an aggressive heavy metal rhythm guitar riff, low-tuned seven-string "
	     "guitar, chugging palm mutes, high-gain distortion",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a screaming electric guitar solo, sweep picking arpeggios, high gain, "
	     "tapping technique, stadium rock reverb",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a driving hard rock bassline loop, played with a heavy pick, overdriven "
	     "tube amplifier, punchy mid-range",
	     "stable-audio-3-medium", "Bass"},
	    {"TrackType: Instrument, Format: Solo, a rustic acoustic mandolin tremolo pattern, bright double-strings, "
	     "traditional folk style, dry studio recording",
	     "stable-audio-3-medium", "Lead"},

	    {"TrackType: Instrument, Format: Solo, a hypnotic West African kora melody, intricate plucked string patterns, "
	     "bright resonant wooden tones, polyrhythmic loop",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a meditative Indian sitar phrase, sympathetic string resonance, "
	     "microtonal bends, mystical traditional vibe",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a haunting Middle Eastern oud loop, microtonal maqam fretless acoustic "
	     "guitar vibe, deep wooden body resonance",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a soulful Japanese shakuhachi flute melody, breathy articulation, "
	     "organic pitch inflections, cinematic zen temple acoustics",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, an energetic Celtic tin whistle reel, fast ornamentation, crisp high "
	     "register, traditional Irish folk style",
	     "stable-audio-3-medium", "Lead"},
	    {"TrackType: Instrument, Format: Solo, a powerful dynamic Taiko drum rhythm, massive wooden barrel resonance, "
	     "heavy accents, syncopated ritual pattern",
	     "stable-audio-3-medium", "Drums"},
	    {"TrackType: Instrument, Format: Solo, intricate Afro-Cuban conga patterns, open tones, slaps and heel-toe "
	     "technique, crisp hand percussion loop",
	     "stable-audio-3-medium", "Percussion"},
	    {"TrackType: Instrument, Format: Solo, a mystic Australian didgeridoo drone, circular breathing textures, deep "
	     "sub-harmonic rhythmic growls, vocal formants",
	     "stable-audio-3-medium", "Bass"},

	    {"TrackType: Instrument, Format: Solo, a heavy neurobass growl loop, modulated wavetable synthesis, aggressive "
	     "tearing textures, sub-bass pressure, EDM style",
	     "stable-audio-3-medium", "Bass"},
	    {"TrackType: Instrument, Format: Solo, a rhythmic glitch percussion loop, bitcrushed micro-samples, "
	     "micro-timing, modular synth blips, dry and sterile space",
	     "stable-audio-3-medium", "Percussion"},
	    {"TrackType: Instrument, Format: Solo, a cyberpunk industrial synth bassline, heavily distorted analog pulse "
	     "wave, rhythmic step-sequencer sequence",
	     "stable-audio-3-medium", "Bass"},
	    {"TrackType: Instrument, Format: Solo, a cinematic modular synth arpeggio, opening low-pass filter sweep, "
	     "cascading notes, dark electronic style",
	     "stable-audio-3-medium", "Lead"},

	    {"TrackType: Instrument, Format: Solo, a dark dystopian cinematic pad, low drone foundation, sweeping "
	     "high-frequency harmonics, tension building",
	     "stable-audio-3-medium", "Pads"},
	    {"TrackType: Instrument, Format: Solo, an angelic ethereal vocal pad, lush multi-layered textures, "
	     "non-intelligible choir harmony, wash of massive hall reverb",
	     "stable-audio-3-medium", "Pads"},
	    {"TrackType: SFX, a massive cinematic sub-bass boom, deep sub-sonic explosion drop, long decaying low-end "
	     "tail, transient impact",
	     "stable-audio-3-medium", "FX"},
	    {"TrackType: SFX, a futuristic electronic riser effect, pitching up oscillator, accelerating noise modulation, "
	     "building tension white noise swoop",
	     "stable-audio-3-medium", "FX"},
	    {"TrackType: SFX, a heavy industrial metal door slamming shut, fast decay, mechanical lock mechanism click, "
	     "recorded in a dead concrete room",
	     "stable-audio-3-medium", "FX"},
	    {"TrackType: SFX, a glitchy digital transition, tape-stop effect artifact, pitch and speed dropping "
	     "simultaneously, grinding to a halt into a sub frequency",
	     "stable-audio-3-medium", "FX"},
	    {"TrackType: SFX, cosmic sci-fi modular synth bleeps, granular delay texture, random pitch modulation, "
	     "spacious echo space",
	     "stable-audio-3-medium", "FX"}};

	seededSA3 = addDefaultPromptsArray(sa3Defaults, juce::numElementsInArray(sa3Defaults));
	return seededSA3;
}