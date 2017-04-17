/*
 * GZX - George's ZX Spectrum Emulator
 * Windows MIDI interface
 */

#include "midi.h"
#include "midi_msg.h"
#include "sysmidi.h"

int sysmidi_init(const char *dev)
{
	return 0;
}

void sysmidi_done(void)
{
}

void sysmidi_send_msg(uint32_t tdelta, midi_msg_t *msg)
{
}

void sysmidi_poll(uint32_t t)
{
}
