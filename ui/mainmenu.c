/*
 * GZX - George's ZX Spectrum Emulator
 * Main menu
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

#include "../gzx.h"
#include "../memio.h"
#include "../mgfx.h"
#include "display.h"
#include "fdlg.h"
#include "hwopts.h"
#include "mainmenu.h"
#include "menu.h"

#define MENU_NENT 9

static const char *mentry_text[MENU_NENT] = {
	"~Load Snapshot",
	"~Save Snapshot",
	"Select ~Tapefile",
	"Reset ~48",
	"Reset ~128",
	"~Display",
	"~Hardware",
	"Lock ~UI",
	"~Quit",
};

static int mkeys[MENU_NENT] = {
	WKEY_L, WKEY_S, WKEY_T, WKEY_4, WKEY_1, WKEY_D, WKEY_H,
	WKEY_U, WKEY_Q
};

static void menu_run_line(int l)
{
	switch (l) {
	case 0:
		load_snap_dialog();
		break;
	case 1:
		save_snap_dialog();
		break;
	case 2:
		select_tapefile_dialog();
		break;
	case 3:
		zx_select_memmodel(ZXM_48K);
		zx_reset();
		break;
	case 4:
		zx_select_memmodel(ZXM_128K);
		zx_reset();
		break;
	case 5:
		display_menu();
		break;
	case 6:
		hwopts_menu();
		break;
	case 7:
		gzx_ui_lock();
		break;
	case 8:
		quit = 1;
		break;
	}
}

static void menu_prev_opt(int l)
{
}

static void menu_next_opt(int l)
{
}

static const char *menu_get_opt(int l)
{
	switch (l) {
	case 0:
		return "F3";
	case 1:
		return "F2";
	case 2:
		return "F9";
	case 3:
		return "F7";
	case 4:
		return "F8";
	case 5:
		return "F4";
	case 6:
		return "F6";
	case 7:
		return "Alt-Sh-U";
	case 8:
		return "F10";
	default:
		return NULL;
	}
}

static menu_t main_menu_spec = {
	.caption = "Main Menu",
	.nent = MENU_NENT,
	.mentry_text = mentry_text,
	.mkeys = mkeys,
	.run_line = menu_run_line,
	.prev_opt = menu_prev_opt,
	.next_opt = menu_next_opt,
	.get_opt = menu_get_opt
};

/** Main menu */
void main_menu(void)
{
	menu_run(&main_menu_spec);
}
