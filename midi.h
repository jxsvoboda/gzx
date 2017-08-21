/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K MIDI port emulation
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

#ifndef MIDI_PORT_H
#define MIDI_PORT_H

#include <stdint.h>
#include "midi_msg.h"

enum {
	midi_sb_min        = 0x80,

	midi_sb_note_off   = 0x80,
	midi_sb_note_on    = 0x90,
	midi_sb_poly_kp    = 0xa0,
	midi_sb_ctl_change = 0xb0,
	midi_sb_pgm_change = 0xc0,
	midi_sb_chan_press = 0xd0,
	midi_sb_pitch_bend = 0xe0,
	midi_sb_system     = 0xf0,

	midi_sb_sysex_start = 0xf0,
	midi_sb_sysex_end = 0xf7
};

/** MIDI message decoder state */
typedef enum {
	/** Waiting for status byte */
	ms_statusb,
	/** Waiting for data byte 1 */
	ms_datab1,
	/** Waiting for data byte 2 */
	ms_datab2,
	/** Waiting for SysEx message byte */
	ms_sysexb
} midi_state_t;

typedef struct {
	/** MIDI decoder state */
	midi_state_t ms;
	/** Last status byte */
	uint8_t sb;
	/** Last data byte 1 */
	uint8_t db1;
	/** MIDI message decoded callback */
	void (*midi_msg)(void *, midi_msg_t *);
	/** midi_msg callback argument */
	void *midi_msg_arg;
} midi_port_t;

extern void midi_port_init(midi_port_t *);
extern void midi_port_write(midi_port_t *, uint8_t);

#endif
