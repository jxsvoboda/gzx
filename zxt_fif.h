/*
 * GZX - George's ZX Spectrum Emulator
 * Tape file interface
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

#ifndef ZXT_FIF_H
#define ZXT_FIF_H

#include "intdef.h"
#include "zx_tape.h"

typedef struct {
  int (*open_file)(char *filename);
  int (*close_file)(void);
  int (*rewind_file)(void);
  
  int (*block_type)(void);
  int (*get_b_data_info)(tb_data_info_t *info);
  int (*get_b_voice_info)(tb_voice_info_t *voice);
  
  int (*skip_block)(void);
  int (*open_block)(void);
  int (*close_block)(void);
  
  int (*b_data_getbytes)(int n, u8 *dst);
  int (*b_voice_getsmps)(int n, unsigned *dst);
  int (*b_tones_gettone)(int *pnum, int *plen);
  
  int (*b_moredata)(void);
} tfr_t;

#endif
