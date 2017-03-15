#ifndef _MEMIO_H
#define _MEMIO_H

#include "global.h"

/* memory models */
#define ZXM_48K   0
#define ZXM_128K  1
#define ZXM_PLUS2 2
#define ZXM_PLUS3 3
#define ZXM_ZX81  4

/* spectrum memory access */
u8 zx_memget8(u16 addr);
void zx_memset8(u16 addr, u8 val);
void zx_memset8f(u16 addr, u8 val);
u16 zx_memget16(u16 addr);
void zx_memset16(u16 addr, u16 val);

/* spectrum i/o port access */
void zx_out8(u16 addr, u8 val);
u8 zx_in8(u16 addr);

int zx_select_memmodel(int model);
void zx_mem_page_select(u8 val);
int gfxram_load(char *fname);
int gfxrom_load(char *fname, unsigned bank);

extern u8 page_reg;
extern u8 border;
extern u8 spk,mic,ear;
extern u8 *zxram,*zxrom,*zxscr,*zxbnk[4];
extern int mem_model;

extern int bnk_lock48;
extern unsigned ram_size,rom_size;

#endif
