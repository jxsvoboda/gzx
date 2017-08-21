/*
 * GZX - George's ZX Spectrum Emulator
 * Windows MIDI interface
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "clock.h"
#include "midi.h"
#include "midi_msg.h"
#include "sysmidi.h"

enum {
	/** Number of MIDI buffers */
	num_buf = 10,
	/** Number of buffers submitted after which we start the stream */
	buf_low_wm = 5,
	/** Since MIDI baud rate is 31250 and it takes 8+2 bits to
	 * transfer a character, there can be at most 3125 messages per second
	 * received from the Spectrum MIDI port. A buffer is good for up
	 * to 1 PAL frame (1/50 s) */
	buf_len = 31250 / 50,
	/** Z80 T states per MIDI tick */
	ts_midi_tick = 3500,
	/** MIDI ticks per PAL frame */
	frame_midi_tick = Z80_CLOCK / 50 / ts_midi_tick
};

/** MIDI stream */
static HMIDISTRM mstrm;
/** Header for each buffer */
static MIDIHDR mhdr[num_buf];
/** MIDI buffer */
static MIDIEVENT mbuf[num_buf][buf_len];
/** Index of first submitted buffer */
static int sbufn;
/** Index of first available buffer */
static int abufn;
/** Number of submitted buffers */
static int ns;
/** Number of available buffers */
static int na;
/** Write position in current buffer */
static int bwi;
/** Current buffer duration */
static uint32_t bdur;
/** Stream is running */
static bool strm_run = false;

static uint64_t last_t;
static uint32_t lframe_t32;
static uint64_t lframe_t;

static void sysmidi_check_finished(void);
static void sysmidi_check_start_stream(void);

int sysmidi_init(const char *dev)
{
	MMRESULT mmrc;
	UINT devid;
	MIDIPROPTIMEDIV prop;
	int i;

	devid = /*MIDI_MAPPER */0;

	mmrc = midiStreamOpen(&mstrm, &devid, 1, 0, 0, CALLBACK_NULL);
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error opening MIDI device.\n");
		goto error;
	}

	prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
	/* SMPTE 25 fps, 40 ticks/frame = 1 ms resolution */
	prop.dwTimeDiv = 0x8000 | ((DWORD)(0x7f - 25) << 8) |
	    ((DWORD) 40);
	mmrc = midiStreamProperty(mstrm, (LPBYTE)&prop, MIDIPROP_SET | 
	    MIDIPROP_TIMEDIV);
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error setting MIDI time base.\n");
		goto error;
	}

	for (i = 0; i < num_buf; i++) {
		mhdr[i].lpData = (LPSTR)mbuf[i];
		mhdr[i].dwBufferLength = sizeof(mbuf[i]);
		mhdr[i].dwFlags = MHDR_ISSTRM;

		mmrc = midiOutPrepareHeader((HMIDIOUT)mstrm,
		    &mhdr[i], sizeof(mhdr[i]));
		if (mmrc != MMSYSERR_NOERROR) {
			printf("Error preparing MIDI buffer.\n");
			goto error;
		}
	}

	sbufn = abufn = 0;
	ns = 0;
	na = num_buf;
	bwi = 0;
	bdur = 0;

	printf("MIDI initialized.n\n");
	return 0;
error:
	sysmidi_done();
	return -1;
}

void sysmidi_done(void)
{
	int i;

	for (i = 0; i < num_buf; i++) {
		if ((mhdr[i].dwFlags & MHDR_PREPARED) != 0) {
			midiOutUnprepareHeader((HMIDIOUT)mstrm,
			    &mhdr[i], sizeof(mhdr[i]));
		}
	}

	if (mstrm != NULL) {
		midiStreamClose(mstrm);
		mstrm = NULL;
	}
}

void sysmidi_send_msg(uint32_t t32, midi_msg_t *msg)
{
	uint64_t t;
	uint64_t tdelta;

	t = lframe_t + (t32 - lframe_t32);
	tdelta = (t - last_t) / ts_midi_tick;

	sysmidi_check_finished();
	if (na < 1) {
		printf("Note: No available MIDI buffer, event dropped.\n");
		return;
	}

	/* Add message to current buffer */
	if (bwi >= buf_len) {
		/* This should not happen */
		printf("Note: MIDI buffer full, event dropped.\n");
		return;
	}

//	printf("enlist message in buffer %d, deltatime=%lu\n",
//	    abufn, (unsigned long)tdelta);
	mbuf[abufn][bwi].dwDeltaTime = tdelta;
	mbuf[abufn][bwi].dwStreamID = 0;
	mbuf[abufn][bwi].dwEvent = ((DWORD)MEVT_F_SHORT << 24) |
	    (DWORD)msg->sb |
	    ((DWORD)msg->db1 << 8) |
	    ((DWORD)msg->db2 << 16);
	++bwi;
	bdur += mbuf[abufn][bwi].dwDeltaTime;
	if (bdur > frame_midi_tick) {
		printf("Buffer duration is too long. This should not happen.\n");
	}

	last_t = last_t + tdelta * ts_midi_tick;
}

/** Submit the current buffer. */
static int sysmidi_submit_buf(void)
{
	MMRESULT mmrc;

	if (na < 1) {
		printf("Note: No available MIDI buffer, frame submit failed.\n");
		return -1;
	}

	/* Add a NOP event to fill up the buffer duration up to 1/50 s */
	if (bwi >= buf_len) {
		/* This should not happen */
		printf("Note: MIDI buffer full, event dropped.\n");
		bwi--;
	}

	if (bdur < frame_midi_tick) {
//		printf("Adding NOP event with deltatime %d\n",
//		    frame_midi_tick - bdur);
		mbuf[abufn][bwi].dwDeltaTime = frame_midi_tick - bdur;
		mbuf[abufn][bwi].dwStreamID = 0;
		mbuf[abufn][bwi].dwEvent = ((DWORD)MEVT_F_SHORT << 24) | MEVT_NOP;
		++bwi;
	}

	mhdr[abufn].dwBytesRecorded = bwi * sizeof(MIDIEVENT);

	/* Submit the buffer */
//	printf("Submitting buffer %d, bytes=%lu\n", abufn, mhdr[abufn].dwBytesRecorded);
	mmrc = midiStreamOut(mstrm, &mhdr[abufn], sizeof(mhdr[abufn]));
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error submitting MIDI buffer.\n");
		return -1;
	}

	abufn = (abufn + 1 ) % num_buf;
	--na;
	++ns;
	bwi = 0;
	bdur = 0;

	return 0;
}

void sysmidi_poll(uint32_t t32)
{
	uint64_t t;

	/** Extend Z80 clock to 64 bits */
	t = lframe_t + (t32 - lframe_t32);
	lframe_t = t;
	lframe_t32 = t32;

	sysmidi_check_finished();
//	printf("sysmidi_poll: after check, ns=%d na=%d\n", ns, na);
	if (sysmidi_submit_buf() < 0)
		return;

	last_t = lframe_t;

	sysmidi_check_start_stream();
}

/** Check for finished buffers. */
static void sysmidi_check_finished(void)
{
	while ((mhdr[sbufn].dwFlags & MHDR_DONE) != 0) {
//		printf("Reclaiming buffer %d\n", sbufn);
		mhdr[sbufn].dwFlags &= ~MHDR_DONE;
		sbufn = (sbufn + 1) % num_buf;
		++na;
		--ns;
	}

	if (na == num_buf) {
		printf("MIDI underrun detected. Replenishing buffers.\n");
		while (ns < buf_low_wm) {
			if (sysmidi_submit_buf() < 0)
				return;
		}
	}
}

static void sysmidi_check_start_stream(void)
{
	MMRESULT mmrc;

	if (strm_run || ns < buf_low_wm)
		return;

	printf("Starting MIDI stream.\n");
	mmrc = midiStreamRestart(mstrm);
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error (re)starting MIDI stream.\n");
		return ;
	}

	strm_run = true;
	return;
}
