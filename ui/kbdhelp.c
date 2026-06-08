/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum Keyboard Layout Help
 *
 * Copyright (c) 1999-2026 Jiri Svoboda
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

#include <stdbool.h>
#include "../fnt.h"
#include "../gzx.h"
#include "../mgfx.h"
#include "../sys_all.h"
#include "font.h"
#include "kbdhelp.h"

const char *key_text[][6] = {
	{ "1",     "",       "!",    "BLUE",    "DEF FN",  "EDIT" },
	{ "2",     "",       "@",    "RED",     "FN",      "CAPS L." },
	{ "3",     "",       "#",    "MAGENTA", "LINE",    "TRUE VID." },
	{ "4",     "",       "$",    "GREEN",   "OPEN #",  "INV. VID." },
	{ "5",     "",       "%",    "CYAN",    "CLOSE #", "LEFT" }, // XXX <=
	{ "6",     "",       "&",    "YELLOW",  "MOVE",    "DOWN" }, // XXX v
	{ "7",     "",       "'",    "WHITE",   "ERASE",   "UP", },  // XXX ^
	{ "8",     "",       "(",    "",        "POINT",   "RIGHT" }, // XXX =>
	{ "9",     "",       "(",    "",        "CAT",     "GRAPH." },
	{ "0",     "",       "_",    "BLACK",   "FORMAT",  "DELETE" },

	{ "Q",     "PLOT",   "<=",   "SIN",     "ASN",     "" },
	{ "W",     "DRAW",   "<>",   "COS",     "ACS",     "" },
	{ "E",     "REM",    ">=",   "TAN",     "ATN",     "" },
	{ "R",     "RUN",    "<",    "INT",     "VERIFY",  "" },
	{ "T",     "RAND",   ">",    "RND",     "MERGE",   "" },
	{ "Y",     "RETURN", "AND",  "STR$",    "[",       "" },
	{ "U",     "IF",     "OR",   "CHR$",    "]",       "" },
	{ "I",     "INPUT",  "AT",   "CODE",    "IN",      "" },
	{ "O",     "POKE",   ";",    "PEEK",    "OUT",     "" },
	{ "P",     "PRINT",  "\"",   "TAB",     "(c)",     "" }, // XXX copyr.

	{ "A",     "NEW",    "STOP", "READ",    "~",       "" },
	{ "S",     "SAVE",   "NOT",  "RESTORE", "|",       "" },
	{ "D",     "DIM",    "STEP", "DATA",    "\\",      "" },
	{ "F",     "FOR",    "TO",   "SGN",     "{",       "" },
	{ "G",     "GOTO",   "THEN", "ABS",     "}",       "" },
	{ "H",     "GOSUB",  "^",    "SQR",     "CIRCLE",  "" }, // XXX up arr.
	{ "J",     "LOAD",   "-",    "VAL",     "VAL$",    "" },
	{ "K",     "LIST",   "+",    "LEN",     "SCREEN$", "" },
	{ "L",     "LET",    "=",    "USR",     "ATTR",    "" },
	{ "ENTER", "",       "",     "",        "",        "" },

	{ "CSHFT", "",       "",     "",        "",        "" },
	{ "Z",     "COPY",   ":",    "LN",      "BEEP",    "" },
	{ "X",     "CLEAR",  "pnd.", "EXP",     "INK",     "" }, // XXX pound
	{ "C",     "CONT",   "?",    "LPRINT",  "PAPER",   "" },
	{ "V",     "CLS",    "/",    "LLIST",   "FLASH",   "" },
	{ "B",     "BORDER", "*",    "BIN",     "BRIGHT",  "" },
	{ "N",     "NEXT",   ",",    "INKEY5",  "OVER",    "" },
	{ "M",     "PAUSE",  ".",    "PI",      "INVERSE", "" },
	{ "SSHFT", "",       "",     "",        "",        "" },
	{ "SPACE", "",       "",     "",        "",        "BREAK" },
};

#define KEY_LETTER 0
#define KEY_KEYWORD 1
#define KEY_SSHIFT 2
#define KEY_EXTEND 3
#define KEY_SHEXTEND 4
#define KEY_CSHIFT 5

#define KEY_HPAD 1
#define KEY_HMARGIN 4
#define KEY_VPITCH 40

static bool end_help;

static int key_x0[4] = {
	5,
	15,
	25,
	5
};

static void kbdhelp_draw(void)
{
	int i, j, k;
	unsigned w, maxw;
	unsigned x;
	const char **key;

	mgfx_selln(3); /* Enable rendering odd and even lines */

	mgfx_fillrect(0, 0, scr_xs - 1, scr_ys - 1, 7);

	gmovec(10, 1);
	fgc = 0;
	bgc = 7;
	gputs("ZX Spectrum Keyboard");

	gmovec(10, 23);
	fgc = 0;
	bgc = 7;
	gputs("Press Escape to return");

	bgc = 0;
	for (j = 0; j < 4; j++) {
		x = key_x0[j];
		for (i = 0; i < 10; i++) {
			key = key_text[j * 10 + i];
			fgc = 7;

			maxw = 0;
			for (k = 0; k < 6; k++) {
				w = fnt_swidth(fnt, key[k]);
				if (w > maxw)
					maxw = w;
			}

			w = fnt_swidth(fnt, key[KEY_LETTER]) +
			    fnt_swidth(fnt, key[KEY_SSHIFT]);
			if (w > maxw)
				maxw = w;

			mgfx_fillrect(x, 3 * 8 + j * KEY_VPITCH - 2, x + maxw,
			    7 * 8 + j * KEY_VPITCH, 0);

			x += KEY_HPAD;

			gmove(x, 4 * 8 + j * KEY_VPITCH);
			fgc = 6;
			fnt_puts(fnt, key[KEY_LETTER]);
			fgc = 2;
			fnt_puts(fnt, key[KEY_SSHIFT]);

			gmove(x, 5 * 8 + j * KEY_VPITCH);
			fgc = 7;
			fnt_puts(fnt, key[KEY_KEYWORD]);

			gmove(x, 5 * 8 + j * KEY_VPITCH);
			fgc = 7;
			fnt_puts(fnt, key[KEY_CSHIFT]);

			gmove(x, 3 * 8 + j * KEY_VPITCH);
			if (j == 0 && i < 8)
				fgc = 1 + i;
			else
				fgc = 4;
			fnt_puts(fnt, key[KEY_EXTEND]);

			gmove(x, 6 * 8 + j * KEY_VPITCH);
			fgc = 2;
			fnt_puts(fnt, key[KEY_SHEXTEND]);

			x += maxw - KEY_HPAD + KEY_HMARGIN;
		}
	}
}

static void kbdhelp_key(wkey_t *k)
{
	switch (k->key) {
	case WKEY_ESC:
		end_help = true;
		break;
	default:
		break;
	}
}

void kbdhelp(void)
{
	wkey_t k;

	end_help = false;
	while (!end_help) {
		kbdhelp_draw();
		mgfx_updscr();

		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			kbdhelp_key(&k);
	}
}
