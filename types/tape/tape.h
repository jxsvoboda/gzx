/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum tape types
 *
 * Copyright (c) 1999-2018 Jiri Svoboda
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
 * @file Spectrum tape types.
 *
 * This is an in-core, editable, representation of Spectrum tape. It should
 * be able to perfectly represent any TZX file.
 */

#ifndef TYPES_TAPE_H
#define TYPES_TAPE_H

#include "../adt/list.h"
#include <stdint.h>

/** TZX file version */
typedef struct {
	/** Major version */
	uint8_t major;
	/** Minor version */
	uint8_t minor;
} tape_ver_t;

/** Tape signal level */
typedef enum {
	/** Low */
	tlvl_low,
	/** High */
	tlvl_high
} tape_lvl_t;

/** Tape block type */
typedef enum {
	/** Standard speed data block */
	tb_data,
	/** Turbo speed data block */
	tb_turbo_data,
	/** Pure tone data block */
	tb_tone,
	/** Sequence of pulses of various lenghts */
	tb_pulses,
	/** Pure data block */
	tb_pure_data,
	/** Direct recording block */
	tb_direct_rec,
	/** CSW recording block */
	tb_csw_rec,
	/** Generalized data block */
	tb_gen_data,
	/** Pause (silence) */
	tb_pause,
	/** Stop the tape */
	tb_stop,
	/** Group start */
	tb_group_start,
	/** Group end */
	tb_group_end,
	/** Jump to block */
	tb_jump,
	/** Loop start */
	tb_loop_start,
	/** Loop end */
	tb_loop_end,
	/** Call sequence */
	tb_call_seq,
	/** Return from sequence */
	tb_return,
	/** Select block */
	tb_select,
	/** Stop the tape if in 48K mode */
	tb_stop_48k,
	/** Set signal level */
	tb_set_lvl,
	/** Text description */
	tb_text_desc,
	/** Message block */
	tb_message,
	/** Archive info */
	tb_archive_info,
	/** Hardware type */
	tb_hw_type,
	/** Custom info block */
	tb_custom_info,
	/** Glue block */
	tb_glue,
	/** C64 ROM type data block (deprecated) */
	tb_c64_rom_data,
	/** C64 turbo tape data block (deprecated) */
	tb_c64_turbo_data,
	/** Emulation info (deprecated) */
	tb_emu_info,
	/** Snapshot block */
	tb_snapshot,
	/** Unknown block conforming to the extension rule */
	tb_unknown
} tape_btype_t;

/** Standard speed data block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes */
	uint16_t data_len;
	/** Data */
	uint8_t *data;
} tblock_data_t;

/** Turbo speed data block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
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
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_turbo_data_t;

/** Pure tone */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Length of one pulse (T) */
	uint16_t pulse_len;
	/** Number of pulses */
	uint16_t num_pulses;
	/** Data */
	uint8_t *data;
} tblock_tone_t;

/** Pulse sequence */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Number of pulses */
	uint16_t num_pulses;
	/** Pulse lengths */
	uint16_t *pulse_len;
} tblock_pulses_t;

/** Pure data block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Length of zero bit pulse (T) */
	uint16_t zero_len;
	/** Length of one bit pulse (T) */
	uint16_t one_len;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_pure_data_t;

/** Direct recording */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Sample duration (T) */
	uint16_t smp_dur;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_direct_rec_t;

/** CSW recording */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Sampling rate (24-bit) */
	uint32_t smp_rate;
	/** Compression type */
	uint8_t compr_type;
	/** Number of stored pulses */
	uint32_t num_pulses;
	/** Data length in bytes */
	uint32_t data_len;
	/** CSW data */
	uint8_t *data;
} tblock_csw_rec_t;

/** Generalized data block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Number of symbols in pilot/sync block */
	uint32_t ps_syms;
	/** Max. number of pulses per pilot/sync symbol */
	uint8_t ps_maxpulse;
	/** Pilot/sync alphabet size (max = 256) */
	uint16_t ps_alphabet;
	/** Number of symbols in data stream */
	uint32_t data_syms;
	/** Max. number of pulses per data symbol */
	uint8_t data_maxpulse;
	/** Data alphabet size (max = 256) */
	uint16_t data_alphabet;
	/** Pilot/sync stream length in bytes */
	uint32_t ps_len;
	/** Pilot/sync data stream */
	uint8_t *ps_stream;
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_gen_data_t;

/** Pause (silence) */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Pause duration in ms (must be > 0) */
	uint16_t pause_len;
} tblock_pause_t;

/** Stop the tape */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
} tblock_stop_t;

/** Group start */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Group name */
	char *name;
} tblock_group_start_t;

/** Group end */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
} tblock_group_end_t;

/** Jump to block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Relative jump value */
	int16_t rjmp;
	/** Jump destination */
	struct tape_block *dest;
} tblock_jump_t;

/** Loop start */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Number of repetitions (> 1) */
	uint16_t nrep;
} tblock_loop_start_t;

/** Loop end */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
} tblock_loop_end_t;

/** Call sequence */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Number of calls */
	uint16_t ncalls;
	/** Relative block offsets */
	int16_t *rcall;
	/** Call destinations */
	struct tape_block *dest;
} tblock_call_seq_t;

/** Return from sequence */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
} tblock_return_t;

/** Tape selection */
typedef struct {
	/** Containing select block */
	struct tblock_select *select;
	/** Destination block relative offset */
	int16_t roffs;
	/** Destination block */
	struct tape_block *dest;
	/** Description */
	char *desc;
} tape_sel_t;

/** Select block */
typedef struct tblock_select {
	/** Containing tape block */
	struct tape_block *block;
	/** Selections */
	list_t sels; /* of tape_sel_t */
} tblock_select_t;

/** Stop the tape if in 48K mode */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
} tblock_stop_48k_t;

/** Set signal level */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Signal level */
	tape_lvl_t lvl;
} tblock_set_lvl__t;

/** Text description */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Text description */
	char *text;
} tblock_text_desc_t;

/** Message block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Time in seconds for which the message should be displayed */
	uint8_t time;
	/** Message */
	char *message;
} tblock_message_t;

/** Text structure */
typedef struct {
	/** Containing archive info block */
	struct tblock_archive_info *ainfo;
	/** Link to @c ainfo->texts */
	link_t lainfo;
	/** Text identification */
	uint8_t text_type;
	/** Text */
	char *text;
} tape_text_t;

/** Archive info */
typedef struct tblock_archive_info {
	/** Containing tape block */
	struct tape_block *block;
	/** Text structures */
	list_t texts; /* of tape_text_t */
} tblock_archive_info_t;

/** Hardware info structure */
typedef struct {
	/** Containing hardware type block */
	struct tblock_hw_type *hw_type;
	/** Link to hw_type->hwinfos */
	link_t lhwinfos;
	/** Hardware type */
	uint8_t hwtype;
	/** Hardware ID */
	uint8_t hwid;
	/** Hardware information */
	uint8_t hwinfo;
} tape_hwinfo_t;

/** Hardware type */
typedef struct tblock_hw_type {
	/** Containing tape block */
	struct tape_block *block;
	/** Hardware info structures */
	list_t hwinfos; /* of tape_hwinfo_t */
} tblock_hw_type_t;

/** Custom info block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Identification string */
	char *id;
	/** Custom info length in bytes */
	uint32_t info_len;
	/** Custom information */
	uint8_t *info;
} tape_custom_info_t;

/** Glue block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** TZX version */
	tape_ver_t version;
} tape_glue_t;

/** C64 ROM type data block (deprecated) */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Length of pilot pulse (T) */
	uint16_t pilot_len;
	/** Number of waves in pilot tone */
	uint16_t pilot_waves;
	/** Length of first sync wave pulse (T) */
	uint16_t sync1_len;
	/** Length of second sync wave pulse (T) */
	uint16_t sync2_len;
	/** Length of first zero bit pulse (T) */
	uint16_t zero1_len;
	/** Length of first second bit pulse (T) */
	uint16_t zero2_len;
	/** Length of first one bit pulse (T) */
	uint16_t one1_len;
	/** Length of second one bit pulse (T) */
	uint16_t one2_len;
	/** Checksum type */
	uint8_t checksum_type;
	/** Length of pulse in first finish byte wave */
	uint8_t finish1_len;
	/** Length of pulse in second finish byte wave */
	uint8_t finish2_len;
	/** Length of trailing tone pulse (T) */
	uint16_t trail_len;
	/** Number of waves in trailing tone */
	uint16_t trail_waves;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Flags */
	uint8_t flags;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_c64_rom_data_t;

/** C64 ROM turbo tape data block (deprecated) */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Zero bit pulse length (T) */
	uint16_t zero_len;
	/** One bit pulse length (T) */
	uint16_t one_len;
	/** Additional bits in bytes (bit field) */
	uint8_t addt_bits;
	/** Number of lead-in bytes */
	uint16_t leadin_bytes;
	/** Lead-in byte */
	uint8_t leadin_byte;
	/** Used bits in last byte */
	uint8_t lb_bits;
	/** Flags */
	uint8_t flags;
	/** Number of trailing bytes */
	uint16_t trail_bytes;
	/** Trailing byte */
	uint8_t trail_byte;
	/** Pause after this block in ms */
	uint16_t pause_after;
	/** Data length in bytes (number is limited to 24 bits) */
	uint32_t data_len;
	/** Data */
	uint8_t *data;
} tblock_c64_turbo_data_t;

/** Emulation info (deprecated) */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** General emulation flags */
	uint16_t gflags;
	/** Screen refresh delay 1 - 255 (interrupts between refreshes) */
	uint8_t scr_delay;
	/** Interrupt frequency 0 - 999 Hz */
	uint16_t int_freq;
	/** Reserved for future expansion */
	uint8_t reserved[3];
} tblock_emu_info_t;

/** Snapshot block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Snapshot type */
	uint8_t snap_type;
	/** Snapshot length in bytes */
	uint16_t snap_len;
	/** Snapshot data */
	uint8_t *snap;
} tblock_snapshot_t;

/** Unknown block */
typedef struct {
	/** Containing tape block */
	struct tape_block *block;
	/** Data length */
	uint32_t data_len;
	/** Block data */
	void *data;
} tblock_unknown_t;

/** Tape block */
typedef struct tape_block {
	/** Containing tape */
	struct tape *tape;
	/** Link to @c tape->blocks */
	link_t ltape;
	/** Block type */
	tape_btype_t btype;
	/** Pointer to data specific to this block type */
	void *ext;
} tape_block_t;

/** Tape */
typedef struct tape {
	/** Tape version */
	tape_ver_t version;
	/** Tape blocks */
	list_t blocks; /* of tape_block_t */
} tape_t;

#endif
