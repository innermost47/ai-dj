#include "DetailPanel.h"

DetailPanel::DetailPanel()
{
	addAndMakeVisible(nameLabel);
	nameLabel.setColour(juce::Label::textColourId, ColourPalette::textPrimary);
	nameLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR));

	addAndMakeVisible(metaLabel);
	metaLabel.setColour(juce::Label::textColourId, ColourPalette::textSecondary);
	metaLabel.setFont(juce::FontOptions(ObsidianSizes::TEXT_INFO));

	addAndMakeVisible(playButton);
	playButton.loadIcon(BinaryData::play_svg, BinaryData::play_svgSize);
	playButton.loadIconToggled(BinaryData::square_svg, BinaryData::square_svgSize);
	playButton.setHasToggledIcon(true);
	playButton.setClickingTogglesState(false);
	playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
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

	addAndMakeVisible(deleteButton);
	deleteButton.loadIcon(BinaryData::x_svg, BinaryData::x_svgSize);
	deleteButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	deleteButton.onClick = [this]()
	{
		if (entry && onDeleteRequested)
			onDeleteRequested(entry->id);
	};
	playButton.setCompactMode(true);
	deleteButton.setCompactMode(true);
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
		return;
	}

	juce::String nameText = entry->originalPrompt;
	if (entry->modelName.isNotEmpty())
		nameText += " - " + entry->modelName;
	nameLabel.setText(nameText, juce::dontSendNotification);

	juce::StringArray parts;
	parts.add(formatDuration(entry->duration));
	parts.add(juce::String(entry->bpm, 1) + " BPM");

	if (entry->usedInProjects.empty())
		parts.add("Unused");
	else
		parts.add(juce::String((int)entry->usedInProjects.size()) + " project(s)");

	if (!entry->categories.empty())
		parts.add("[" + entry->categories[0] + "]");

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
	int w = waveformBounds.getWidth() - 4;
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

	if (entry && !entry->categories.empty())
	{
		float cy = 6.0f + 18.0f * 0.5f;
		juce::Colour col = this->categoryColourResolver ? this->categoryColourResolver(entry->categories[0])
		                                                : getCategoryColor(entry->categories[0]);
		g.setColour(col);
		g.fillEllipse(8.0f, cy - 4.0f, 8.0f, 8.0f);
	}

	if (entry)
	{
		auto nameArea = juce::Rectangle<int>(20, 6, getWidth() - 80, 18);
		g.setColour(ColourPalette::textPrimary);
		g.setFont(juce::FontOptions(ObsidianSizes::TEXT_REGULAR, juce::Font::bold));
		g.drawText(entry->originalPrompt, nameArea, juce::Justification::centredLeft, true);
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

	g.setColour(ColourPalette::backgroundDeep.withAlpha(ObsidianShades::ALPHA_08));
	g.fillRect(waveformBounds);

	auto innerBounds = waveformBounds.reduced(6, 4);

	size_t sz = std::min(thumbL.size(), thumbR.size());
	float ppx = (float)innerBounds.getWidth() / (float)sz;
	bool stereo = (thumbR.size() == thumbL.size() && audioBuf.getNumChannels() > 1);

	auto drawChannel = [&](const std::vector<float> &thumb, float centerY, float halfH)
	{
		juce::Path top, bot;
		bool ts = false, bs = false;
		for (size_t i = 0; i < sz; ++i)
		{
			float x = innerBounds.getX() + i * ppx;
			float h = thumb[i] * halfH;
			if (!ts)
			{
				top.startNewSubPath(x, centerY);
				ts = true;
			}
			if (!bs)
			{
				bot.startNewSubPath(x, centerY);
				bs = true;
			}
			top.lineTo(x, centerY - h);
			bot.lineTo(x, centerY + h);
		}
		g.setColour(ColourPalette::textPrimary.withAlpha(0.8f));
		g.strokePath(top, juce::PathStrokeType(1.0f));
		g.strokePath(bot, juce::PathStrokeType(1.0f));
	};

	if (stereo)
	{
		float qY = innerBounds.getY() + innerBounds.getHeight() * 0.25f;
		float tqY = innerBounds.getY() + innerBounds.getHeight() * 0.75f;
		float hH = innerBounds.getHeight() * 0.22f;
		drawChannel(thumbL, qY, hH);
		drawChannel(thumbR, tqY, hH);
		g.setColour(ColourPalette::backgroundLight.withAlpha(0.15f));
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
		g.setColour(ColourPalette::playArmed);
		g.drawLine(hx, (float)innerBounds.getY(), hx, (float)innerBounds.getBottom(), 2.0f);
	}

	g.restoreState();

	g.setColour(ColourPalette::backgroundLight.withAlpha(0.3f));
	g.drawRoundedRectangle(waveformBounds.toFloat(), ObsidianSizes::HALF_CORNER, 0.5f);
}

void DetailPanel::resized()
{
	auto area = getLocalBounds().reduced(6, 4);
	auto titleRow = area.removeFromTop(24);
	auto bottomRow = area;

	auto btnCol = bottomRow.removeFromRight(36);
	bottomRow.removeFromRight(4);

	playButton.setBounds(btnCol.removeFromTop(bottomRow.getHeight() / 2).reduced(2));
	deleteButton.setBounds(btnCol.reduced(2));

	waveformBounds = bottomRow.reduced(2);

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
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::buttonDangerDark);
	}
	else
	{
		playButton.setToggleState(false, juce::dontSendNotification);
		playButton.setColour(juce::TextButton::buttonColourId, ColourPalette::mossGreen);
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