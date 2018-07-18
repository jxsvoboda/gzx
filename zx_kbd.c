/*
 * GZX - George's ZX Spectrum Emulator
 * ZX keyboard
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

#include <stdint.h>
#include <stdio.h>
#include "mgfx.h"
#include "zx_kbd.h"
#include "zx_keys.h"

uint8_t zx_keymap[8];
int key_state[KST_SIZE];
uint8_t key_mask[KST_SIZE * 8];

/** Returns the 6 keyboard bits.
 *
 * @param pwr power mask
 */
uint8_t zx_key_in(uint8_t pwr)
{
	uint8_t res;
	int i;
	/* don't simulate matrix behaviour for now.. */
	res = 0x00;

	for (i = 0; i < 8; i++)
		if ((pwr & (0x01 << i)) == 0)
			res |= zx_keymap[i];

	return res ^ 0x1f;
}

/* figure out which crossings are connected */
static void zx_keys_recalc(void)
{
	int i, j;

	for (i = 0; i < 8; i++)
		zx_keymap[i] = 0x00;

	for (i = 0; i < KST_SIZE; i++) {
		if (key_state[i]) {
			for (j = 0; j < 8; j++) {
				zx_keymap[j] |= key_mask[8 * i + j];
			}
		}
	}
}

static void zx_key_register(int key, uint16_t m0, uint16_t m1, uint16_t m2, uint16_t m3,
    uint16_t m4, uint16_t m5, uint16_t m6, uint16_t m7)
{
	if (key >= KST_SIZE) {
		printf("error: key cannot be registered - scancode too high. enlarge KST_SIZE\n");
		return;
	}

	key_mask[8 * key + 0] = m0;
	key_mask[8 * key + 1] = m1;
	key_mask[8 * key + 2] = m2;
	key_mask[8 * key + 3] = m3;
	key_mask[8 * key + 4] = m4;
	key_mask[8 * key + 5] = m5;
	key_mask[8 * key + 6] = m6;
	key_mask[8 * key + 7] = m7;
}

int zx_keys_init(void)
{
	int i;

	for (i = 0; i < KST_SIZE; i++)
		key_state[i] = 0;
	for (i = 0; i < KST_SIZE * 8; i++)
		key_mask[i] = 0;

	/* create a zx-mask for each key */
	/* ZX48-like mapping */
	zx_key_register(WKEY_V,	     ZX_KEY_V, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_C,	     ZX_KEY_C, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_X,	     ZX_KEY_X, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_Z,	     ZX_KEY_Z, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_LSHIFT, ZX_KEY_CS, 0, 0, 0, 0, 0, 0, 0);

	zx_key_register(WKEY_G,	0, ZX_KEY_G, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_F,	0, ZX_KEY_F, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_D,	0, ZX_KEY_D, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_S,	0, ZX_KEY_S, 0, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_A,	0, ZX_KEY_A, 0, 0, 0, 0, 0, 0);

	zx_key_register(WKEY_T,	0, 0, ZX_KEY_T, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_R,	0, 0, ZX_KEY_R, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_E,	0, 0, ZX_KEY_E, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_W,	0, 0, ZX_KEY_W, 0, 0, 0, 0, 0);
	zx_key_register(WKEY_Q,	0, 0, ZX_KEY_Q, 0, 0, 0, 0, 0);
  
	zx_key_register(WKEY_5,	0, 0, 0, ZX_KEY_5, 0, 0, 0, 0);
	zx_key_register(WKEY_4,	0, 0, 0, ZX_KEY_4, 0, 0, 0, 0);
	zx_key_register(WKEY_3,	0, 0, 0, ZX_KEY_3, 0, 0, 0, 0);
	zx_key_register(WKEY_2,	0, 0, 0, ZX_KEY_2, 0, 0, 0, 0);
	zx_key_register(WKEY_1,	0, 0, 0, ZX_KEY_1, 0, 0, 0, 0);

	zx_key_register(WKEY_6,	0, 0, 0, 0, ZX_KEY_6, 0, 0, 0);
	zx_key_register(WKEY_7,	0, 0, 0, 0, ZX_KEY_7, 0, 0, 0);
	zx_key_register(WKEY_8,	0, 0, 0, 0, ZX_KEY_8, 0, 0, 0);
	zx_key_register(WKEY_9,	0, 0, 0, 0, ZX_KEY_9, 0, 0, 0);
	zx_key_register(WKEY_0,	0, 0, 0, 0, ZX_KEY_0, 0, 0, 0);

	zx_key_register(WKEY_Y,	0, 0, 0, 0, 0, ZX_KEY_Y, 0, 0);
	zx_key_register(WKEY_U,	0, 0, 0, 0, 0, ZX_KEY_U, 0, 0);
	zx_key_register(WKEY_I,	0, 0, 0, 0, 0, ZX_KEY_I, 0, 0);
	zx_key_register(WKEY_O,	0, 0, 0, 0, 0, ZX_KEY_O, 0, 0);
	zx_key_register(WKEY_P,	0, 0, 0, 0, 0, ZX_KEY_P, 0, 0);

	zx_key_register(WKEY_H,     0, 0, 0, 0, 0, 0, ZX_KEY_H, 0);
	zx_key_register(WKEY_J,     0, 0, 0, 0, 0, 0, ZX_KEY_J, 0);
	zx_key_register(WKEY_K,     0, 0, 0, 0, 0, 0, ZX_KEY_K, 0);
	zx_key_register(WKEY_L,	    0, 0, 0, 0, 0, 0, ZX_KEY_L, 0);
	zx_key_register(WKEY_ENTER, 0, 0, 0, 0, 0, 0, ZX_KEY_ENT, 0);

	zx_key_register(WKEY_B,	     0, 0, 0, 0, 0, 0, 0, ZX_KEY_B);
	zx_key_register(WKEY_N,	     0, 0, 0, 0, 0, 0, 0, ZX_KEY_N);
	zx_key_register(WKEY_M,	     0, 0, 0, 0, 0, 0, 0, ZX_KEY_M);
	zx_key_register(WKEY_RSHIFT, 0, 0, 0, 0, 0, 0, 0, ZX_KEY_SS);
	zx_key_register(WKEY_SPACE,  0, 0, 0, 0, 0, 0, 0, ZX_KEY_SP);

	/* some more keys */
	zx_key_register(WKEY_BS,    ZX_KEY_CS, 0, 0, 0, ZX_KEY_0, 0, 0, 0);
	zx_key_register(WKEY_LEFT,  0, 0, 0, ZX_KEY_5, 0, 0, 0, 0);
	zx_key_register(WKEY_DOWN,  0, 0, 0, 0, ZX_KEY_6, 0, 0, 0);
	zx_key_register(WKEY_UP,    0, 0, 0, 0, ZX_KEY_7, 0, 0, 0);
	zx_key_register(WKEY_RIGHT, 0, 0, 0, 0, ZX_KEY_8, 0, 0, 0);
	zx_key_register(WKEY_N0,    0, 0, 0, 0, ZX_KEY_0, 0, 0, 0);
	zx_key_register(WKEY_LCTRL, 0, 0, 0, 0, 0, 0, 0, ZX_KEY_SS);

	return 0;
}

void zx_key_state_set(int key, int press)
{
	key_state[key] = press ? 1 : 0;
	zx_keys_recalc();
}
