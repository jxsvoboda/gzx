/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K MIDI port emulation
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
