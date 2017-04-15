/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K MIDI port emulation
 */
#ifndef MIDI_PORT_H
#define MIDI_PORT_H

#include <stdint.h>

typedef struct {
} midi_port_t;

extern void midi_port_init(midi_port_t *);
extern void midi_port_write(midi_port_t *, uint8_t);

#endif
