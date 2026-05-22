#pragma once

/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    MiniBPM
    A fixed-tempo BPM estimator for music audio
    Copyright 2012-2025 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for MiniBPM
    obtained by agreement with the copyright holders, you may
    redistribute and/or modify it under the terms described in that
    licence.

    If you wish to distribute code using MiniBPM under terms other
    than those of the GNU General Public License, you must obtain a
    valid commercial licence before doing so.
*/

#ifndef BQ_MINI_BPM_H
#define BQ_MINI_BPM_H

#include <vector>

namespace breakfastquay
{

/**
 * A fixed-tempo BPM estimator, self-contained and implemented in a
 * single C++ file.
 *
 * This may be used in two ways. After constructing a MiniBPM object,
 * you can call estimateTempoOfSamples() on it with a single in-memory
 * buffer of all audio samples. Or, if the input data is streamed or
 * cannot fit in memory, call process() repeatedly with consecutive
 * sample blocks of any size, followed by estimateTempo() when all
 * samples have been supplied.
 *
 * A single channel of audio only may be supplied (multi-channel is
 * not supported). To process multi-channel audio, average the
 * channels first.
 */
class MiniBPM
{
  public:
	/**
	 * Construct a MiniBPM object to process audio at the given sample
	 * rate.
	 */
	MiniBPM(float sampleRate);
	~MiniBPM();

	/**
	 * Set the range of valid tempi. The default is 55-190bpm.
	 */
	void setBPMRange(double min, double max);

	/**
	 * Get the current range of valid tempi.
	 */
	void getBPMRange(double &min, double &max) const;

	/**
	 * Set the number of beats per bar, if known. If unknown, leave at
	 * the default (which is 4).
	 */
	void setBeatsPerBar(int bpb);

	/**
	 * Get the current number of beats per bar. (This simply returns
	 * the value set by setBeatsPerBar, which is a hint to the tempo
	 * estimator: the estimator does not estimate meter.)
	 */
	int getBeatsPerBar() const;

	/**
	 * Return the estimated tempo in bpm of the music audio in the
	 * given sequence of samples. nsamples contains the number of
	 * samples. If the tempo cannot be estimated because the clip is
	 * too short, return 0.
	 *
	 * You should use either this function, or a series of process()
	 * calls followed by an estimateTempo() call. Do not call both
	 * process() and estimateTempoOfSamples() on the same estimator
	 * object.
	 *
	 * If you wish to subsequently obtain a tempo estimate of a
	 * different clip, you must call reset() before calling this
	 * function again. (This is so that the data that is available to
	 * be returned from getTempoCandidates can be cleared.)
	 */
	double estimateTempoOfSamples(const float *samples, int nsamples);

	/**
	 * Supply a single block of audio for processing. The block may be
	 * of any length. Blocks are assumed to be contiguous
	 * (i.e. without overlap).
	 */
	void process(const float *samples, int nsamples);

	/**
	 * Return the estimated tempo in bpm of the music audio in the
	 * sequence of samples previously supplied through a series of
	 * calls to process(). If the tempo cannot be estimated because
	 * the clip is too short, return 0.
	 */
	double estimateTempo();

	/**
	 * Return all candidate tempi for the last tempo estimation that
	 * was carried out, in order of likelihood (best first). The value
	 * returned from estimateTempo or estimateTempoOfSamples will
	 * therefore be the first in the returned sequence. Calling
	 * reset() will clear this information.
	 */
	std::vector<double> getTempoCandidates() const;

	/**
	 * Prepare the object to carry out another tempo estimation on a
	 * new audio clip. You can either call this between uses, or
	 * simply destroy this object and construct a new one.
	 */
	void reset();

  private:
	class D;
	D *m_d;

	MiniBPM(const MiniBPM &);            // not provided
	MiniBPM &operator=(const MiniBPM &); // not provided
};

/**
 * @mainpage
 * A fixed-tempo BPM estimator, self-contained and implemented in a
 * single C++ file. The entire API is defined by the MiniBPM class.
 */

} // namespace breakfastquay

#endif