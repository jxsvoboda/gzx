/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 GPU (a.k.a. Spec256)
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

#ifndef Z80G_H
#define Z80G_H

#include <stdbool.h>
#include "intdef.h"
#include "z80.h"

/* Number of graphical planes */
#define NGP 8

extern z80s gpus[NGP];
/** Always points to real CPU, never GPU */
extern z80s *rcpus;

extern u8 *gfxrom[NGP];
extern u8 *gfxram[NGP];
extern u8 *gfxscr[NGP];

void gpu_init(void);
int gpu_enable(void);
void gpu_disable(void);
int gpu_reset(void);
bool gpu_is_on(void);
void z80_g_execinstr(void); /* execute both on CPU and GPU */
void z80_g_int(u8 bus);       /* INT both on CPU and GPU */
void gfx_select_memmodel(int model);

#endif
