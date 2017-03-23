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
int z80_nmi(void);
int z80_int(u8 data);

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
