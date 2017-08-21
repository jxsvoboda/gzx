/*
 * GZX - George's ZX Spectrum Emulator
 * Tape support
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
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

#ifndef ZX_TAPE_H
#define ZX_TAPE_H

#define ZX_LDBYTES_TRAP	0x056a
#define ZX_SABYTES_TRAP	0x04d1

#define ROM_PILOT_LEN 2168
#define ROM_SYNC1_LEN  667
#define ROM_SYNC2_LEN  735
#define ROM_ZERO_LEN   855
#define ROM_ONE_LEN   1710
#define ROM_PPULSES_H 8064
#define ROM_PPULSES_D 3220
#define ROM_PAUSE_LEN 3500000

typedef struct {  
  unsigned data_bytes;
  unsigned used_bits;		/* in last byte */

  int rom_timing;		/* 1=ROM-timed, 0=turbo-timed */
  int has_leadin;		/* 1=play pilot/sync, 0=data only */
  unsigned pause_after_len;     /* in T-states */
  
  /* these must be set correctly even for ROM-timed blocks! */
  unsigned pilot_len;
  unsigned sync1_len;
  unsigned sync2_len;
  unsigned zero_len;
  unsigned one_len;
  unsigned pilot_pulses;
} tb_data_info_t;

typedef struct {  
  unsigned pause_after_len;     /* in T-states */
  unsigned samples;
  unsigned smp_len;		/* int T-states */
} tb_voice_info_t;

#define BT_EOT     0
#define BT_UNKNOWN 1
#define BT_DATA    2
#define BT_VOICE   3
#define BT_TONES   4

#include "intdef.h"

/* quick tape loading */
void zx_tape_ldbytes(void);
void zx_tape_sabytes(void);

/* real-time tape */
void zx_tape_getsmp(u8 *smp);
void zx_tape_play(void);
void zx_tape_pause(void);
void zx_tape_stop(void);

/* common */
int zx_tape_init(int delta_t);
void zx_tape_done(void);
int zx_tape_selectfile(char *name);
void zx_tape_rewind(void);

#endif
