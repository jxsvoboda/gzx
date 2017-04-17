/*
 * GZX - George's ZX Spectrum Emulator
 * Windows MIDI interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "midi.h"
#include "midi_msg.h"
#include "sysmidi.h"

static HMIDIOUT hmo;

int sysmidi_init(const char *dev)
{
	MMRESULT mmrc;

	mmrc = midiOutOpen(&hmo, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error opening MIDI device.\n");
		return -1;
	}

	return 0;
}

void sysmidi_done(void)
{
	if (hmo != NULL) {
		midiOutClose(hmo);
		hmo = NULL;
	}
}

void sysmidi_send_msg(uint32_t tdelta, midi_msg_t *msg)
{
	MMRESULT mmrc;
	DWORD dw;

	dw = msg->sb |
	    ((DWORD)msg->db1 << 8) |
	    ((DWORD)msg->db2 << 16);

	printf("Send message 0x%lx\n", dw);
	mmrc = midiOutShortMsg(hmo, dw);
	if (mmrc != MMSYSERR_NOERROR) {
		printf("Error sending MIDI message.\n");
	}
}

void sysmidi_poll(uint32_t t)
{
}
