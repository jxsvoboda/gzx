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

#include <stdlib.h>

#include "../gzx.h"
#include "../memio.h"
#include "../mgfx.h"
#include "../video/ula.h"
#include "../z80g.h"
#include "../zx.h"
#include "../zx_scr.h"
#include "menu.h"
#include "hwopts.h"

static void hwopts_next_opt(int l);

#define HWOPTS_NENT 5

static const char *hwopts_text[HWOPTS_NENT] = {
	"~Model",
	"~AY-3-8192 PSG",
	"~Kempston Joy.",
	"~ULAplus",
	"Spec256 ~GPU"
};

static int hwopts_keys[HWOPTS_NENT] = {
	WKEY_M, WKEY_A, WKEY_K, WKEY_U, WKEY_G
};

static void hwopts_run_line(int l)
{
	hwopts_next_opt(l);
}

static void hwopts_prev_opt(int l)
{
	switch (l) {
	case 0:
		if (mem_model > ZXM_48K) {
			zx_select_memmodel(mem_model - 1);
			zx_reset();
		}
		break;
	case 1:
		ay0_enable = !ay0_enable;
		break;
	case 2:
		kjoy0_enable = !kjoy0_enable;
		break;
	case 3:
		video_ula_enable_plus(&video_ula, !video_ula.plus_enable);
		break;
	case 4:
		gpu_set_allow(!gpu_allow);
		break;
	}
}

static void hwopts_next_opt(int l)
{
	switch (l) {
	case 0:
		if (mem_model < ZXM_PLUS3) {
			zx_select_memmodel(mem_model + 1);
			zx_reset();
		}
		break;
	case 1:
		ay0_enable = !ay0_enable;
		break;
	case 2:
		kjoy0_enable = !kjoy0_enable;
		break;
	case 3:
		video_ula_enable_plus(&video_ula, !video_ula.plus_enable);
		break;
	case 4:
		gpu_set_allow(!gpu_allow);
		break;
	}
}

static const char *hwopts_model_str(int model)
{
	switch (model) {
	case ZXM_48K:
		return "48K";
	case ZXM_128K:
		return "128K";
	case ZXM_PLUS2:
		return "+2";
	case ZXM_PLUS3:
		return "+3";
	default:
		return NULL;
	}
}

static const char *hwopts_get_opt(int l)
{
	switch (l) {
	case 0:
		return hwopts_model_str(mem_model);
	case 1:
		return ay0_enable ? "On" : "Off";
	case 2:
		return kjoy0_enable ? "On" : "Off";
	case 3:
		return video_ula.plus_enable ? "On" : "Off";
	case 4:
		return gpu_allow ? "Auto" : "Off";
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
