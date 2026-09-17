#include "TrackEffectsPanel.h"
#include "BitCrusherComponent.h"
#include "ChorusComponent.h"
#include "CompressorComponent.h"
#include "DistortionComponent.h"
#include "EqualizerComponent.h"
#include "FilterComponent.h"
#include "FlangerComponent.h"
#include "GateComponent.h"
#include "LimiterComponent.h"
#include "PhaserComponent.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

TrackEffectsPanel::TrackEffectsPanel(DjIaVstProcessor &processor, DjIaVstEditor &editor)
    : audioProcessor(processor), editor(editor)
{
	setupUI();
}

TrackEffectsPanel::~TrackEffectsPanel() = default;

void TrackEffectsPanel::refresh()
{
	removeAllChildren();
	resetComponents();
	setupUI();
	resized();
}

void TrackEffectsPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds().toFloat();

	g.setColour(ColourPalette::backgroundDeep.withAlpha(Obsidian::ALPHA_04));
	g.fillRoundedRectangle(bounds, Obsidian::LIST_PANEL_CORNER_SIZE);
	g.setColour(ColourPalette::sliderTrack.withAlpha(0.3f));
	g.drawRoundedRectangle(bounds.reduced(0.5f), Obsidian::LIST_PANEL_CORNER_SIZE, 1.0f);
}

void TrackEffectsPanel::resized()
{
	auto area = getLocalBounds().reduced(4, 2);

	if (gateComponent)
	{
		gateComponent->setBounds(area.removeFromTop(Obsidian::GATE_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (distortionComponent)
	{
		distortionComponent->setBounds(area.removeFromTop(Obsidian::DISTORTION_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (bitCrusherComponent)
	{
		bitCrusherComponent->setBounds(area.removeFromTop(Obsidian::BITCRUSHER_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (equalizerComponent)
	{
		equalizerComponent->setBounds(area.removeFromTop(Obsidian::EQ_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (filterComponent)
	{
		filterComponent->setBounds(area.removeFromTop(Obsidian::FILTER_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (chorusComponent)
	{
		chorusComponent->setBounds(area.removeFromTop(Obsidian::CHORUS_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (flangerComponent)
	{
		flangerComponent->setBounds(area.removeFromTop(Obsidian::FLANGER_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (phaserComponent)
	{
		phaserComponent->setBounds(area.removeFromTop(Obsidian::PHASER_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (compressorComponent)
	{
		compressorComponent->setBounds(area.removeFromTop(Obsidian::COMPRESSOR_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
	if (limiterComponent)
	{
		limiterComponent->setBounds(area.removeFromTop(Obsidian::LIMITER_HEIGHT));
		area.removeFromTop(Obsidian::GAP_4);
	}
}

void TrackEffectsPanel::updateModelUI()
{
	if (distortionComponent)
		distortionComponent->updateModelUI();
	if (filterComponent)
		filterComponent->updateModelUI();
	if (chorusComponent)
		chorusComponent->updateModelUI();
	if (phaserComponent)
		phaserComponent->updateModelUI();
	if (compressorComponent)
		compressorComponent->updateModelUI();
	if (equalizerComponent)
		equalizerComponent->updateModelUI();
	if (limiterComponent)
		limiterComponent->updateModelUI();
	if (flangerComponent)
		flangerComponent->updateModelUI();
	if (bitCrusherComponent)
		bitCrusherComponent->updateModelUI();
	if (gateComponent)
		gateComponent->updateModelUI();
}

void TrackEffectsPanel::setupUI()
{
	auto trackIds = audioProcessor.getAllTrackIds();
	for (const auto &trackId : trackIds)
	{
		auto *track = audioProcessor.getTrack(trackId);
		if (track && track->isSelected.load())
		{
			resetComponents();
			addComponents(trackId);
			return;
		}
	}

	if (!trackIds.empty())
	{
		resetComponents();
		addComponents(trackIds[0]);
	}
}

void TrackEffectsPanel::showTrack(const juce::String &trackId)
{
	if (trackId == activeTrackId && !isMasterView)
		return;

	resetComponents();
	addComponents(trackId);
}

void TrackEffectsPanel::showMaster()
{
	if (isMasterView)
		return;

	resetComponents();
	addComponents();
}

void TrackEffectsPanel::addComponents(const juce::String &trackId)
{
	if (auto *currentTrack = audioProcessor.getTrack(trackId))
	{
		isMasterView = false;
		activeTrackId = trackId;

		distortionComponent = std::make_unique<DistortionComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*distortionComponent);
		filterComponent = std::make_unique<FilterComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*filterComponent);
		equalizerComponent = std::make_unique<EqualizerComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*equalizerComponent);
		compressorComponent = std::make_unique<CompressorComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*compressorComponent);
		limiterComponent = std::make_unique<LimiterComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*limiterComponent);
		chorusComponent = std::make_unique<ChorusComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*chorusComponent);
		phaserComponent = std::make_unique<PhaserComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*phaserComponent);
		flangerComponent = std::make_unique<FlangerComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*flangerComponent);
		bitCrusherComponent = std::make_unique<BitCrusherComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*bitCrusherComponent);
		gateComponent = std::make_unique<GateComponent>(audioProcessor, currentTrack);
		addAndMakeVisible(*gateComponent);

		resized();

		if (onContentChanged)
			onContentChanged();
	}
}

void TrackEffectsPanel::addComponents()
{
	isMasterView = true;
	activeTrackId = "";

	equalizerComponent = std::make_unique<EqualizerComponent>(audioProcessor, nullptr, true);
	addAndMakeVisible(*equalizerComponent);
	compressorComponent = std::make_unique<CompressorComponent>(audioProcessor, nullptr, true);
	addAndMakeVisible(*compressorComponent);
	limiterComponent = std::make_unique<LimiterComponent>(audioProcessor, nullptr, true);
	addAndMakeVisible(*limiterComponent);

	resized();

	if (onContentChanged)
		onContentChanged();
}

void TrackEffectsPanel::resetComponents()
{
	distortionComponent = nullptr;
	equalizerComponent = nullptr;
	filterComponent = nullptr;
	compressorComponent = nullptr;
	limiterComponent = nullptr;
	chorusComponent = nullptr;
	phaserComponent = nullptr;
	flangerComponent = nullptr;
	bitCrusherComponent = nullptr;
	gateComponent = nullptr;
}

int TrackEffectsPanel::getPreferredHeight() const
{
	int height = 4;

	if (distortionComponent)
		height += Obsidian::DISTORTION_HEIGHT + Obsidian::GAP_4;
	if (bitCrusherComponent)
		height += Obsidian::BITCRUSHER_HEIGHT + Obsidian::GAP_4;
	if (equalizerComponent)
		height += Obsidian::EQ_HEIGHT + Obsidian::GAP_4;
	if (filterComponent)
		height += Obsidian::FILTER_HEIGHT + Obsidian::GAP_4;
	if (chorusComponent)
		height += Obsidian::CHORUS_HEIGHT + Obsidian::GAP_4;
	if (flangerComponent)
		height += Obsidian::FLANGER_HEIGHT + Obsidian::GAP_4;
	if (phaserComponent)
		height += Obsidian::PHASER_HEIGHT + Obsidian::GAP_4;
	if (compressorComponent)
		height += Obsidian::COMPRESSOR_HEIGHT + Obsidian::GAP_4;
	if (limiterComponent)
		height += Obsidian::LIMITER_HEIGHT + Obsidian::GAP_4;
	if (gateComponent)
		height += Obsidian::GATE_HEIGHT + Obsidian::GAP_4;

	return height;
}