/*
 * GZX - George's ZX Spectrum Emulator
 * File dialogs
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../gzx.h"
#include "../mgfx.h"
#include "../snap.h"
#include "../sys_all.h"
#include "../tape/deck.h"
#include "../zx.h"
#include "fdlg.h"
#include "fsel.h"
#include "menu.h"
#include "teline.h"

/** Select tapefile dialog */
void select_tapefile_dialog(void)
{
	char *fname;

	if (file_sel(&fname, "Select Tapefile") > 0) {
		fprintf(logfi, "selecting tape file\n");
		fflush(logfi);
		(void) tape_deck_open(tape_deck, fname);
		fprintf(logfi, "freeing filename\n");
		fflush(logfi);
		free(fname);
	}
}

/** Load snapshot dialog */
void load_snap_dialog(void)
{
	char *fname;

	if (file_sel(&fname, "Load Snapshot") > 0) {
		zx_load_snap(fname);
		free(fname);
	}
}

/** Save snapshot dialog */
void save_snap_dialog(void)
{
	wkey_t k;
	int fscols;
	int flist_cx0;
	teline_t fn_line;

	fscols = 20;
	flist_cx0 = scr_xs / 16 - fscols / 2;
	teline_init(&fn_line, flist_cx0, 12, 20);
	fn_line.focus = 1;

	while (1) {
		mgfx_fillrect(flist_cx0 * 8 - 8, 0, flist_cx0 * 8 +
		    8 * (fscols + 1), scr_ys - 1, 1);
		teline_draw(&fn_line);
		gmovec(scr_xs / 16 - (strlen("Save Snapshot") / 2), 0);
		fgc = 7;
		bgc = 1;
		gputs("Save Snapshot");

		mgfx_updscr();
		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			switch (k.key) {
			case WKEY_ESC:
				return;

			case WKEY_ENTER:
				fn_line.buf[fn_line.len] = 0;
				zx_save_snap(fn_line.buf);
				return;

			default:
				teline_key(&fn_line, &k);
				break;
			}
	}
}

/** Save Tape As dialog. */
void save_tape_as_dialog(void)
{
	wkey_t k;
	int fscols;
	int flist_cx0;
	teline_t fn_line;

	fscols = 20;
	flist_cx0 = scr_xs / 16 - fscols / 2;
	teline_init(&fn_line, flist_cx0, 12, 20);
	fn_line.focus = 1;

	while (1) {
		mgfx_fillrect(flist_cx0 * 8 - 8, 0, flist_cx0 * 8 +
		    8 * (fscols + 1), scr_ys - 1, 1);
		teline_draw(&fn_line);
		gmovec(scr_xs / 16 - (strlen("Save Tape As") / 2), 0);
		fgc = 7;
		bgc = 1;
		gputs("Save Tape As");

		mgfx_updscr();
		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			switch (k.key) {
			case WKEY_ESC:
				return;

			case WKEY_ENTER:
				fn_line.buf[fn_line.len] = 0;
				tape_deck_save_as(tape_deck, fn_line.buf);
				return;

			default:
				teline_key(&fn_line, &k);
				break;
			}
	}
}
