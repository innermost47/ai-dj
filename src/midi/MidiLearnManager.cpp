#include "MidiLearnManager.h"
#include "ColourPalette.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

MidiLearnManager::MidiLearnManager()
{
}

MidiLearnManager::~MidiLearnManager()
{
	const juce::ScopedLock lock(learnLock);
	stopTimer();
	currentLearningComponent = nullptr;
	learningProcessor = nullptr;
	isLearning = false;
}

void MidiLearnManager::startLearning(const juce::String &parameterName, DjIaVstProcessor *processor,
                                     std::function<void(float)> uiCallback, const juce::String &description,
                                     MidiLearnableBase *component)
{
	const juce::ScopedLock lock(learnLock);
	if (isLearning)
	{
		return;
	}
	stopLearning();
	learningParameter = parameterName;
	learningProcessor = processor;
	learningUiCallback = uiCallback;
	learningDescription = description;
	currentLearningComponent = component;
	isLearning = true;
	learnStartTime = juce::Time::currentTimeMillis();
	startTimerHz(10);

	if (currentLearningComponent)
	{
		currentLearningComponent->setLearningMode(true);
	}
}

void MidiLearnManager::stopLearning()
{
	const juce::ScopedLock lock(learnLock);
	if (!isLearning)
		return;

	if (currentLearningComponent)
	{
		currentLearningComponent->setLearningMode(false);
		currentLearningComponent = nullptr;
	}

	if (learningUiCallback != nullptr)
	{
		if (learningProcessor != nullptr)
		{
			auto *param = learningProcessor->getParameters().getParameter(learningParameter);
			if (param != nullptr)
			{
				float currentValue = param->getValue();
				learningUiCallback(currentValue);
			}
		}
	}

	isLearning = false;
	stopTimer();
	learningUiCallback = nullptr;
	learningDescription.clear();
}

void MidiLearnManager::timerCallback()
{
	if (juce::Time::currentTimeMillis() - learnStartTime > LEARN_TIMEOUT_MS)
	{
		stopLearning();

		juce::MessageManager::callAsync(
		    [this]()
		    {
			    if (learningProcessor && learningProcessor->getActiveEditor())
			    {
				    if (auto *editor = dynamic_cast<DjIaVstEditor *>(learningProcessor->getActiveEditor()))
				    {
					    editor->statusLabel.setText("MIDI Learn timeout - no controller received",
					                                juce::dontSendNotification);
					    editor->uiStatusManager->updateLCD();
				    }
			    }
		    });

		return;
	}
}

bool MidiLearnManager::processMidiForLearning(const juce::MidiMessage &message)
{
	if (!isLearning)
	{
		return false;
	}
	int midiType = -1;
	int midiNumber = 0;
	int midiChannel = message.getChannel() - 1;

	if (message.isController())
	{
		midiType = 1;
		midiNumber = message.getControllerNumber();
	}
	else if (message.isPitchWheel())
	{
		midiType = 2;
		midiNumber = 0;
	}
	else if (message.isNoteOnOrOff())
	{
		int noteNumber = message.getNoteNumber();
		bool isInSampleRange = (noteNumber >= 60 && noteNumber <= 67);

		if (!isInSampleRange)
		{
			midiType = 0;
			midiNumber = noteNumber;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	removeMapping(learningParameter);

	MidiMapping mapping;
	mapping.midiType = midiType;
	mapping.midiNumber = midiNumber;
	mapping.midiChannel = midiChannel;
	mapping.processor = learningProcessor;
	mapping.uiCallback = learningUiCallback;
	mapping.description = learningDescription;
	mapping.parameterName = learningParameter;

	mappings.push_back(mapping);

	juce::String midiDescription;
	switch (midiType)
	{
	case 0:
		midiDescription = "Note " + juce::MidiMessage::getMidiNoteName(midiNumber, true, true, 3);
		break;
	case 1:
		midiDescription = "CC " + juce::String(midiNumber);
		break;
	case 2:
		midiDescription = "Pitchbend";
		break;
	}

	juce::String fullMessage = "MIDI mapping created: " + midiDescription + " >> " + learningDescription;
	showStatus(mapping, fullMessage);

	stopLearning();

	return true;
}

int MidiLearnManager::getSlotNumberFromParam(const juce::String &parameterName)
{
	if (!parameterName.startsWith("slot"))
		return -1;
	return parameterName.substring(4, 5).getIntValue();
}

bool MidiLearnManager::isTrackSequenceParam(const juce::String &parameterName)
{
	return parameterName.contains("slot") && parameterName.contains("Seq") && !parameterName.contains("Glitch");
}

bool MidiLearnManager::handleNoteMapping(const MidiMapping &mapping, const juce::MidiMessage &message, float &value,
                                         juce::String &statusMessage, bool &isWarning)
{
	if (!message.isNoteOnOrOff())
		return false;

	int noteNumber = message.getNoteNumber();
	if (noteNumber >= 60 && noteNumber <= 67)
		return false;

	if (noteNumber != mapping.midiNumber)
		return false;

	bool isBool = isBooleanParameter(mapping.parameterName);
	statusMessage = "Note " + juce::String(mapping.midiNumber) + " >> " + mapping.parameterName;

	if (!message.isNoteOn())
	{
		if (isBool)
			mustCheckForMidiEvent.store(true);
		return false;
	}

	if (!isBool)
	{
		value = message.getVelocity() / 127.0f;
		statusMessage += " (vel: " + juce::String(message.getVelocity()) + ")";
		return true;
	}

	auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
	if (!param)
		return false;

	if (mapping.parameterName.contains("Generate"))
	{
		value = 1.0f;
		statusMessage += " (trigger)";
		if (mapping.processor->getIsGenerating())
		{
			statusMessage += " - Generation already in progress, please wait";
			isWarning = true;
		}
	}
	else
	{
		value = (param->getValue() > 0.5f) ? 0.0f : 1.0f;
		statusMessage += " (toggle: " + juce::String(value > 0.5f ? "ON" : "OFF") + ")";
	}

	return true;
}

bool MidiLearnManager::handleControllerMapping(const MidiMapping &mapping, const juce::MidiMessage &message,
                                               float &value, juce::String &statusMessage)
{
	if (!message.isController() || message.getControllerNumber() != mapping.midiNumber)
		return false;

	int ccVal = message.getControllerValue();
	value = ccVal / 127.0f;
	statusMessage =
	    "CC" + juce::String(mapping.midiNumber) + " >> " + mapping.parameterName + " (" + juce::String(ccVal) + ")";

	if (mapping.parameterName.endsWith("Page"))
	{
		int slotNum = getSlotNumberFromParam(mapping.parameterName);
		if (slotNum < 1 || slotNum > Obsidian::MAX_TRACKS)
			return false;

		juce::String suffix = (ccVal >= 96) ? "D" : (ccVal >= 64) ? "C" : (ccVal >= 32) ? "B" : "A";
		juce::String realParam = "slot" + juce::String(slotNum) + "Page" + suffix;

		if (auto *p = mapping.processor->getParameterTreeState().getParameter(realParam))
		{
			p->setValueNotifyingHost(1.0f);
			showStatus(mapping, "Slot " + juce::String(slotNum) + " -> Page " + suffix, false);
		}
		return false;
	}

	if (isTrackSequenceParam(mapping.parameterName) && mapping.parameterName.endsWith("Seq"))
	{
		int seqIdx = juce::jlimit(1, 8, (ccVal / 16) + 1);

		if (auto *p = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName))
		{
			p->setValueNotifyingHost((seqIdx - 1) / 7.0f);
			showStatus(mapping, "Slot Seq -> " + juce::String(seqIdx), false);
		}
		return false;
	}

	return true;
}

void MidiLearnManager::applyMapping(const MidiMapping &mapping, float value, juce::String statusMessage, bool isWarning)
{
	if (mapping.parameterName.startsWith("promptSelector_slot"))
	{
		if (mapping.uiCallback && mapping.processor->getActiveEditor())
		{
			mapping.uiCallback(value);
			showStatus(mapping, statusMessage);
		}
		return;
	}

	if (mapping.parameterName == "nextTrack" || mapping.parameterName == "prevTrack")
	{
		showStatus(mapping, statusMessage);
		return;
	}

	if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Page"))
	{
		int slotNumber = getSlotNumberFromParam(mapping.parameterName);
		int pageIndex = -1;

		if (mapping.parameterName.contains("PageA"))
			pageIndex = 0;
		else if (mapping.parameterName.contains("PageB"))
			pageIndex = 1;
		else if (mapping.parameterName.contains("PageC"))
			pageIndex = 2;
		else if (mapping.parameterName.contains("PageD"))
			pageIndex = 3;

		if (slotNumber < 1 || slotNumber > Obsidian::MAX_TRACKS || pageIndex < 0)
			return;

		if (auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName))
		{
			param->setValueNotifyingHost(1.0f);
			statusMessage += " (Page " + juce::String((char)('A' + pageIndex)) + " triggered)";
			showStatus(mapping, statusMessage);
		}
		return;
	}

	if (isTrackSequenceParam(mapping.parameterName))
	{
		if (auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName))
		{
			param->setValueNotifyingHost(1.0f);
			statusMessage +=
			    " (Sequence " + mapping.parameterName.fromLastOccurrenceOf("Seq", false, false) + " selected)";
			showStatus(mapping, statusMessage);
		}
		return;
	}

	auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
	if (!param)
		return;

	if (mapping.parameterName.startsWith("slot"))
	{
		TrackData *track = mapping.processor->getTrackFromParamId(mapping.parameterName);
		if (!track || track->slotIndex < 0 || track->slotIndex >= Obsidian::MAX_TRACKS)
			return;

		if (mapping.parameterName.contains("Play"))
		{
			if (track->getCurrentPage().numSamples <= 0)
				return;
			changedPlaySlotIndex.store(track->slotIndex);
		}
		else if (mapping.parameterName.contains("Generate"))
		{
			if (mapping.processor->getIsGenerating())
				return;
			changedGenerateSlotIndex.store(track->slotIndex);
		}
	}

	mustCheckForMidiEvent.store(true);
	param->setValueNotifyingHost(value);
	showStatus(mapping, statusMessage, isWarning);
}

void MidiLearnManager::processMidiMappings(const juce::MidiMessage &message)
{
	int midiChannel = message.getChannel() - 1;

	for (auto &mapping : mappings)
	{
		if (!mapping.processor || mapping.midiChannel != midiChannel)
			continue;

		float value = 0.0f;
		juce::String statusMessage = "";
		bool isWarning = false;
		bool matches = false;

		if (mapping.midiType == 0)
			matches = handleNoteMapping(mapping, message, value, statusMessage, isWarning);
		else if (mapping.midiType == 1)
			matches = handleControllerMapping(mapping, message, value, statusMessage);
		else if (mapping.midiType == 2 && message.isPitchWheel())
		{
			matches = true;
			value = juce::jlimit(0.0f, 1.0f, message.getPitchWheelValue() / 16383.0f);
			statusMessage =
			    "Pitch Wheel >> " + mapping.parameterName + " (" + juce::String(message.getPitchWheelValue()) + ")";
		}

		if (matches)
			applyMapping(mapping, value, statusMessage, isWarning);
	}
}

void MidiLearnManager::showStatus(const MidiMapping &mapping, const juce::String &text, bool isWarning)
{
	juce::MessageManager::callAsync(
	    [mapping, text, isWarning]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(mapping.processor->getActiveEditor()))
		    {
			    editor->statusLabel.setText(text, juce::dontSendNotification);
			    editor->uiStatusManager->updateLCD();
			    if (isWarning)
				    editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::textWarning);
		    }
	    });
}

bool MidiLearnManager::isBooleanParameter(const juce::String &parameterName)
{
	return parameterName.contains("Play") || parameterName.contains("Stop") || parameterName.contains("Mute") ||
	       parameterName.contains("Solo") || parameterName.contains("Generate") ||
	       parameterName.contains("BeatRepeatActive") || parameterName.contains("ReverseActive") ||
	       parameterName.contains("TransientScatterActive") || parameterName.contains("GlitchSeq") ||
	       parameterName.contains("GlitchChain") || parameterName.contains("Bypassed") ||
	       parameterName.contains("Page") || parameterName == "nextTrack" || parameterName == "prevTrack" ||
	       parameterName == "generate" || parameterName == "play" || parameterName == "useCrossfader";
}

void MidiLearnManager::registerUICallback(const juce::String &parameterName, std::function<void(float)> callback)
{
	registeredUICallbacks[parameterName] = callback;
	for (auto &mapping : mappings)
	{
		if (mapping.parameterName == parameterName)
		{
			mapping.uiCallback = callback;
			break;
		}
	}
}

void MidiLearnManager::restoreUICallbacks()
{
	for (auto &mapping : mappings)
	{
		auto it = registeredUICallbacks.find(mapping.parameterName);
		if (it != registeredUICallbacks.end())
		{
			mapping.uiCallback = it->second;
		}
	}
}

void MidiLearnManager::addMapping(const MidiMapping &midiMapping)
{
	mappings.push_back(midiMapping);
}

void MidiLearnManager::removeMapping(juce::String parameterName)
{
	mappings.erase(std::remove_if(mappings.begin(), mappings.end(), [parameterName](const MidiMapping &mapping)
	                              { return mapping.parameterName == parameterName; }),
	               mappings.end());
}

void MidiLearnManager::clearAllMappings()
{
	mappings.clear();
}

bool MidiLearnManager::removeMappingForParameter(const juce::String &parameterName)
{
	auto mappingIt = std::find_if(mappings.begin(), mappings.end(), [parameterName](const MidiMapping &mapping)
	                              { return mapping.parameterName == parameterName; });

	if (mappingIt == mappings.end())
	{
		return false;
	}

	DjIaVstProcessor *processor = mappingIt->processor;
	juce::String description = mappingIt->description;

	mappings.erase(mappingIt);
	juce::String statusMessage = "MIDI mapping removed: " + description;
	juce::MessageManager::callAsync(
	    [processor, statusMessage]()
	    {
		    if (auto *editor = dynamic_cast<DjIaVstEditor *>(processor->getActiveEditor()))
		    {
			    editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
			    editor->uiStatusManager->updateLCD();
		    }
	    });

	return true;
}

void MidiLearnManager::loadDefaultMappings(DjIaVstProcessor *processor)
{
	if (!mappings.empty())
		return;

	const int CH_PERF = 0;
	const int CH_SHAPE = 1;
	const int CH_XFADER = 2;
	const int CH_FX = 3;

	auto addNote = [&](const juce::String &param, int note, int channel, const juce::String &desc)
	{
		MidiMapping m;
		m.midiType = 0;
		m.midiNumber = note;
		m.midiChannel = channel;
		m.processor = processor;
		m.parameterName = param;
		m.description = desc;
		mappings.push_back(m);
	};

	auto addCC = [&](const juce::String &param, int cc, int channel, const juce::String &desc)
	{
		MidiMapping m;
		m.midiType = 1;
		m.midiNumber = cc;
		m.midiChannel = channel;
		m.processor = processor;
		m.parameterName = param;
		m.description = desc;
		mappings.push_back(m);
	};

	addCC("masterVolume", 7, CH_PERF, "Master Volume");
	addCC("masterPan", 10, CH_PERF, "Master Pan");

	addCC("delayFeedback", 20, CH_FX, "Delay Feedback");
	addCC("delayDivision", 21, CH_FX, "Delay Division");
	addCC("delayMode", 22, CH_FX, "Delay Mode");
	addCC("reverbSize", 23, CH_FX, "Reverb Size");
	addCC("reverbDamping", 24, CH_FX, "Reverb Damping");
	addCC("reverbWidth", 25, CH_FX, "Reverb Width");
	addCC("reverbMix", 26, CH_FX, "Reverb Mix");

	for (int i = 1; i <= Obsidian::MAX_TRACKS; ++i)
	{
		const juce::String s = "slot" + juce::String(i);
		const juce::String d = "Slot " + juce::String(i);

		addNote(s + "Play", 35 + i, CH_PERF, d + " Play");

		addCC(s + "Volume", 19 + i, CH_PERF, d + " Volume");
		addCC(s + "Pan", 29 + i, CH_PERF, d + " Pan");
		addCC(s + "Mute", 39 + i, CH_PERF, d + " Mute");
		addCC(s + "Solo", 49 + i, CH_PERF, d + " Solo");
		addCC(s + "Generate", 59 + i, CH_PERF, d + " Generate");
	}

	for (int i = 1; i <= Obsidian::MAX_TRACKS; ++i)
	{
		const juce::String s = "slot" + juce::String(i);
		const juce::String d = "Slot " + juce::String(i);

		addCC(s + "Pitch", 19 + i, CH_SHAPE, d + " Pitch");
		addCC(s + "Fine", 29 + i, CH_SHAPE, d + " Fine");
		addCC(s + "AdsrAttack", 39 + i, CH_SHAPE, d + " ADSR Attack");
		addCC(s + "AdsrDecay", 49 + i, CH_SHAPE, d + " ADSR Decay");
		addCC(s + "AdsrSustain", 59 + i, CH_SHAPE, d + " ADSR Sustain");
		addCC(s + "AdsrRelease", 69 + i, CH_SHAPE, d + " ADSR Release");
		addCC(s + "BeatRepeatActive", 79 + i, CH_SHAPE, d + " Beat Repeat");
		addCC(s + "Page", 89 + i, CH_SHAPE, d + " Page");
		addCC(s + "DelaySend", 30 + i, CH_FX, d + " Delay Send");
		addCC(s + "ReverbSend", 39 + i, CH_FX, d + " Reverb Send");
		addCC(s + "Seq", 99 + i, CH_SHAPE, d + " Seq");
	}

	addCC("pairCrossfader1", 20, CH_XFADER, "Crossfader 1 <-> 5");
	addCC("pairCrossfader2", 21, CH_XFADER, "Crossfader 2 <-> 6");
	addCC("pairCrossfader3", 22, CH_XFADER, "Crossfader 3 <-> 7");
	addCC("pairCrossfader4", 23, CH_XFADER, "Crossfader 4 <-> 8");
	addCC("globalCrossfader", 24, CH_XFADER, "Global Crossfader (Deck A/B)");
	addCC("crossfaderCurveMode", 25, CH_XFADER, "Crossfader Curve Mode");
	addCC("masterHigh", 26, CH_XFADER, "Master High EQ");
	addCC("masterMid", 27, CH_XFADER, "Master Mid EQ");
	addCC("masterLow", 28, CH_XFADER, "Master Low EQ");
}
