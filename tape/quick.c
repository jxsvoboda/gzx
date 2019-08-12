/*
 * GZX - George's ZX Spectrum Emulator
 * Tape support
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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../gzx.h"
#include "../memio.h"
#include "../z80.h"
#include "deck.h"
#include "defs.h"
#include "quick.h"
#include "tape.h"

/** Quick load.
 *
 * Emulate (most of) the ROM LD-BYTES routine using a virtual tape deck.
 * This can only load a standard speed data block.
 *
 * @param deck Source tape deck
 */
void tape_quick_ldbytes(tape_deck_t *deck)
{
	bool verify;
	uint8_t req_flag;
	uint16_t toload, addr;
	size_t u;
	uint8_t flag, b, chksum;
	uint8_t x = 0;
	tblock_data_t *data;
	tape_block_t *tblock;

	fprintf(logfi, "tape_quick_ldbytes()\n");

	fprintf(logfi, "!tape_playing?\n");
	if (tape_deck_is_playing(deck))
		return;

	tblock = tape_deck_cur_block(deck);
	while (tblock != NULL && tblock->btype != tb_data) {
		tape_deck_next(deck);
		tblock = tape_deck_cur_block(deck);
	}

	fprintf(logfi, "tblock?\n");
	if (!tblock)
		return;

	assert(tblock->btype == tb_data);
	data = (tblock_data_t *)tblock->ext;

	fprintf(logfi, "...\n");
	req_flag = cpus.r_[rA];
	toload = ((uint16_t)cpus.r[rD] << 8) | (uint16_t)cpus.r[rE];
	addr = cpus.IX;
	verify = (cpus.F_ & fC) == 0;

	if (data->data_len < 1) {
		printf("Data block too short.\n");
		goto error;
	}

	flag = data->data[0];

	fprintf(logfi, "req:len %d, flag 0x%02x, addr 0x%04x, verify:%d\n",
	    toload, req_flag, addr, verify);
	fprintf(logfi, "block len %u, block flag:0x%02x\n", data->data_len,
	    flag);
	fprintf(logfi, "z80 F:%02x\n", cpus.F_);

	if (flag != req_flag)
		goto error;

	if (verify)
		fprintf(logfi, "verifying\n");
	else
		fprintf(logfi, "loading\n");

	x = flag;
	for (u = 0; u < toload; u++) {
		if (1 + u >= data->data_len) {
			fprintf(logfi, "out of data\n");
			goto error;
		}

		b = data->data[1 + u];
		if (!verify)
			zx_memset8(addr + u, b);
		x ^= b;
	}

	if (1 + toload >= data->data_len) {
		fprintf(logfi, "out of data\n");
		goto error;
	}

	chksum = data->data[1 + toload];

	fprintf(logfi, "stored chksum:$%02x computed:$%02x\n", chksum, x);
	if (chksum != x) {
		fprintf(logfi, "wrong checksum\n");
		goto error;
	}

	cpus.F |= fC;
	fprintf(logfi, "load ok\n");
	goto common;
error:
	cpus.F &= ~fC;
	fprintf(logfi, "load error\n");
common:
	tape_deck_next(deck);

	/* RET */
	fprintf(logfi, "returning\n");
	cpus.PC = zx_memget16(cpus.SP);
	cpus.SP += 2;
}

/** Quick save.
 *
 * Emulate (most of) the ROM SA-BYTES routine using a virtual tape deck.
 * This produces a standard speed data block.
 *
 * @param deck Destination tape deck
 */
void tape_quick_sabytes(tape_deck_t *deck)
{
	uint8_t flag;
	uint16_t tosave;
	uint16_t addr;
	size_t u;
	uint8_t x, b;
	bool error;
	tblock_data_t *data = NULL;
	tape_block_t *cur;
	int rc;

	rc = tblock_data_create(&data);
	if (rc != 0) {
		printf("Out of memory\n");
		error = true;
		goto done;
	}

	flag = cpus.r_[rA];
	tosave = ((uint16_t)cpus.r[rD] << 8) | (uint16_t)cpus.r[rE];
	addr = cpus.IX;

	data->data_len = (size_t)tosave + 2;
	data->data = malloc(data->data_len);
	if (data->data == NULL) {
		printf("Out of memory\n");
		tblock_data_destroy(data);
		data = NULL;
		error = true;
		goto done;
	}

	data->data[0] = flag;

	fprintf(logfi, "wr:len %d, flag 0x%02x, addr 0x%04x\n",
	    tosave, flag, addr);

	error = false;

	fprintf(logfi, "writing\n");
	x = flag;
	for (u = 0; u < tosave; u++) {
		b = zx_memget8(addr + u);
		data->data[1 + u] = b;
		x ^= b;
	}

	/* Write checksum */
	data->data[1 + (size_t)tosave] = x;

done:
	cpus.F = error ? (cpus.F & (~fC)) : (cpus.F | fC);
	if (!error)
		fprintf(logfi, "write ok\n");

	/* RET */
	cpus.PC = zx_memget16(cpus.SP);
	cpus.SP += 2;

	if (data != NULL) {
		data->pause_after = ROM_PAUSE_LEN_MS;

		cur = tape_deck_cur_block(deck);
		if (cur != NULL) {
			/* Insert before current block */
			tape_insert_before(data->block, cur);
		} else {
			/* We're at the end of the tape, so append */
			tape_append(deck->tape, data->block);
		}
	}
}
