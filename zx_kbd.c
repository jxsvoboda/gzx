/*
 * GZX - George's ZX Spectrum Emulator
 * ZX keyboard
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mgfx.h"
#include "zx_kbd.h"
#include "zx_keys.h"

/** Clear keyboard matrix.
 *
 * @param km ZX keyboard matrix
 */
static void zx_keymtx_clear(zx_keymtx_t *km)
{
	int i;

	for (i = 0; i < zx_keymtx_rows; i++)
		km->mask[i] = 0x00;
}

/** Logically OR keyboard matrix to another matrix.
 *
 * Performs @a dst <- @a dst OR @a src
 *
 * @param dst Destination matrix
 * @param src Source matrix
 */
static void zx_keymtx_or(zx_keymtx_t *dst, zx_keymtx_t *src)
{
	int i;

	for (i = 0; i < zx_keymtx_rows; i++)
		dst->mask[i] |= src->mask[i];
}

/** Returns the 6 keyboard bits.
 *
 * @param keys ZX keyboard
 * @param out State of output lines
 * @return Corresponding input line state
 */
uint8_t zx_key_in(zx_keys_t *keys, uint8_t out)
{
	uint8_t res;
	int i;

	/* don't simulate matrix behaviour for now.. */
	res = 0x00;

	for (i = 0; i < zx_keymtx_rows; i++)
		if ((out & (0x01 << i)) == 0)
			res |= keys->kmstate.mask[i];

	return res ^ 0x1f;
}

/** Update keyboard matrix state.
 *
 * Update internal keyboard matrix state based on emulator key states
 *
 * @param keys Emulated keyboard
 */
static void zx_keys_recalc(zx_keys_t *keys)
{
	int i;

	for (i = 0; i < zx_keymtx_rows; i++)
		zx_keymtx_clear(&keys->kmstate);

	for (i = 0; i < KST_SIZE; i++) {
		if (keys->pressed[i])
			zx_keymtx_or(&keys->kmstate, &keys->map.mtx[i]);
	}
}

/** Register key mapping.
 *
 * @param keys Emulated keyboard
 * @param key Emulator key code
 * @param m0 Mask for keyboard matrix row 0
 * @param m1 Mask for keyboard matrix row 1
 * @param m2 Mask for keyboard matrix row 2
 * @param m3 Mask for keyboard matrix row 3
 * @param m4 Mask for keyboard matrix row 4
 * @param m5 Mask for keyboard matrix row 5
 * @param m6 Mask for keyboard matrix row 6
 * @param m7 Mask for keyboard matrix row 7
 */
static void zx_key_register(zx_keys_t *keys, int key, uint16_t m0, uint16_t m1,
    uint16_t m2, uint16_t m3, uint16_t m4, uint16_t m5, uint16_t m6,
    uint16_t m7)
{
	if (key >= KST_SIZE) {
		printf("error: key cannot be registered - scancode too high. "
		    "enlarge KST_SIZE\n");
		return;
	}

	keys->map.mtx[key].mask[0] = m0;
	keys->map.mtx[key].mask[1] = m1;
	keys->map.mtx[key].mask[2] = m2;
	keys->map.mtx[key].mask[3] = m3;
	keys->map.mtx[key].mask[4] = m4;
	keys->map.mtx[key].mask[5] = m5;
	keys->map.mtx[key].mask[6] = m6;
	keys->map.mtx[key].mask[7] = m7;
}

/** Initialize emulated keyboard.
 *
 * @param keys Emulated keyboard
 */
void zx_keys_init(zx_keys_t *keys)
{
	int i;

	for (i = 0; i < KST_SIZE; i++)
		keys->pressed[i] = false;

	for (i = 0; i < KST_SIZE; i++)
		zx_keymtx_clear(&keys->map.mtx[i]);

	/* create zx-mask for each key */

	zx_key_register(keys, WKEY_V,	   ZX_KEY_V, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_C,	   ZX_KEY_C, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_X,	   ZX_KEY_X, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_Z,      ZX_KEY_Z, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_LSHIFT, ZX_KEY_CS, 0, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_RSHIFT, ZX_KEY_CS, 0, 0, 0, 0, 0, 0, 0);

	zx_key_register(keys, WKEY_G,	0, ZX_KEY_G, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_F,	0, ZX_KEY_F, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_D,	0, ZX_KEY_D, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_S,	0, ZX_KEY_S, 0, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_A,	0, ZX_KEY_A, 0, 0, 0, 0, 0, 0);

	zx_key_register(keys, WKEY_T,	0, 0, ZX_KEY_T, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_R,	0, 0, ZX_KEY_R, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_E,	0, 0, ZX_KEY_E, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_W,	0, 0, ZX_KEY_W, 0, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_Q,	0, 0, ZX_KEY_Q, 0, 0, 0, 0, 0);

	zx_key_register(keys, WKEY_5,	0, 0, 0, ZX_KEY_5, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_4,	0, 0, 0, ZX_KEY_4, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_3,	0, 0, 0, ZX_KEY_3, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_2,	0, 0, 0, ZX_KEY_2, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_1,	0, 0, 0, ZX_KEY_1, 0, 0, 0, 0);

	zx_key_register(keys, WKEY_6,	0, 0, 0, 0, ZX_KEY_6, 0, 0, 0);
	zx_key_register(keys, WKEY_7,	0, 0, 0, 0, ZX_KEY_7, 0, 0, 0);
	zx_key_register(keys, WKEY_8,	0, 0, 0, 0, ZX_KEY_8, 0, 0, 0);
	zx_key_register(keys, WKEY_9,	0, 0, 0, 0, ZX_KEY_9, 0, 0, 0);
	zx_key_register(keys, WKEY_0,	0, 0, 0, 0, ZX_KEY_0, 0, 0, 0);

	zx_key_register(keys, WKEY_Y,	0, 0, 0, 0, 0, ZX_KEY_Y, 0, 0);
	zx_key_register(keys, WKEY_U,	0, 0, 0, 0, 0, ZX_KEY_U, 0, 0);
	zx_key_register(keys, WKEY_I,	0, 0, 0, 0, 0, ZX_KEY_I, 0, 0);
	zx_key_register(keys, WKEY_O,	0, 0, 0, 0, 0, ZX_KEY_O, 0, 0);
	zx_key_register(keys, WKEY_P,	0, 0, 0, 0, 0, ZX_KEY_P, 0, 0);

	zx_key_register(keys, WKEY_H,     0, 0, 0, 0, 0, 0, ZX_KEY_H, 0);
	zx_key_register(keys, WKEY_J,     0, 0, 0, 0, 0, 0, ZX_KEY_J, 0);
	zx_key_register(keys, WKEY_K,     0, 0, 0, 0, 0, 0, ZX_KEY_K, 0);
	zx_key_register(keys, WKEY_L,	  0, 0, 0, 0, 0, 0, ZX_KEY_L, 0);
	zx_key_register(keys, WKEY_ENTER, 0, 0, 0, 0, 0, 0, ZX_KEY_ENT, 0);

	zx_key_register(keys, WKEY_B,	   0, 0, 0, 0, 0, 0, 0, ZX_KEY_B);
	zx_key_register(keys, WKEY_N,	   0, 0, 0, 0, 0, 0, 0, ZX_KEY_N);
	zx_key_register(keys, WKEY_M,	   0, 0, 0, 0, 0, 0, 0, ZX_KEY_M);
	zx_key_register(keys, WKEY_SPACE,  0, 0, 0, 0, 0, 0, 0, ZX_KEY_SP);

	zx_key_register(keys, WKEY_BS,    ZX_KEY_CS, 0, 0, 0, ZX_KEY_0, 0, 0, 0);
	zx_key_register(keys, WKEY_LEFT,  0, 0, 0, ZX_KEY_5, 0, 0, 0, 0);
	zx_key_register(keys, WKEY_DOWN,  0, 0, 0, 0, ZX_KEY_6, 0, 0, 0);
	zx_key_register(keys, WKEY_UP,    0, 0, 0, 0, ZX_KEY_7, 0, 0, 0);
	zx_key_register(keys, WKEY_RIGHT, 0, 0, 0, 0, ZX_KEY_8, 0, 0, 0);
	zx_key_register(keys, WKEY_N0,    0, 0, 0, 0, ZX_KEY_0, 0, 0, 0);
	zx_key_register(keys, WKEY_LCTRL, 0, 0, 0, 0, 0, 0, 0, ZX_KEY_SS);
	zx_key_register(keys, WKEY_RCTRL, 0, 0, 0, 0, 0, 0, 0, ZX_KEY_SS);
}

/** Set key state.
 *
 * Set the state of an emulator key.
 *
 * @param keys Emulated keyboard
 * @param key Emulator key code
 * @param press Pressed (not zero) or released (zero)
 */
void zx_key_state_set(zx_keys_t *keys, int key, int press)
{
	keys->pressed[key] = press ? true : false;
	zx_keys_recalc(keys);
}
