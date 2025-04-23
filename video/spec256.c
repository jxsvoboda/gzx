/*
 * GZX - George's ZX Spectrum Emulator
 * Spec256 video generator
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "../clock.h"
#include "../gzx.h"
#include "../memio.h"
#include "../sys_all.h"
#include "../xtrace.h"
#include "../z80g.h"
#include "out.h"
#include "spec256.h"

/*
 * Spec256 is designed for VGA's 320x200x256 mode and that's the resolution
 * of the backgrounds (even though I've not seen one that would use the
 * area outside of the Spectrum's 256x192 paper area).
 *
 * We push the emulated Spec256 video through the same 352x288 video field as
 * for ULA.
 */
enum {
	/** Width of Spec256 background */
	spec256_img_w = 320,
	/** Height of Spec256 background */
	spec256_img_h = 200,
	/** X coordinate of top-left corner of Spec256 BG within video field */
	spec256_bg_paper_x0 = spec256_img_w / 2 - zx_paper_w / 2,
	/** Y coordinate of top-left corner of Spec256 BG within video field */
	spec256_bg_paper_y0 = spec256_img_h / 2 - zx_paper_h / 2
};

/** Flip video address bits as ULA does.
 *
 * @param ofs Address or offset within video page
 */
static uint16_t vxswapb(uint16_t ofs)
{
	return (ofs & 0xf81f) | ((ofs & 0x00e0) << 3) | ((ofs & 0x0700) >> 3);
}

/** Display a Spec256 paper element.
 *
 * @param spec Spec256 video generator
 * @param x Column (0-31)
 * @param y Line (0-191)
 */
static void video_spec256_disp_fast_elem(video_spec256_t *spec, int x, int y)
{
	int i, j;
	uint8_t b;
	uint8_t color;
	uint16_t offs;
	uint16_t bgoff;

	offs = vxswapb(y * 32 + x);

	for (i = 0; i < 8; i++) {
		b = 0;
		for (j = 0; j < 8; j++) {
			if (gfxscr[j][offs] & (1 << (7 - i)))
				b |= (1 << j);
		}
		if (b != 0) {
			color = b;
		} else {
			if (spec->cur_bg >= 0) {
				bgoff = (spec256_bg_paper_y0 + y) * spec256_img_w +
				    spec256_bg_paper_x0 + x * 8 + i;
				color = spec->background[spec->cur_bg][bgoff];
			} else {
				color = 0;
			}
		}

		video_out_pixel(spec->vout, zx_paper_x0 + x * 8 + i,
		    zx_paper_y0 + y, color);
	}
}

/** Crude and fast Spec256 display routine, called 50 times a second.
 *
 * @param spec Spec256 video generator
 *
 * This can be used to draw the screen instantly, but ignoring the fact
 * that the video is generated over time. Thus high-speed effects
 * such as tape loading stripes are not displayed correctly.
 */
void video_spec256_disp_fast(video_spec256_t *spec)
{
	int x, y;

	/*
	 * Draw border
	 */

	/* top + corners */
	video_out_rect(spec->vout, 0, 0, zx_field_w - 1, zx_paper_y0 - 1, border);

	/* bottom + corners */
	video_out_rect(spec->vout, 0, zx_paper_y1, zx_field_w - 1,
	    zx_field_h - 1, border);

	/* left */
	video_out_rect(spec->vout, 0, zx_paper_y0, zx_paper_x0 - 1,
	    zx_paper_y1, border);

	/* right */
	video_out_rect(spec->vout, zx_paper_x1, zx_paper_y0, zx_field_w - 1,
	    zx_paper_y1, border);

	/*
	 * Draw paper
	 */

	for (y = 0; y < 24 * 8; y++)
		for (x = 0; x < 32; x++)
			video_spec256_disp_fast_elem(spec, x, y);

	spec->clock += ULA_FIELD_TICKS;

#ifdef XTRACE
	xtrace_int();
#endif
	z80_int();

	if (gpu_is_on())
		z80_g_int();
}

/** Initialize Spec256 video generator.
 *
 * @param spec Spec256 video generator
 * @param vout Video output
 * @return Zero on success or an error code
 */
int video_spec256_init(video_spec256_t *spec, video_out_t *vout)
{
	spec->gfxpal = NULL;
	spec->vout = vout;
	spec->background = NULL;
	spec->cur_bg = -1;
	spec->clock = 0;

	return 0;
}

/** Initialize Spec256 video generator palette.
 *
 * @param spec Spec256 video generator
 * @param vout Video output
 * @return Zero on success or an error code
 */
int video_spec256_init_pal(video_spec256_t *spec)
{
	int b;
	int i;
	FILE *f = NULL;
	char *cur_dir = NULL;

	if (spec->gfxpal != NULL)
		return 0;

	cur_dir = sys_getcwd(NULL, 0);
	if (start_dir)
		sys_chdir(start_dir);

	f = fopen("sp256.pal", "rt");
	if (!f)
		goto error;

	spec->gfxpal = malloc(3 * 256);
	if (spec->gfxpal == NULL)
		goto error;

	for (i = 0; i < 3 * 256; i++) {
		fscanf(f, "%d", &b);
		spec->gfxpal[i] = b >> 2;
	}

	fclose(f);
	sys_chdir(cur_dir);
	free(cur_dir);
	return 0;
error:
	if (f != NULL)
		fclose(f);
	if (cur_dir != NULL) {
		sys_chdir(cur_dir);
		free(cur_dir);
	}
	return -1;
}

/** Load Spec256 background.
 *
 * Load a file containing a Spec256 background as raw 320x200x256 bitmap.
 * This gets laid under the actual pixels based on a color key.
 *
 * @param spec Spec256 video generator
 * @param fname Background file name
 * @param idx Index of background to load (>=0).
 * @return Zero on success or an error code
 */
int video_spec256_load_bg(video_spec256_t *spec, const char *fname, int idx)
{
	FILE *f;
	uint8_t **bgs;
	uint8_t *bg;

	f = fopen(fname, "rb");
	if (!f)
		return -1;
	if (spec->nbgs < idx + 1) {
		bg = malloc(64000);
		if (bg == NULL) {
			fclose(f);
			return -1;
		}
		bgs = realloc(spec->background, (idx + 1) * sizeof(uint8_t *));
		if (bgs == NULL) {
			free(bg);
			fclose(f);
			return -1;
		}

		spec->background = bgs;
		spec->nbgs = idx + 1;
		spec->background[idx] = bg;
	}

	fread(spec->background[idx], 1, 64000, f);
	fclose(f);
	printf("Loaded background '%s'\n", fname);
	if (spec->cur_bg == -1)
		spec->cur_bg = 0;
	return 0;
}

/** Switch to previous background.
 *
 * @param spec Spec256 video generator
 */
void video_spec256_prev_bg(video_spec256_t *spec)
{
	if (spec->cur_bg >= 0)
		--spec->cur_bg;
}

/** Switch to next background.
 *
 * @param spec Spec256 video generator
 */
void video_spec256_next_bg(video_spec256_t *spec)
{
	if (spec->cur_bg < spec->nbgs - 1)
		++spec->cur_bg;
}

/** Clear all backgrounds.
 *
 * @param spec Spec256 video generator
 */
void video_spec256_clear_bg(video_spec256_t *spec)
{
	int i;
	for (i = 0; i < spec->nbgs; i++) {
		if (spec->background[i] != NULL)
			free(spec->background[i]);
	}
	free(spec->background);
	spec->background = NULL;
	spec->cur_bg = -1;
	spec->nbgs = 0;
}

/** Set up palette for Spec256 video generator.
 *
 * Configure video output palette for use with Spec256 video.
 * This needs to be called explicitly when switching from a different
 * video generator.
 *
 * @param ula Spec256 video generator
 */
void video_spec256_setpal(video_spec256_t *spec)
{
	video_out_set_palette(spec->vout, 256, spec->gfxpal);
}

/** Get current Spec256 video clock.
 *
 * @param ula Spec256 video generator
 * @return Current clock value
 */
unsigned long video_spec256_get_clock(video_spec256_t *spec)
{
	return spec->clock;
}
