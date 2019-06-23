/*
 * GZX - George's ZX Spectrum Emulator
 * Tape deck types
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
 * @file Tape deck types
 */

#ifndef TYPES_TAPE_DECK_H
#define TYPES_TAPE_DECK_H

#include <stdbool.h>
#include <stdint.h>
#include "player.h"
#include "tape.h"

/** Tape deck */
typedef struct {
	/** Tape */
	tape_t *tape;
	/** Tape file name or @c NULL */
	char *fname;
	/** Current tape block (if not playing) */
	tape_block_t *cur_block;
	/** Tape player */
	tape_player_t *player;

	/** Delay between tape samples */
	int delta_t;
	/** Tape is playing */
	bool playing;
	/** Tape is paused */
	bool paused;
	/** Mode is 48K */
	bool mode48k;

	/** Current level */
	tape_lvl_t cur_lvl;
	/** Delay until next event */
	uint32_t next_delay;
	/** Next level */
	tape_lvl_t next_lvl;
} tape_deck_t;

#endif
