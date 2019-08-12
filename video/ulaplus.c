/*
 * GZX - George's ZX Spectrum Emulator
 * ULAplus
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

#include "ulaplus.h"

static uint8_t color_2_to_3bit(uint8_t);
static uint8_t color_3_to_8bit(uint8_t);

/** Initialize ULAplus.
 *
 * @param plus ULAplus
 */
void ulaplus_init(ulaplus_t *plus)
{
	int i;

	plus->selreg = 0;
	plus->mode = 0;
	for (i = 0; i < 64; i++)
		plus->pal[i] = 0;
}

/** Write to ULAplus register selection register.
 *
 * @param plus ULAplus
 * @param reg Register selection
 */
void ulaplus_write_regsel(ulaplus_t *plus, uint8_t reg)
{
	plus->selreg = reg;
}

/** Write data to pre-selected ULAplus register.
 *
 * @param plus ULAplus
 * @param data Data byte to write
 */
void ulaplus_write_data(ulaplus_t *plus, uint8_t data)
{
	switch (plus->selreg & ULAPLUS_GROUP) {
	case ULAPLUS_PAL_GROUP:
		plus->pal[plus->selreg & ULAPLUS_SUBGROUP] = data;
		break;
	case ULAPLUS_MODE_GROUP:
		plus->mode = data;
		break;
	default:
		break;
	}
}

/** Get RGB values for ULAplus palette entry.
 *
 * @param plus ULAplus
 * @param idx Color index (0 - 63)
 * @param rgb Place to store red, green, blue (0 - 255 each)
 */
void ulaplus_get_pal_rgb(ulaplus_t *plus, uint8_t idx, uint8_t *rgb)
{
	uint8_t clr;

	idx = idx & 0x3f;
	clr = plus->pal[idx];

	if (plus->mode & ULAPLUS_MODE_GRAYSCALE) {
		/* Grayscale mode */
		rgb[0] = rgb[1] = rgb[2] = clr;
	} else {
		/* Color mode */
		rgb[0] = color_3_to_8bit((clr >> 2) & 0x7);
		rgb[1] = color_3_to_8bit(clr >> 5);
		rgb[2] = color_3_to_8bit(color_2_to_3bit(clr & 0x3));
	}
}

/** Upscale 2-bit color value to 3-bit.
*
* @param c 2-bit color
* @return 3-bit color
*/
static uint8_t color_2_to_3bit(uint8_t c)
{
	/* Lowest bit becomes the or of the other bits */
	return (c << 1) | ((c != 0) ? 0x1 : 0x0);
}

/** Upscale 3-bit color value to 8-bit.
*
* @param c 3-bit color
* @return 8-bit color
*/
static uint8_t color_3_to_8bit(uint8_t c)
{
	/* Concatenate the bits repeatedly */
	return (c << 5) | (c << 2) | (c >> 1);
}
