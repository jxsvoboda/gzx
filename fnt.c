/*
 * GZX - George's ZX Spectrum Emulator
 * FNT proportional fonts
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

#include <stdio.h>
#include <stdlib.h>
#include "byteorder.h"
#include "fnt.h"
#include "mgfx.h"

/** Load proportional font.
 *
 * @param path Path to '.fnt' file
 * @param rfnt Place to store pointer to font
 * @return Zero on success, non-zero on failure
 */
int fnt_font_load(const char *path, fnt_font_t **rfnt)
{
	fnt_font_t *fnt;
	FILE *f = NULL;
	unsigned nchars;
	unsigned bmsize;
	size_t nr;

	fnt = calloc(1, sizeof(fnt_font_t));
	if (fnt == NULL)
		goto error;

	f = fopen(path, "rb");
	if (f == NULL)
		goto error;

	/* load font header */
	nr = fread(&fnt->hdr, 1, sizeof(fnt->hdr), f);
	if (nr != sizeof(fnt->hdr))
		goto error;

	/* load offset table */
	nchars = uint16_t_le2host(fnt->hdr.ade_hi) -
	    uint16_t_le2host(fnt->hdr.ade_lo) + 1;
	fnt->ofst = calloc(2, nchars + 1);
	if (fnt->ofst == NULL)
		goto error;

	nr = fread(fnt->ofst, 2, nchars + 1, f);
	if (nr != nchars + 1)
		goto error;

	/* skip Mac table */
	if (uint16_t_le2host(fnt->hdr.id) == 0x9000) {
		if (fseek(f, 2 * nchars, SEEK_CUR) < 0)
			goto error;
	}

	/* load bitmap */
	bmsize = uint16_t_le2host(fnt->hdr.frm_wdt) *
	    uint16_t_le2host(fnt->hdr.frm_hgt);

	fnt->bitmap = calloc(1, bmsize);
	if (fnt->bitmap == NULL)
		goto error;

	nr = fread(fnt->bitmap, 1, bmsize, f);
	if (nr != bmsize)
		goto error;

	fclose(f);
	*rfnt = fnt;
	return 0;
error:
	if (f != NULL)
		fclose(f);
	if (fnt != NULL) {
		if (fnt->bitmap != NULL)
			free(fnt->bitmap);
		if (fnt->ofst != NULL)
			free(fnt->ofst);
		free(fnt);
	}
	return -1;
}

/** Render character and advance screen position.
 *
 * @param fnt Font
 * @param c Character
 */
void fnt_putc(fnt_font_t *fnt, char c)
{
	int x0, x, y;
	int xmax;
	unsigned char uc;
	uint8_t *bp;
	uint8_t b;
	int bit;

	uc = (unsigned char)c;

	if (uc < uint16_t_le2host(fnt->hdr.ade_lo) ||
	    uc > uint16_t_le2host(fnt->hdr.ade_hi))
		return;

	x0 = fnt->ofst[uc - uint16_t_le2host(fnt->hdr.ade_lo)];
	xmax = fnt->ofst[uc - uint16_t_le2host(fnt->hdr.ade_lo) + 1];

	for (y = 0; y < uint16_t_le2host(fnt->hdr.frm_hgt); y++) {

		bit = x0 % 8;
		bp = &fnt->bitmap[y * uint16_t_le2host(fnt->hdr.frm_wdt) +
		    x0 / 8];
		b = *bp++;
		b <<= bit;
		for (x = x0; x < xmax; x++) {
			mgfx_setcolor((b & 0x80) ? fgc : bgc);
			mgfx_drawpixel(gdx + x - x0, gdy + y);
			b <<= 1;
			++bit;
			if (bit >= 8) {
				bit = 0;
				b = *bp++;
			}
		}
	}

	gdx += xmax - x0;
}

/** Determine character width.
 *
 * @param fnt Proportional font
 * @param c Character
 * @return Character width in pixels
 */
unsigned fnt_cwidth(fnt_font_t *fnt, char c)
{
	int x0;
	int xmax;
	unsigned char uc;

	uc = (unsigned char)c;

	if (uc < uint16_t_le2host(fnt->hdr.ade_lo) ||
	    uc > uint16_t_le2host(fnt->hdr.ade_hi))
		return 0;

	x0 = fnt->ofst[uc - uint16_t_le2host(fnt->hdr.ade_lo)];
	xmax = fnt->ofst[uc - uint16_t_le2host(fnt->hdr.ade_lo) + 1];

	return xmax - x0;
}

/** Render string and advance screen position.
 *
 * @param fnt Font
 * @param String
 */
void fnt_puts(fnt_font_t *fnt, const char *s)
{
	while (*s)
		fnt_putc(fnt, *s++);
}

/** Determine string width.
 *
 * @param fnt Proportional font
 * @param s String
 * @return String width in pixels
 */
unsigned fnt_swidth(fnt_font_t *fnt, const char *s)
{
	unsigned w = 0;

	printf("fmt_swidth('%s')\n", s);
	while (*s)
		w += fnt_cwidth(fnt, *s++);
	return w;
}
