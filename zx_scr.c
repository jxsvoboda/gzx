/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum Screen
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

#include "video/spec256.h"
#include "video/ula.h"
#include "mgfx.h"
#include "zx_scr.h"

#ifdef USE_GPU
static void g_scr_disp_fast(void);
#endif

static void n_scr_disp_fast(void);
static void n_scr_disp(void);

void (*zx_scr_disp_fast)(void) = n_scr_disp_fast;
void (*zx_scr_disp)(void)      = n_scr_disp;

static video_ula_t video_ula;

#ifdef USE_GPU
static video_spec256_t video_spec256;
#endif

/* crude and fast display routine, called 50 times a second */
static void n_scr_disp_fast(void)
{
	video_ula_disp_fast(&video_ula);
}

#ifdef USE_GPU
static void g_scr_disp_fast(void)
{
	video_spec256_disp_fast(&video_spec256);
}
#endif

/* slow and fine display routine, called after each instruction! */
static void n_scr_disp(void)
{
	video_ula_disp(&video_ula);
}

void zx_scr_mode(int mode)
{
#ifdef USE_GPU
	if (mode) {
		zx_scr_disp_fast = g_scr_disp_fast;
		video_spec256_setpal(&video_spec256);
	} else {
#endif
		zx_scr_disp_fast = n_scr_disp_fast;
		video_ula_setpal(&video_ula);
#ifdef USE_GPU
	}
#endif
}

int zx_scr_init(unsigned long clock)
{
	if (mgfx_init())
		return -1;

	if (video_ula_init(&video_ula, clock))
		return -1;

#ifdef USE_GPU
	if (video_spec256_init(&video_spec256))
		return -1;
#endif

	return 0;
}

int zx_scr_load_bg(const char *fname)
{
#ifdef USE_GPU
	return video_spec256_load_bg(&video_spec256, fname);
#else
	return 0;
#endif
}

void zx_scr_clear_bg(void)
{
#ifdef USE_GPU
	video_spec256_clear_bg(&video_spec256);
#endif
}

unsigned long zx_scr_get_clock(void)
{
	return video_ula_get_clock(&video_ula);
}
