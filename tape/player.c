/*
 * GZX - George's ZX Spectrum Emulator
 * Tape player
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
 * @file Tape player.
 *
 * Produce waveform from spectrum tape.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "defs.h"
#include "tape.h"
#include "tonegen.h"
#include "player.h"

static void tape_player_next(tape_player_t *);
static void tape_player_end_block(tape_player_t *);
static void tape_player_data_init(tape_player_t *, tblock_data_t *);
static void tape_player_data_next(tape_player_t *, tblock_data_t *);
static void tape_player_turbo_data_init(tape_player_t *, tblock_turbo_data_t *);
static void tape_player_turbo_data_next(tape_player_t *, tblock_turbo_data_t *);
static void tape_player_tone_init(tape_player_t *, tblock_tone_t *);
static void tape_player_tone_next(tape_player_t *, tblock_tone_t *);
static void tape_player_pulses_init(tape_player_t *, tblock_pulses_t *);
static void tape_player_pulses_next(tape_player_t *, tblock_pulses_t *);
static void tape_player_pure_data_init(tape_player_t *, tblock_pure_data_t *);
static void tape_player_pure_data_next(tape_player_t *, tblock_pure_data_t *);
static void tape_player_direct_rec_init(tape_player_t *, tblock_direct_rec_t *);
static void tape_player_direct_rec_next(tape_player_t *, tblock_direct_rec_t *);
static void tape_player_pause_init(tape_player_t *, tblock_pause_t *);
static void tape_player_pause_next(tape_player_t *, tblock_pause_t *);
static void tape_player_stop_init(tape_player_t *, tblock_stop_t *);
static void tape_player_stop_next(tape_player_t *, tblock_stop_t *);
static void tape_player_loop_start_init(tape_player_t *, tblock_loop_start_t *);
static void tape_player_loop_start_next(tape_player_t *, tblock_loop_start_t *);
static void tape_player_loop_end_init(tape_player_t *, tblock_loop_end_t *);
static void tape_player_loop_end_next(tape_player_t *, tblock_loop_end_t *);
static void tape_player_stop_48k_init(tape_player_t *, tblock_stop_48k_t *);
static void tape_player_stop_48k_next(tape_player_t *, tblock_stop_48k_t *);

/** Create tape player.
 *
 * @param rplayer Place to store pointer to new tape player
 * @return Zero on success, ENOMEM if out of memory
 */
int tape_player_create(tape_player_t **rplayer)
{
	tape_player_t *player;

	player = calloc(1, sizeof(tape_player_t));
	if (player == NULL)
		return ENOMEM;

	player->cur_block = NULL;
	player->next_block = NULL;

	*rplayer = player;
	return 0;
}

/** Initialize tape player.
 *
 * @param player Tape player
 * @param block Tape block to start playing at
 * @return EOK on success, ENOMEM if out of memory
 */
void tape_player_init(tape_player_t *player, tape_block_t *block)
{
	tonegen_init(&player->tgen, tlvl_low);
	player->cur_block = NULL;
	player->cur_idx = 0;
	player->pause_done = false;
	player->next_block = block;
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
	return tonegen_is_end(&player->tgen) && player->sig == tps_none;
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

/** Return currently playing block.
 *
 * @param player Tape player
 * @return Currently playing block
 */
tape_block_t *tape_player_cur_block(tape_player_t *player)
{
	if (player->cur_block != NULL)
		return player->cur_block;

	return player->next_block;
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
	while (tonegen_is_end(&player->tgen) && player->sig == tps_none) {

		while (player->cur_block == NULL) {
			player->cur_block = player->next_block;
			player->next_block = NULL;

			if (player->cur_block == NULL)
				break;

			switch (player->cur_block->btype) {
			case tb_data:
				tape_player_data_init(player,
				    (tblock_data_t *) player->cur_block->ext);
				break;
			case tb_turbo_data:
				tape_player_turbo_data_init(player,
				    (tblock_turbo_data_t *)
				    player->cur_block->ext);
				break;
			case tb_tone:
				tape_player_tone_init(player,
				    (tblock_tone_t *) player->cur_block->ext);
				break;
			case tb_pulses:
				tape_player_pulses_init(player,
				    (tblock_pulses_t *) player->cur_block->ext);
				break;
			case tb_pure_data:
				tape_player_pure_data_init(player,
				    (tblock_pure_data_t *)
				    player->cur_block->ext);
				break;
			case tb_direct_rec:
				tape_player_direct_rec_init(player,
				    (tblock_direct_rec_t *)
				    player->cur_block->ext);
				break;
			case tb_pause:
				tape_player_pause_init(player,
				    (tblock_pause_t *)
				    player->cur_block->ext);
				break;
			case tb_stop:
				tape_player_stop_init(player,
				    (tblock_stop_t *)
				    player->cur_block->ext);
				break;
			case tb_loop_start:
				tape_player_loop_start_init(player,
				    (tblock_loop_start_t *)
				    player->cur_block->ext);
				break;
			case tb_loop_end:
				tape_player_loop_end_init(player,
				    (tblock_loop_end_t *)
				    player->cur_block->ext);
				break;
			case tb_stop_48k:
				tape_player_stop_48k_init(player,
				    (tblock_stop_48k_t *)
				    player->cur_block->ext);
				break;
			default:
				/* Skip other blocks */
				tape_player_end_block(player);
				break;
			}
		}

		if (player->cur_block == NULL)
			break;

		switch (player->cur_block->btype) {
		case tb_data:
			tape_player_data_next(player,
			    (tblock_data_t *) player->cur_block->ext);
			break;
		case tb_turbo_data:
			tape_player_turbo_data_next(player,
			    (tblock_turbo_data_t *) player->cur_block->ext);
			break;
		case tb_tone:
			tape_player_tone_next(player,
			    (tblock_tone_t *) player->cur_block->ext);
			break;
		case tb_pulses:
			tape_player_pulses_next(player,
			    (tblock_pulses_t *) player->cur_block->ext);
			break;
		case tb_pure_data:
			tape_player_pure_data_next(player,
			    (tblock_pure_data_t *) player->cur_block->ext);
			break;
		case tb_direct_rec:
			tape_player_direct_rec_next(player,
			    (tblock_direct_rec_t *) player->cur_block->ext);
			break;
		case tb_pause:
			tape_player_pause_next(player,
			    (tblock_pause_t *) player->cur_block->ext);
			break;
		case tb_stop:
			tape_player_stop_next(player,
			    (tblock_stop_t *) player->cur_block->ext);
			break;
		case tb_loop_start:
			tape_player_loop_start_next(player,
			    (tblock_loop_start_t *) player->cur_block->ext);
			break;
		case tb_loop_end:
			tape_player_loop_end_next(player,
			    (tblock_loop_end_t *) player->cur_block->ext);
			break;
		case tb_stop_48k:
			tape_player_stop_48k_next(player,
			    (tblock_stop_48k_t *) player->cur_block->ext);
			break;
		default:
			assert(false);
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
    tape_lvl_t *rlvl, tape_player_sig_t *rsig)
{
	tape_player_next(player);

	if (player->sig == tps_none) {
		tonegen_get_next(&player->tgen, rdelay, rlvl);
		*rsig = 0;
	} else {
		*rdelay = 0;
		*rlvl = tonegen_cur_lvl(&player->tgen);
		*rsig = player->sig;
		player->sig = tps_none;
	}
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

/** Program playback of up to 8 bits of data.
 *
 * @param player Tape player
 * @param b Byte containing the bits (starting with bit 7)
 * @param nb Number of bits to play
 * @param one_len Length of one pulse in T states
 * @param zero_len Length of zero pulse in T states
 */
static void tape_player_program_bits(tape_player_t *player, uint8_t b,
    int nb, uint16_t one_len, uint16_t zero_len)
{
	uint16_t plen;
	int i;

	for (i = 0; i < nb; i++) {
		plen = (b & (0x80 >> i)) != 0 ? one_len : zero_len;

		tonegen_add_tone(&player->tgen, plen, 2);
	}
}

/** Program playback of up to 8 bits of direct recording.
 *
 * @param player Tape player
 * @param b Byte containing the bits (starting with bit 7)
 * @param nb Number of bits to play
 * @param smp_dur Sample duration
 */
static void tape_player_program_dr_bits(tape_player_t *player, uint8_t b,
    int nb, uint16_t smp_dur)
{
	tape_lvl_t lvl;
	int i;

	for (i = 0; i < nb; i++) {
		lvl = (b & (0x80 >> i)) != 0 ? tlvl_high : tlvl_low;
		tonegen_add_dpulse(&player->tgen, lvl, smp_dur);
	}
}

/** Program playback of pause.
 *
 * - pause is always low level
 * - if and only if the pause follows a rising edge, start with 1 ms of high
 * - current level stays low at the end of pause (so the next pulse will not
 *   start with an edge)
 *
 * @param player Tape player
 * @param pause_len Pause length in ms
 */
static void tape_player_program_pause(tape_player_t *player,
    uint16_t pause_len)
{
	if (pause_len == 0)
		return;

	if (tonegen_pprev_lvl(&player->tgen) == tlvl_low &&
	    tonegen_plast_lvl(&player->tgen) == tlvl_high) {
		/* Just following a rising edge */
		tonegen_add_dpulse(&player->tgen, tlvl_high,
		    TAPE_PAUSE_MULT * 1);
		tonegen_add_dpulse(&player->tgen, tlvl_low,
		    TAPE_PAUSE_MULT * (pause_len - 1));
	} else {
		/* All other cases */
		tonegen_add_dpulse(&player->tgen, tlvl_low,
		    TAPE_PAUSE_MULT * pause_len);
	}
}

/** Initialize playback of standard speed data block.
 *
 * @param player Tape player
 * @param data Standard speed data block
 */
static void tape_player_data_init(tape_player_t *player, tblock_data_t *data)
{
	tonegen_clear(&player->tgen);

	/* Pilot tone */
	tonegen_add_tone(&player->tgen, ROM_PILOT_LEN,
	    data->data[0] == 0x00 ? ROM_PPULSES_H : ROM_PPULSES_D);

	/* Sync pulses */
	tonegen_add_tone(&player->tgen, ROM_SYNC1_LEN, 1);
	tonegen_add_tone(&player->tgen, ROM_SYNC2_LEN, 1);

	/* Index of next data byte to program */
	player->cur_idx = 0;
	player->pause_done = false;
}

/** Next step in playback of standard speed data block.
 *
 * @param player Tape player
 * @param data Standard speed data block
 */
static void tape_player_data_next(tape_player_t *player, tblock_data_t *data)
{
	if (!tonegen_is_end(&player->tgen))
		return;

	if (player->cur_idx < data->data_len) {
		tonegen_clear(&player->tgen);

		tape_player_program_bits(player, data->data[player->cur_idx],
		    8, ROM_ONE_LEN, ROM_ZERO_LEN);
		++player->cur_idx;

	} else if (!player->pause_done) {
		tonegen_clear(&player->tgen);

		tape_player_program_pause(player, data->pause_after);
		player->pause_done = true;
	} else {
		tape_player_end_block(player);
	}
}

/** Initialize playback of turbo speed data block.
 *
 * @param player Tape player
 * @param tdata Turbo speed data block
 */
static void tape_player_turbo_data_init(tape_player_t *player,
    tblock_turbo_data_t *tdata)
{
	tonegen_clear(&player->tgen);

	/* Pilot tone */
	tonegen_add_tone(&player->tgen, tdata->pilot_len, tdata->pilot_pulses);

	/* Sync pulses */
	tonegen_add_tone(&player->tgen, tdata->sync1_len, 1);
	tonegen_add_tone(&player->tgen, tdata->sync2_len, 1);

	/* Index of next data byte to program */
	player->cur_idx = 0;
	player->pause_done = false;
}

/** Next step in playback of turbo speed data block.
 *
 * @param player Tape player
 * @param tdata Turbo speed data block
 */
static void tape_player_turbo_data_next(tape_player_t *player,
    tblock_turbo_data_t *tdata)
{
	int nb;

	if (!tonegen_is_end(&player->tgen))
		return;

	if (player->cur_idx < tdata->data_len) {
		tonegen_clear(&player->tgen);

		nb = player->cur_idx < tdata->data_len - 1 ? 8 : tdata->lb_bits;
		tape_player_program_bits(player, tdata->data[player->cur_idx],
		    nb, tdata->one_len, tdata->zero_len);
		++player->cur_idx;

	} else if (!player->pause_done) {
		tonegen_clear(&player->tgen);

		tape_player_program_pause(player, tdata->pause_after);
		player->pause_done = true;
	} else {
		tape_player_end_block(player);
	}
}

/** Initialize playback of tone block.
 *
 * @param player Tape player
 * @param tone Tone block
 */
static void tape_player_tone_init(tape_player_t *player, tblock_tone_t *tone)
{
	tonegen_clear(&player->tgen);
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
	if (!tonegen_is_end(&player->tgen))
		return;

	tonegen_clear(&player->tgen);
	tonegen_add_tone(&player->tgen, pulses->pulse_len[player->cur_idx], 1);
	++player->cur_idx;

	if (player->cur_idx >= pulses->num_pulses) {
		tape_player_end_block(player);
	}
}

/** Initialize playback of pure data block.
 *
 * @param player Tape player
 * @param pdata Pure data block
 */
static void tape_player_pure_data_init(tape_player_t *player,
    tblock_pure_data_t *pdata)
{
	tonegen_clear(&player->tgen);

	/* Index of next data byte to program */
	player->cur_idx = 0;
	player->pause_done = false;
}

/** Next step in playback of pure data block.
 *
 * @param player Tape player
 * @param pdata Pure data block
 */
static void tape_player_pure_data_next(tape_player_t *player,
    tblock_pure_data_t *pdata)
{
	int nb;

	if (!tonegen_is_end(&player->tgen))
		return;

	if (player->cur_idx < pdata->data_len) {
		tonegen_clear(&player->tgen);

		nb = player->cur_idx < pdata->data_len - 1 ? 8 : pdata->lb_bits;
		tape_player_program_bits(player, pdata->data[player->cur_idx],
		    nb, pdata->one_len, pdata->zero_len);
		++player->cur_idx;

	} else if (!player->pause_done) {
		tonegen_clear(&player->tgen);

		tape_player_program_pause(player, pdata->pause_after);
		player->pause_done = true;
	} else {
		tape_player_end_block(player);
	}
}

/** Initialize playback of direct recording block.
 *
 * @param player Tape player
 * @param drec Direct recording block
 */
static void tape_player_direct_rec_init(tape_player_t *player,
    tblock_direct_rec_t *drec)
{
	tonegen_clear(&player->tgen);

	/* Index of next byte to program */
	player->cur_idx = 0;
	player->pause_done = false;
}

/** Next step in playback of direct recording block.
 *
 * @param player Tape player
 * @param drec Direct recording block
 */
static void tape_player_direct_rec_next(tape_player_t *player,
    tblock_direct_rec_t *drec)
{
	int nb;

	if (!tonegen_is_end(&player->tgen))
		return;

	if (player->cur_idx < drec->data_len) {
		tonegen_clear(&player->tgen);

		nb = player->cur_idx < drec->data_len - 1 ? 8 : drec->lb_bits;
		tape_player_program_dr_bits(player, drec->data[player->cur_idx],
		    nb, drec->smp_dur);
		++player->cur_idx;

	} else if (!player->pause_done) {
		tonegen_clear(&player->tgen);

		tape_player_program_pause(player, drec->pause_after);
		player->pause_done = true;
	} else {
		tape_player_end_block(player);
	}
}

/** Initialize playback of pause block.
 *
 * @param player Tape player
 * @param pause Pause block
 */
static void tape_player_pause_init(tape_player_t *player,
    tblock_pause_t *pause)
{
	tonegen_clear(&player->tgen);
	tape_player_program_pause(player, pause->pause_len);
}

/** Next step in playback of pause block.
 *
 * @param player Tape player
 * @param pause Pause block
 */
static void tape_player_pause_next(tape_player_t *player,
    tblock_pause_t *pause)
{
	tape_player_end_block(player);
}

/** Initialize playback of stop the tape block.
 *
 * @param player Tape player
 * @param stop Stop the tape block
 */
static void tape_player_stop_init(tape_player_t *player, tblock_stop_t *stop)
{
	player->sig = tps_stop;
}

/** Next step in playback of stop the tape block.
 *
 * @param player Tape player
 * @param stop Stop the tape block
 */
static void tape_player_stop_next(tape_player_t *player, tblock_stop_t *stop)
{
	tape_player_end_block(player);
}

/** Initialize playback of loop start block.
 *
 * @param player Tape player
 * @param lstart Loop start block
 */
static void tape_player_loop_start_init(tape_player_t *player,
    tblock_loop_start_t *lstart)
{
	player->loop_cnt = lstart->num_rep;
}

/** Next step in playback of loop start block.
 *
 * @param player Tape player
 * @param lstart Loop end block
 */
static void tape_player_loop_start_next(tape_player_t *player,
    tblock_loop_start_t *lstart)
{
	tape_player_end_block(player);
}

/** Initialize playback of loop end block.
 *
 * @param player Tape player
 * @param lend Loop end block
 */
static void tape_player_loop_end_init(tape_player_t *player,
    tblock_loop_end_t *lend)
{
	tape_block_t *block;
	tape_block_t *prev;

	if (player->loop_cnt > 0)
		--player->loop_cnt;

	/* Next iteration? */
	if (player->loop_cnt > 0) {
		/* Find loop start */
		block = lend->block;
		prev = tape_prev(block);
		while (prev != NULL && prev->btype != tb_loop_start) {
			block = prev;
			prev = tape_prev(block);
		}

		/* Continue at first block after loop start */
		player->cur_block = NULL;
		player->next_block = block;
	}
}

/** Next step in playback of loop end block.
 *
 * @param player Tape player
 * @param lend Loop start block
 */
static void tape_player_loop_end_next(tape_player_t *player,
    tblock_loop_end_t *lend)
{
	tape_player_end_block(player);
}

/** Initialize playback of loop start if in 48K mode block.
 *
 * @param player Tape player
 * @param stop48k Loop start if in 48K mode block
 */
static void tape_player_stop_48k_init(tape_player_t *player,
    tblock_stop_48k_t *stop48k)
{
	player->sig = tps_stop_48k;
}

/** Next step in playback of stop the tape if in 48K mode block.
 *
 * @param player Tape player
 * @param stop48k Stop the tape if in 48K mode block
 */
static void tape_player_stop_48k_next(tape_player_t *player,
    tblock_stop_48k_t *stop48k)
{
	tape_player_end_block(player);
}
