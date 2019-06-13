/*
 * GZX - George's ZX Spectrum Emulator
 * Tape utility
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
 * @file Tape utility.
 *
 * Utility to manipulate tape files.
 */

#include <errno.h>
#include <stdio.h>
#include "tape/deck.h"
#include "tape/tape.h"
#include "tape/tzx.h"

/** Print command line syntax help. */
static void print_syntax(void)
{
	fprintf(stderr, "GZX tape utility\n");
	fprintf(stderr, "syntax: gtap <tape-file>\n");
}

/** List blocks in tape file.
 *
 * @param fname Tape file name
 * @return Zero on success or an error code
 */
static int gtap_list(const char *fname)
{
	tape_deck_t *deck;
	tape_block_t *tblock;
	tblock_data_t *data;
	tblock_turbo_data_t *tdata;
	tblock_pure_data_t *pdata;
	tblock_pause_t *pause;
	tblock_group_start_t *gstart;
	tblock_text_desc_t *tdesc;
	unsigned bcount;
	unsigned bidx;
	int rc;

	rc = tape_deck_create(&deck);
	if (rc != 0) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	rc = tape_deck_open(deck, fname);
	if (rc != 0) {
		printf("Error loading tape file '%s'.\n", fname);
		return ENOENT;
	}

	bcount = 0;
	tblock = tape_deck_cur_block(deck);
	while (tblock != NULL) {
		++bcount;
		tblock = tape_next(tblock);
	}

	printf("Listing tape file '%s'\n", fname);
	printf("%u blocks\n\n", bcount);

	tblock = tape_deck_cur_block(deck);
	bidx = 1;
	while (tblock != NULL) {
		printf("%d. %s", bidx, tape_btype_str(tblock->btype));

		if (tblock->btype == tb_data) {
			data = (tblock_data_t *) tblock->ext;
			printf(" %02Xh, %d B\n", data->data[0], data->data_len);
		} else if (tblock->btype == tb_turbo_data) {
			tdata = (tblock_turbo_data_t *) tblock->ext;
			printf(" %02Xh, %d B\n", tdata->data[0], tdata->data_len);
		} else if (tblock->btype == tb_pure_data) {
			pdata = (tblock_pure_data_t *) tblock->ext;
			printf(" %02Xh, %d B\n", pdata->data[0], pdata->data_len);
		} else if (tblock->btype == tb_pause) {
			pause = (tblock_pause_t *) tblock->ext;
			printf(" %u ms\n", (unsigned) pause->pause_len);
		} else if (tblock->btype == tb_group_start) {
			gstart = (tblock_group_start_t *) tblock->ext;
			printf(" \"%s\"\n", gstart->name);
		} else if (tblock->btype == tb_text_desc) {
			tdesc = (tblock_text_desc_t *) tblock->ext;
			printf(" \"%s\"\n", tdesc->text);
		} else {
			printf("\n");
		}

		++bidx;
		tblock = tape_next(tblock);
	}

	tape_deck_destroy(deck);
	printf("\n");

	return 0;
}

int main(int argc, char **argv)
{
	int rc;

	if (argc < 2) {
		print_syntax();
		return 1;
	}

	rc = gtap_list(argv[1]);
	if (rc != 0)
		return 1;

	return 0;
}
