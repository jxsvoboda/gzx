/*
 * GZX - George's ZX Spectrum Emulator
 * Tape deck
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
 * @file Tape deck.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "deck.h"
#include "player.h"
#include "sampler.h"
#include "tap.h"
#include "tape.h"
#include "tzx.h"
#include "wav.h"
#include "../strutil.h"

static void tape_deck_process_sig(tape_deck_t *, tape_player_sig_t);

/** Create tape deck.
 *
 * @param mode48k Start in 48K mode
 * @param delta_t Clock ticks per sample
 * @param rdeck Place to store pointer to new tape deck
 * @return Zero on success, ENOMEM if out of memory
 */
int tape_deck_create(tape_deck_t **rdeck, uint32_t delta_t, bool mode48k)
{
	tape_deck_t *deck;
	tape_player_t *player = NULL;
	tape_sampler_t *sampler = NULL;
	int rc;

	deck = calloc(1, sizeof(tape_deck_t));
	if (deck == NULL)
		return ENOMEM;

	rc = tape_player_create(&player);
	if (rc != 0) {
		printf("Out of memory.\n");
		free(deck);
		return rc;
	}

	deck->player = player;

	rc = tape_sampler_create(player, delta_t, &sampler);
	if (rc != 0) {
		tape_player_destroy(deck->player);
		free(deck);
		return rc;
	}

	deck->sampler = sampler;

	rc = tape_deck_new(deck);
	if (rc != 0) {
		tape_sampler_destroy(deck->sampler);
		tape_player_destroy(deck->player);
		free(deck);
		return rc;
	}

	deck->mode48k = mode48k;
	*rdeck = deck;
	return 0;
}

/** Destroy tape deck.
 *
 * @param deck Tape deck
 */
void tape_deck_destroy(tape_deck_t *deck)
{
	if (deck->tape != NULL)
		tape_destroy(deck->tape);

	if (deck->player != NULL)
		tape_player_destroy(deck->player);

	if (deck->sampler != NULL)
		tape_sampler_destroy(deck->sampler);

	free(deck);
}

/** Close tape in tape deck.
 *
 * @param deck Tape deck
 */
static void tape_deck_close(tape_deck_t *deck)
{
	tape_deck_stop(deck);

	if (deck->tape != NULL) {
		tape_destroy(deck->tape);
		deck->tape = NULL;
	}

	if (deck->fname != NULL) {
		free(deck->fname);
		deck->fname = NULL;
	}
}

/** Insert new (empty) tape in tape deck.
 *
 * @param deck Tape deck
 * @return Zero on success or an error code
 */
int tape_deck_new(tape_deck_t *deck)
{
	tape_t *tape = NULL;
	int rc;

	rc = tape_create(&tape);
	if (rc != 0)
		return rc;

	tape_deck_close(deck);

	deck->fname = strdupl("newtape.tzx");
	if (deck->fname == NULL) {
		tape_destroy(tape);
		return ENOMEM;
	}

	deck->tape = tape;
	deck->cur_block = NULL;

	return 0;
}

/** Open tape file in tape deck.
 *
 * @param deck Tape deck
 * @param fname Tape file name
 * @return Zero on success or an error code
 */
int tape_deck_open(tape_deck_t *deck, const char *fname)
{
	char *name;
	const char *ext;
	tape_t *tape;
	int rc;

	tape_deck_close(deck);
	ext = strrchr(fname, '.');
	if (ext == NULL) {
		printf("File has no extension.\n");
		return ENOTSUP;
	}

	if (strcmpci(ext, ".tap") == 0) {
		rc = tap_tape_load(fname, &tape);
	} else if (strcmpci(ext, ".tzx") == 0) {
		rc = tzx_tape_load(fname, &tape);
	} else if (strcmpci(ext, ".wav") == 0) {
		rc = wav_tape_load(fname, &tape);
	} else {
		printf("Uknown extension '%s'.\n", ext);
		return ENOTSUP;
	}

	if (rc != 0) {
		printf("Error opening tape file\n");
		return rc;
	}

	name = strdupl(fname);
	if (name == NULL) {
		printf("Out of memory.\n");
		return rc;
	}

	deck->tape = tape;
	free(deck->fname);
	deck->fname = name;
	deck->cur_block = tape_first(tape);

	return 0;
}

/** Save tape deck.
 *
 * @param deck Tape deck
 * @return Zero on success or an error code
 */
int tape_deck_save(tape_deck_t *deck)
{
	const char *ext;
	int rc;

	ext = strrchr(deck->fname, '.');
	assert(ext != NULL);

	if (strcmpci(ext, ".tap") == 0) {
		rc = tap_tape_save(deck->tape, deck->fname);
	} else if (strcmpci(ext, ".tzx") == 0) {
		rc = tzx_tape_save(deck->tape, deck->fname);
	} else if (strcmpci(ext, ".wav") == 0) {
		rc = wav_tape_save(deck->tape, deck->fname);
	} else {
		assert(false);
		return ENOTSUP;
	}

	if (rc != 0) {
		printf("Error saving tape file\n");
		return rc;
	}

	return 0;
}

/** Save tape deck as new file.
 *
 * @param deck Tape deck
 * @param fname New file name
 * @return Zero on success or an error code
 */
int tape_deck_save_as(tape_deck_t *deck, const char *fname)
{
	const char *ext;
	char *name;
	int rc;

	name = strdupl(fname);
	if (name == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	ext = strrchr(fname, '.');
	assert(ext != NULL);

	if (strcmpci(ext, ".tap") == 0) {
		rc = tap_tape_save(deck->tape, fname);
	} else if (strcmpci(ext, ".tzx") == 0) {
		rc = tzx_tape_save(deck->tape, fname);
	} else if (strcmpci(ext, ".wav") == 0) {
		rc = wav_tape_save(deck->tape, fname);
	} else {
		free(name);
		printf("Invalid file extension.\n");
		return ENOTSUP;
	}

	if (rc != 0) {
		printf("Error saving tape file\n");
		free(name);
		return rc;
	}

	if (deck->fname != NULL)
		free(deck->fname);

	deck->fname = name;
	return 0;
}

/** Play tape.
 *
 * @param deck Tape deck
 */
void tape_deck_play(tape_deck_t *deck)
{
	if (deck->paused) {
		deck->paused = false;
		return;
	}

	tape_sampler_init(deck->sampler, deck->cur_block);

	if (tape_sampler_is_end(deck->sampler))
		return;

	deck->playing = true;
}

/** Pause tape.
 *
 * @param deck Tape deck
 */
void tape_deck_pause(tape_deck_t *deck)
{
	if (deck->playing)
		deck->paused = true;
}

/** Stop tape.
 *
 * @param deck Tape deck
 */
void tape_deck_stop(tape_deck_t *deck)
{
	if (!deck->playing)
		return;

	deck->playing = false;
	deck->paused = false;
	deck->cur_block = tape_sampler_cur_block(deck->sampler);
}

/** Rewind tape.
 *
 * @param deck Tape deck
 */
void tape_deck_rewind(tape_deck_t *deck)
{
	tape_deck_stop(deck);
	deck->cur_block = tape_first(deck->tape);
}

/** Move to next tape block.
 *
 * @param deck Tape deck
 */
void tape_deck_next(tape_deck_t *deck)
{
	tape_deck_stop(deck);
	deck->cur_block = tape_next(deck->cur_block);
}

/** Enable or disable 48K mode.
 *
 * This affects the behavior of Stop the tape if in 48K mode block.
 *
 * @param deck Tape deck
 * @param mode48k @c true if in 48K mode
 */
void tape_deck_set_48k(tape_deck_t *deck, bool mode48k)
{
	deck->mode48k = mode48k;
}

/** Check if tape is playing.
 *
 * @param deck Tape deck
 * @return @c true iff the tape is playing (even when paused)
 */
bool tape_deck_is_playing(tape_deck_t *deck)
{
	return deck->playing;
}

/** Get tape sample.
 *
 * @param deck Tape deck
 * @param smp Place to store sample
 */
void tape_deck_getsmp(tape_deck_t *deck, uint8_t *smp)
{
	tape_player_sig_t sig;

	if (!deck->playing || deck->paused) {
		*smp = deck->cur_smp;
		return;
	}

	tape_sampler_getsmp(deck->sampler, &deck->cur_smp, &sig);
	if (sig != tps_none)
		tape_deck_process_sig(deck, sig);

	*smp = deck->cur_smp;
}

/** Get current tape block.
 *
 * @param deck Tape deck
 * @return Current tape block or @c NULL if none
 */
tape_block_t *tape_deck_cur_block(tape_deck_t *deck)
{
	if (deck->playing)
		return tape_sampler_cur_block(deck->sampler);

	return deck->cur_block;
}

/** Process tape player signal and act accordingly.
 *
 * @param deck Tape deck
 * @param sig Tape player signal
 */
static void tape_deck_process_sig(tape_deck_t *deck, tape_player_sig_t sig)
{
	switch (sig) {
	case tps_none:
		break;
	case tps_stop:
		deck->playing = false;
		break;
	case tps_stop_48k:
		if (deck->mode48k)
			deck->playing = false;
		break;
	}
}
