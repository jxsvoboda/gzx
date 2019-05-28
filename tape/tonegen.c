/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Tape tone generator.
 *
 * Produce waveform based on a description, consiting of a series of
 * tones.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tape.h"
#include "tonegen.h"

/** Initialize tone generator.
 *
 * @param tgen Tone generator
 * @param lvl Initial signal level
 */
void tonegen_init(tonegen_t *tgen, tape_lvl_t lvl)
{
	int i;

	tgen->num_tones = 0;
	for (i = 0; i < tonegen_max_tones; i++) {
		tgen->pulse_len[i] = 0;
		tgen->num_pulses[i] = 0;
	}

	tgen->tidx = 0;
	tgen->cur_lvl = lvl;
	tgen->rem_pulses = 0;
	tgen->cur_pulse_len = 0;
}

/** Program one tone into the tone generator.
 *
 * @param tgen Tone generator
 * @param pulse_len Pulse length
 * @param num_pulses Number of pulses
 */
void tonegen_add_tone(tonegen_t *tgen, unsigned pulse_len,
    unsigned num_pulses)
{
	assert(tgen->num_tones < tonegen_max_tones);

	tgen->pulse_len[tgen->num_tones] = pulse_len;
	tgen->num_pulses[tgen->num_tones] = num_pulses;
	++tgen->num_tones;
}

/** Determine if tone generator is at the end.
 *
 * @param tgen Tone generator
 * @return @c true iff player is at the end of tape
 */
bool tonegen_is_end(tonegen_t *tgen)
{
	return tgen->rem_pulses == 0 && tgen->tidx == tgen->num_tones;
}

/** Determine current tape player signal level.
 *
 * @param player Tape player
 * @return Current signal level.
 */
tape_lvl_t tonegen_cur_lvl(tonegen_t *tgen)
{
	return tgen->cur_lvl;
}

/** Get next possible tone generator waveform event.
 *
 * @param tgen Tone generator
 * @param rdelay Place to store delay until next event
 * @param rlvl Place to store next signal level
 */
void tonegen_get_next(tonegen_t *tgen, uint32_t *rdelay, tape_lvl_t *rlvl)
{
	/* Advance tones until we get one with non-zero number of pulses */
	while (tgen->rem_pulses == 0) {
		assert(tgen->tidx < tgen->num_tones);

		tgen->rem_pulses = tgen->num_pulses[tgen->tidx];
		tgen->cur_pulse_len = tgen->pulse_len[tgen->tidx];
		++tgen->tidx;
	}

	*rdelay = tgen->cur_pulse_len;
	*rlvl = !tgen->cur_lvl;

	tgen->cur_lvl = !tgen->cur_lvl;
	--tgen->rem_pulses;
}
