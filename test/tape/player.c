/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator unit tests
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
 * @file Tape tone generator unit tests.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../tape/defs.h"
#include "../../tape/player.h"
#include "../../tape/tape.h"
#include "player.h"

enum {
	data_dbytes = 4,
	tone_np = 3,
	pulses_np = 3
};

/** Check waveform generated by tape player matches template.
 *
 * @param player Tape player
 * @param delays Array of pulse lengths
 * @param num_pulses Number of pulses
 * @param start_lvl Starting level
 *
 * @return Zero on success, non-zero on mismatch or other failure
 */
static int test_check_waveform(tape_player_t *player, uint32_t *delays,
    int num_pulses, tape_lvl_t start_lvl)
{
	int i;
	uint32_t delay;
	tape_lvl_t lvl;
	tape_lvl_t tlvl;

	tlvl = start_lvl;

	for (i = 0; i < num_pulses; i++) {
		if (tape_player_is_end(player)) {
			printf("Premature end of waveform.\n");
			return 1;
		}

		lvl = tape_player_cur_lvl(player);
		if (lvl != tlvl) {
			printf("Incorrect level (actual=%d expected=%d).\n",
			    lvl, tlvl);
			return 1;
		}

		tlvl = !tlvl;

		tape_player_get_next(player, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length (%d != %d).\n", delay, delays[i]);
			return 1;
		}

		if (lvl != tlvl) {
			printf("Incorrect level (actual=%d expected=%d).\n",
			    lvl, tlvl);
			return 1;
		}
	}

	return 0;
}

/** Check waveform generated by tape player matches direct pulse.
 *
 * @param player Tape player
 * @param pulse_lvl Pulse level
 * @param pulse_len Pulse length
 *
 * @return Zero on success, non-zero on mismatch or other failure
 */
static int test_check_dpulse(tape_player_t *player, tape_lvl_t pulse_lvl,
    uint32_t pulse_len)
{
	uint32_t delay;
	tape_lvl_t lvl;

	if (tape_player_is_end(player)) {
		printf("Premature end of waveform.\n");
		return 1;
	}

	lvl = tape_player_cur_lvl(player);
	if (lvl != pulse_lvl) {
		printf("Incorrect level (actual=%d expected=%d).\n",
		    lvl, pulse_lvl);
		return 1;
	}

	tape_player_get_next(player, &delay, &lvl);
	if (delay != pulse_len) {
		printf("Incorrect pulse length (%d != %d).\n", delay,
		    pulse_len);
		return 1;
	}

	return 0;
}

/** Test tape player with standard speed data block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_data(void)
{
	tape_t *tape;
	tblock_data_t *data;
	tape_player_t *player;
	uint8_t dbytes[data_dbytes] = { 0xff, 10, 100, 200 };
	uint32_t pilot_pulses[1] = { ROM_PILOT_LEN };
	uint32_t sync_pulses[2] = { ROM_SYNC1_LEN, ROM_SYNC2_LEN };
	uint32_t one_pulses[2] = { ROM_ONE_LEN, ROM_ONE_LEN };
	uint32_t zero_pulses[2] = { ROM_ZERO_LEN, ROM_ZERO_LEN };
	uint32_t pause_pulses[1] = { TAPE_PAUSE_MULT * 10 };
	uint32_t *ppulses;
	tape_lvl_t tlvl;
	int i, j;
	int rc;

	printf("Test tape player with standard speed data block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_data_create(&data);
	if (rc != 0)
		return 1;

	data->pause_after = 10;
	data->data = malloc(data_dbytes);
	if (data->data == NULL)
		return 1;

	for (i = 0; i < data_dbytes; i++)
		data->data[i] = dbytes[i];

	data->data_len = data_dbytes;

	tape_append(tape, data->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	tlvl = tlvl_low;
	for (i = 0; i < ROM_PPULSES_D; i++) {
		rc = test_check_waveform(player, pilot_pulses, 1, tlvl);
		if (rc != 0)
			return 1;

		tlvl = !tlvl;
	}

	rc = test_check_waveform(player, sync_pulses, 2, tlvl);
	if (rc != 0)
		return 1;

	for (i = 0; i < data_dbytes; i++) {
		for (j = 0; j < 8; j++) {
			if ((data->data[i] & (0x80 >> j)) != 0)
				ppulses = one_pulses;
			else
				ppulses = zero_pulses;

			rc = test_check_waveform(player, ppulses, 2, tlvl);
			if (rc != 0)
				return 1;
		}
	}

	rc = test_check_waveform(player, pause_pulses, 1, tlvl);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with turbo speed data block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_turbo_data(void)
{
	tape_t *tape;
	tblock_turbo_data_t *tdata;
	tape_player_t *player;
	uint8_t dbytes[data_dbytes] = { 0xff, 10, 100, 200 };
	int lb_bits = 8;
	uint32_t pilot_pulses[1] = { 2000 };
	uint32_t sync_pulses[2] = { 600, 700 };
	uint32_t one_pulses[2] = { 1000, 1000 };
	uint32_t zero_pulses[2] = { 500, 500 };
	uint32_t pause_pulses[1] = { TAPE_PAUSE_MULT * 10 };
	uint32_t *ppulses;
	tape_lvl_t tlvl;
	int i, j;
	int nb;
	int rc;

	printf("Test tape player with turbo speed data block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_turbo_data_create(&tdata);
	if (rc != 0)
		return 1;

	tdata->pilot_len = pilot_pulses[0];
	tdata->sync1_len = sync_pulses[0];
	tdata->sync2_len = sync_pulses[1];
	tdata->zero_len = zero_pulses[0];
	tdata->one_len = one_pulses[1];
	tdata->pilot_pulses = 3000;
	tdata->lb_bits = lb_bits;
	tdata->pause_after = 10;
	tdata->data = malloc(data_dbytes);
	if (tdata->data == NULL)
		return 1;

	for (i = 0; i < data_dbytes; i++)
		tdata->data[i] = dbytes[i];

	tdata->data_len = data_dbytes;

	tape_append(tape, tdata->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	tlvl = tlvl_low;
	for (i = 0; i < tdata->pilot_pulses; i++) {
		rc = test_check_waveform(player, pilot_pulses, 1, tlvl);
		if (rc != 0)
			return 1;

		tlvl = !tlvl;
	}

	rc = test_check_waveform(player, sync_pulses, 2, tlvl);
	if (rc != 0)
		return 1;

	for (i = 0; i < data_dbytes; i++) {
		nb = i < data_dbytes - 1 ? 8 : lb_bits;
		for (j = 0; j < nb; j++) {
			if ((tdata->data[i] & (0x80 >> j)) != 0)
				ppulses = one_pulses;
			else
				ppulses = zero_pulses;

			rc = test_check_waveform(player, ppulses, 2, tlvl);
			if (rc != 0)
				return 1;
		}
	}

	rc = test_check_waveform(player, pause_pulses, 1, tlvl);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with tone block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_tone(void)
{
	tape_t *tape;
	tblock_tone_t *tone;
	tape_player_t *player;
	uint32_t delays[tone_np] = { 10, 10, 10 };
	int rc;

	printf("Test tape player with tone block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_tone_create(&tone);
	if (rc != 0)
		return 1;

	tone->num_pulses = 3;
	tone->pulse_len = 10;

	tape_append(tape, tone->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	rc = test_check_waveform(player, delays, tone_np, tlvl_low);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with pulses block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_pulses(void)
{
	tape_t *tape;
	tblock_pulses_t *pulses;
	tape_player_t *player;
	uint32_t delays[pulses_np] = { 10, 20, 30 };
	int rc;

	printf("Test tape player with pulses block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_pulses_create(&pulses);
	if (rc != 0)
		return 1;

	pulses->num_pulses = 3;
	pulses->pulse_len = calloc(3, sizeof(uint16_t));
	if (pulses->pulse_len == NULL)
		return 1;

	pulses->pulse_len[0] = 10;
	pulses->pulse_len[1] = 20;
	pulses->pulse_len[2] = 30;

	tape_append(tape, pulses->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	rc = test_check_waveform(player, delays, pulses_np, tlvl_low);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with pure data block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_pure_data(void)
{
	tape_t *tape;
	tblock_pure_data_t *pdata;
	tape_player_t *player;
	uint8_t dbytes[data_dbytes] = { 0xff, 10, 100, 200 };
	int lb_bits = 7;
	uint32_t one_pulses[2] = { 1000, 1000 };
	uint32_t zero_pulses[2] = { 500, 500 };
	uint32_t pause_pulses[1] = { TAPE_PAUSE_MULT * 10 };
	uint32_t *ppulses;
	tape_lvl_t tlvl;
	int i, j;
	int nb;
	int rc;

	printf("Test tape player with pure data block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_pure_data_create(&pdata);
	if (rc != 0)
		return 1;

	pdata->zero_len = zero_pulses[0];
	pdata->one_len = one_pulses[1];
	pdata->lb_bits = lb_bits;
	pdata->pause_after = 10;
	pdata->data = malloc(data_dbytes);
	if (pdata->data == NULL)
		return 1;

	for (i = 0; i < data_dbytes; i++)
		pdata->data[i] = dbytes[i];

	pdata->data_len = data_dbytes;

	tape_append(tape, pdata->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	tlvl = tlvl_low;

	for (i = 0; i < data_dbytes; i++) {
		nb = i < data_dbytes - 1 ? 8 : lb_bits;
		for (j = 0; j < nb; j++) {
			if ((pdata->data[i] & (0x80 >> j)) != 0)
				ppulses = one_pulses;
			else
				ppulses = zero_pulses;

			rc = test_check_waveform(player, ppulses, 2, tlvl);
			if (rc != 0)
				return 1;
		}
	}

	printf("check pause\n");
	rc = test_check_waveform(player, pause_pulses, 1, tlvl);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Test tape player with direct recording block.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tape_player_direct_rec(void)
{
	tape_t *tape;
	tblock_direct_rec_t *drec;
	tape_player_t *player;
	uint8_t dbytes[data_dbytes] = { 0xff, 10, 100, 200 };
	int lb_bits = 5;
	uint32_t pause_pulses[1] = { TAPE_PAUSE_MULT * 10 };
	tape_lvl_t tlvl;
	int i, j;
	int nb;
	int rc;

	printf("Test tape player with direct recording block...\n");

	rc = tape_create(&tape);
	if (rc != 0)
		return 1;

	rc = tblock_direct_rec_create(&drec);
	if (rc != 0)
		return 1;

	drec->smp_dur = 10;
	drec->lb_bits = lb_bits;
	drec->pause_after = 10;
	drec->data = malloc(data_dbytes);
	if (drec->data == NULL)
		return 1;

	for (i = 0; i < data_dbytes; i++)
		drec->data[i] = dbytes[i];

	drec->data_len = data_dbytes;

	tape_append(tape, drec->block);

	rc = tape_player_create(&player);
	if (rc != 0)
		return 1;

	tape_player_init(player, tape_first(tape));

	for (i = 0; i < data_dbytes; i++) {
		nb = i < data_dbytes - 1 ? 8 : lb_bits;
		for (j = 0; j < nb; j++) {
			if ((drec->data[i] & (0x80 >> j)) != 0)
				tlvl = tlvl_high;
			else
				tlvl = tlvl_low;

			rc = test_check_dpulse(player, tlvl, drec->smp_dur);
			if (rc != 0)
				return 1;
		}
	}

	rc = test_check_waveform(player, pause_pulses, 1, tlvl);
	if (rc != 0)
		return 1;

	if (!tape_player_is_end(player)) {
		printf("Expected end of waveform not found.\n");
		return 1;
	}

	tape_player_destroy(player);
	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Run tape player unit tests.
 *
 * @return Zero on success, non-zero on failure
 */
int test_tape_player(void)
{
	int rc;

	rc = test_tape_player_data();
	if (rc != 0)
		return 1;

	rc = test_tape_player_turbo_data();
	if (rc != 0)
		return 1;

	rc = test_tape_player_tone();
	if (rc != 0)
		return 1;

	rc = test_tape_player_pulses();
	if (rc != 0)
		return 1;

	rc = test_tape_player_pure_data();
	if (rc != 0)
		return 1;

	rc = test_tape_player_direct_rec();
	if (rc != 0)
		return 1;

	return 0;
}
