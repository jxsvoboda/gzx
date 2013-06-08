#ifndef _ZX_SCR_H
#define _ZX_SCR_H

#include "global.h"

#define ZX_PIXEL_START	0x0000
#define ZX_ATTR_START	0x1800
#define ZX_ATTR_END	0x1aff

extern unsigned long disp_clock;
extern unsigned long disp_cbase;
extern unsigned long disp_t;
extern int fl_rev;

int zx_scr_init(void);
void zx_scr_mode(int mode);

extern void (*zx_scr_disp)(void);
extern void (*zx_scr_disp_fast)(void);

#endif
