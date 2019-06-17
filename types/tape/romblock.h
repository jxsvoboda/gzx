/*
 * GZX - George's ZX Spectrum Emulator
 * Standard ROM tape block format
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
 * @file Standard ROM tape block format
 */

#ifndef TYPES_ROMBLOCK_H
#define TYPES_ROMBLOCK_H

#include <stdint.h>

enum {
	/** Standard file header */
	bflag_header = 0x00,
	/** Standard file data */
	bflag_data = 0xff
};

typedef enum {
	/** Program */
	ftype_program = 0x00,
	/** Number array */
	ftype_number_array = 0x01,
	/** Character array */
	ftype_character_array = 0x02,
	/** Bytes */
	ftype_bytes = 0x03
} rom_ftype_t;

/** Standard ROM header block (19 bytes) */
typedef struct {
	/** 0x00 for standard header */
	uint8_t flag;
	/** File type */
	uint8_t ftype;
	/** File name */
	uint8_t fname[10];
	/** Length of data block */
	uint16_t dblen;
	/** Parameter 1 */
	uint16_t param1;
	/** Parameter 2 */
	uint16_t param2;
	/** Parity byte */
	uint8_t parity;
} __attribute__((packed)) rom_tape_header_t;

/** Structure for holding Spectrum tape file name */
typedef struct {
	/** Buffer to hold file name and null character */
	char fname[11];
} rom_filename_t;

#endif
