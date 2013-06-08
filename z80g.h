#ifndef _Z80G_H
#define _Z80G_H

#include "global.h"

/* pocet bitovych rovin GPU */
#define NGP 8

extern z80s gpus[NGP];

extern u8 *gfxrom[NGP];
extern u8 *gfxram[NGP];
extern u8 *gfxscr[NGP];


int gpu_reset(void);
void z80_g_execinstr(void); /* execute both on CPU and GPU */
void z80_g_int(u8 bus);       /* INT both on CPU and GPU */
void gfx_select_memmodel(int model);

#endif
