/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ay.h"

uint8_t ay_reg[16];
int ay_cur_reg;

static unsigned long tone_cnt[3];
static unsigned      tone_smp[3];
static unsigned long env_cnt[3];
static unsigned long env_pn[3];
static unsigned long env_pp[3];
static unsigned      env_smp[3];
static unsigned long noise_cnt;
static uint8_t       noise_smp;
static unsigned long d_clocks;

static unsigned      chn_out[3];

/** Envelope fragments: const. 0, rising, falling, const. 1 */
static int env_v_tab[4][16] = {
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0 },
	{ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }
};

/** Envelope shapes expressed as indices into @c env_v_tab */
static int env_shape_tab[16][3] = {
  { 2, 0, 0 }, { 2, 0, 0 }, { 2, 0, 0}, { 2, 0, 0 },
  { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0}, { 1, 0, 0 },
  { 2, 2, 2 }, { 2, 0, 0 }, { 2, 1, 2}, { 2, 3, 3 },
  { 1, 1, 1 }, { 1, 3, 3 }, { 1, 2, 1}, { 1, 0, 0 }
};

/** Select AY register.
 *
 * @param regn Register
 */
void ay_reg_select(uint8_t regn)
{
	ay_cur_reg = regn & 0x0f;
}

/** Reset envelope generator. */
static void reset_env_gen(void)
{
	env_cnt[0] = 0;
	env_cnt[1] = 0;
	env_cnt[2] = 0;
	env_pn[0] = 0;
	env_pn[1] = 0;
	env_pn[2] = 0;
	env_pp[0] = 0;
	env_pp[1] = 0;
	env_pp[2] = 0;
}

/** Write AY register.
 *
 * @param val Value
 */
void ay_reg_write(uint8_t val)
{
	switch (ay_cur_reg) {
	case 13: reset_env_gen(); break;
	}

	ay_reg[ay_cur_reg] = val;
//	printf("ay_reg_write %d,0x%02x\n", ay_cur_reg, val);
}

/** Read AY register.
 *
 * @return Register value
 */
uint8_t ay_reg_read(void)
{
	return ay_reg[ay_cur_reg];
}

/** Reset AY. */
void ay_reset(void)
{
	ay_reg[7] = 0x3f;
	noise_cnt = 0;
}

/** Initialize AY emulation.
 *
 * @param d_t_states Number of T states per sample period.
 */
int ay_init(unsigned long d_t_states)
{
	d_clocks = d_t_states;
	ay_reset();
	return 0;
}

/** Get next AY sample.
 *
 * @return AY sample
 */
int ay_get_sample(void)
{
	int i,smp;
	unsigned long period;
	int is_tone, is_noise;
	int vol;

	int eshape, eblk;

	/* tone generator */
	for(i = 0; i < 3; i++) {
		period = ay_reg[2 * i] +
		    ((unsigned)(ay_reg[2 * i + 1] & 0x0f) << 8);
		if (period == 0)
			period = 1;
		period <<= 4;

		if (period != 0) {
			tone_cnt[i] += d_clocks;
			if (tone_cnt[i] >= period) { /* generate new sample */
				tone_smp[i] ^= (tone_cnt[i] / period) & 1;
				tone_cnt[i]  =  tone_cnt[i] % period;
			}
		} else tone_cnt[i] = 0;
	}

	/* envelope generator */
	eshape = ay_reg[13] & 0x0f;

	for (i = 0; i < 3; i++) {
		period = ((unsigned)ay_reg[11]) | ((unsigned)ay_reg[12] << 8);
		if (period == 0)
		    period = 1;
		period <<= 4;
		env_cnt[i] += d_clocks;

		if (env_cnt[i] >= period) {
			/* advance in envelope */
			env_pp[i]  +=  env_cnt[i] / period;
			env_cnt[i]  =  env_cnt[i] % period;

			env_pn[i]  += env_pp[i] >> 4;
			env_pp[i]   = env_pp[i] & 0x0f;

			if (env_pn[i] > 2)
				env_pn[i] = (env_pn[i] & 1) ? 1 : 2;

			/* const.0/rising/falling/const.1 */
			eblk = env_shape_tab[eshape][env_pn[i]];

			/* specific volume */
			env_smp[i] = env_v_tab[eblk][env_pp[i]];
		}
	}

	/* noise generator */
	period = ay_reg[6] & 0x1f;
	if (period == 0)
		period = 1;
	period <<= 4;

	if (noise_cnt < period) {
		noise_cnt += d_clocks;
		if (noise_cnt >= period) {
			/*
			 * Generate new sample.
			 *
			 * we don't have to generate skipped samples so just
			 * generate the last one
			 */
			noise_smp = (rand() & 0x8000) ? 1 : 0;
			noise_cnt = noise_cnt % period;
		}
	}

	/* mixer */
	for (i = 0; i < 3; i++) {
		is_tone = !!(ay_reg[7] & (1 << i));
		is_noise = !!(ay_reg[7] & (1 << (i + 3)));

		chn_out[i] = (tone_smp[i] || is_tone) &&
		    (noise_smp || is_noise);
	}

	smp = 0;
	for (i = 0; i < 3; i++) {
		if (ay_reg[8 + i] & 0x10) {
			 /* use envelope generator */
			vol = env_smp[i];
		} else {
			/* use direct value for volume */
			vol = ay_reg[8 + i] & 0x0f;
		}

		smp += chn_out[i] ? vol : 0;
	}

	return smp;
}
