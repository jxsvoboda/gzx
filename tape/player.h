/*
 * GZX - George's ZX Spectrum Emulator
 * Tape player
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
 * @file Tape player.
 *
 * Produce waveform from spectrum tape.
 */

#ifndef TAPE_PLAYER_H
#define TAPE_PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include "../types/tape/player.h"
#include "../types/tape/tape.h"

extern int tape_player_create(tape_player_t **);
extern void tape_player_init(tape_player_t *, tape_block_t *);
extern void tape_player_destroy(tape_player_t *);
extern bool tape_player_is_end(tape_player_t *);
extern tape_lvl_t tape_player_cur_lvl(tape_player_t *);
extern tape_block_t *tape_player_cur_block(tape_player_t *);
extern void tape_player_get_next(tape_player_t *, uint32_t *, tape_lvl_t *,
    tape_player_sig_t *);

#endif
