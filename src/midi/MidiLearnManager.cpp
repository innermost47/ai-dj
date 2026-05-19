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

void MidiLearnManager::processMidiMappings(const juce::MidiMessage &message)
{
	int midiChannel = message.getChannel() - 1;
	bool isWarning = false;
	for (auto &mapping : mappings)
	{
		bool matches = false;
		float value = 0.0f;
		juce::String statusMessage = "";

		if (mapping.midiType == 0 && message.isNoteOnOrOff() && mapping.midiChannel == midiChannel)
		{
			int noteNumber = message.getNoteNumber();
			bool isInSampleRange = (noteNumber >= 60 && noteNumber <= 67);

			if (isInSampleRange)
			{
				continue;
			}
			if (message.getNoteNumber() == mapping.midiNumber)
			{
				matches = true;
				statusMessage = "Note " + juce::String(mapping.midiNumber) + " >> " + mapping.parameterName;

				if (message.isNoteOn() && isBooleanParameter(mapping.parameterName))
				{
					auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
					if (param)
					{
						if (mapping.parameterName.contains("Generate"))
						{
							value = 1.0f;
							statusMessage += " (trigger)";
							if (mapping.processor->getIsGenerating())
							{
								statusMessage += " (trigger) - Generation already in progress, please wait";
								isWarning = true;
							}
						}
						else
						{
							float currentValue = param->getValue();
							value = (currentValue > 0.5f) ? 0.0f : 1.0f;
							statusMessage += " (toggle: " + juce::String(value > 0.5f ? "ON" : "OFF") + ")";
						}
					}
				}
				else if (message.isNoteOn())
				{
					value = message.getVelocity() / 127.0f;
					statusMessage += " (vel: " + juce::String(message.getVelocity()) + ")";
				}
				else
				{
					if (isBooleanParameter(mapping.parameterName))
						mustCheckForMidiEvent.store(true);
					continue;
				}
			}
		}
		else if (mapping.midiType == 1 && message.isController() && mapping.midiChannel == midiChannel)
		{
			if (message.getControllerNumber() == mapping.midiNumber)
			{
				matches = true;
				value = message.getControllerValue() / 127.0f;
				statusMessage = "CC" + juce::String(mapping.midiNumber) + " >> " + mapping.parameterName + " (" +
				                juce::String(message.getControllerValue()) + ")";
				int ccVal = message.getControllerValue();
				if (mapping.parameterName.endsWith("Page") && matches)
				{
					int slotNum = mapping.parameterName.substring(4, 5).getIntValue();

					juce::String suffix = (ccVal >= 96) ? "D" : (ccVal >= 64) ? "C" : (ccVal >= 32) ? "B" : "A";
					juce::String realParam = "slot" + juce::String(slotNum) + "Page" + suffix;

					if (auto *p = mapping.processor->getParameterTreeState().getParameter(realParam))
					{
						p->setValueNotifyingHost(1.0f);
						showStatus(mapping, "Slot " + juce::String(slotNum) + " -> Page " + suffix, false);
					}
					continue;
				}
				if (mapping.parameterName.endsWith("Seq"))
				{
					int seqIdx = (ccVal / 16) + 1;
					if (seqIdx > 8)
						seqIdx = 8;
					juce::String targetParam = mapping.parameterName;

					if (auto *p = mapping.processor->getParameterTreeState().getParameter(targetParam))
					{
						float normalizedValue = (static_cast<float>(seqIdx) - 1.0f) / 7.0f;

						p->setValueNotifyingHost(normalizedValue);
						statusMessage = "Slot Seq -> " + juce::String(seqIdx);
						showStatus(mapping, statusMessage, false);
					}
					continue;
				}
			}
		}
		else if (mapping.midiType == 2 && message.isPitchWheel() && mapping.midiChannel == midiChannel)
		{
			matches = true;
			value = (message.getPitchWheelValue() + 8192) / 16383.0f;
			statusMessage =
			    "Pitch Wheel >> " + mapping.parameterName + " (" + juce::String(message.getPitchWheelValue()) + ")";
		}

		if (matches && mapping.processor)
		{
			if (mapping.parameterName.startsWith("promptSelector_slot"))
			{
				if (mapping.uiCallback && mapping.processor->getActiveEditor())
				{
					mapping.uiCallback(value);
					showStatus(mapping, statusMessage);
				}
				continue;
			}
			if (mapping.parameterName == "nextTrack" || mapping.parameterName == "prevTrack")
			{
				if (message.isNoteOn() && isBooleanParameter(mapping.parameterName))
				{
					showStatus(mapping, statusMessage);
				}
				continue;
			}

			if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Page"))
			{
				if (message.isNoteOn())
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();

					int pageIndex = -1;
					if (mapping.parameterName.contains("PageA"))
						pageIndex = 0;
					else if (mapping.parameterName.contains("PageB"))
						pageIndex = 1;
					else if (mapping.parameterName.contains("PageC"))
						pageIndex = 2;
					else if (mapping.parameterName.contains("PageD"))
						pageIndex = 3;

					if (slotNumber >= 1 && slotNumber <= ObsidianDataConst::MAX_TRACKS && pageIndex >= 0)
					{
						auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
						if (param)
						{
							param->setValueNotifyingHost(1.0f);

							statusMessage += " (Page " + juce::String((char)('A' + pageIndex)) + " triggered)";

							showStatus(mapping, statusMessage);
						}
					}
				}

				continue;
			}
			if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Seq"))
			{
				if (message.isNoteOn())
				{
					auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
					if (param)
					{
						param->setValueNotifyingHost(1.0f);

						juce::String slotStr = mapping.parameterName.substring(4, 5);
						juce::String seqStr = mapping.parameterName.fromLastOccurrenceOf("Seq", false, false);

						statusMessage += " (Sequence " + seqStr + " selected)";

						showStatus(mapping, statusMessage);
					}
				}
				continue;
			}
			auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
			if (param)
			{
				param->setValueNotifyingHost(value);
				showStatus(mapping, statusMessage, isWarning);

				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Play"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= ObsidianDataConst::MAX_TRACKS)
					{
						changedPlaySlotIndex.store(slotNumber - 1);
						mustCheckForMidiEvent.store(true);
					}
				}
				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Generate"))
				{
					if (mapping.processor->getIsGenerating())
						return;
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= ObsidianDataConst::MAX_TRACKS)
					{
						changedGenerateSlotIndex.store(slotNumber - 1);
						mustCheckForMidiEvent.store(true);
					}
				}
				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("RandomRetrigger"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= ObsidianDataConst::MAX_TRACKS)
					{
						mustCheckForMidiEvent.store(true);
					}
				}

				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("RetriggerInterval"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= ObsidianDataConst::MAX_TRACKS)
					{
						mustCheckForMidiEvent.store(true);
					}
				}
				if (mapping.parameterName.contains("slot") &&
				    (mapping.parameterName.contains("AdsrAttack") || mapping.parameterName.contains("AdsrDecay") ||
				     mapping.parameterName.contains("AdsrSustain") || mapping.parameterName.contains("AdsrRelease")))
				{
					mustCheckForMidiEvent.store(true);
				}
			}
		}
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
	       parameterName.contains("RandomRetrigger") || parameterName == "nextTrack" || parameterName == "prevTrack" ||
	       parameterName == "generate";
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

	for (int i = 1; i <= ObsidianDataConst::MAX_TRACKS; ++i)
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

	for (int i = 1; i <= ObsidianDataConst::MAX_TRACKS; ++i)
	{
		const juce::String s = "slot" + juce::String(i);
		const juce::String d = "Slot " + juce::String(i);

		addCC(s + "Pitch", 19 + i, CH_SHAPE, d + " Pitch");
		addCC(s + "Fine", 29 + i, CH_SHAPE, d + " Fine");
		addCC(s + "AdsrAttack", 39 + i, CH_SHAPE, d + " ADSR Attack");
		addCC(s + "AdsrDecay", 49 + i, CH_SHAPE, d + " ADSR Decay");
		addCC(s + "AdsrSustain", 59 + i, CH_SHAPE, d + " ADSR Sustain");
		addCC(s + "AdsrRelease", 69 + i, CH_SHAPE, d + " ADSR Release");
		addCC(s + "RandomRetrigger", 79 + i, CH_SHAPE, d + " Beat Repeat");
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
