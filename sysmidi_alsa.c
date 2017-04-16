/*
 * GZX - George's ZX Spectrum Emulator
 * ALSA sequencer MIDI interface
 */

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include "midi.h"
#include "midi_msg.h"
#include "sysmidi.h"

static snd_seq_t *seq;
static int portid = -1;

int sysmidi_init(const char *dev)
{
	int rc;
	int dclient;
	int dport;

	if (dev == NULL) {
		printf("No MIDI device specified.\n");
		goto error;
	}

	rc = sscanf(dev, "%d:%d", &dclient, &dport);
	if (rc != 2) {
		printf("Invalid MIDI device, ALSA client:port needed.\n");
		goto error;
	}

	rc = snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
	if (rc < 0) {
		printf("Error opening ALSA sequencer.\n");
		goto error;
	}

	rc = snd_seq_set_client_name(seq, "GZX");
	if (rc < 0) {
		printf("Error setting ALSA sequencer client name.\n");
		goto error;
	}

	portid = snd_seq_create_simple_port(seq, "128K MIDI",
	    SND_SEQ_PORT_CAP_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC);
	if (portid < 0) {
		printf("Error creating ALSA sequencer port.\n");
		goto error;
	}

	rc = snd_seq_connect_to(seq, portid, dclient, dport);
	if (rc < 0) {
		printf("Error connecting to ALSA port %d:%d\n", dclient,
		    dport);
		goto error;
	}

	return 0;
error:
	sysmidi_done();
	return -1;
}

void sysmidi_done(void)
{
	if (seq != NULL && portid >= 0) {
		snd_seq_delete_simple_port(seq, portid);
		portid = -1;
	}

	if (seq != NULL) {
		snd_seq_close(seq);
		seq = NULL;
	}
}

void sysmidi_send_msg(uint32_t tdelta, midi_msg_t *msg)
{
	snd_seq_event_t ev;
	uint8_t ch;

	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);

	ch = msg->sb & 0x0f;
	switch (msg->sb & 0xf0) {
	case midi_sb_note_off:
		snd_seq_ev_set_noteoff(&ev, ch, msg->db1, msg->db2);
		break;
	case midi_sb_note_on:
		snd_seq_ev_set_noteon(&ev, ch, msg->db1, msg->db2);
		break;
	case midi_sb_poly_kp:
		snd_seq_ev_set_keypress(&ev, ch, msg->db1, msg->db2);
		break;
	case midi_sb_ctl_change:
		snd_seq_ev_set_controller(&ev, ch, msg->db1, msg->db2);
		break;
	case midi_sb_pgm_change:
		snd_seq_ev_set_pgmchange(&ev, ch, msg->db1);
		break;
	case midi_sb_chan_press:
		snd_seq_ev_set_chanpress(&ev, ch, msg->db2);
		break;
	case midi_sb_pitch_bend:
		snd_seq_ev_set_pitchbend(&ev, ch, msg->db1);
		break;
	}

	snd_seq_event_output(seq, &ev);
	snd_seq_drain_output(seq);
}
