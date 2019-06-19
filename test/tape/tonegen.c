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
	test_np = 6,
	testd_np = 9
};

/** Test tone generator with normal tones.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tonegen_normal(void)
{
	int i;
	tonegen_t tgen;
	uint32_t delay;
	tape_lvl_t lvl;
	uint32_t delays[test_np] = { 10, 10, 10, 20, 20, 30 };
	tape_lvl_t lvls[test_np] = {
		tlvl_high, tlvl_low, tlvl_high, tlvl_low, tlvl_high, tlvl_low
	};

	printf("Test tonegen with normal tones...\n");

	/* Set program */

	tonegen_init(&tgen, tlvl_low);
	tonegen_add_tone(&tgen, 10, 3);
	tonegen_add_tone(&tgen, 20, 2);
	tonegen_add_tone(&tgen, 30, 1);

	/* Test playback */

	if (tonegen_cur_lvl(&tgen) != tlvl_low) {
		printf("Incorrect initial level.\n");
		return 1;
	}

	for (i = 0; i < test_np; i++) {
		if (tonegen_is_end(&tgen)) {
			printf("Premature end of tone.\n");
			return 1;
		}

		tonegen_get_next(&tgen, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length %d != %d.\n",
			    delay, delays[i]);
			return 1;
		}

		if (lvl != lvls[i]) {
			printf("Incorrect pulse level %d != %d.\n",
			    lvl, lvls[i]);
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

/** Run Test tone generator with direct pulses.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tonegen_direct(void)
{
	int i;
	tonegen_t tgen;
	uint32_t delay;
	tape_lvl_t lvl;
	uint32_t delays[testd_np] = { 10, 20, 30, 40, 50, 60, 70, 80, 90 };
	tape_lvl_t lvls[testd_np] = {
		tlvl_low, tlvl_low, tlvl_high, tlvl_high, tlvl_low, tlvl_low,
		tlvl_high, tlvl_low
	};

	printf("Test tonegen with direct pulses...\n");

	tonegen_init(&tgen, tlvl_low);
	tonegen_add_dpulse(&tgen, tlvl_high, 10);
	tonegen_add_dpulse(&tgen, tlvl_low, 20);
	tonegen_add_dpulse(&tgen, tlvl_low, 30);
	tonegen_add_dpulse(&tgen, tlvl_high, 40);
	tonegen_add_dpulse(&tgen, tlvl_high, 50);
	tonegen_add_dpulse(&tgen, tlvl_low, 60);
	tonegen_add_tone(&tgen, 70, 1);
	tonegen_add_tone(&tgen, 80, 1);
	tonegen_add_dpulse(&tgen, tlvl_low, 90);

	if (tonegen_cur_lvl(&tgen) != tlvl_high) {
		printf("Incorrect initial level.\n");
		return 1;
	}

	for (i = 0; i < testd_np; i++) {
		if (tonegen_is_end(&tgen)) {
			printf("Premature end of tone.\n");
			return 1;
		}

		tonegen_get_next(&tgen, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length %d != %d.\n",
			    delay, delays[i]);
			return 1;
		}

		if (lvl != lvls[i]) {
			printf("Incorrect pulse level %d != %d.\n",
			    lvl, lvls[i]);
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

/** Test tone generator with two consecutive series of commands.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tonegen_tworuns(void)
{
	int i;
	tonegen_t tgen;
	uint32_t delay;
	tape_lvl_t lvl;
	uint32_t delays[test_np] = { 10, 10, 10, 20, 20, 30 };
	tape_lvl_t lvls[test_np] = {
		tlvl_high, tlvl_low, tlvl_high, tlvl_low, tlvl_high, tlvl_low
	};

	printf("Test tonegen with two series of commands...\n");

	tonegen_init(&tgen, tlvl_low);
	tonegen_add_tone(&tgen, 10, 3);
	tonegen_add_tone(&tgen, 20, 2);
	tonegen_add_tone(&tgen, 30, 1);

	if (tonegen_cur_lvl(&tgen) != tlvl_low) {
		printf("Incorrect initial level.\n");
		return 1;
	}

	for (i = 0; i < test_np; i++) {
		if (tonegen_is_end(&tgen)) {
			printf("Premature end of tone.\n");
			return 1;
		}

		tonegen_get_next(&tgen, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length %d != %d.\n",
			    delay, delays[i]);
			return 1;
		}

		if (lvl != lvls[i]) {
			printf("Incorrect pulse level %d != %d.\n",
			    lvl, lvls[i]);
			return 1;
		}
	}

	if (!tonegen_is_end(&tgen)) {
		printf("Expected end of tone not found.\n");
		return 1;
	}

	tonegen_clear(&tgen);
	tonegen_add_tone(&tgen, 10, 3);
	tonegen_add_tone(&tgen, 20, 2);
	tonegen_add_tone(&tgen, 30, 1);

	/* Level should be low again after 6 pulses */
	if (tonegen_cur_lvl(&tgen) != tlvl_low) {
		printf("Incorrect initial level.\n");
		return 1;
	}

	for (i = 0; i < test_np; i++) {
		if (tonegen_is_end(&tgen)) {
			printf("Premature end of tone.\n");
			return 1;
		}

		tonegen_get_next(&tgen, &delay, &lvl);
		if (delay != delays[i]) {
			printf("Incorrect pulse length %d != %d.\n",
			    delay, delays[i]);
			return 1;
		}

		if (lvl != lvls[i]) {
			printf("Incorrect pulse level %d != %d.\n",
			    lvl, lvls[i]);
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

/** Test tone generator programmed levels reporting.
 *
 * @return Zero on success, non-zero on failure
 */
static int test_tonegen_plevels(void)
{
	tonegen_t tgen;

	printf("Test tonegen programmed level reporting...\n");

	tonegen_init(&tgen, tlvl_low);
	if (tonegen_pprev_lvl(&tgen) != tlvl_low) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_low) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	tonegen_add_tone(&tgen, 10, 3);
	if (tonegen_pprev_lvl(&tgen) != tlvl_low) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_high) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	tonegen_add_tone(&tgen, 20, 2);
	if (tonegen_pprev_lvl(&tgen) != tlvl_low) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_high) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	tonegen_add_tone(&tgen, 30, 1);
	if (tonegen_pprev_lvl(&tgen) != tlvl_high) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_low) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	tonegen_add_dpulse(&tgen, tlvl_high, 10);
	if (tonegen_pprev_lvl(&tgen) != tlvl_high) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_high) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	tonegen_add_dpulse(&tgen, tlvl_low, 20);
	if (tonegen_pprev_lvl(&tgen) != tlvl_low) {
		printf("Incorrect previous programmed level.\n");
		return 1;
	}
	if (tonegen_plast_lvl(&tgen) != tlvl_low) {
		printf("Incorrect last programmed level.\n");
		return 1;
	}

	printf(" ... passed\n");

	return 0;
}

/** Run tone generator unit tests.
 *
 * @return Zero on success, non-zero on failure
 */
int test_tonegen(void)
{
	int rc;

	rc = test_tonegen_normal();
	if (rc != 0)
		return 1;

	rc = test_tonegen_direct();
	if (rc != 0)
		return 1;

	rc = test_tonegen_tworuns();
	if (rc != 0)
		return 1;

	rc = test_tonegen_plevels();
	if (rc != 0)
		return 1;

	return 0;
}
