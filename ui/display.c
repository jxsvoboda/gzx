/*
 * GZX - George's ZX Spectrum Emulator
 * Display menu
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

#include "../mgfx.h"
#include "../gzx.h"
#include "display.h"
#include "menu.h"

static void display_prev_opt(int l);

#define DISPLAY_NENT 2

static const char *display_text[DISPLAY_NENT] = {
	"~Double Line",
	"~Windowed"
};

static int display_keys[DISPLAY_NENT] = {
	WKEY_D, WKEY_W
};

static void display_run_line(int l)
{
	display_prev_opt(l);
}

static void display_prev_opt(int l)
{
	switch (l) {
	case 0:
		gzx_toggle_dbl_ln();
		break;
	case 1:
		mgfx_toggle_fs();
		break;
	}
}

static void display_next_opt(int l)
{
	switch (l) {
	case 0:
		gzx_toggle_dbl_ln();
		break;
	case 1:
		mgfx_toggle_fs();
		break;
	}
}

static const char *display_get_opt(int l)
{
	switch (l) {
	case 0:
		return dbl_ln ? "On" : "Off";
	case 1:
		return mgfx_is_fs() ? "Off" : "On";
	default:
		return NULL;
	}
}

static menu_t display_menu_spec = {
	.caption = "Display Options",
	.nent = DISPLAY_NENT,
	.mentry_text = display_text,
	.mkeys = display_keys,
	.run_line = display_run_line,
	.prev_opt = display_prev_opt,
	.next_opt = display_next_opt,
	.get_opt = display_get_opt
};

/** Display options menu */
void display_menu(void)
{
	menu_run(&display_menu_spec);
}
