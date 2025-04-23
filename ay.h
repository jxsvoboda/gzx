/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
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
#ifndef AY_H
#define AY_H

#include <stdint.h>

enum {
	ay_nchan = 3,
	ay_nreg = 16,

	/** Channel A coarse tune register */
	ay_rn_ctr_a  = 0x0,
	/** Channel A fine tune register */
	ay_rn_ftr_a  = 0x1,
	/** Channel B coarse tune register */
	ay_rn_ctr_b  = 0x2,
	/** Channel B fine tune register */
	ay_rn_ftr_b  = 0x3,
	/** Channel C coarse tune register */
	ay_rn_ctr_c  = 0x4,
	/** Channel C fine tune register */
	ay_rn_ftr_c  = 0x5,
	/** Noise period register */
	ay_rn_npr    = 0x6,
	/** Mixer control - I/O enable */
	ay_rn_mcioen = 0x7,
	/** Channel A amplitude control register */
	ay_rn_amp_a  = 0x8,
	/** Channel B amplitude control register */
	ay_rn_amp_b  = 0x9,
	/** Channel C amplitude control register */
	ay_rn_amp_c  = 0xa,
	/** Envelope coarse tune register */
	ay_rn_ectr   = 0xb,
	/** Envelope fine tune register */
	ay_rn_eftr   = 0xc,
	/** Envelope shape, cycle control register */
	ay_rn_esccr  = 0xd,
	/** I/O port A data store */
	ay_rn_io_a   = 0xe
};

typedef struct {
	/** AY register state */
	uint8_t reg[ay_nreg];
	/** Selected register */
	uint8_t cur_reg;

	uint16_t tone_cnt[ay_nchan];
	uint8_t  tone_smp[ay_nchan];
	uint32_t env_cnt[ay_nchan];
	uint32_t env_pn[ay_nchan];
	uint8_t  env_pp[ay_nchan];
	uint8_t  env_smp[ay_nchan];
	uint16_t noise_cnt;
	uint8_t  noise_smp;
	uint32_t d_clocks;

	void (*ioport_write)(void *, uint8_t);
	void *ioport_write_arg;
} ay_t;

extern void ay_reg_select(ay_t *, uint8_t);
extern void ay_reg_write(ay_t *, uint8_t);
extern uint8_t ay_reg_read(ay_t *);

extern uint8_t ay_get_sel_regn(ay_t *);
extern uint8_t ay_get_reg_contents(ay_t *, uint8_t);

extern void ay_init(ay_t *, uint32_t);
extern void ay_reset(ay_t *);
extern int ay_get_sample(ay_t *);

#endif
