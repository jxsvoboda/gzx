/*
 * GZX - George's ZX Spectrum Emulator
 * TAP file format unit tests
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
 * @file TAP file format unit tests
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../tape/tape.h"
#include "../../tape/tap.h"
#include "tap.h"

/** Test saving and loading back simple TAP file.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tap_save_load(void)
{
	tape_t *tape = NULL;
	char fname[L_tmpnam];
	int rc;

	printf("Test saving and loading simple tape to/from TAP...\n");

	rc = tape_create(&tape);
	if (rc != 0) {
		printf("tape_create -> %d\n", rc);
		return 1;
	}

	if (tmpnam(fname) == NULL) {
		printf("tmpnam -> %d\n", rc);
		return 1;
	}

	rc = tap_tape_save(tape, fname);
	if (rc != 0) {
		printf("tap_tape_save -> %d\n", rc);
		return 1;
	}

	rc = tap_tape_load(fname, &tape);
	if (rc != 0) {
		printf("tap_tape_load -> %d\n", rc);
		return 1;
	}

	tape_destroy(tape);

	printf(" ... passed\n");

	return 0;
}

/** Run TAP file format unit tests.
 *
 * @return Zero on success, non-zero on failure
 */
int test_tap(void)
{
	int rc;

	rc = test_tap_save_load();
	if (rc != 0)
		return 1;

	return 0;
}
