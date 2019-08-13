/*
 * GZX - George's ZX Spectrum Emulator
 * Hardware options menu
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

#include <stdlib.h>

#include "../mgfx.h"
#include "../zx.h"
#include "menu.h"
#include "hwopts.h"

static void hwopts_next_opt(int l);

#define HWOPTS_NENT 1

static const char *hwopts_text[HWOPTS_NENT] = {
	"~Kempston Joy."
};

static int hwopts_keys[HWOPTS_NENT] = {
	WKEY_K
};

static void hwopts_run_line(int l)
{
	hwopts_next_opt(l);
}

static void hwopts_prev_opt(int l)
{
	switch (l) {
	case 0:
		kjoy0_enable = !kjoy0_enable;
		break;
	}
}

static void hwopts_next_opt(int l)
{
	switch (l) {
	case 0:
		kjoy0_enable = !kjoy0_enable;
		break;
	}
}

static const char *hwopts_get_opt(int l)
{
	switch (l) {
	case 0:
		return kjoy0_enable ? "On" : "Off";
	default:
		return NULL;
	}
}

static menu_t hwopts_menu_spec = {
	.caption = "Hardware Options",
	.nent = HWOPTS_NENT,
	.mentry_text = hwopts_text,
	.mkeys = hwopts_keys,
	.run_line = hwopts_run_line,
	.prev_opt = hwopts_prev_opt,
	.next_opt = hwopts_next_opt,
	.get_opt = hwopts_get_opt
};

/** Hardware options menu */
void hwopts_menu(void)
{
	menu_run(&hwopts_menu_spec);
}
