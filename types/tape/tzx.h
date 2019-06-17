/*
 * GZX - George's ZX Spectrum Emulator
 * TZX file format types
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file TZX file format types
 */

#ifndef TYPES_TZX_H
#define TYPES_TZX_H

#include <stdint.h>

/** TZX header */
typedef struct {
	/** TZX signature */
	uint8_t signature[7];
	/** End of file marker */
	uint8_t eof_mark;
	/** Major version */
	uint8_t major;
	/** Minor version */
	uint8_t minor;
} tzx_header_t;

/** TZX block types */
typedef enum {
	/** Standard speed data block */
	tzxb_data = 0x10,
	/** Turbo speed data block */
	tzxb_turbo_data = 0x11,
	/** Pure tone */
	tzxb_tone = 0x12,
	/** Sequence of pulses of various lengths */
	tzxb_pulses = 0x13,
	/** Pure data block */
	tzxb_pure_data = 0x14,
	/** Direct recording block */
	tzxb_direct_rec = 0x15,
	/** CSW recording block */
	tzxb_csw_rec = 0x18,
	/** Generalized data block */
	tzxb_gen_data = 0x19,
	/** Pause (silence) or 'Stop the tape' command */
	tzxb_pause_stop = 0x20,
	/** Group start */
	tzxb_group_start = 0x21,
	/** Group end */
	tzxb_group_end = 0x22,
	/** Jump to block */
	tzxb_jump = 0x23,
	/** Loop start */
	tzxb_loop_start = 0x24,
	/** Loop end */
	tzxb_loop_end = 0x25,
	/** Call sequence */
	tzxb_call_seq = 0x26,
	/** Return from sequence */
	tzxb_return = 0x27,
	/** Select block */
	tzxb_select = 0x28,
	/** Stop the tape if in 48K mode */
	tzxb_stop_48k = 0x2a,
	/** Set signal level */
	tzxb_set_lvl = 0x2b,
	/** Text description */
	tzxb_text_desc = 0x30,
	/** Message block */
	tzxb_message = 0x31,
	/** Archive info */
	tzxb_archive_info = 0x32,
	/** Hardware type */
	tzxb_hw_type = 0x33,
	/** Custom info block */
	tzxb_custom_info = 0x35,
	/** Glue block */
	tzxb_glue = 0x5a,
	/** C64 ROM type data block (deprecated) */
	tzxb_c64_rom_data = 0x16,
	/** C64 turbo tape data block (deprecated) */
	tzxb_c64_turbo_data = 0x17,
	/** Emulation info */
	tzxb_emu_info = 0x35,
	/** Snapshot block */
	tzxb_snapshot = 0x40
} tzx_btype_t;

/** Standard speed data header */
typedef struct {
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes */
	uint16_t data_len;
} __attribute__((packed)) tzx_block_data_t;

/** Turbo speed data header */
typedef struct {
	/** Length of pilot pulse (T) */
	uint16_t pilot_len;
	/** Length of first sync pulse (T) */
	uint16_t sync1_len;
	/** Length of second sync pulse (T) */
	uint16_t sync2_len;
	/** Length of zero bit pulse (T) */
	uint16_t zero_len;
	/** Length of one bit pulse (T) */
	uint16_t one_len;
	/** Number of pulses in pilot tone */
	uint16_t pilot_pulses;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes */
	uint8_t data_len[3];
} __attribute__((packed)) tzx_block_turbo_data_t;

/** Pure tone */
typedef struct {
	/** Pulse duration (T) */
	uint16_t pulse_len;
	/** Number of pulses */
	uint16_t num_pulses;
} __attribute__((packed)) tzx_block_tone_t;

/** Pulse sequence */
typedef struct {
	/** Number of pulses */
	uint8_t num_pulses;
} __attribute__((packed)) tzx_block_pulses_t;

/** Pure data header */
typedef struct {
	/** Length of zero bit pulse (T) */
	uint16_t zero_len;
	/** Length of one bit pulse (T) */
	uint16_t one_len;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes */
	uint8_t data_len[3];
} __attribute__((packed)) tzx_block_pure_data_t;

/** Direct recording header */
typedef struct {
	/** Sample duration (T) */
	uint16_t smp_dur;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Data length in bytes */
	uint8_t data_len[3];
} __attribute__((packed)) tzx_block_direct_rec_t;

/** Pause (silence) or 'Stop the Tape' header */
typedef struct {
	/** Pause duration in ms or zero to stop the tape */
	uint16_t pause_len;
} __attribute__((packed)) tzx_block_pause_t;

/** Group start header */
typedef struct {
	/** Length of the group name string */
	uint8_t name_len;
} __attribute__((packed)) tzx_block_group_start_t;

/** Loop start header */
typedef struct {
	/** Number of repetitions (> 1) */
	uint16_t num_rep;
} __attribute__((packed)) tzx_block_loop_start_t;

/** Text description header */
typedef struct {
	/** Length of text string */
	uint8_t text_len;
} __attribute__((packed)) tzx_block_text_desc_t;

/** Text structure */
typedef struct {
	/** Text identification */
	uint8_t text_type;
	/** Length of text string */
	uint8_t text_len;
} __attribute__((packed)) tzx_text_t;

/** Archive info header */
typedef struct {
	/** Block length */
	uint16_t block_len;
	/** Number of strings */
	uint8_t nstrings;
} __attribute__((packed)) tzx_block_archive_info_t;

/** Hardware info */
typedef struct {
	/** Hardware type */
	uint8_t hw_type;
	/** Hardware ID */
	uint8_t hw_id;
	/** Hardware information */
	uint8_t hw_info;
} __attribute__((packed)) tzx_hwinfo_t;

/** Hardware type */
typedef struct {
	/** Number of hardware info structures */
	uint8_t ninfos;
} __attribute__((packed)) tzx_block_hw_type_t;

/** Unkown block header (conforming to the extension rule) */
typedef struct {
	/** Block length */
	uint32_t block_len;
} __attribute__((packed)) tzx_block_unknown_t;

#endif
