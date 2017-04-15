/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K MIDI port emulation
 */
#include <stdio.h>
#include "midi.h"

void midi_port_init(midi_port_t *mp)
{
}

void midi_port_write(midi_port_t *mp, uint8_t val)
{
	printf("MIDI port write: 0x%02x\n", val);
}
