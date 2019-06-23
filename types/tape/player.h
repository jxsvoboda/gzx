/*
 * GZX - George's ZX Spectrum Emulator
 * Tape player types
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
 * @file Tape player types
 */

#ifndef TYPES_TAPE_PLAYER_H
#define TYPES_TAPE_PLAYER_H

#include <stdint.h>
#include <stdio.h>
#include "tape.h"
#include "tonegen.h"

/** Tape player output signal */
typedef enum {
	/** No signal */
	tps_none,
	/** Stop the tape */
	tps_stop,
	/** Stop the tape if in 48K mode */
	tps_stop_48k
} tape_player_sig_t;

/** Tape player */
typedef struct {
	/** Current tape block */
	tape_block_t *cur_block;
	/** Index in current block. Exact meaning depends on block type */
	uint32_t cur_idx;
	/** Done programming pause */
	bool pause_done;
	/** Loop counter */
	uint16_t loop_cnt;

	/** Output signal */
	tape_player_sig_t sig;

	/** Next tape block */
	tape_block_t *next_block;

	/** Tone generator */
	tonegen_t tgen;
} tape_player_t;

#endif
