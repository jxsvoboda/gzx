/*
 * GZX - George's ZX Spectrum Emulator
 * PCM playback through Hound
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
#include <hound/client.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../sndw.h"

static size_t audio_buf_size;
static size_t rsbuf_size;
static size_t rsbuf_pos = 0;
static size_t rsbuf_rem;
static size_t rs_n;
static size_t rs_d;
static uint8_t *rsbuf;
static hound_context_t *hound;

int sndw_init(int bufs)
{
	pcm_format_t fmt;
	int rc;

	fmt.channels = 1;
	fmt.sampling_rate = /*28000*/44100;
	fmt.sample_format = PCM_SAMPLE_UINT8;

	audio_buf_size = bufs;
	rsbuf_size = audio_buf_size;
	rsbuf = malloc(rsbuf_size);
	if (rsbuf == NULL)
		return -1;

	rs_n = 28000;
	rs_d = 44100;

	hound = hound_context_create_playback(NULL, fmt, audio_buf_size * 3);
	if (hound == NULL)
		return -1;

	rc = hound_context_connect_target(hound, HOUND_DEFAULT_TARGET);
	if (rc != EOK) {
		hound_context_destroy(hound);
		hound = NULL;
		return -1;
	}

	return 0;
}

void sndw_done(void)
{
	hound_context_destroy(hound);
}

static void sndw_resample(uint8_t smp)
{
	int rc;

	while (rsbuf_rem < rs_d) {
		rsbuf[rsbuf_pos++] = smp;
		if (rsbuf_pos >= rsbuf_size) {
			rc = hound_write_main_stream(hound, rsbuf, rsbuf_size);
			if (rc != EOK) {
				printf("Error writing audio stream.\n");
				exit(1);
			}

			rsbuf_pos = 0;
		}
		rsbuf_rem += rs_n;
	}

	rsbuf_rem -= rs_d;
}

void sndw_write(uint8_t *buf)
{
	size_t i;

	for (i = 0; i < audio_buf_size; i++) {
		sndw_resample(buf[i]);
	}
}
