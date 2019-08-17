/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 CPU emulation
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

#ifndef Z80_H
#define Z80_H

#include "intdef.h"

/* flags */
#define fS  0x80
#define fZ  0x40
#define fHC 0x10
#define fPV 0x04
#define fN  0x02
#define fC  0x01

/* undocumented flags */
#define fU1 0x08
#define fU2 0x20

/* documented/undocumented flags masks */
#define fU  0x28
#define fD  0xd7


#define rA  0x7
#define rB  0x0
#define rC  0x1
#define rD  0x2
#define rE  0x3
#define rH  0x4
#define rL  0x5

typedef struct _z80s {     /*** registers of the Z80 CPU ***/
  u8 r[8];                 /* all-purpose registers,HL reg */
  u8 F;                    /* flags register */

  u8 r_[8];      	   /* alternative registers */
  u8 F_;

  u8 I;                    /* interrupt page address register */
  u16 IX, IY;              /* index registers */
  u16 PC;                  /* program counter */
  u8 R;                    /* memory refresh register */
  u16 SP;                  /* stack pointer */

  int IFF1,IFF2;	   /* interrupt flip-flops */
  int int_mode;		   /* interrupt mode */
  int int_lock;		   /* forbids INT&NMI (after EI/DI/DD/FD) */
  int int_pending;         /* an interrupt is pending */
  int nmi_pending;         /* an NMI is pending */
  int modifier;            /* 0/0xdd/0xfd */
  int halted;		   /* halted by the HALT instruction? */
} z80s;

extern z80s cpus;
extern unsigned long z80_clock;
extern unsigned long uoc;
extern unsigned long smc;

void z80_init_tables(void);
void z80_execinstr(void);
int z80_reset(void);
void z80_nmi(void);
void z80_int(void);

void z80_resetstat(void);
unsigned z80_getstat(int, u8);

u16 z80_getAF(void);
u16 z80_getBC(void);
u16 z80_getDE(void);
u16 z80_getHL(void);
u16 z80_getAF_(void);
u16 z80_getBC_(void);
u16 z80_getDE_(void);
u16 z80_getHL_(void);

#endif
