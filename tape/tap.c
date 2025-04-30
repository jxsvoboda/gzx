/*
 * GZX - George's ZX Spectrum Emulator
 * Gerton Lunter's TAP file format support
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

/**
 * @file Gerton Lunter's TAP file format support
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../byteorder.h"
#include "defs.h"
#include "tape.h"
#include "tap.h"

/** Need to set some version corresponding to TZX */
enum {
	tap_ver_major = 1,
	tap_ver_minor = 1
};

/** Load data block.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success, ENOENT if we are at end of file, EIO on I/O error,
 *         ENOMEM if out of memory
 */
static int tap_load_data(FILE *f, tape_t *tape)
{
	tblock_data_t *data;
	uint16_t data_len;
	size_t nread;
	int rc;

	nread = fread(&data_len, 1, sizeof(data_len), f);
	if (nread != sizeof(data_len)) {
		/* Check for end of file */
		if (feof(f))
			return ENOENT;
		return EIO;
	}

	rc = tblock_data_create(&data);
	if (rc != 0)
		goto error;

	data->pause_after = ROM_PAUSE_LEN_MS;
	data->data_len = uint16_t_le2host(data_len);

	data->data = calloc(data->data_len, 1);
	if (data->data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	nread = fread(data->data, 1, data->data_len, f);
	if (nread != data->data_len) {
		rc = EIO;
		goto error;
	}

	tape_append(tape, data->block);
	return 0;
error:
	if (data != NULL)
		tblock_data_destroy(data);
	return rc;
}

/** Save data block.
 *
 * @param data Standard speed data block
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tap_save_data(tblock_data_t *data, FILE *f)
{
	size_t nwr;
	uint16_t data_len;

	data_len = host2uint16_t_le(data->data_len);

	nwr = fwrite(&data_len, 1, sizeof(data_len), f);
	if (nwr != sizeof(data_len))
		return EIO;

	nwr = fwrite(data->data, 1, data->data_len, f);
	if (nwr != data->data_len)
		return EIO;

	return 0;
}

/** Load tape from tap file.
 *
 * @param fname File name
 * @param rtape Place to store pointer to loaded tape
 * @return Zero on success or error code
 */
int tap_tape_load(const char *fname, tape_t **rtape)
{
	FILE *f;
	tape_t *tape = NULL;
	int rc;

	f = fopen(fname, "rb");
	if (f == NULL)
		return ENOENT;

	rc = tape_create(&tape);
	if (rc != 0)
		goto error;

	tape->version.major = tap_ver_major;
	tape->version.minor = tap_ver_minor;

	while (true) {
		rc = tap_load_data(f, tape);
		if (rc != 0) {
			if (rc == ENOENT)
				break;
			goto error;
		}
	}

	fclose(f);
	*rtape = tape;
	return 0;
error:
	if (tape != NULL)
		tape_destroy(tape);
	fclose(f);
	return rc;
}

/** Save tape to tap file.
 *
 * @param tape Tape
 * @param fname File name
 * @return Zero on success or error code
 */
int tap_tape_save(tape_t *tape, const char *fname)
{
	FILE *f;
	tape_block_t *block;
	int rc;
	int rv;

	f = fopen(fname, "wb");
	if (f == NULL)
		return ENOENT;

	block = tape_first(tape);
	while (block != NULL) {
		switch (block->btype) {
		case tb_data:
			rc = tap_save_data((tblock_data_t *) block->ext, f);
			break;
		default:
			rc = ENOTSUP;
			goto error;
		}

		if (rc != 0)
			goto error;

		block = tape_next(block);
	}

	rv = fclose(f);
	if (rv < 0) {
		rc = EIO;
		goto error;
	}

	return 0;
error:
	fclose(f);
	return rc;
}
