/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator types
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
 * @file Tape tone generator types
 */

#ifndef TYPES_TAPE_TONEGEN_H
#define TYPES_TAPE_TONEGEN_H

#include <stdint.h>
#include "tape.h"

enum {
	tonegen_max_tones = 11
};

/** Tone generator */
typedef struct {
	/** Number of programmed tones */
	int num_tones;
	/** Pulse length for each tone */
	uint32_t pulse_len[tonegen_max_tones];
	/** Number of pulses for each tone */
	unsigned num_pulses[tonegen_max_tones];
	/** Level is directly specified for this tone/pulse */
	bool direct[tonegen_max_tones];
	/** Direct level (if direct[] is true) */
	tape_lvl_t dlvl[tonegen_max_tones];
	/** Last programmed level */
	tape_lvl_t plast_lvl;
	/** Previous programmed level just before last programmed instant */
	tape_lvl_t pprev_lvl;

	/** Next tone index */
	int tidx;
	/** Current level */
	tape_lvl_t cur_lvl;
	/** Pulse length for current tone */
	uint32_t cur_pulse_len;
	/** Remaining number of pulses in current tone */
	unsigned rem_pulses;
	/** Current pulse is direct */
	bool cur_is_direct;
} tonegen_t;

#endif
