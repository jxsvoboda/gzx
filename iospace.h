/*
 * GZX - George's ZX Spectrum Emulator
 * I/O port definitions
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
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

#ifndef IOSPACE_H
#define IOSPACE_H

#define AY_REG_SEL_PORT   0xfffd
#define AY_REG_READ_PORT  0xfffd
#define AY_REG_WRITE_PORT 0xbffd

/** Primary Kempston joystick port */
#define KEMPSTON_JOY_A_PORT 0x1f
/** Secondary Kempston joystick port (e.g. Didaktik Gama) */
#define KEMPSTON_JOY_B_PORT 0x21

/*
 * The nominal 128K page select port is 0x7ffd, but 128K just checks
 * bits 15 and 1. Some software depends on this (e.g. The Cube / TGM).
 */
#define ZX128K_PAGESEL_PORT_MASK 0x8002
#define ZX128K_PAGESEL_PORT_VAL  0x0000

#define ZXPLUS_PAGESEL_PORT 0x7ffd
#define ZXPLUS_EPG_PORT 0x1ffd

#define ULA_PORT_MASK 0x00ff
#define ULA_PORT      0x00fe

#define ULAPLUS_REGSEL_PORT 0xbf3b
#define ULAPLUS_DATA_PORT   0xff3b

#endif
