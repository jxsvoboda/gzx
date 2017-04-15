/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
 */
#ifndef AY_H
#define AY_H

#include <stdint.h>

enum {
	ay_nchan = 3,
	ay_nreg = 16
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
