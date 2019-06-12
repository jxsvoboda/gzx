/*
 * GZX - George's ZX Spectrum Emulator
 * Tape deck
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
 * @file Tape deck.
 */

#ifndef TAPE_DECK_H
#define TAPE_DECK_H

#include <stdbool.h>
#include <stdint.h>
#include "../types/tape/deck.h"

extern int tape_deck_create(tape_deck_t **);
extern void tape_deck_destroy(tape_deck_t *);

extern int tape_deck_new(tape_deck_t *);
extern int tape_deck_open(tape_deck_t *, const char *);
extern int tape_deck_save(tape_deck_t *);
extern int tape_deck_save_as(tape_deck_t *, const char *);

extern void tape_deck_play(tape_deck_t *);
extern void tape_deck_pause(tape_deck_t *);
extern void tape_deck_stop(tape_deck_t *);
extern void tape_deck_rewind(tape_deck_t *);
extern void tape_deck_next(tape_deck_t *);

extern bool tape_deck_is_playing(tape_deck_t *);
extern void tape_deck_getsmp(tape_deck_t *, uint8_t *smp);
extern tape_block_t *tape_deck_cur_block(tape_deck_t *);

#endif
