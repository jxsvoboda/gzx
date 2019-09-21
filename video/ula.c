/*
 * GZX - George's ZX Spectrum Emulator
 * ULA video generator
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "../clock.h"
#include "../memio.h"
#include "../xtrace.h"
#include "../z80.h"
#include "out.h"
#include "ula.h"
#include "ulaplus.h"

#include "../z80g.h"

/* 64 scanline times pass before paper starts - 48 lines of border are displayed */
#define SCR_SCAN_TOP     16
/* 16 skip + 48 top border+192 screen+48 bottom border */
#define SCR_SCAN_BOTTOM  304
/* 48 left border + 256 screen + 48 right b. */
#define SCR_SCAN_RIGHT   352

#define PLUS_PAL_BASE 16

/* ULA palette, taken from X128 */
static const int zxpal[3 * 16] = {
	0,  0,  0,    0,  0,  159,    223,  0,  0,      224,  0,  176,
	0, 208, 0,    0, 208, 208,    207, 207,  0,     192, 192, 192,

	0,  0,  0,    0,  0,  175,    239,  0,  0,      255,  0,  223,
	0, 239, 0,    0, 255, 255,    255, 255,  0,     255, 255, 255
};

static void video_ula_next_field(video_ula_t *);

/** Flip video address bits as ULA does.
 *
 * @param ofs Address or offset within video page
 */
static uint16_t vxswapb(uint16_t ofs)
{
	return (ofs & 0xf81f) | ((ofs & 0x00e0) << 3) | ((ofs & 0x0700) >> 3);
}

/** Convert attribute to foreground and background color.
 *
 * Based on attributes (background, foreground, bright, flash) and ULA
 * flash counter, produce background and foreground colors suitable for
 * video output.
 *
 * @param ula ULA video generator
 * @param attr Attribute
 * @param fgc Place to store video-out foreground color
 * @param bgc Place to store video-out background color
 */
static void video_ula_attr_to_colors(video_ula_t *ula, uint8_t attr,
    uint8_t *fgc, uint8_t *bgc)
{
	uint8_t rev;
	uint8_t br;
	uint8_t fg;
	uint8_t bg;
	uint8_t pal;

	if ((ula->plus.mode & ULAPLUS_MODE_PALETTE) == 0) {
		/* Standard mode */
		rev = (attr >> 7) && ula->fl_rev;
		br = (attr >> 6) & 1;
		fg = (attr & 7) | (br << 3);
		bg = ((attr >> 3) & 7) | (br << 3);

		if (rev == 0) {
			*fgc = fg;
			*bgc = bg;
		} else {
			*fgc = bg;
			*bgc = fg;
		}
	} else {
		/* ULAplus palette mode */
		pal = attr >> 6;
		*fgc = PLUS_PAL_BASE + pal * 16 + (attr & 0x7);
		*bgc = PLUS_PAL_BASE + pal * 16 + 8 + ((attr >> 3) & 0x7);
	}
}

/** Start generating next video field.
 *
 * @param ula ULA video generator
 *
 * The video signal has 50 fields per second.
 */
static void video_ula_next_field(video_ula_t *ula)
{
	video_out_end_field(ula->vout);

	ula->clock = 0;
	ula->cbase += ULA_FIELD_TICKS;

	++ula->field_no;
	if (ula->field_no >= 2) {
		ula->field_no = 0;
		++ula->frame_no;
	}

	/* ULA swaps fg/bg colours every 16/50th of a second when flashing */
	if (ula->frame_no >= 16) {
		ula->frame_no = 0;
		ula->fl_rev = !ula->fl_rev;
	}

#ifdef XTRACE
	xtrace_int();
#endif
	z80_int();

	if (gpu_is_on())
		z80_g_int();
}

/** Crude and fast ULA display routine, called 50 times a second.
 *
 * @param ula ULA video generator
 *
 * This can be used to draw the screen instantly, but ignoring the fact
 * that the video is generated over time. Thus high-speed effects
 * such as tape loading stripes are not displayed correctly.
 */
void video_ula_disp_fast(video_ula_t *ula)
{
	int x, y, xx, yy;
	uint8_t attr;
	uint8_t a, b, fgc, bgc;

	/*
	 * Draw border
	 */

	/* top + corners */
	video_out_rect(ula->vout, 0, 0, zx_field_w - 1, zx_paper_y0 - 1, border);

	/* bottom + corners */
	video_out_rect(ula->vout, 0, zx_paper_y1, zx_field_w - 1,
	    zx_field_h - 1, border);

	/* left */
	video_out_rect(ula->vout, 0, zx_paper_y0, zx_paper_x0 - 1,
	    zx_paper_y1, border);

	/* right */
	video_out_rect(ula->vout, zx_paper_x1, zx_paper_y0,
	    zx_field_w - 1, zx_paper_y1, border);

	/*
	 * Draw paper
	 */

	for (y = 0; y < 24; y++) {
		for (x = 0; x < 32; x++) {
			attr = zxscr[ZX_ATTR_START + y * 32 + x];
			video_ula_attr_to_colors(ula, attr, &fgc, &bgc);

			for (yy = 0; yy < 8; yy++) {
				a = zxscr[ZX_PIXEL_START +
				    vxswapb((y * 8 + yy) * 32 + x)];
				for (xx = 0; xx < 8; xx++) {
					b = (a & 0x80);
					video_out_pixel(ula->vout,
					    zx_paper_x0 + x * 8 + xx,
					    zx_paper_y0 + y * 8 + yy,
					    b ? fgc : bgc);
					a <<= 1;
				}
			}
		}
	}

	video_ula_next_field(ula);
}

/** Display paper element.
 *
 * Displays a flight of eight pixels of paper.
 *
 * @param ula ULA video generator
 * @param x X coordinate of leftmost pixel (divisible by 8)
 * @param y Y coordinate
 */
static void scr_dispscrelem(video_ula_t *ula, int x, int y)
{
	uint8_t attr;
	uint8_t pix;
	uint8_t fgc, bgc;
	uint8_t color;
	uint8_t col;
	uint8_t line;
	int i;

	col = (x - zx_paper_x0) >> 3;
	line = y - zx_paper_y0;

	attr = zxscr[ZX_ATTR_START + (line >> 3) * 32 + col];
	pix = zxscr[ZX_PIXEL_START + vxswapb(line * 32 + col)];

	/*
	 * In reality attr/pix are read at different times and we can get
	 * either as the bus byte.
	 */
	ula->idle_bus_byte = attr;

	video_ula_attr_to_colors(ula, attr, &fgc, &bgc);

	for (i = 0; i < 8; i++) {
		color = (pix & 0x80) ? fgc : bgc;
		video_out_pixel(ula->vout, x + i, y, color);
		pix <<= 1;
	}
}

/** Display border element.
 *
 * Displays a flight of eight border pixels.
 *
 * @param ula ULA video generator
 * @param x X coordinate of leftmost pixel
 * @param y Y coordinate of pixels
 */
static void scr_dispbrdelem(video_ula_t *ula, int x, int y)
{
	video_out_rect(ula->vout, x, y, x + 7, y, border);
	ula->idle_bus_byte = 0xff;
}

/** Slow and fine display routine, called after each instruction.
 *
 * @param ula ULA video generator
 */
void video_ula_disp(video_ula_t *ula)
{
	unsigned line, x, y;

	line = (ula->clock + zx_paper_x0) / 224;
	x = ((ula->clock + zx_paper_x0) % 224) * 2;
	if (line >= SCR_SCAN_TOP && line < SCR_SCAN_BOTTOM && x < SCR_SCAN_RIGHT) {
		y = line - SCR_SCAN_TOP;
		if (x >= zx_paper_x0 && x <= zx_paper_x1 && y >= zx_paper_y0 &&
		    y <= zx_paper_y1) {
			scr_dispscrelem(ula, x, y);
		} else {
			scr_dispbrdelem(ula, x, y);
		}
	}

	ula->clock += 4;
	if (ula->clock >= ULA_FIELD_TICKS)
		video_ula_next_field(ula);
}

/** Initialize ULA video generator.
 *
 * @param ula ULA video generator
 * @param clock Initial clock value
 * @param vout Video output
 * @return EOK on success or an error code
 */
int video_ula_init(video_ula_t *ula, unsigned long clock, video_out_t *vout)
{
	ula->vout = vout;

	ula->clock = 0;
	ula->cbase = clock;
	ula->plus_enable = true;

	video_ula_reset(ula);
	return 0;
}

/** Reset ULA video generator.
 *
 * @param ula ULA video generator
 */
void video_ula_reset(video_ula_t *ula)
{
	ulaplus_init(&ula->plus);
	video_ula_setpal(ula);
}

/** Set up palette for ULA video generator.
 *
 * Configure video output palette for use with ULA video.
 * This needs to be called explicitly when switching from a different
 * video generator.
 *
 * @param ula ULA video generator
 */
void video_ula_setpal(video_ula_t *ula)
{
	int i;
	uint8_t pal[3 * 16 + 3 * 64];
	uint8_t rgb[3];
	int ncolors;

	for (i = 0; i < 3 * 16; i++)
		pal[i] = zxpal[i] >> 2;

	ncolors = 16;

	if ((ula->plus.mode & ULAPLUS_MODE_PALETTE) != 0) {
		for (i = 0; i < 64; i++) {
			ulaplus_get_pal_rgb(&ula->plus, i, rgb);
			pal[3 * (PLUS_PAL_BASE + i)] = rgb[0] >> 2;
			pal[3 * (PLUS_PAL_BASE + i) + 1] = rgb[1] >> 2;
			pal[3 * (PLUS_PAL_BASE + i) + 2] = rgb[2] >> 2;
		}
		ncolors += 64;
	}

	video_out_set_palette(ula->vout, ncolors, pal);
}

/** Get current ULA video clock.
 *
 * @param ula ULA video generator
 * @return Current clock value
 */
unsigned long video_ula_get_clock(video_ula_t *ula)
{
	return ula->cbase + ula->clock;
}

void video_ula_enable_plus(video_ula_t *ula, bool enable)
{
	ula->plus_enable = enable;
	if (!enable)
		ulaplus_init(&ula->plus);
}
