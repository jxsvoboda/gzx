/*
 * GZX - George's ZX Spectrum Emulator
 * Hardware options menu
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
#include "menu.h"
#include "model.h"

static void model_next_opt(int l);

#define MODEL_NENT 4

static const char *model_text[MODEL_NENT] = {
	"~48K",
	"~128K",
	"+~2",
	"+2~A"
};

static int model_keys[MODEL_NENT] = {
	WKEY_4, WKEY_1, WKEY_2, WKEY_A
};

static void model_run_line(int l)
{
	switch (l) {
	case 0:
		zx_select_memmodel(ZXM_48K);
		zx_reset();
		break;
	case 1:
		zx_select_memmodel(ZXM_128K);
		zx_reset();
		break;
	case 2:
		zx_select_memmodel(ZXM_PLUS2);
		zx_reset();
		break;
	case 3:
		zx_select_memmodel(ZXM_PLUS2A);
		zx_reset();
		break;
	}
}

static void model_prev_opt(int l)
{
}

static void model_next_opt(int l)
{
}

static const char *model_get_opt(int l)
{
	return NULL;
}

static menu_t model_menu_spec = {
	.caption = "Select Model",
	.nent = MODEL_NENT,
	.mentry_text = model_text,
	.mkeys = model_keys,
	.run_line = model_run_line,
	.prev_opt = model_prev_opt,
	.next_opt = model_next_opt,
	.get_opt = model_get_opt
};

/** Hardware options menu */
void model_menu(void)
{
	menu_run(&model_menu_spec, mem_model);
}
