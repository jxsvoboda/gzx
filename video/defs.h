/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum Screen
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
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
#ifndef VIDEO_DEFS_H
#define VIDEO_DEFS_H

#define ZX_PIXEL_START	0x0000
#define ZX_ATTR_START	0x1800
#define ZX_ATTR_END	0x1aff

enum {
	/** Border width */
	zx_border_w = 48,
	/** Border height */
	zx_border_h = 48,
	/** X coordinate of paper within 352x288 field */
	zx_paper_x0 = zx_border_w,
	/** Y coordinate of paper within 352x288 field */
	zx_paper_y0 = zx_border_h,
	/** Paper width in pixels */
	zx_paper_w = 256,
	/** Paper height in rows */
	zx_paper_h = 192,
	/** X coordinate of rightmost paper pixel */
	zx_paper_x1 = zx_paper_x0 + zx_paper_w - 1,
	/** Y coordinate of bottommost paper pixel */
	zx_paper_y1 = zx_paper_y0 + zx_paper_h - 1,
	/** Entire field width (352) */
	zx_field_w = zx_paper_x1 + zx_border_w,
	/** Entire field height (288) */
	zx_field_h = zx_paper_y1 + zx_border_h
};

#endif
