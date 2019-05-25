/*
 * GZX - George's ZX Spectrum Emulator
 * ZX key matrix definitions
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

#ifndef ZX_KEYS_H
#define ZX_KEYS_H

enum {
	zx_keymtx_rows = 8
};

/* half-row v,c,x,z,CS */
#define ZX_KEY_V	0x10
#define ZX_KEY_C	0x08
#define ZX_KEY_X	0x04
#define ZX_KEY_Z	0x02
#define ZX_KEY_CS	0x01

/* half-row g,f,d,s,a */
#define ZX_KEY_G	0x10
#define ZX_KEY_F	0x08
#define ZX_KEY_D	0x04
#define ZX_KEY_S	0x02
#define ZX_KEY_A	0x01

/* half-row t,r,e,w,q */
#define ZX_KEY_T	0x10
#define ZX_KEY_R	0x08
#define ZX_KEY_E	0x04
#define ZX_KEY_W	0x02
#define ZX_KEY_Q	0x01

/* half-row 5,4,3,2,1 */
#define ZX_KEY_5	0x10
#define ZX_KEY_4	0x08
#define ZX_KEY_3	0x04
#define ZX_KEY_2	0x02
#define ZX_KEY_1	0x01

/* half-row 6,7,8,9,0 */
#define ZX_KEY_6	0x10
#define ZX_KEY_7	0x08
#define ZX_KEY_8	0x04
#define ZX_KEY_9	0x02
#define ZX_KEY_0	0x01

/* half-row y,u,i,o,p */
#define ZX_KEY_Y	0x10
#define ZX_KEY_U	0x08
#define ZX_KEY_I	0x04
#define ZX_KEY_O	0x02
#define ZX_KEY_P	0x01

/* half-row h,j,k,l,ENT */
#define ZX_KEY_H	0x10
#define ZX_KEY_J	0x08
#define ZX_KEY_K	0x04
#define ZX_KEY_L	0x02
#define ZX_KEY_ENT	0x01

/* half-row B,N,M,SS,SP */
#define ZX_KEY_B	0x10
#define ZX_KEY_N	0x08
#define ZX_KEY_M	0x04
#define ZX_KEY_SS	0x02
#define ZX_KEY_SP	0x01

#endif
