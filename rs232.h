/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K RS-232 port emulation
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

#ifndef RS232_H
#define RS232_H

#include <stdint.h>

/** Bits within the RS232 register
 *
 * Note that the Spectrum Service manual notation used here refers
 * to the name of the signals as seen by the other side (i.e. CTS, RXD
 * are output lines, TXD, DTR are input lines.
 */
enum {
	rs232_txd = 7,
	rs232_dtr = 6,
	rs232_rxd = 3,
	rs232_cts = 2
};

typedef enum {
	rs232_idle,
	rs232_sending
} rs232_state_t;

typedef struct {
	/** Z80 T states between successive bits (rel. to baud rate) */
	uint32_t d_clocks;
	/** State */
	rs232_state_t state;
	/** Bit number */
	uint8_t bit_no;
	/** Transfer buffer */
	uint8_t buf;
	/** A symbol was sent */
	void (*sendchar)(void *, uint8_t);
	/** Argument to sendchar callback */
	void *sendchar_arg;
} rs232_t;

extern void rs232_init(rs232_t *, uint32_t);
extern void rs232_write(rs232_t *, uint8_t);

#endif
