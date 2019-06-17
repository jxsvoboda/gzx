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

#include <string.h>
#include "romblock.h"
#include "tape.h"

/** Extract file name from ROM tape header block.
 *
 * The file name is converted from space-padded representation to
 * C-style null-terminated string.
 *
 * @param header ROM tape header block
 * @param fname Structure to hold file name
 */
void rom_tape_header_get_fname(rom_tape_header_t *header, rom_filename_t *fname)
{
	int i;

	/* Copy characters, null terminate */
	memcpy(fname->fname, header->fname, sizeof(header->fname));
	fname->fname[sizeof(header->fname)] = '\0';

	/* Convert space padding at the end into null characters */
	i = sizeof(header->fname) - 1;
	while (i > 0 && fname->fname[i] == ' ') {
		fname->fname[i] = '\0';
		--i;
	}
}

/** Get text description of ROM file type.
 *
 * @param ftype ROM file type
 * @return Pointer to static string
 */
const char *rom_tape_get_ftype_desc(rom_ftype_t ftype)
{
	switch (ftype) {
	case ftype_program:
		return "Program";
	case ftype_number_array:
		return "Number array";
	case ftype_character_array:
		return "Character array";
	case ftype_bytes:
		return "Bytes";
	}

	return "Unknown";
}
