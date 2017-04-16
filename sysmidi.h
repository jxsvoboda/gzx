/*
 * GZX - George's ZX Spectrum Emulator
 * System MIDI interface
 */

#ifndef SYSMIDI_H
#define SYSMIDI_H

#include <stdint.h>
#include "midi_msg.h"

extern int sysmidi_init(const char *);
extern void sysmidi_done(void);
extern void sysmidi_send_msg(uint32_t, midi_msg_t *);

#endif
