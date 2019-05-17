/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum tape
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

/**
 * @file Spectrum tape.
 *
 * This is an in-core, editable, representation of Spectrum tape. It should
 * be able to perfectly represent any TZX file.
 */

#ifndef TAPE_TAPE_H
#define TAPE_TAPE_H

#include <stdint.h>
#include "../types/tape/tape.h"

extern int tape_create(tape_t **);
extern void tape_destroy(tape_t *);
extern tape_block_t *tape_first(tape_t *);
extern tape_block_t *tape_last(tape_t *);
extern tape_block_t *tape_next(tape_block_t *);
extern tape_block_t *tape_prev(tape_block_t *);
extern void tape_append(tape_t *, tape_block_t *);
extern void tape_block_destroy(tape_block_t *);
extern int tblock_data_create(tblock_data_t **);
extern void tblock_data_destroy(tblock_data_t *);
extern int tblock_turbo_data_create(tblock_turbo_data_t **);
extern void tblock_turbo_data_destroy(tblock_turbo_data_t *);
extern int tblock_direct_rec_create(tblock_direct_rec_t **);
extern void tblock_direct_rec_destroy(tblock_direct_rec_t *);
extern int tblock_pause_create(tblock_pause_t **);
extern void tblock_pause_destroy(tblock_pause_t *);
extern int tblock_stop_create(tblock_stop_t **);
extern void tblock_stop_destroy(tblock_stop_t *);
extern int tblock_archive_info_create(tblock_archive_info_t **);
extern void tblock_archive_info_destroy(tblock_archive_info_t *);
extern int tblock_archive_info_create(tblock_archive_info_t **);
extern void tblock_archive_info_destroy(tblock_archive_info_t *);
extern tape_text_t *tblock_archive_info_first(tblock_archive_info_t *);
extern tape_text_t *tblock_archive_info_last(tblock_archive_info_t *);
extern tape_text_t *tblock_archive_info_next(tape_text_t *);
extern tape_text_t *tblock_archive_info_prev(tape_text_t *);
extern int tape_text_create(tape_text_t **);
extern void tape_text_destroy(tape_text_t *);
extern int tblock_unknown_create(tblock_unknown_t **);
extern void tblock_unknown_destroy(tblock_unknown_t *);
extern const char *tape_btype_str(tape_btype_t);

#endif
