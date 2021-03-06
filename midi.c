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

#include <stdio.h>
#include "midi.h"

/** Initialize MIDI port.
 *
 * @param mp MIDI port
 */
void midi_port_init(midi_port_t *mp)
{
	mp->ms = ms_statusb;
	/* No status byte seen yet */
	mp->sb = 0x00;
}

/** Return number of data bytes for a message type.
 *
 * @param statusb Status byte
 * @return Number of data bytes
 */
static uint8_t midi_msg_ndata_bytes(uint8_t statusb)
{
	switch (statusb & 0xf0) {
	case midi_sb_note_on:
	case midi_sb_note_off:
	case midi_sb_poly_kp:
	case midi_sb_ctl_change:
	case midi_sb_pitch_bend:
		return 2;
	case midi_sb_pgm_change:
	case midi_sb_chan_press:
		return 1;
	default:
		return 0;
	}
}

/** Message was sent through MIDI port.
 *
 * @param mp MIDI port
 * @param sb Status byte
 * @param db1 Data byte 1
 * @param db2 Data byte 2
 */
static void midi_port_msg(midi_port_t *mp, uint8_t sb, uint8_t db1,
    uint8_t db2)
{
	midi_msg_t msg;

	msg.sb = sb;
	msg.db1 = db1;
	msg.db2 = db2;

	if (mp->midi_msg != NULL)
		mp->midi_msg(mp->midi_msg_arg, &msg);
}

/** Write byte to MIDI port.
 *
 * @param mp MIDI port
 * @param val Value to write
 */
void midi_port_write(midi_port_t *mp, uint8_t val)
{
	uint8_t stype;

again:
	switch (mp->ms) {
	case ms_statusb:
		stype = val & 0xf0;

		if (stype == midi_sb_system) {
			if (val == midi_sb_sysex_start)
				mp->ms = ms_sysexb;
		} else if (val >= midi_sb_min) {
			mp->sb = val;
			mp->ms = ms_datab1;
		} else {
			mp->ms = ms_datab1;
			goto again;
		}
		break;
	case ms_datab1:
		if (val >= midi_sb_min) {
			mp->ms = ms_statusb;
			goto again;
		}
		if (midi_msg_ndata_bytes(mp->sb) > 1) {
			mp->db1 = val;
			mp->ms = ms_datab2;
		} else {
			midi_port_msg(mp, mp->sb, val, 0);
		}
		break;
	case ms_datab2:
		if (val >= midi_sb_min) {
			mp->ms = ms_statusb;
			goto again;
		}
		midi_port_msg(mp, mp->sb, mp->db1, val);
		break;
	case ms_sysexb:
		if (val == midi_sb_sysex_end)
			mp->ms = ms_statusb;
		if (val >= midi_sb_min) {
			mp->ms = ms_statusb;
			goto again;
		}
		break;
	}
}
