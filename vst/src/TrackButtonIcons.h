#pragma once
#include <JuceHeader.h>

class TrackButtonIcons
{
public:
	static juce::Path draw()
	{
		juce::Path p;
		p.startNewSubPath(17.0f, 3.0f);
		p.lineTo(21.0f, 7.0f);
		p.lineTo(8.0f, 20.0f);
		p.lineTo(3.0f, 21.0f);
		p.lineTo(4.0f, 16.0f);
		p.lineTo(17.0f, 3.0f);
		p.closeSubPath();
		p.startNewSubPath(14.0f, 6.0f);
		p.lineTo(18.0f, 10.0f);
		return p;
	}

	static juce::Path generate()
	{
		juce::Path p;
		const float cx = 12.0f, cy = 12.0f;
		const float longArm = 9.0f;
		const float shortArm = 7.0f;
		const float innerRadius = 2.2f;

		p.startNewSubPath(cx, cy - longArm);
		p.lineTo(cx + innerRadius, cy - innerRadius);
		p.lineTo(cx + shortArm, cy);
		p.lineTo(cx + innerRadius, cy + innerRadius);
		p.lineTo(cx, cy + longArm);
		p.lineTo(cx - innerRadius, cy + innerRadius);
		p.lineTo(cx - shortArm, cy);
		p.lineTo(cx - innerRadius, cy - innerRadius);
		p.closeSubPath();

		return p;
	}

	static juce::Path sync()
	{
		juce::Path p;
		p.startNewSubPath(4.0f, 10.0f);
		p.cubicTo(4.0f, 5.0f, 9.0f, 3.0f, 14.0f, 5.0f);
		p.startNewSubPath(11.0f, 3.0f);
		p.lineTo(14.5f, 5.0f);
		p.lineTo(12.5f, 7.5f);
		p.startNewSubPath(20.0f, 14.0f);
		p.cubicTo(20.0f, 19.0f, 15.0f, 21.0f, 10.0f, 19.0f);
		p.startNewSubPath(13.0f, 21.0f);
		p.lineTo(9.5f, 19.0f);
		p.lineTo(11.5f, 16.5f);
		return p;
	}

	static juce::Path play()
	{
		juce::Path p;
		p.startNewSubPath(7.0f, 4.0f);
		p.lineTo(20.0f, 12.0f);
		p.lineTo(7.0f, 20.0f);
		p.closeSubPath();
		return p;
	}

	static juce::Path stop()
	{
		juce::Path p;
		p.addRoundedRectangle(5.0f, 5.0f, 14.0f, 14.0f, 1.5f);
		return p;
	}

	static juce::Path waveform()
	{
		juce::Path p;
		auto bar = [&p](float x, float h) {
			float centerY = 12.0f;
			p.addRoundedRectangle(x - 1.25f, centerY - h * 0.5f, 2.5f, h, 1.0f);
			};
		bar(3.5f, 6.0f);
		bar(8.0f, 14.0f);
		bar(12.0f, 9.0f);
		bar(16.0f, 16.0f);
		bar(20.5f, 7.0f);
		return p;
	}

	static juce::Path sequencer()
	{
		juce::Path p;
		const float size = 3.5f;
		const float gap = 1.2f;
		const float startX = 3.5f;
		const float startY = 7.0f;
		for (int row = 0; row < 2; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				float x = startX + col * (size + gap);
				float y = startY + row * (size + gap);
				p.addRoundedRectangle(x, y, size, size, 0.6f);
			}
		}
		return p;
	}

	static juce::Path repeat()
	{
		juce::Path p;
		p.startNewSubPath(5.0f, 8.0f);
		p.lineTo(17.0f, 8.0f);
		p.cubicTo(20.0f, 8.0f, 20.0f, 16.0f, 17.0f, 16.0f);
		p.lineTo(7.0f, 16.0f);
		p.cubicTo(4.0f, 16.0f, 4.0f, 8.0f, 7.0f, 8.0f);
		p.startNewSubPath(15.0f, 5.5f);
		p.lineTo(17.5f, 8.0f);
		p.lineTo(15.0f, 10.5f);
		p.startNewSubPath(9.0f, 13.5f);
		p.lineTo(6.5f, 16.0f);
		p.lineTo(9.0f, 18.5f);
		return p;
	}

	static juce::Path random()
	{
		juce::Path p;
		p.addRoundedRectangle(4.0f, 4.0f, 16.0f, 16.0f, 2.5f);
		auto dot = [&p](float x, float y) {
			p.addEllipse(x - 1.0f, y - 1.0f, 2.0f, 2.0f);
			};
		dot(8.0f, 8.0f);
		dot(16.0f, 8.0f);
		dot(12.0f, 12.0f);
		dot(8.0f, 16.0f);
		dot(16.0f, 16.0f);
		return p;
	}

	static juce::Path trash()
	{
		juce::Path p;
		p.startNewSubPath(3.5f, 6.0f);
		p.lineTo(20.5f, 6.0f);
		p.startNewSubPath(9.0f, 6.0f);
		p.lineTo(9.0f, 3.5f);
		p.lineTo(15.0f, 3.5f);
		p.lineTo(15.0f, 6.0f);
		p.startNewSubPath(5.5f, 6.0f);
		p.lineTo(6.5f, 20.5f);
		p.cubicTo(6.5f, 21.0f, 7.0f, 21.0f, 7.5f, 21.0f);
		p.lineTo(16.5f, 21.0f);
		p.cubicTo(17.0f, 21.0f, 17.5f, 21.0f, 17.5f, 20.5f);
		p.lineTo(18.5f, 6.0f);
		p.startNewSubPath(10.0f, 10.0f);
		p.lineTo(10.0f, 17.0f);
		p.startNewSubPath(14.0f, 10.0f);
		p.lineTo(14.0f, 17.0f);
		return p;
	}
};

