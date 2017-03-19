#ifndef Z80G_H
#define Z80G_H

#include "intdef.h"
#include "z80.h"

/* Number of graphical planes */
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
