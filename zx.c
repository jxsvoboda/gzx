/*
 * GZX - George's ZX Spectrum Emulator
 * ZX machine state
 */

#include "ay.h"
#include "rs232.h"
#include "zx.h"

/** First AY */
ay_t ay0;

/** MIDI port */
midi_port_t midi;

/** RS-232 port */
rs232_t rs232;
