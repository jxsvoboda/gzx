/*
 * GZX - George's ZX Spectrum Emulator
 * Tape menu
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

#include "../tape/deck.h"
#include "../gzx.h"
#include "../mgfx.h"
#include "../zx.h"
#include "fdlg.h"
#include "menu.h"
#include "tapemenu.h"

#define TMENU_NENT 7

static const char *tmentry_text[TMENU_NENT] = {
	"~Play",
	"~Stop",
	"~Rewind",
	"~Quick Tape",
	"~New",
	"Sa~ve",
	"Save ~As"
};

static int tmkeys[TMENU_NENT] = {
	WKEY_P, WKEY_S, WKEY_R, WKEY_Q, WKEY_N, WKEY_V, WKEY_A
};

static void tmenu_run_line(int l)
{
	switch (l) {
	case 0:
		tape_deck_play(tape_deck);
		break;
	case 1:
		tape_deck_stop(tape_deck);
		break;
	case 2:
		tape_deck_rewind(tape_deck);
		break;
	case 3:
		slow_load = !slow_load;
		break;
	case 4:
		tape_deck_new(tape_deck);
		break;
	case 5:
		tape_deck_save(tape_deck);
		break;
	case 6:
		save_tape_as_dialog();
		break;
	}
}

static void tmenu_prev_opt(int l)
{
	switch (l) {
	case 3:
		slow_load = !slow_load;
		break;
	}
}

static void tmenu_next_opt(int l)
{
	switch (l) {
	case 3:
		slow_load = !slow_load;
		break;
	}
}

static const char *tmenu_get_opt(int l)
{
	switch (l) {
	case 3:
		return slow_load ? "Off" : "On";
	default:
		return NULL;
	}
}

static menu_t tape_menu_spec = {
	.caption = "Tape Menu",
	.nent = TMENU_NENT,
	.mentry_text = tmentry_text,
	.mkeys = tmkeys,
	.run_line = tmenu_run_line,
	.prev_opt = tmenu_prev_opt,
	.next_opt = tmenu_next_opt,
	.get_opt = tmenu_get_opt
};

/** Tape menu */
void tape_menu(void)
{
	menu_run(&tape_menu_spec);
}
