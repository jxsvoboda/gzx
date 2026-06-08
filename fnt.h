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
#ifndef FNT_H
#define FNT_H

#include <stdint.h>

/** Font file header */
typedef struct {
	/** Identifier. 0x9000 if Mac-converted */
	uint16_t id;
	/** point size of font */
	uint16_t size;
	/** font face name */
	uint8_t facename[32];
	/** lowest character */
	uint16_t ade_lo;
	/** highest character */
	uint16_t ade_hi;
	uint16_t top_dist;
	uint16_t asc_dist;
	uint16_t hlf_dist;
	uint16_t des_dist;
	uint16_t bot_dist;
	/** widest character width */
	uint16_t wchr_wdt;
	/** widest character cell width */
	uint16_t wcell_width;
	uint16_t lft_ofst;
	uint16_t rgt_ofst;
	/** how bold is bold */
	uint16_t thcking;
	uint16_t undrline;
	/** lightening mask */
	uint16_t lghtng_m;
	/** skewing mask */
	uint16_t skewng_m;
	/** bit 2 != 0 -- swap byte order */
	uint16_t flags;
	uint32_t hz_ofst;
	uint32_t ch_ofst;
	uint32_t fnt_dta;
	/** byte width of bitmap */
	uint16_t frm_wdt;
	/** pixel height of bitmap */
	uint16_t frm_hgt;
	uint32_t nxt_fnt;
} fnt_file_hdr_t;

/** FNT proportional font */
typedef struct {
	/** header */
	fnt_file_hdr_t hdr;
	/** offset table */
	uint16_t *ofst;
	/** font bitmap */
	uint8_t *bitmap;
} fnt_font_t;

extern int fnt_font_load(const char *, fnt_font_t **);
extern void fnt_putc(fnt_font_t *, char);
extern unsigned fnt_cwidth(fnt_font_t *, char);
extern void fnt_puts(fnt_font_t *, const char *);
extern unsigned fnt_swidth(fnt_font_t *, const char *);

#endif
