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
#include "../mgfx.h"
#include "out.h"

/** Render rectangle to video output.
 *
 * @param vout Video output
 * @param x0 X coordinate of top-left corner
 * @param y0 Y coordinate of top-left corner
 * @param x1 X coordinate of bottom-right corner
 * @param y1 Y coordinate of bottom-right corner
 * @param color Color
 */
void video_out_rect(video_out_t *vout, int x0, int y0, int x1, int y1,
    uint8_t color)
{
	mgfx_fillrect(vout->x0 + x0, vout->y0 + y0, vout->x0 + x1,
	    vout->y0 + y1, color);
}

/** Render pixel to video output.
 *
 * @param vout Video output
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color
 */
void video_out_pixel(video_out_t *vout, int x, int y, uint8_t color)
{
	mgfx_setcolor(color);
	mgfx_drawpixel(vout->x0 + x, vout->y0 + y);
}

/** Signal end of current field.
 *
 * Should be called after rendering the entire field.
 *
 * @param vout Video output
 */
void video_out_end_field(video_out_t *vout)
{
	vout->field_no ^= 1;

	/* Enable rendering odd/even lines for the next field */
	mgfx_selln(1 << vout->field_no);
}

/** Set the color palette.
 *
 * This is used to set the entire color palette at once. This replaces
 * any previous color palette entirely (even if less colors are used).
 *
 * @param vout Video output
 * @param ncolors Number of colors in palette
 * @param pal Pallete, ncolors RGB triplets
 */
void video_out_set_palette(video_out_t *vout, int ncolors, uint8_t *pal)
{
	int ipal[3 * 256];
	int i;

	for (i = 0; i < 3 * ncolors; i++)
		ipal[i] = pal[i];

	mgfx_setpal(0, ncolors, ipal);
}
