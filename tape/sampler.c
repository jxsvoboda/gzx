/*
 * GZX - George's ZX Spectrum Emulator
 * Tape sampler
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
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
 * @file Tape sampler.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "sampler.h"
#include "tap.h"
#include "tape.h"
#include "tzx.h"
#include "wav.h"
#include "../strutil.h"

/** Create tape sampler.
 *
 * @param player Tape player
 * @param smp_dur Sample duration in Z80 clock ticks
 * @param rsampler Place to store pointer to new tape sampler
 * @return Zero on success, ENOMEM if out of memory
 */
int tape_sampler_create(tape_player_t *player, uint32_t smp_dur,
    tape_sampler_t **rsampler)
{
	tape_sampler_t *sampler;

	sampler = calloc(1, sizeof(tape_sampler_t));
	if (sampler == NULL)
		return ENOMEM;

	sampler->player = player;
	sampler->delta_t = smp_dur;
	*rsampler = sampler;
	return 0;
}

/** Initialize sampler at beginning of playback or after a position change.
 *
 * @param sampler Sampler
 * @param block Block where to start playing
 */
void tape_sampler_init(tape_sampler_t *sampler, tape_block_t *block)
{
	tape_player_sig_t sig;

	tape_player_init(sampler->player, block);

	sampler->cur_lvl = tape_player_cur_lvl(sampler->player);
	if (!tape_player_is_end(sampler->player)) {
		tape_player_get_next(sampler->player, &sampler->next_delay,
		    &sampler->next_lvl, &sig);
		(void)sig;
	}
}

/** Destroy tape sampler.
 *
 * @param sampler Tape sampler
 */
void tape_sampler_destroy(tape_sampler_t *sampler)
{
	free(sampler);
}

/** Determine if sampler is at the end of tape.
 *
 * @param sampler Sampler
 * @return @c true iff player is at the end of tape
 */
bool tape_sampler_is_end(tape_sampler_t *sampler)
{
	return tape_player_is_end(sampler->player) &&
	    sampler->next_delay < sampler->delta_t;
}

/** Return currently playing block.
 *
 * @param player Sampler
 * @return Currently playing block
 */
tape_block_t *tape_sampler_cur_block(tape_sampler_t *sampler)
{
	return tape_player_cur_block(sampler->player);
}

/** Get next sample.
 *
 * @param sampler Tape sampler
 * @param smp Place to store sample
 * @parma sig Place to store signal
 */
void tape_sampler_getsmp(tape_sampler_t *sampler, uint8_t *smp,
    tape_player_sig_t *sig)
{
	uint32_t td;

	td = sampler->delta_t;
	*sig = tps_none;
	while (sampler->next_delay <= td &&
	    !tape_player_is_end(sampler->player) && *sig == tps_none) {
		td -= sampler->next_delay;
		sampler->cur_lvl = sampler->next_lvl;
		tape_player_get_next(sampler->player, &sampler->next_delay,
		    &sampler->next_lvl, sig);
	}

	if (sampler->next_delay > td)
		sampler->next_delay -= td;

	*smp = sampler->cur_lvl;
}
