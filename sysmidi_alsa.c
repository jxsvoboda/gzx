/*
 * GZX - George's ZX Spectrum Emulator
 * ALSA sequencer MIDI interface
 */

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "midi.h"
#include "midi_msg.h"
#include "sysmidi.h"

enum {
	/** Play MIDI with 0.1 second buffer/latency */
	midi_latency_usec = 100000,
	/** Stop playback after being idle for 2 seconds */
	midi_stop_time_ticks = 2 * 3500000
};

static snd_seq_t *seq;
static int portid = -1;
static int queueid = -1;

static bool started = false;
static uint64_t first_t;
static uint64_t last_t;

static uint64_t lframe_t;
static uint32_t lframe_t32;

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

	queueid = snd_seq_alloc_queue(seq);
	if (queueid < 0) {
		printf("Error creating sequencer queue.\n");
		goto error;
	}

	return 0;
error:
	sysmidi_done();
	return -1;
}

void sysmidi_done(void)
{
	if (seq != NULL && queueid >= 0) {
		snd_seq_free_queue(seq, queueid);
		queueid = -1;
	}

	if (seq != NULL && portid >= 0) {
		snd_seq_delete_simple_port(seq, portid);
		portid = -1;
	}

	if (seq != NULL) {
		snd_seq_close(seq);
		seq = NULL;
	}
}

static void sysmidi_check_start(uint64_t t)
{
	snd_seq_queue_status_t *status;
	snd_seq_real_time_t const *rt;
	int rc;

	if (!started) {
		printf("Starting queue\n");
		rc = snd_seq_start_queue(seq, queueid, NULL);
		if (rc < 0) {
			printf("Error starting queue\n");
		}
		first_t = t;
		started = true;
		snd_seq_queue_status_alloca(&status);
		rc = snd_seq_get_queue_status(seq, queueid, status);
		if (rc == 0) {
			rt = snd_seq_queue_status_get_real_time(status);
			printf("queue time: %d.%d\n", rt->tv_sec,
			    rt->tv_nsec);
		}
	}
}

void sysmidi_send_msg(uint32_t t32, midi_msg_t *msg)
{
	snd_seq_event_t ev;
	snd_seq_real_time_t rtime;
	uint8_t ch;
	uint64_t usec;
	uint64_t t;

	t = lframe_t + (t32 - lframe_t32);

	sysmidi_check_start(t);

	usec = ((uint64_t)(t - first_t) * 10 / 35) + midi_latency_usec;
	rtime.tv_sec = usec / 1000000;
	rtime.tv_nsec = (usec % 1000000) * 1000;
	printf("schedule event at %d.%d\n", rtime.tv_sec, rtime.tv_nsec);

	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_schedule_real(&ev, queueid, 0, &rtime);

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

	last_t = t;
}

void sysmidi_poll(uint32_t t32)
{
	int rc;
	uint64_t t;

	/** Extend Z80 clock to 64 bits */
	t = lframe_t + (t32 - lframe_t32);
	lframe_t = t;
	lframe_t32 = t32;

	if (started && t - last_t >= midi_stop_time_ticks) {
		printf("Stopping queue.\n");
		rc = snd_seq_stop_queue(seq, queueid, NULL);
		if (rc < 0) {
			printf("Error stopping queue.\n");
		}
		started = false;
	}
}
