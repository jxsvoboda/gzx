/*
 * GZX - George's ZX Spectrum Emulator
 * MIDI message
 */
#ifndef MIDI_MSG_H
#define MIDI_MSG_H

#include <stdint.h>

typedef struct {
	uint8_t sb;
	uint8_t db1;
	uint8_t db2;
} midi_msg_t;

#endif
