/*
 * GZX - George's ZX Spectrum Emulator
 * I/O recording (output only)
 *
 * Copyright (c) 1999-2018 Jiri Svoboda
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileutil.h"
#include "iorec.h"

int iorec_open(const char *fname, iorec_t **riorec)
{
	iorec_t *iorec;

	iorec = calloc(1, sizeof(iorec_t));
	if (iorec == NULL)
		return ENOMEM;

	printf("I/O recording start to %s\n", fname);

	iorec->iorf = fopen(fname, "wb");
	if (iorec->iorf == NULL) {
		printf("Failed opening file.\n");
		free(iorec);
		return EIO;
	}

	iorec->have_tick = 0;
	*riorec = iorec;
	return 0;
}

int iorec_close(iorec_t *iorec)
{
	int rc;

	printf("I/O recording stop\n");

	rc = fclose(iorec->iorf);
	if (rc < 0) {
		printf("Failed closing file.\n");
		return EIO;
	}

	free(iorec);
	return 0;
}

static void fputvlc(FILE *f, unsigned long val)
{
	uint8_t cur;
	uint8_t cont;

	do {
		cur = val & 0x7f;
		cont = (val & ~0x7f) != 0 ? 0x80 : 0x00;
		fputu8(f, cur | cont);
		val = val >> 7;
	} while (val != 0);
}

void iorec_out(iorec_t *iorec, unsigned long tick, uint16_t addr, uint8_t data)
{
	if (!iorec->have_tick) {
		iorec->last_tick = tick;
		iorec->have_tick = true;
	}

	printf("%lu,0x%04x,0x%02x\n", tick - iorec->last_tick, addr, data);
	fputvlc(iorec->iorf, tick - iorec->last_tick);
	fputu16le(iorec->iorf, addr);
	fputu8(iorec->iorf, data);

	iorec->last_tick = tick;
}
