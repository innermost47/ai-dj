#include "DetailPanel.h"

DetailPanel::DetailPanel()
{
	addAndMakeVisible(nameLabel);
	nameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	nameLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));

	addAndMakeVisible(modelLabel);
	modelLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	modelLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO));

	addAndMakeVisible(metaLabel);
	metaLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	metaLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO));

	infoSvg = juce::Drawable::createFromImageData(BinaryData::warning_svg, BinaryData::warning_svgSize);
	if (!juce::JUCEApplicationBase::isStandaloneApp())
	{
		infoSvg->replaceColour(juce::Colours::black, ColourPalette::textSecondary);
		addAndMakeVisible(infoSvg.get());
	}

	addAndMakeVisible(tipLabel);
	tipLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	tipLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_XXS, juce::Font::italic));
	tipLabel.setText("Preview routed to Output 9. Ensure Multi-Output mode is enabled in your DAW.",
	                 juce::dontSendNotification);

	addAndMakeVisible(playButton);
	playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
	playButton.loadIconToggled(BinaryData::square_svg, BinaryData::square_svgSize);
	playButton.setHasToggledIcon(true);
	playButton.setClickingTogglesState(false);
	playButton.onClick = [this]()
	{
		if (!entry)
			return;
		if (isPlaying)
		{
			if (onStopRequested)
				onStopRequested();
		}
		else
		{
			if (onPlayRequested)
				onPlayRequested(entry);
		}
	};
	playButton.setCompactMode(true);

	playButton.setTooltip("Preview sound on Output 9");
	setVisible(false);
}

DetailPanel::~DetailPanel()
{
	destroyed.store(true);
	validity->store(false);
	stopTimer();
	removeAllChildren();
}

void DetailPanel::setEntry(SampleBankEntry *e)
{
	if (entry != e)
	{
		validity->store(false);
		validity = std::make_shared<std::atomic<bool>>(true);
		audioBuf.setSize(0, 0);
		thumbL.clear();
		thumbR.clear();
	}
	entry = e;
	isPlaying = false;
	playbackPos = 0.0f;
	updatePlayButton();
	if (!entry)
	{
		nameLabel.setText("", juce::dontSendNotification);
		metaLabel.setText("", juce::dontSendNotification);
		modelLabel.setText("", juce::dontSendNotification);
		return;
	}

	juce::String nameText = entry->originalPrompt;
	nameLabel.setText(nameText, juce::dontSendNotification);

	if (entry->modelName.isNotEmpty())
		modelLabel.setText(entry->modelName, juce::dontSendNotification);
	else
		modelLabel.setText("Unknown model", juce::dontSendNotification);

	juce::StringArray parts;
	parts.add(formatDuration(entry->duration));
	parts.add(juce::String(entry->bpm, 1) + " BPM");

	if (entry->usedInProjects.empty())
		parts.add("Unused");
	else
		parts.add(juce::String((int)entry->usedInProjects.size()) + " project(s)");

	if (!entry->category.isEmpty())
		parts.add("[" + entry->category + "]");

	metaLabel.setText(parts.joinIntoString(" - "), juce::dontSendNotification);

	setVisible(true);
	resized();
	loadAudio();
}

void DetailPanel::loadAudio()
{
	if (!entry)
		return;
	juce::File f(entry->filePath);
	if (!f.exists())
		return;

	auto v = validity;
	juce::Thread::launch(
	    [this, f, v]()
	    {
		    if (!v->load())
			    return;

		    juce::AudioFormatManager fm;
		    fm.registerBasicFormats();
		    auto reader = std::unique_ptr<juce::AudioFormatReader>(fm.createReaderFor(f));
		    if (!reader || !v->load())
			    return;

		    const int total = (int)reader->lengthInSamples;
		    const int target = 4096;
		    const int ratio = std::max(1, total / target);
		    const int num = total / ratio;

		    juce::AudioBuffer<float> full(reader->numChannels, total);
		    reader->read(&full, 0, total, 0, true, true);
		    if (!v->load())
			    return;

		    auto buf = std::make_shared<juce::AudioBuffer<float>>(reader->numChannels, num);
		    for (int i = 0; i < num; ++i)
			    for (int ch = 0; ch < (int)reader->numChannels; ++ch)
				    buf->setSample(ch, i, full.getSample(ch, i * ratio));

		    juce::MessageManager::callAsync(
		        [this, buf, v]()
		        {
			        if (!v->load() || destroyed.load())
				        return;
			        audioBuf = *buf;
			        generateThumbnail();
			        repaint();
		        });
	    });
}

void DetailPanel::generateThumbnail()
{
	thumbL.clear();
	thumbR.clear();
	if (audioBuf.getNumSamples() == 0 || waveformBounds.isEmpty())
		return;

	int n = audioBuf.getNumSamples();
	int ch = audioBuf.getNumChannels();
	bool mono = (ch == 1);
	int w = waveformBounds.getWidth();
	if (w <= 0)
		return;
	int spp = std::max(1, n / w);

	for (int p = 0; p < w; ++p)
	{
		int s0 = p * spp, s1 = std::min(s0 + spp, n);
		float rL = 0, rR = 0, pL = 0, pR = 0;
		int cnt = 0;
		for (int s = s0; s < s1; ++s)
		{
			float vL = audioBuf.getSample(0, s);
			float vR = mono ? vL : audioBuf.getSample(1, s);
			rL += vL * vL;
			rR += vR * vR;
			pL = std::max(pL, std::abs(vL));
			pR = std::max(pR, std::abs(vR));
			++cnt;
		}
		float fL = cnt > 0 ? std::sqrt(rL / cnt) * 0.7f + pL * 0.3f : 0.0f;
		float fR = cnt > 0 ? std::sqrt(rR / cnt) * 0.7f + pR * 0.3f : 0.0f;
		thumbL.push_back(fL);
		thumbR.push_back(fR);
	}
}

void DetailPanel::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();
	g.fillAll(ColourPalette::backgroundDark);

	if (entry && !entry->category.isEmpty())
	{
		const float ellipseSize = 8.0f;
		juce::Colour col = this->categoryColourResolver ? this->categoryColourResolver(entry->category)
		                                                : getCategoryColor(entry->category);
		g.setColour(col);
		g.fillEllipse(ellipseSize, ObsidianSizes::GAP_4 + ellipseSize + 1.0f, ellipseSize, ellipseSize);
	}

	drawWaveform(g);
}

void DetailPanel::drawWaveform(juce::Graphics &g)
{
	if (thumbL.empty() || waveformBounds.isEmpty())
		return;

	g.saveState();
	juce::Path clip;
	clip.addRoundedRectangle(waveformBounds.toFloat(), ObsidianSizes::HALF_CORNER);
	g.reduceClipRegion(clip);

	g.setColour(ColourPalette::backgroundDeep);
	g.fillRect(waveformBounds);

	auto innerBounds = waveformBounds;

	size_t sz = std::min(thumbL.size(), thumbR.size());
	float ppx = (float)innerBounds.getWidth() / (float)sz;
	bool stereo = (thumbR.size() == thumbL.size() && audioBuf.getNumChannels() > 1);

	auto drawChannel = [&](const std::vector<float> &thumb, float centerY, float halfH)
	{
		juce::Path wavePath;
		bool started = false;

		for (size_t i = 0; i < sz; i++)
		{
			float x = innerBounds.getX() + i * ppx;
			float h = thumb[i] * halfH;
			float yTop = centerY - h;

			if (!started)
			{
				wavePath.startNewSubPath(x, yTop);
				started = true;
			}
			else
			{
				wavePath.lineTo(x, yTop);
			}
		}

		for (int i = (int)sz - 1; i >= 0; --i)
		{
			float x = innerBounds.getX() + i * ppx;
			float h = thumb[i] * halfH;
			float yBot = centerY + h;

			wavePath.lineTo(x, yBot);
		}

		wavePath.closeSubPath();

		g.setColour(ColourPalette::textPrimary);
		g.fillPath(wavePath);
		g.strokePath(wavePath, juce::PathStrokeType(0.6f));
	};

	if (stereo)
	{
		float qY = innerBounds.getY() + innerBounds.getHeight() * 0.25f;
		float tqY = innerBounds.getY() + innerBounds.getHeight() * 0.75f;
		float hH = innerBounds.getHeight() * 0.22f;
		drawChannel(thumbL, qY, hH);
		drawChannel(thumbR, tqY, hH);
		g.setColour(ColourPalette::textPrimary.withAlpha(ObsidianShades::ALPHA_06));
		g.drawLine((float)innerBounds.getX(), (float)innerBounds.getCentreY(), (float)innerBounds.getRight(),
		           (float)innerBounds.getCentreY(), 0.5f);
	}
	else
	{
		float cY = (float)innerBounds.getCentreY();
		float hH = innerBounds.getHeight() * 0.42f;
		drawChannel(thumbL, cY, hH);
	}

	if (isPlaying && entry && entry->duration > 0.0f)
	{
		float prog = playbackPos / entry->duration;
		float hx = innerBounds.getX() + prog * innerBounds.getWidth();
		g.setColour(ColourPalette::textPrimary);
		g.drawLine(hx, (float)innerBounds.getY(), hx, (float)innerBounds.getBottom(), 0.5f);
	}

	g.restoreState();

	g.setColour(ColourPalette::textSecondary.withAlpha(ObsidianShades::ALPHA_04));
	g.drawRoundedRectangle(waveformBounds.toFloat(), ObsidianSizes::HALF_CORNER, ObsidianSizes::BORDER_WIDTH_XS);
}

void DetailPanel::resized()
{
	auto area = getLocalBounds().reduced(6, 4);
	area.removeFromTop(ObsidianSizes::GAP_4);
	area.removeFromBottom(ObsidianSizes::GAP);
	auto titleRow = area.removeFromTop(18);
	auto modelRow = area.removeFromTop(14);
	auto metaRow = area.removeFromTop(14);
	juce::Rectangle<int> iconArea;
	juce::Rectangle<int> tipRow;
	int iconSize = 10;
	if (!juce::JUCEApplicationBase::isStandaloneApp())
	{
		tipRow = area.removeFromBottom(22);
		area.removeFromBottom(ObsidianSizes::GAP_4);
		tipRow.removeFromLeft(ObsidianSizes::GAP_4);
		iconArea = tipRow.removeFromLeft(iconSize).removeFromTop(tipRow.getHeight());
	}
	auto bottomRow = area;

	titleRow.removeFromLeft(10);
	nameLabel.setBounds(titleRow);
	modelLabel.setBounds(modelRow);
	metaLabel.setBounds(metaRow);

	if (!juce::JUCEApplicationBase::isStandaloneApp())
	{
		if (infoSvg != nullptr)
		{
			infoSvg->setTransformToFit(iconArea.toFloat(), juce::RectanglePlacement::centred);
		}
		tipLabel.setBounds(tipRow);
	}

	bottomRow.removeFromTop(ObsidianSizes::GAP_4);

	auto btnCol = bottomRow.removeFromRight(30);

	playButton.setBounds(btnCol.removeFromTop(bottomRow.getHeight()));
	bottomRow.removeFromRight(ObsidianSizes::GAP_4);
	bottomRow.removeFromLeft(ObsidianSizes::GAP_4);
	waveformBounds = bottomRow.reduced(0, 2);
	generateThumbnail();
	repaint();
}

void DetailPanel::setIsPlaying(bool playing)
{
	isPlaying = playing;
	updatePlayButton();
	if (playing)
	{
		playbackPos = 0.0f;
		lastTimerCall = juce::Time::getMillisecondCounterHiRes() / 1000.0;
		startTimer(30);
	}
	else
	{
		stopTimer();
		playbackPos = 0.0f;
	}
	repaint();
}

void DetailPanel::updatePlaybackPosition(float pos)
{
	playbackPos = pos;
	repaint();
}

void DetailPanel::timerCallback()
{
	if (!isPlaying || !entry)
	{
		stopTimer();
		return;
	}
	double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	playbackPos += (float)(now - lastTimerCall);
	lastTimerCall = now;
	if (playbackPos >= entry->duration)
	{
		playbackPos = entry->duration;
		setIsPlaying(false);
		if (onStopRequested)
			onStopRequested();
	}
	repaint();
}

void DetailPanel::updatePlayButton()
{
	if (isPlaying)
	{
		playButton.setToggleState(true, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonOnColourId, ColourPalette::mossGreen);
	}
	else
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::slate);
	}
	playButton.repaint();
}

juce::String DetailPanel::formatDuration(float s)
{
	int m = (int)(s / 60);
	int sc = (int)s % 60;
	int ms = (int)((s - (int)s) * 100);
	return m > 0 ? juce::String::formatted("%d:%02d.%02d", m, sc, ms) : juce::String::formatted("%d.%02ds", sc, ms);
}

juce::Colour DetailPanel::getCategoryColor(const juce::String &category)
{
	static const std::map<juce::String, juce::Colour> colors = {{"Drums", ColourPalette::indigo},
	                                                            {"Bass", ColourPalette::teal},
	                                                            {"Melody", ColourPalette::coral},
	                                                            {"Ambient", ColourPalette::emerald},
	                                                            {"Percussion", ColourPalette::slate},
	                                                            {"Vocal", ColourPalette::amber},
	                                                            {"FX", ColourPalette::backgroundLight},
	                                                            {"Loops", ColourPalette::buttonSuccess},
	                                                            {"One-shots", ColourPalette::buttonSecondary},
	                                                            {"House", ColourPalette::buttonDangerDark},
	                                                            {"Techno", ColourPalette::lime},
	                                                            {"Hip-Hop", ColourPalette::violet},
	                                                            {"Jazz", ColourPalette::amber},
	                                                            {"Rock", ColourPalette::buttonDanger},
	                                                            {"Electronic", ColourPalette::cyan},
	                                                            {"Piano", ColourPalette::textSecondary},
	                                                            {"Guitar", ColourPalette::textWarning},
	                                                            {"Synth", ColourPalette::textSecondary}};
	auto it = colors.find(category);
	return it != colors.end() ? it->second : ColourPalette::backgroundLight;
}