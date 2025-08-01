/*
 * GZX - George's ZX Spectrum Emulator
 * Memory and I/O port access
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

#ifndef MEMIO_H
#define MEMIO_H

#include <stdint.h>

/* memory models */
#define ZXM_48K    0
#define ZXM_128K   1
#define ZXM_PLUS2  2
#define ZXM_PLUS2A 3
#define ZXM_PLUS3  4
#define ZXM_ZX81   5

/* spectrum memory access */
extern uint8_t zx_memget8(uint16_t addr);
extern uint8_t zx_imemget8(uint16_t addr);
extern void zx_memset8(uint16_t addr, uint8_t val);
extern void zx_memset8f(uint16_t addr, uint8_t val);
extern uint16_t zx_memget16(uint16_t addr);
extern void zx_memset16(uint16_t addr, uint16_t val);

/* spectrum i/o port access */
extern void zx_out8(uint16_t addr, uint8_t val);
extern uint8_t zx_in8(uint16_t addr);

extern int zx_select_memmodel(int model);
extern void zx_mem_page_select(uint16_t, uint8_t val);
extern void zx_mem_page_reset(void);
extern int zx_mem_is_48k_basic_rom(void);
extern int gfxrom_load(char *fname, unsigned bank);

extern uint8_t page_reg;
extern uint8_t epg_reg;
extern uint8_t border;
extern uint8_t spk, mic, ear;
extern uint8_t *zxram, *zxrom, *zxscr, *zxbnk[4];
extern int mem_model;

extern int bnk_lock48;
extern uint32_t ram_size, rom_size;
extern int has_banksw;
extern int has_epg;

#endif
