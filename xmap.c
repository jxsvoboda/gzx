/*
 * GZX - George's ZX Spectrum Emulator
 * Execution map
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
#include "xmap.h"
#include "z80.h"

#ifdef XMAP

static uint8_t xmap[8 * 1024];

void xmap_clear(void)
{
	unsigned u;

	for (u = 0; u < 8 * 1024; u++)
		xmap[u] = 0;
}

void xmap_mark(void)
{
	uint8_t mask;
	unsigned offs;

	mask = 1 << (cpus.PC & 7);
	offs = cpus.PC >> 3;
	xmap[offs] = xmap[offs] | mask;
}

void xmap_save(void)
{
	FILE *f;
	unsigned u, v;

	f = fopen("xmap.txt", "wt");
	for (u = 0; u < 8 * 1024; u++) {
		fprintf(f, "%04X ", u * 8);
		for (v = 0; v < 8; v++) {
			fputc((xmap[u] & (1 << v)) ? '1' : '0', f);
			fputc(v < 7 ? ',' : '\n', f);
		}
	}
	fclose(f);
}

#endif
