/*
 * GZX - George's ZX Spectrum Emulator
 * Tape player
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
 * @file Tape player.
 *
 * Produce waveform from spectrum tape.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tape.h"
#include "tonegen.h"
#include "player.h"

static void tape_player_next(tape_player_t *);
static void tape_player_tone_init(tape_player_t *, tblock_tone_t *);
static void tape_player_tone_next(tape_player_t *, tblock_tone_t *);
static void tape_player_pulses_init(tape_player_t *, tblock_pulses_t *);
static void tape_player_pulses_next(tape_player_t *, tblock_pulses_t *);

/** Create tape player.
 *
 * @param block Tape block to start playing at
 * @param rplayer Place to store pointer to new tape player
 * @return EOK on success, ENOMEM if out of memory
 */
int tape_player_create(tape_block_t *block, tape_player_t **rplayer)
{
	tape_player_t *player;

	player = calloc(1, sizeof(tape_player_t));
	if (player == NULL)
		return ENOMEM;

	tonegen_init(&player->tgen, tlvl_low);
	player->cur_block = NULL;
	player->next_block = block;

	*rplayer = player;
	return 0;
}

/** Destroy tape player.
 *
 * @param player Tape player
 */
void tape_player_destroy(tape_player_t *player)
{
	free(player);
}

/** Determine if player is at the end of tape.
 *
 * @param player Tape player
 * @return @c true iff player is at the end of tape
 */
bool tape_player_is_end(tape_player_t *player)
{
	tape_player_next(player);
	return tonegen_is_end(&player->tgen);
}

/** Determine current tape player signal level.
 *
 * @param player Tape player
 * @return Current signal level.
 */
tape_lvl_t tape_player_cur_lvl(tape_player_t *player)
{
	return tonegen_cur_lvl(&player->tgen);
}

/** Make sure tone generator is programmed with next tones.
 *
 * If there is nothing more to program, do nothing.
 *
 * @param player Tape player
 * @param rdelay Place to store delay until next event
 * @param rlvl Place to store next signal level
 */
static void tape_player_next(tape_player_t *player)
{
	while (true) {
		if (!tonegen_is_end(&player->tgen))
			break;

		if (player->cur_block == NULL) {
			player->cur_block = player->next_block;
			player->next_block = NULL;

			if (player->cur_block == NULL)
				break;

			switch (player->cur_block->btype) {
			case tb_tone:
				tape_player_tone_init(player,
				    (tblock_tone_t *) player->cur_block->ext);
				break;
			case tb_pulses:
				tape_player_pulses_init(player,
				    (tblock_pulses_t *) player->cur_block->ext);
				break;
			default:
				break;
			}
		}

		switch (player->cur_block->btype) {
		case tb_tone:
			tape_player_tone_next(player,
			    (tblock_tone_t *) player->cur_block->ext);
			break;
		case tb_pulses:
			tape_player_pulses_next(player,
			    (tblock_pulses_t *) player->cur_block->ext);
			break;
		default:
			break;
		}
	}
}

/** Get next possible tape player waveform event.
 *
 * @param player Tape player
 * @param rdelay Place to store delay until next event
 * @param rlvl Place to store next signal level
 */
void tape_player_get_next(tape_player_t *player, uint32_t *rdelay,
    tape_lvl_t *rlvl)
{
	tape_player_next(player);
	tonegen_get_next(&player->tgen, rdelay, rlvl);
}

/** End processing current block.
 *
 * @param player Tape player
 */
static void tape_player_end_block(tape_player_t *player)
{
	player->next_block = tape_next(player->cur_block);
	player->cur_block = NULL;
}

/** Initialize playback of tone block.
 *
 * @param player Tape player
 * @param tone Tone block
 */
static void tape_player_tone_init(tape_player_t *player, tblock_tone_t *tone)
{
	tonegen_init(&player->tgen, tonegen_cur_lvl(&player->tgen));
	tonegen_add_tone(&player->tgen, tone->pulse_len, tone->num_pulses);
}

/** Next step in playback of tone block.
 *
 * @param player Tape player
 * @param tone Tone block
 */
static void tape_player_tone_next(tape_player_t *player, tblock_tone_t *tone)
{
	tape_player_end_block(player);
}

/** Initialize playback of pulse sequence block.
 *
 * @param player Tape player
 * @param pulses Pulse sequence block
 */
static void tape_player_pulses_init(tape_player_t *player,
    tblock_pulses_t *pulses)
{
	tonegen_init(&player->tgen, tonegen_cur_lvl(&player->tgen));
	player->cur_idx = 0;

	tape_player_pulses_next(player, pulses);
}

/** Next step in playback of pulse sequence block.
 *
 * @param player Tape player
 * @param pulses Pulse sequence block
 */
static void tape_player_pulses_next(tape_player_t *player,
    tblock_pulses_t *pulses)
{
	tonegen_add_tone(&player->tgen, pulses->pulse_len[player->cur_idx], 1);
	++player->cur_idx;

	if (player->cur_idx >= pulses->num_pulses) {
		tape_player_end_block(player);
	}
}
