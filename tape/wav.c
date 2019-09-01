/*
 * GZX - George's ZX Spectrum Emulator
 * WAV as tape file format support
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

/**
 * @file WAV as tape file format support
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../byteorder.h"
#include "../clock.h"
#include "../wav/rwave.h"
#include "tape.h"
#include "wav.h"

enum {
	wav_buf_size = 4096
};

/** Need to set some version corresponding to TZX */
enum {
	wav_ver_major = 1,
	wav_ver_minor = 1
};

/** Load direct recording block.
 *
 * @param wr RIFF file to read from
 * @param params RIFF file parameters
 * @param tape to add block to
 * @return Zero on success, ENOENT if we are at end of file, EIO on I/O error,
 *         ENOMEM if out of memory
 */
static int wav_load_direct_rec(rwaver_t *wr, rwave_params_t *params,
    tape_t *tape)
{
	tblock_direct_rec_t *drec;
	uint32_t data_len;
	int lb_bits;
	uint8_t *buf = NULL;
	int16_t *bp16;
	uint8_t *dptr;
	size_t nread;
	size_t smp_read;
	size_t to_read;
	size_t i;
	size_t di;
	size_t bytes_smp;
	size_t wb_obytes;
	int dbitn;
	uint8_t bit;
	int rc;

	if (params->channels != 1) {
		printf("WAV file has != 1 channels\n");
		return ENOTSUP;
	}

	if (params->bits_smp != 8 && params->bits_smp != 16) {
		printf("WAV file bits/smp != 8 or 16\n");
		return ENOTSUP;
	}

	bytes_smp = params->bits_smp / 8;

	printf("load direct recording block\n");
	buf = calloc(1, wav_buf_size);
	if (buf == NULL) {
		rc = ENOMEM;
		goto error;
	}

	bp16 = (int16_t *) buf;

	rc = tblock_direct_rec_create(&drec);
	if (rc != 0)
		goto error;

	if (params->smp_freq < Z80_CLOCK / 0xffff) {
		rc = ENOTSUP;
		goto error;
	}

	if (Z80_CLOCK / params->smp_freq < 1) {
		rc = ENOTSUP;
		goto error;
	}

	drec->smp_dur = Z80_CLOCK / params->smp_freq;
	drec->pause_after = 0;
	drec->data = NULL;
	drec->data_len = 0;

	data_len = 0;
	lb_bits = 0;
	while (true) {
		/* Prevent exceeding maximum direct rec. block size */
		wb_obytes = (wav_buf_size / bytes_smp + 7) / 8;
		if (data_len + wb_obytes <= tb_drec_data_len_max) {
			to_read = wav_buf_size;
		} else {
			to_read = (tb_drec_data_len_max - data_len) * 8 *
			    bytes_smp;
		}

		rc = rwave_read_samples(wr, buf, to_read, &nread);
		if (rc != 0)
			goto error;

		if (nread == 0)
			break;

		smp_read = nread / bytes_smp;

		di = drec->data_len;

		/* Compute new size of data */

		data_len += smp_read / 8;
		lb_bits += smp_read % 8;
		if (lb_bits > 8) {
			lb_bits -= 8;
			data_len++;
		}

		dptr = realloc(drec->data, data_len);
		if (dptr == NULL) {
			rc = ENOMEM;
			goto error;
		}

		drec->data = dptr;
		dbitn = drec->lb_bits;

		/* Pack in new bits */
		for (i = 0; i < smp_read; i++) {
			if (bytes_smp == 1)
				bit = buf[i] > 0x7f;
			else
				bit = (int16_t) uint16_t_le2host(bp16[i]) > 0;

			drec->data[di] &= ~(1 << (7 - dbitn));
			drec->data[di] |= bit << (7 - dbitn);

			if (++dbitn >= 8) {
				dbitn = 0;
				++di;
			}
		}

		drec->data_len = data_len;
		drec->lb_bits = lb_bits;
	}

	if (data_len == 0) {
		rc = ENOENT;
		goto error;
	}

	printf("pause after:%u\n", (unsigned) drec->pause_after);
	printf("data len:%u\n", (unsigned) drec->data_len);
	printf("lb bits:%u\n", (unsigned) drec->lb_bits);

	tape_append(tape, drec->block);
	free(buf);
	return 0;
error:
	if (buf != NULL)
		free(buf);
	if (drec != NULL)
		tblock_direct_rec_destroy(drec);
	return rc;
}

/** Save direct recording block.
 *
 * @param data Direct recording block
 * @param ww RIFF file to write to
 * @return Zero on success or error code
 */
static int wav_save_direct_rec(tblock_direct_rec_t *drec, rwavew_t *ww)
{
	uint8_t *buf = NULL;
	size_t bwidx;
	size_t doff;
	uint8_t bit;
	int i;
	int nb;
	int rc;

	printf("save direct recording block\n");

	buf = calloc(1, wav_buf_size);
	if (buf == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Assert we can always fit entire unpacked byte to the buffer */
	assert(wav_buf_size % 8 == 0);

	/* Unpack bits into samples */

	bwidx = 0;
	doff = 0;
	while (doff < drec->data_len) {
		nb = doff < drec->data_len - 1 ? 8 : drec->lb_bits;
		for (i = 0; i < nb; i++) {
			bit = (drec->data[doff] << i) & 0x80;
			buf[bwidx++] = bit ? 0xff : 0x00;
		}

		++doff;

		if (bwidx >= wav_buf_size) {
			/* Write out buffer */
			rc = rwave_write_samples(ww, buf, wav_buf_size);
			if (rc != 0)
				return EIO;

			bwidx = 0;
		}
	}

	/* Write out the rest */
	rc = rwave_write_samples(ww, buf, bwidx);
	if (rc != 0)
		return EIO;

	return 0;
error:
	if (buf != NULL)
		free(buf);
	return rc;
}

/** Load tape from WAV file.
 *
 * @param fname File name
 * @param rtape Place to store pointer to loaded tape
 * @return Zero on success or error code
 */
int wav_tape_load(const char *fname, tape_t **rtape)
{
	rwaver_t *rw;
	rwave_params_t params;
	tape_t *tape = NULL;
	int rc;

	rc = rwave_ropen(fname, &params, &rw);
	if (rc != 0)
		goto error;

	rc = tape_create(&tape);
	if (rc != 0)
		goto error;

	tape->version.major = wav_ver_major;
	tape->version.minor = wav_ver_minor;

	printf("read blocks\n");
	while (true) {
		/* May need to split large WAV into multiple blocks */
		rc = wav_load_direct_rec(rw, &params, tape);
		if (rc != 0) {
			if (rc == ENOENT)
				break;
			goto error;
		}
	}

	(void) rwave_rclose(rw);
	*rtape = tape;
	return 0;
error:
	if (tape != NULL)
		tape_destroy(tape);
	(void) rwave_rclose(rw);
	return rc;
}

/** Save tape to WAV file.
 *
 * @param tape Tape
 * @param fname File name
 * @return Zero on success or error code
 */
int wav_tape_save(tape_t *tape, const char *fname)
{
	rwavew_t *ww = NULL;
	rwave_params_t params;
	tape_block_t *block;
	tblock_direct_rec_t *drec;
	uint16_t smp_dur = 0;
	int rc;
	int rv;

	/* Determine sample rate */

	block = tape_first(tape);
	while (block != NULL) {
		switch (block->btype) {
		case tb_direct_rec:
			drec = (tblock_direct_rec_t *) block->ext;
			if (smp_dur == 0) {
				smp_dur = drec->smp_dur;
			} else if (smp_dur != drec->smp_dur) {
				rc = ENOTSUP;
				goto error;
			}
			break;
		default:
			rc = ENOTSUP;
			goto error;
		}

		block = tape_next(block);
	}

	if (smp_dur == 0)
		smp_dur = 79; /* ~44100 Hz */

	/* Initialize params */
	params.channels = 1;
	params.bits_smp = 8;
	params.smp_freq = Z80_CLOCK / smp_dur;

	rc = rwave_wopen(fname, &params, &ww);
	if (rc != 0)
		return rc;

	printf("write blocks\n");
	block = tape_first(tape);
	while (block != NULL) {
		printf("write block\n");
		switch (block->btype) {
		case tb_direct_rec:
			rc = wav_save_direct_rec(
			    (tblock_direct_rec_t *) block->ext, ww);
			break;
		default:
			rc = ENOTSUP;
			goto error;
		}

		if (rc != 0)
			goto error;

		block = tape_next(block);
	}

	rv = rwave_wclose(ww);
	if (rv < 0) {
		rc = EIO;
		goto error;
	}

	return 0;
error:
	if (ww != NULL)
		rwave_wclose(ww);
	return rc;
}
