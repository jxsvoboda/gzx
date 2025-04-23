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
#ifndef ZX_KBD_H
#define ZX_KBD_H

#include <stdbool.h>
#include <stdint.h>
#include "mgfx.h"
#include "zx_keys.h"

/** ZX keyboard matrix (state for each ZX key) */
typedef struct {
	uint8_t mask[zx_keymtx_rows];
} zx_keymtx_t;

/** Map emulator keys to ZX Spectrum keys */
typedef struct {
	/** For each emulator key we have the corresponding ZX keyboard matrix */
	zx_keymtx_t mtx[KST_SIZE];
} zx_keymap_t;

/** Emulated keyboard */
typedef struct {
	bool pressed[KST_SIZE];
	zx_keymtx_t kmstate;
	zx_keymap_t map;
} zx_keys_t;

uint8_t zx_key_in(zx_keys_t *, uint8_t pwr);
void zx_key_state_set(zx_keys_t *, int key, int press);
void zx_keys_init(zx_keys_t *);

#endif
