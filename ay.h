/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
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

extern int ay_init(ay_t *, uint32_t);
extern void ay_reset(ay_t *);
extern int ay_get_sample(ay_t *);

#endif
