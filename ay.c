/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "ay.h"

/** Envelope fragments: const. 0, rising, falling, const. 1 */
static int const env_v_tab[4][16] = {
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0 },
	{ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }
};

/** Envelope shapes expressed as indices into @c env_v_tab */
static int const env_shape_tab[16][3] = {
	{ 2, 0, 0 }, { 2, 0, 0 }, { 2, 0, 0}, { 2, 0, 0 },
	{ 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0}, { 1, 0, 0 },
	{ 2, 2, 2 }, { 2, 0, 0 }, { 2, 1, 2}, { 2, 3, 3 },
	{ 1, 1, 1 }, { 1, 3, 3 }, { 1, 2, 1}, { 1, 0, 0 }
};

/** Select AY register.
 *
 * @param ay AY
 * @param regn Register
 */
void ay_reg_select(ay_t *ay, uint8_t regn)
{
	ay->cur_reg = regn & 0x0f;
}

/** Reset envelope generator.
 *
 * @param ay AY
 */
static void reset_env_gen(ay_t *ay)
{
	uint8_t i;

	for (i = 0; i < ay_nchan; i++) {
		ay->env_cnt[i] = 0;
		ay->env_pn[i] = 0;
		ay->env_pp[i] = 0;
	}
}

/** Write AY I/O port.
 *
 * @param ay AY
 * @param val Value
 */
static void ay_io_port_write(ay_t *ay, uint8_t val)
{
	if (ay->ioport_write != NULL)
		ay->ioport_write(ay->ioport_write_arg, val);
}

/** Write AY register.
 *
 * @param ay AY
 * @param val Value
 */
void ay_reg_write(ay_t *ay, uint8_t val)
{
	switch (ay->cur_reg) {
	case ay_rn_esccr: reset_env_gen(ay); break;
	case ay_rn_io_a: ay_io_port_write(ay, val); break;
	}

	ay->reg[ay->cur_reg] = val;
}

/** Read currently selected AY register.
 *
 * @param ay AY
 * @return Register value
 */
uint8_t ay_reg_read(ay_t *ay)
{
	return ay->reg[ay->cur_reg];
}

/** Reset AY.
 *
 * @param ay AY
 */
void ay_reset(ay_t *ay)
{
	ay->reg[ay_rn_mcioen] = 0x3f;
	ay->noise_cnt = 0;
}

/** Initialize AY emulation.
 *
 * @param ay AY
 * @param d_t_states Number of T states per sample period.
 */
int ay_init(ay_t *ay, uint32_t d_t_states)
{
	ay->d_clocks = d_t_states;
	ay_reset(ay);
	return 0;
}

/** Get next AY sample.
 *
 * @param ay AY
 * @return AY sample
 */
int ay_get_sample(ay_t *ay)
{
	int i, smp;
	uint32_t period;
	uint32_t cnt;
	int is_tone, is_noise;
	uint8_t chn_out[ay_nchan];
	int vol;

	int eshape, eblk;

	/* tone generator */
	for(i = 0; i < ay_nchan; i++) {
		period = ay->reg[ay_rn_ctr_a + 2 * i] +
		    ((unsigned)(ay->reg[ay_rn_ftr_a + 2 * i] & 0x0f) << 8);
		if (period == 0)
			period = 1;
		period <<= 4;

		if (period != 0) {
			cnt = ay->tone_cnt[i] + ay->d_clocks;
			if (cnt >= period) { /* generate new sample */
				ay->tone_smp[i] ^= (cnt / period) & 1;
				ay->tone_cnt[i]  = (uint16_t)(cnt % period);
			} else {
				ay->tone_cnt[i] = cnt;
			}
		} else ay->tone_cnt[i] = 0;
	}

	/* envelope generator */
	eshape = ay->reg[ay_rn_esccr] & 0x0f;

	for (i = 0; i < ay_nchan; i++) {
		period = ((uint16_t)ay->reg[ay_rn_ectr]) |
		    ((uint16_t)ay->reg[ay_rn_eftr] << 8);
		if (period == 0)
		    period = 1;
		period <<= 4;
		ay->env_cnt[i] += ay->d_clocks;

		if (ay->env_cnt[i] >= period) {
			/* advance in envelope */
			ay->env_pp[i]  +=  ay->env_cnt[i] / period;
			ay->env_cnt[i]  =  ay->env_cnt[i] % period;

			ay->env_pn[i]  += ay->env_pp[i] >> 4;
			ay->env_pp[i]   = ay->env_pp[i] & 0x0f;

			if (ay->env_pn[i] > 2)
				ay->env_pn[i] = (ay->env_pn[i] & 1) ? 1 : 2;

			/* const.0/rising/falling/const.1 */
			eblk = env_shape_tab[eshape][ay->env_pn[i]];

			/* specific volume */
			ay->env_smp[i] = env_v_tab[eblk][ay->env_pp[i]];
		}
	}

	/* noise generator */
	period = ay->reg[ay_rn_npr] & 0x1f;
	if (period == 0)
		period = 1;
	period <<= 4;

	cnt = ay->noise_cnt + ay->d_clocks;
	if (cnt >= period) {
		/*
		 * Generate new sample.
		 *
		 * we don't have to generate skipped samples so just
		 * generate the last one
		 */
		ay->noise_smp = (rand() & 0x8000) ? 1 : 0;
		ay->noise_cnt = cnt % period;
	} else {
		ay->noise_cnt = cnt;
	}

	/* mixer */
	for (i = 0; i < ay_nchan; i++) {
		is_tone = !!(ay->reg[ay_rn_mcioen] & (1 << i));
		is_noise = !!(ay->reg[ay_rn_mcioen] & (1 << (i + 3)));

		chn_out[i] = (ay->tone_smp[i] || is_tone) &&
		    (ay->noise_smp || is_noise);
	}

	smp = 0;
	for (i = 0; i < ay_nchan; i++) {
		if (ay->reg[ay_rn_amp_a + i] & 0x10) {
			 /* use envelope generator */
			vol = ay->env_smp[i];
		} else {
			/* use direct value for volume */
			vol = ay->reg[ay_rn_amp_a + i] & 0x0f;
		}

		smp += chn_out[i] ? vol : 0;
	}

	return smp;
}

/** Get selected register number.
 *
 * @param ay AY
 * @return Selected register number
 */
uint8_t ay_get_sel_regn(ay_t *ay)
{
	return ay->cur_reg;
}

/** Get contents of a specific register.
 *
 * @param regn Register number
 * @return Register contents
 */
uint8_t ay_get_reg_contents(ay_t *ay, uint8_t regn)
{
	assert(regn < ay_nreg);
	return ay->reg[regn];
}
