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
	    .getChildFile("OBSIDIAN-Neural")
	    .getChildFile("prompts.json");
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

void PromptBank::seedDefaultPromptsAndCategories()
{
	if (seeded)
		return;
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

	struct DefaultPrompt
	{
		const char *text;
		const char *model;
		const char *category;
	};

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

	    {"Synth Bass, 303, Acid, Buzzy, Bassline, Fast Speed, Medium Distortion", "foundation-1", "Bass"},
	    {"Synth Bass, Reese Bass, Wide, Thick, Growl, Sustained, Bassline", "foundation-1", "Bass"},
	    {"Synth Lead, Supersaw, Bright, Wide, Catchy, Melody, Medium Reverb", "foundation-1", "Lead"},
	    {"Synth Lead, Wavetable Synth, Gritty, Buzzy, Top Melody, Pitch Bend, Medium Delay", "foundation-1", "Lead"},
	    {"Pad, Atmosphere, Warm, Soft, Dreamy, Sustained, High Reverb", "foundation-1", "Pads"},
	    {"Pad, Texture, Dark, Airy, Slow Speed, Sustained, Plate Reverb", "foundation-1", "Pads"},
	    {"Pluck, Bell, Bright, Glassy, Arp, Triplets, Ping Pong Delay", "foundation-1", "Lead"},
	    {"Bowed Strings, Cello, Warm, Deep, Melody, Slow Speed, Medium Reverb", "foundation-1", "Lead"},

	    {"Lead, Saw, Warm, Supersaw, Epic, Medium Speed, Medium Reverb", "audialab-edm-elements", "Lead"},
	    {"Supersaw, Synth, Saw, Rising, Fast Speed, Rising Low-Pass", "audialab-edm-elements", "Lead"},
	    {"Pluck, Bell, Triplets, Bounce, Medium Speed, Small Reverb", "audialab-edm-elements", "Lead"},
	    {"Bass, Punchy, Pluck, Sub, Simple, Medium Speed", "audialab-edm-elements", "Bass"},
	    {"Lead, Square, Buzzy, Legato, Complex, Fast Speed, Quarter-Beat Gate", "audialab-edm-elements", "Lead"},

	    {"Grand Piano, simple, melody only, Low Reverb", "rc-infinite-pianos", "Piano & Keys"},
	    {"Grand Piano, jazzy, chord progression with top catchy melody, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Soft E. Piano, smooth, chord progression only, Low Tremolo, Medium Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Medium E. Piano, dance plucky, alternating top arp melody, Medium Tremolo, High Reverb", "rc-infinite-pianos",
	     "Piano & Keys"},
	    {"Grand Piano, complex strummed, chord progression with top catchy melody, High Spacey Reverb",
	     "rc-infinite-pianos", "Piano & Keys"},

	    {"Female Vocal Texture, Chord Progression, angelic, ethereal space, high reverb", "rc-vocal-textures", "Vocal"},
	    {"Male Vocal Texture, Chord Progression, deep, haunting, washy textures, high reverb", "rc-vocal-textures",
	     "Vocal"},
	    {"Ensemble Vocal Texture, Chord Progression, operatic, long attacks, atmospheric, high reverb",
	     "rc-vocal-textures", "Vocal"},
	    {"Female Vocal Texture, Chord Progression, pure, atmospheric, ethereal space", "rc-vocal-textures", "Vocal"},

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

	for (const auto &dp : defaults)
	{
		bool exists = false;
		for (auto &pair : prompts)
			if (pair.second->text == dp.text && pair.second->modelName == dp.model)
			{
				exists = true;
				break;
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
	}

	seeded = true;
	saveToFile();
	if (onBankChanged)
		onBankChanged();
}