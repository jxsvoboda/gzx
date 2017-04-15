/*
 * GZX - George's ZX Spectrum Emulator
 * MIDI event
 */
#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

#include <stdint.h>

typedef struct {
	uint8_t sb;
	uint8_t db1;
	uint8_t db2;
} midi_event_t;

#endif
