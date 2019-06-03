/*
 * GZX - George's ZX Spectrum Emulator
 * Tape tone generator unit tests
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
 * @file Tape tone generator unit tests.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "../../tape/tonegen.h"
#include "tonegen.h"

enum {
	test_np = 6
};

/** Run tone generator unit tests.
 *
 * @return Zero on success, non-zero on failure
 */
int test_tonegen(void)
{
	tonegen_t tgen;
	uint32_t delay;
	tape_lvl_t lvl;
	uint32_t delays[test_np] = { 10, 10, 10, 20, 20, 30 };
	tape_lvl_t lvls[test_np] = { tlvl_high, tlvl_low, tlvl_high, tlvl_low,
	    tlvl_high, tlvl_low };
	int i;

	printf("Test tonegen...\n");

	tonegen_init(&tgen, tlvl_low);
	tonegen_add_tone(&tgen, 10, 3);
	tonegen_add_tone(&tgen, 20, 2);
	tonegen_add_tone(&tgen, 30, 1);

	for (i = 0; i < 6; i++) {
		if (tonegen_is_end(&tgen)) {
			printf("Premature end of tone.\n");
			return 1;
		}

		tonegen_get_next(&tgen, &delay, &lvl);
		if (delay != delays[i] || lvl != lvls[i]) {
			printf("Incorrect pulse length.\n");
			return 1;
		}
	}

	if (!tonegen_is_end(&tgen)) {
		printf("Expected end of tone not found.\n");
		return 1;
	}

	printf(" ... passed\n");

	return 0;
}
