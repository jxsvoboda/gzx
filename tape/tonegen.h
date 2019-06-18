/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
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

/**
 * @file Tape tone generator.
 *
 * Produce waveform based on a description, consiting of a series of
 * tones.
 */

#ifndef TAPE_TONEGEN_H
#define TAPE_TONEGEN_H

#include <stdbool.h>
#include <stdint.h>
#include "../types/tape/tape.h"
#include "../types/tape/tonegen.h"

extern void tonegen_init(tonegen_t *, tape_lvl_t);
extern void tonegen_clear(tonegen_t *);
extern void tonegen_add_tone(tonegen_t *, unsigned, unsigned);
extern void tonegen_add_dpulse(tonegen_t *, tape_lvl_t, unsigned);
extern bool tonegen_is_end(tonegen_t *);
extern tape_lvl_t tonegen_cur_lvl(tonegen_t *);
extern void tonegen_get_next(tonegen_t *, uint32_t *, tape_lvl_t *);

#endif
