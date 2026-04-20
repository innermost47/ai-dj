#include <JuceHeader.h>
#include "MidiLearnManager.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ColourPalette.h"

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

void MidiLearnManager::startLearning(const juce::String &parameterName,
									 DjIaVstProcessor *processor,
									 std::function<void(float)> uiCallback,
									 const juce::String &description,
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
	DBG("MIDI Learn stopped");
}

void MidiLearnManager::timerCallback()
{
	if (juce::Time::currentTimeMillis() - learnStartTime > LEARN_TIMEOUT_MS)
	{
		DBG("MIDI Learn timeout");

		stopLearning();

		juce::MessageManager::callAsync([this]()
										{
				if (learningProcessor && learningProcessor->getActiveEditor())
				{
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(learningProcessor->getActiveEditor()))
					{
						editor->statusLabel.setText("MIDI Learn timeout - no controller received", juce::dontSendNotification);
						editor->updateLCD();
						juce::Timer::callAfterDelay(2000, [editor]()
							{
								if (editor)
								{
									editor->statusLabel.setText("Ready", juce::dontSendNotification);
									editor->updateLCD();
								}
							});
					}
				} });

		return;
	}
}

void MidiLearnManager::removeMappingsForSlot(int slotNumber)
{
	juce::String slotPrefix = "slot" + juce::String(slotNumber);
	for (int i = static_cast<int>(mappings.size()) - 1; i >= 0; --i)
	{
		if (mappings[i].parameterName.startsWith(slotPrefix))
		{
			mappings.erase(mappings.begin() + i);
		}
	}
}

void MidiLearnManager::moveMappingsFromSlotToSlot(int fromSlot, int toSlot)
{
	if (fromSlot == toSlot)
		return;

	juce::String fromPrefix = "slot" + juce::String(fromSlot);
	juce::String toPrefix = "slot" + juce::String(toSlot);

	DBG("Moving MIDI mappings from " << fromPrefix << " to " << toPrefix);

	removeMappingsForSlot(toSlot);
	DBG("Cleared existing mappings for slot " << toSlot);

	std::vector<MidiMapping> mappingsToMove;

	for (auto it = mappings.begin(); it != mappings.end();)
	{
		if (it->parameterName.startsWith(fromPrefix))
		{
			MidiMapping movedMapping = *it;

			juce::String suffix = it->parameterName.substring(fromPrefix.length());
			movedMapping.parameterName = toPrefix + suffix;

			movedMapping.description = movedMapping.description.replace(
				"Slot " + juce::String(fromSlot),
				"Slot " + juce::String(toSlot));

			mappingsToMove.push_back(movedMapping);
			it = mappings.erase(it);

			DBG("Moved mapping: " << movedMapping.parameterName);
		}
		else
		{
			++it;
		}
	}

	for (const auto &mapping : mappingsToMove)
	{
		mappings.push_back(mapping);
	}
}
bool MidiLearnManager::processMidiForLearning(const juce::MidiMessage &message)
{
	if (!isLearning)
	{
		return false;
	}
	DBG("MIDI received: " + message.getDescription());
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
	DBG(fullMessage);
	juce::MessageManager::callAsync([mapping, fullMessage]()
									{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
			{
				editor->statusLabel.setText(fullMessage, juce::dontSendNotification);
				editor->updateLCD();
				juce::Timer::callAfterDelay(2000, [mapping]() {
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
						editor->statusLabel.setText("Ready", juce::dontSendNotification);
						editor->updateLCD();
					}
					});
			} });

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
				statusMessage = "CC" + juce::String(mapping.midiNumber) + " >> " + mapping.parameterName +
								" (" + juce::String(message.getControllerValue()) + ")";
				int ccVal = message.getControllerValue();
				if (mapping.parameterName.endsWith("Page") && matches)
				{
					int ccVal = message.getControllerValue();
					int slotNum = mapping.parameterName.substring(4, 5).getIntValue();

					juce::String suffix = (ccVal >= 96) ? "D" : (ccVal >= 64) ? "C"
															: (ccVal >= 32)	  ? "B"
																			  : "A";
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
					juce::String targetParam = mapping.parameterName + juce::String(seqIdx);

					if (auto *p = mapping.processor->getParameterTreeState().getParameter(targetParam))
					{
						p->setValueNotifyingHost(1.0f);
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
			statusMessage = "Pitch Wheel >> " + mapping.parameterName +
							" (" + juce::String(message.getPitchWheelValue()) + ")";
		}

		if (matches && mapping.processor)
		{
			if (mapping.parameterName == "promptPresetSelector")
			{
				if (mapping.uiCallback && mapping.processor->getActiveEditor())
				{
					mapping.uiCallback(value);

					juce::MessageManager::callAsync([mapping, statusMessage]()
													{
							if (mapping.processor->getActiveEditor())
							{
								if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
								{
									editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
									editor->updateLCD();
									juce::Timer::callAfterDelay(2000, [mapping]() {
										if (mapping.processor->getActiveEditor()) {
											if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
												editor->statusLabel.setText("Ready", juce::dontSendNotification);
												editor->updateLCD();
											}
										}
										});
								}
							} });
				}
				continue;
			}
			if (mapping.parameterName.startsWith("promptSelector_slot"))
			{
				if (mapping.uiCallback && mapping.processor->getActiveEditor())
				{
					mapping.uiCallback(value);

					juce::MessageManager::callAsync([mapping, statusMessage]()
													{
							if (mapping.processor->getActiveEditor())
							{
								if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
								{
									editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
									editor->updateLCD();
									juce::Timer::callAfterDelay(2000, [mapping]() {
										if (mapping.processor->getActiveEditor()) {
											if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
												editor->statusLabel.setText("Ready", juce::dontSendNotification);
												editor->updateLCD();
											}
										}
										});
								}
							} });
				}
				continue;
			}
			if (mapping.parameterName == "nextTrack" || mapping.parameterName == "prevTrack")
			{
				if (message.isNoteOn() && isBooleanParameter(mapping.parameterName))
				{
					if (mapping.parameterName == "nextTrack")
					{
						mapping.processor->selectNextTrack();
						statusMessage += " (Next Track triggered)";
					}
					else if (mapping.parameterName == "prevTrack")
					{
						mapping.processor->selectPreviousTrack();
						statusMessage += " (Previous Track triggered)";
					}

					juce::MessageManager::callAsync([mapping, statusMessage]()
													{
							if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
							{
								editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
								editor->updateLCD();
								juce::Timer::callAfterDelay(2000, [mapping]() {
									if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
										editor->statusLabel.setText("Ready", juce::dontSendNotification);
										editor->updateLCD();
									}
									});
							} });
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

					if (slotNumber >= 1 && slotNumber <= 8 && pageIndex >= 0)
					{
						auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
						if (param)
						{
							param->setValueNotifyingHost(1.0f);

							statusMessage += " (Page " + juce::String((char)('A' + pageIndex)) + " triggered)";

							juce::MessageManager::callAsync([mapping, statusMessage]()
															{
									if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
									{
										editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
										editor->updateLCD();
										juce::Timer::callAfterDelay(2000, [mapping]() {
											if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
												editor->statusLabel.setText("Ready", juce::dontSendNotification);
												editor->updateLCD();
											}
											});
									} });
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

						juce::MessageManager::callAsync([mapping, statusMessage]()
														{
								if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
								{
									editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
									editor->updateLCD();
									juce::Timer::callAfterDelay(2000, [mapping]() {
										if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
											editor->statusLabel.setText("Ready", juce::dontSendNotification);
											editor->updateLCD();
										}
										});
								} });
					}
				}
				continue;
			}
			if (mapping.parameterName == "generate")
			{
				if (message.isNoteOn() && isBooleanParameter(mapping.parameterName))
				{
					if (mapping.processor->getIsGenerating())
					{
						statusMessage += " (Generation already in progress)";
						isWarning = true;
					}
					else
					{
						mapping.processor->triggerGlobalGeneration();
						statusMessage += " (Generation triggered)";
					}

					juce::MessageManager::callAsync([mapping, statusMessage, isWarning]()
													{
							if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
							{
								editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
								editor->updateLCD();
								if (isWarning) {
									editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::textWarning);
								}
								juce::Timer::callAfterDelay(2000, [mapping]() {
									if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
										editor->statusLabel.setText("Ready", juce::dontSendNotification);
										editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
										editor->updateLCD();
									}
									});
							} });
				}
				continue;
			}
			auto *param = mapping.processor->getParameterTreeState().getParameter(mapping.parameterName);
			if (param)
			{
				if (mapping.parameterName.startsWith("slot"))
				{
					juce::String slotPart = mapping.parameterName.substring(0, 5);
					auto trackIds = mapping.processor->getAllTrackIds();
					for (const auto &trackId : trackIds)
					{
						TrackData *track = mapping.processor->getTrack(trackId);
						if (track)
						{
							juce::String expectedSlot = "slot" + juce::String(track->slotIndex + 1);
							if (slotPart == expectedSlot)
							{
								break;
							}
						}
					}
				}
				param->setValueNotifyingHost(value);
				juce::MessageManager::callAsync([mapping, statusMessage, isWarning]()
												{
						if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor()))
						{
							editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
							editor->updateLCD();
							if (isWarning) {
								editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::textWarning);
							}
							juce::Timer::callAfterDelay(2000, [mapping]() {
								if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
									editor->statusLabel.setText("Ready", juce::dontSendNotification);
									editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
									editor->updateLCD();
								}
								});
						} });

				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("Play"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= 8)
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
					if (slotNumber >= 1 && slotNumber <= 8)
					{
						changedGenerateSlotIndex.store(slotNumber - 1);
						mustCheckForMidiEvent.store(true);
					}
				}
				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("RandomRetrigger"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= 8)
					{
						mustCheckForMidiEvent.store(true);
					}
				}

				if (mapping.parameterName.contains("slot") && mapping.parameterName.contains("RetriggerInterval"))
				{
					juce::String slotStr = mapping.parameterName.substring(4, 5);
					int slotNumber = slotStr.getIntValue();
					if (slotNumber >= 1 && slotNumber <= 8)
					{
						mustCheckForMidiEvent.store(true);
					}
				}
			}
		}
	}
}

void MidiLearnManager::showStatus(const MidiMapping &mapping, const juce::String &text, bool isWarning)
{
	juce::MessageManager::callAsync([mapping, text, isWarning]()
									{
		if (auto* editor = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
			editor->statusLabel.setText(text, juce::dontSendNotification);
			editor->updateLCD();
			if (isWarning)
				editor->statusLabel.setColour(juce::Label::textColourId, ColourPalette::textWarning);

			juce::Timer::callAfterDelay(2000, [mapping]() {
				if (auto* ed = dynamic_cast<DjIaVstEditor*>(mapping.processor->getActiveEditor())) {
					ed->statusLabel.setText("Ready", juce::dontSendNotification);
					ed->statusLabel.setColour(juce::Label::textColourId, ColourPalette::violet);
					ed->updateLCD();
				}
				});
		} });
}

bool MidiLearnManager::isBooleanParameter(const juce::String &parameterName)
{
	return parameterName.contains("Play") ||
		   parameterName.contains("Stop") ||
		   parameterName.contains("Mute") ||
		   parameterName.contains("Solo") ||
		   parameterName.contains("Generate") ||
		   parameterName.contains("RandomRetrigger") ||
		   parameterName == "nextTrack" ||
		   parameterName == "prevTrack" ||
		   parameterName == "generate";
}

void MidiLearnManager::clearUICallbacks()
{
	registeredUICallbacks.clear();
	for (auto &mapping : mappings)
	{
		mapping.uiCallback = nullptr;
	}
	DBG("UI callbacks cleared");
}

void MidiLearnManager::registerUICallback(const juce::String &parameterName,
										  std::function<void(float)> callback)
{
	registeredUICallbacks[parameterName] = callback;
	for (auto &mapping : mappings)
	{
		if (mapping.parameterName == parameterName)
		{
			mapping.uiCallback = callback;
			DBG("Immediately restored callback for existing mapping: " + parameterName);
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
	mappings.erase(
		std::remove_if(mappings.begin(), mappings.end(),
					   [parameterName](const MidiMapping &mapping)
					   {
						   return mapping.parameterName == parameterName;
					   }),
		mappings.end());
}

void MidiLearnManager::clearAllMappings()
{
	mappings.clear();
	DBG("All MIDI mappings cleared");
}

bool MidiLearnManager::removeMappingForParameter(const juce::String &parameterName)
{
	auto mappingIt = std::find_if(mappings.begin(), mappings.end(),
								  [parameterName](const MidiMapping &mapping)
								  {
									  return mapping.parameterName == parameterName;
								  });

	if (mappingIt == mappings.end())
	{
		return false;
	}

	DjIaVstProcessor *processor = mappingIt->processor;
	juce::String description = mappingIt->description;

	mappings.erase(mappingIt);
	juce::String statusMessage = "MIDI mapping removed: " + description;
	DBG(statusMessage);
	juce::MessageManager::callAsync([processor, statusMessage]()
									{
			if (auto* editor = dynamic_cast<DjIaVstEditor*>(processor->getActiveEditor()))
			{
				editor->statusLabel.setText(statusMessage, juce::dontSendNotification);
				editor->updateLCD();
				juce::Timer::callAfterDelay(2000, [processor]() {
					if (auto* editor = dynamic_cast<DjIaVstEditor*>(processor->getActiveEditor())) {
						editor->statusLabel.setText("Ready", juce::dontSendNotification);
						editor->updateLCD();
					}
					});
			} });

	return true;
}

bool MidiLearnManager::hasMappingForParameter(const juce::String &parameterName) const
{
	return std::any_of(mappings.begin(), mappings.end(),
					   [parameterName](const MidiMapping &mapping)
					   {
						   return mapping.parameterName == parameterName;
					   });
}

juce::String MidiLearnManager::getMappingDescription(const juce::String &parameterName) const
{
	auto it = std::find_if(mappings.begin(), mappings.end(),
						   [parameterName](const MidiMapping &mapping)
						   {
							   return mapping.parameterName == parameterName;
						   });

	if (it != mappings.end())
	{
		juce::String midiDescription;
		switch (it->midiType)
		{
		case 0:
			midiDescription = "Note " + juce::MidiMessage::getMidiNoteName(it->midiNumber, true, true, 3);
			break;
		case 1:
			midiDescription = "CC " + juce::String(it->midiNumber);
			break;
		case 2:
			midiDescription = "Pitchbend";
			break;
		}
		return midiDescription + " (Ch." + juce::String(it->midiChannel + 1) + ")";
	}

	return juce::String();
}
void MidiLearnManager::loadDefaultMappings(DjIaVstProcessor *processor)
{
	if (!mappings.empty())
		return;

	auto addNote = [&](const juce::String &param, int note, const juce::String &desc)
	{
		MidiMapping m;
		m.midiType = 0;
		m.midiNumber = note;
		m.midiChannel = 0;
		m.processor = processor;
		m.parameterName = param;
		m.description = desc;
		mappings.push_back(m);
	};

	auto addCC = [&](const juce::String &param, int cc, const juce::String &desc)
	{
		MidiMapping m;
		m.midiType = 1;
		m.midiNumber = cc;
		m.midiChannel = 0;
		m.processor = processor;
		m.parameterName = param;
		m.description = desc;
		mappings.push_back(m);
	};

	addCC("masterVolume", 7, "Master Volume");
	addCC("masterPan", 10, "Master Pan");

	for (int i = 1; i <= 8; ++i)
	{
		juce::String s = "slot" + juce::String(i);

		addNote(s + "Play", 35 + i, "Slot " + juce::String(i) + " Play");

		addCC(s + "Volume", 19 + i, "Slot " + juce::String(i) + " Volume");
		addCC(s + "Pan", 29 + i, "Slot " + juce::String(i) + " Pan");
		addCC(s + "Mute", 39 + i, "Slot " + juce::String(i) + " Mute");
		addCC(s + "Solo", 49 + i, "Slot " + juce::String(i) + " Solo");
		addCC(s + "Generate", 59 + i, "Slot " + juce::String(i) + " Generate");
		addCC(s + "Pitch", 99 + i, "Slot " + juce::String(i) + " Pitch");
		addCC(s + "Fine", 109 + i, "Slot " + juce::String(i) + " Fine");
		addCC(s + "RandomRetrigger", 119 + i, "Slot " + juce::String(i) + " Beat Repeat");

		addCC(s + "Page", 89 + i, "Slot " + juce::String(i) + " Page");
		addCC(s + "Seq", 15 + i, "Slot " + juce::String(i) + " Seq");
	}

	DBG("Default MIDI mappings loaded (" + juce::String(mappings.size()) + " mappings)");
}
