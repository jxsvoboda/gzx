/*
 * GZX - George's ZX Spectrum Emulator
 * Menu
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

#include <stdlib.h>
#include <string.h>

#include "../mgfx.h"
#include "../sys_all.h"
#include "menu.h"

/*
 * Draw a string at fixed width, with ~=highlight
 */
static void gputs_hl(int n, int fgc_, int hlc, int bgc_, const char *s)
{
	bgc = bgc_;
	while (n > 0) {
		if (*s == '~') {
			fgc = hlc;
			s++;
		} else
			fgc = fgc_;

		gputc(*s);
		if (*s)
			s++;
		n--;
	}
}

static void menu_run_line(menu_t *menu, int l)
{
	menu->run_line(l);
}

static void menu_prev_opt(menu_t *menu, int l)
{
	if (menu->prev_opt != NULL)
		menu->prev_opt(l);
}

static void menu_next_opt(menu_t *menu, int l)
{
	if (menu->next_opt != NULL)
		menu->next_opt(l);
}

static void menu_draw(menu_t *menu, int mpos)
{
	int i;
	int fgc_, bgc_, hlc_;
	int optc;
	const char *opt;

	mgfx_selln(3); /* Enable rendering odd and even lines */

	mgfx_fillrect(menu->cx0 * 8, 0, menu->cx0 * 8 + 8 * 20, scr_ys - 1, 1);
	gmovec(scr_xs / 16 - (strlen(menu->caption) / 2), 0);
	fgc = 7;
	bgc = 1;
	gputs(menu->caption);

	for (i = 0; i < menu->nent; i++) {
		if (i == mpos) {
			fgc_ = hlc_ = optc = 1;
			bgc_ = 5;
		} else {
			fgc_ = 7;
			hlc_ = 14;
			bgc_ = 1;
			optc = 6;
		}

		gmovec(menu->cx0 + 1, 2 + i);
		gputs_hl(18, fgc_, hlc_, bgc_, menu->mentry_text[i]);

		opt = NULL;
		if (menu->get_opt != NULL)
			opt = menu->get_opt(i);

		if (opt != NULL) {
			gmovec(menu->cx0 + 19 - strlen(opt), 2 + i);
			fgc = optc;
			bgc = bgc_;
			gputs(opt);
		}
	}
}

static void menu_undraw(menu_t *menu)
{
	mgfx_fillrect(0, 0, scr_xs, scr_ys, 0);
}

void menu_run(menu_t *menu, int start_pos)
{
	wkey_t k;
	int end_menu = 0;
	int j;
	int mpos;

	mpos = start_pos;
	menu->cx0 = scr_xs / 16 - 10;

	while (!end_menu) {
		menu_draw(menu, mpos);
		mgfx_updscr();

		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			switch (k.key) {
			case WKEY_ESC:
				end_menu = 1;
				break;

			case WKEY_ENTER:
				menu_run_line(menu, mpos);
				end_menu = 1;
				break;

			case WKEY_UP:
				if (mpos > 0)
					--mpos;
				else
					mpos = menu->nent - 1;
				break;

			case WKEY_DOWN:
				if (mpos < menu->nent - 1)
					++mpos;
				else
					mpos = 0;
				break;

			case WKEY_LEFT:
				menu_prev_opt(menu, mpos);
				break;

			case WKEY_RIGHT:
				menu_next_opt(menu, mpos);
				break;

			case WKEY_PGUP:
				mpos = 0;
				break;

			case WKEY_PGDN:
				mpos = menu->nent - 1;
				break;

			default:
				for (j = 0; j < menu->nent; j++)
					if (k.key == menu->mkeys[j]) {
						menu_run_line(menu, j);
						end_menu = 1;
					}
				break;
			}
	}

	menu_undraw(menu);
}
